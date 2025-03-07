//
//  Archive.cpp
//

#include "Archive.hpp"
#include <cstring>
#include <memory>
#include <filesystem>

using namespace std;
namespace ECE141 {

    //Archive class
    // ----------------------------------------------------------------------------------------------------------

    Archive::Archive(const std::string &aFullPath, ECE141::AccessMode aMode){
        thePath = filesystem::current_path().string();

        //existing
        if(aMode ==  AccessMode::AsExisting)
            theArcFile = fstream (aFullPath,std::ios::binary | std::ios::in | std::ios::out);

            //creating, truncate
        else
            theArcFile = fstream (aFullPath,std::ios::binary | std::ios::in | std::ios::out | std:: ios::trunc);
    }

    Archive::~Archive(){
        theArcFile.flush(); //clear buffer
        theArcFile.close(); //close
    }

    ArchiveStatus<std::shared_ptr<Archive>> Archive::createArchive(const std::string &anArchiveName) {
        string aName = anArchiveName;  //add .arc
        if(!has_arc_ext(anArchiveName))
            aName += ".arc";

        try{ //make new archive, return if good
            auto *newArchive = new Archive(aName,AccessMode::AsNew);
            return ArchiveStatus{shared_ptr<Archive>(newArchive)};
        }
        catch(...) //else return error
        {
            cerr<<"error creating archive"<<'\n';
            return ArchiveStatus<shared_ptr<Archive>>(ArchiveErrors:: fileOpenError);
        }
    }

    ArchiveStatus<std::shared_ptr<Archive>> Archive::openArchive(const std::string &anArchiveName) {
        string aName = anArchiveName; //add .arc
        if (!has_arc_ext(anArchiveName))
            aName += ".arc";

        try { //make new archive, return if good
            auto *ExistingArchive = new Archive(aName, AccessMode::AsExisting);
            return ArchiveStatus{shared_ptr<Archive>(ExistingArchive)};
        }
        catch (...) //else return error
        {
            cerr << "error creating archive" << '\n';
            return ArchiveStatus<shared_ptr<Archive>>(ArchiveErrors::fileOpenError);
        }
    }

    ArchiveStatus<bool> Archive::add(const std::string &aFileName, IDataProcessor* aProcessor) {
        std::fstream temp;
        size_t compsize = 0; // Default

        if (aProcessor) { // Check process flag
            // Process the file using the IDataProcessor
            // (compress or modify the file content)
            std::fstream inputFile(aFileName, std::ios::binary | std::ios::in | std::ios::out);
            if (!inputFile.is_open()) {
                notifyObservers(ActionType::added, aFileName, false);
                return ArchiveStatus<bool>(false);
            }

            std::vector<uint8_t> theVec; // Compress vector
            read_to_vec(theVec, inputFile);
            theVec = aProcessor->process(theVec);
            compsize = theVec.size();

            // Temp file to hold compressed data
            temp.open("/tmp/TEMP.txt", std::ios::binary | std::ios::out);
            if (!temp.is_open()) {
                notifyObservers(ActionType::added, aFileName, false);
                inputFile.close();
                return ArchiveStatus<bool>(false);
            }
            writeToFile(theVec, temp); // Write to temp file
            inputFile.close();
            temp.close(); // Close to clear for edit
            temp.open("/tmp/TEMP.txt", std::ios::binary | std::ios::in);
        } else {
            // No processing, just open the file
            temp.open(aFileName, std::ios::binary | std::ios::in | std::ios::out);
            if (!temp.is_open()) {
                notifyObservers(ActionType::added, aFileName, false);
                return ArchiveStatus<bool>(false);
            }
        }

        temp.clear(); //clear buffer and set pointer
        temp.seekg(0);
        temp.seekp(0);

        static size_t pos = 0; // Begin chunking
        Chunker chunker(temp);
        bool success = chunker.chunk_em([&](Chunk &chunk) {
            assign_meta(chunk, pos, aFileName, compsize); // Assign meta
            // Write the chunk to the output stream
            theArcFile.write(reinterpret_cast<const char *>(&chunk), kChunkSize);

            pos += kChunkSize;
            return true;
        });

        temp.close();
        std::remove("/tmp/TEMP.txt"); // Remove temp file

        if (!success) {
            notifyObservers(ActionType::added, aFileName, false);
            return ArchiveStatus<bool>(false);
        }

        notifyObservers(ActionType::added, aFileName, true);
        return ArchiveStatus<bool>(true);
    }

    ArchiveStatus<bool> Archive::extract(const std::string &aFilename, const std::string &aFullPath) {
        std::ofstream outputFileStream(aFullPath, std::ios::binary | std::ios::out);
        if (!outputFileStream.is_open()) {
            std::cerr << "Error: Could not open output file" << std::endl;
            return ArchiveStatus<bool>(ArchiveErrors::fileOpenError);
        }

        if (!theArcFile.is_open()) {
            std::cerr << "Error: Could not open archive file" << std::endl;
            notifyObservers(ActionType::extracted, aFilename, false);
            return ArchiveStatus<bool>(ArchiveErrors::fileOpenError);
        }

        theArcFile.seekg(0, std::ios::beg); //set

        bool foundFile = false;
        while (!theArcFile.eof() && theArcFile.good()) { //read into each chunk
            Chunk chunk;
            memset(&chunk, 0, sizeof(Chunk));
            size_t prePosition = theArcFile.tellg();
            theArcFile.read(reinterpret_cast<char*>(&chunk), kChunkSize);

            if (theArcFile.eof() || !theArcFile.good()) {
                break;
            }

            std::string extractedFilename(chunk.meta.name);

            if (extractedFilename == aFilename) {
                foundFile = true;

                // Check compression
                if (chunk.meta.comp_size) {
                    // File is compressed, decompress it
                    std::vector<uint8_t> compressedData(chunk.data, chunk.data + chunk.meta.comp_size);
                    std::vector<uint8_t> uncompressedData;
                    IDataProcessor* theProcessor = new Compression(); //uncompress
                    uncompressedData = theProcessor->reverseProcess(compressedData);

                    if (!uncompressedData.empty()) {
                        outputFileStream.write(reinterpret_cast<const char*>(uncompressedData.data()), uncompressedData.size());
                        delete theProcessor; //free
                    } else {
                        std::cerr << "Error: Failed to decompress file" << std::endl;
                        outputFileStream.close();
                        return ArchiveStatus<bool>(ArchiveErrors::badProcessor);
                    }
                } else {
                    // File is not compressed, write it directly
                    outputFileStream.write(reinterpret_cast<const char*>(chunk.data), std::min(chunk.meta.filesize, static_cast<uint32_t>(kChunkSize - sizeof(ChunkHeader))));
                }

                // Move to the next block if the file spans multiple blocks
                size_t current_size = chunk.meta.comp_size ? chunk.meta.comp_size : chunk.meta.filesize;
                size_t blocksToMove = current_size / kChunkSize;
                for (size_t i = 1; i < blocksToMove; ++i) {
                    theArcFile.seekg(prePosition + i * kChunkSize, std::ios::beg); //track back
                    theArcFile.read(reinterpret_cast<char*>(&chunk), kChunkSize);
                    outputFileStream.write(chunk.data, kChunkSize - sizeof(ChunkHeader));
                }
                break;
            }
        }

        outputFileStream.close();

        if (foundFile) {
            notifyObservers(ActionType::extracted, aFilename, true);
            return ArchiveStatus<bool>(true);
        } else {
            notifyObservers(ActionType::extracted, aFilename, false);
            return ArchiveStatus<bool>(ArchiveErrors::fileNotFound);
        }
    }

    ArchiveStatus<bool> Archive::remove(const std::string& aFilename) {

        if (!theArcFile.is_open()) {
            std::cerr << "Error: Could not open archive file" << std::endl;
            notifyObservers(ActionType::removed, aFilename, false);
            return ArchiveStatus<bool>(ArchiveErrors::fileOpenError);
        }

        theArcFile.seekg(0, std::ios::beg); //find begin

        bool foundFile = false;
        while (!theArcFile.eof() && theArcFile.good()) {
            // Read the chunk
            Chunk chunk;
//            memset(&chunk, 0, kChunkSize);
            size_t prePosition = theArcFile.tellg();
            theArcFile.read(reinterpret_cast<char*>(&chunk), kChunkSize);//read data

            if (theArcFile.eof() || !theArcFile.good()) {
                break; // Exit the loop if reading the chunk fails
            }

            // Check if the chunk's filename matches the specified filename
            std::string extractedFilename(chunk.meta.name);

            if (extractedFilename == aFilename) {
                foundFile = true;

                //block moving logic
                size_t current_size = chunk.meta.comp_size ? chunk.meta.comp_size : chunk.meta.filesize;
                size_t blocksToRemove = current_size / kChunkSize; //get num blocks
                for (size_t i = 0; i < blocksToRemove; ++i) {

                    theArcFile.seekg(i*kChunkSize+prePosition, std::ios::beg); //out of bounds??
                    theArcFile.read(reinterpret_cast<char*>(&chunk), kChunkSize);//read data
                    chunk.meta.occupied=0;

                    //get next block in file
                    theArcFile.seekp(i*kChunkSize+prePosition, std::ios::beg);
                    theArcFile.write(reinterpret_cast<const char*>(&chunk), kChunkSize);
                }
            }
        }

        if (foundFile) {
            notifyObservers(ActionType::removed, aFilename, true);
            return ArchiveStatus<bool>(true);
        } else {
            // File not found in the archive
            notifyObservers(ActionType::removed, aFilename, false);
            return ArchiveStatus<bool>(ArchiveErrors::fileNotFound);
        }
    }

    ArchiveStatus<size_t> Archive::list(std::ostream &outputStream) {
        size_t fileCount = 0;

        if (!theArcFile.is_open()) {
            std::cerr << "Error: Could not open archive file" << std::endl;
            notifyObservers(ActionType::listed, "", false);
            return ArchiveStatus<size_t>(fileCount);
        }

        std::string theOut;
        theOut += "###  name                 size          date added\n";
        theOut += "---------------------------------------------------------------\n";

        while (!theArcFile.eof() && theArcFile.good()) {
            // Read the chunk
            Chunk aChunk;
            memset(&aChunk, 0, kChunkSize);
            theArcFile.read(reinterpret_cast<char*>(&aChunk), kChunkSize); //read only the header

            if (theArcFile.eof() || !theArcFile.good()) {
                break; // Exit the loop if reading the header fails
            }

            // Extract relevant metadata from the header
            // For example, if the header contains the filename, size, and date added:
            // convert ms to nice time string
            time_t t_added = aChunk.meta.dateAdded;
            std::string date = std::ctime(&t_added);

            //set size dependent to compression status
            size_t current_size = aChunk.meta.comp_size ? aChunk.meta.comp_size : aChunk.meta.filesize;
            //keep track of filename and filesize
            uint32_t blocksToMove = current_size/kChunkSize; //skip other blocks, to next file

            if (aChunk.meta.occupied) { //formatting
                theOut += std::to_string(fileCount + 1) + ".\t " + std::string(aChunk.meta.name) + "\t  " + std::to_string(current_size) + "\t\t\t" + date;
            }

            //verify there is valid file
            if (blocksToMove) {
                theArcFile.seekg((blocksToMove-1) * kChunkSize, std::ios::cur); //correctly shift
                ++fileCount;
            } else {
                break;
            }

        }
        outputStream << theOut; //write to file

        notifyObservers(ActionType::listed, "", true);
        return ArchiveStatus<size_t>(fileCount);
    }

    ArchiveStatus<size_t> Archive::debugDump(std::ostream &aStream) {
        size_t numBlocks = 0;
        string theOut;

        if (!theArcFile.is_open()) {
            std::cerr << "Error: Could not open archive file" << std::endl;
            notifyObservers(ActionType::dumped, "", false);
            return ArchiveStatus<size_t>(numBlocks);
        }

        // Move to the beginning of the archive file
        theArcFile.seekg(0, std::ios::beg);

        theOut +="###  status            name\n";
        theOut+= "-----------------------------\n";

        while (!theArcFile.eof() && theArcFile.good()) {
            // Read the chunk header
            Chunk chunk;
            theArcFile.read(reinterpret_cast<char *>(&chunk), kChunkSize);

            if (theArcFile.eof() || !theArcFile.good()) {
                break; // Exit the loop if reading the  fails
            }

            std::string status = (chunk.meta.occupied) ? "used" : "empty";
            std::string name = (chunk.meta.occupied) ? std::string(chunk.meta.name) : ""; //return name when occupied

            theOut += to_string(numBlocks + 1) += ".   "; //formatting
            theOut += status += "\t";
            theOut += name += '\n';

            //next
            theArcFile.seekg(0, std::ios::cur);

            ++numBlocks;
        }
        aStream<<theOut;

        notifyObservers(ActionType::dumped, "", true);
        return ArchiveStatus<size_t>(numBlocks);
    }

    ArchiveStatus<size_t> Archive::compact() {

        //temporary filename
        std::fstream compactedFile("temp_archive", std::ios::binary);
        if (!compactedFile.is_open()) {
            std::cerr << "Error: Could not create compacted archive file" << std::endl;
            return ArchiveStatus<size_t>(0);
        }

        if (!theArcFile.is_open()) {
            std::cerr << "Error: Could not open archive file" << std::endl;
            notifyObservers(ActionType::compacted, "", false);
            return ArchiveStatus<size_t>(0);
        }

        // Move to the beginning of the archive file
        theArcFile.seekg(0, std::ios::beg);

        size_t compactedSize = 0;

        while (!theArcFile.eof() && theArcFile.good()) {
            // Read the chunk
            Chunk chunk;
            memset(&chunk, 0, kChunkSize);
            theArcFile.read(reinterpret_cast<char *>(&chunk), kChunkSize);

            if (theArcFile.eof() || !theArcFile.good()) {
                break; // Exit the loop if reading the chunk fails
            }

            // Check if the chunk is occupied
            if (chunk.meta.occupied) {
                // Write the chunk to the compacted archive file
                compactedFile.write(reinterpret_cast<const char *>(&chunk), kChunkSize);
                compactedSize += chunk.meta.filesize / kChunkSize;
            }
            compactedSize *= kChunkSize;

            //close old file
            theArcFile.flush();
            theArcFile.close();


            //rename or throw error
            const string old = "compactedFile";
            const string aNew = "theArcFile";

            renamefile(old,aNew);

            notifyObservers(ActionType::compacted, "", true);
            return ArchiveStatus<size_t>(compactedSize);
        }
        notifyObservers(ActionType::compacted, "", false);
        return ArchiveStatus<size_t>(compactedSize);
    }

    ArchiveStatus<std::string> Archive::getFullPath() const {
        return ArchiveStatus<string>(thePath);
    }

    size_t Archive::calculateFileSize(const string & aPath) {
        size_t filesize = filesystem::file_size(aPath);
        return filesize;
    }

    void Archive ::read_to_vec(vector<uint8_t> &vec, fstream &inputFile) {
        // Read file contents into vec
        inputFile.seekg(0, std::ios::end);
        std::streampos fileSize = inputFile.tellg();
        inputFile.seekg(0, std::ios::beg);

        vec.resize(fileSize);
        inputFile.read(reinterpret_cast<char*>(vec.data()), fileSize);
    }

    void Archive::writeToFile(const std::vector<uint8_t>& vec, std::fstream& file) {

        std::string str(vec.begin(), vec.end()); // Convert the vector to a string
        file << str; // Write the string to the file
    }

    void Archive:: renamefile(const string &old, const string &anew)
    {   //rename or throw error
        std::error_code ec;
        std::filesystem::rename(old, anew, ec);

        if (ec) {
            std::cerr << "Error renaming file: " << ec.message() << std::endl;
        } else {
            std::cout << "Archive compacted successfully." << std::endl;
        }
    }

    void Archive::assign_meta(Chunk &chunk, size_t aPos, const string &aName, size_t compsize) {
        static uint16_t partNum = 1; // Part 1, 2, 3, ...
        static size_t thefileSize = kChunkSize; // Min
        static size_t compressedsize = 0;
        static string thename;

        if (thename != aName) { // Only needed once for every unique file
            thename = aName;
            partNum = 1;
            thefileSize = closestMultipleOf1024(calculateFileSize(aName));
            compressedsize = closestMultipleOf1024(compsize); //if 0 just zero
        }

        //chunk pos logic
        chunk.meta.occupied = true;
        chunk.meta.partNum = partNum++;
        chunk.meta.hashNum = ECE141::Chunk::calc_hash(aName);

        string temp = extractFilename(aName.substr(0, 29)); // Ensure the filename does not exceed 29 characters
        strncpy(chunk.meta.name, temp.c_str(), 29);
        chunk.meta.name[29] = '\0'; // Null-terminate the string

        //file size logic
        chunk.meta.comp_size = compressedsize ? compressedsize : 0;
        chunk.meta.filesize = thefileSize ? thefileSize : kChunkSize;

        chunk.meta.nextBlock = static_cast<uint16_t>((aPos + kChunkSize) / kChunkSize);
        //time stamp
        time_t currentTime;
        time(&currentTime);
        chunk.meta.dateAdded = currentTime;
        chunk.meta.checkSum = chunk.meta.calc_check_sum(); //checksum for integrity
    }

    //Archive Observer
    //-----------------------------------------------------------------------------------------------------------------
    //visitor pattern here, what observer does with info
    void ArchiveObserver::operator()(ECE141::ActionType anAction, const std::string &aName, bool status) {
    }

    bool Archive::addObserver(std::shared_ptr<ArchiveObserver> anObserver) {
        // Check if the observer is already in the list
        auto it = std::find(observers.begin(), observers.end(), anObserver);
        if (it != observers.end()) {
            return false;   // Observer already exists in the list
        }
        // Add the observer to the list
        observers.push_back(anObserver);
        return true;
    }

    void Archive:: notifyObservers(ActionType action, const string& filename, bool success) {
        for (const auto& observer : observers) { //call to each observer with functor
            (*observer)(action, filename, success);
        }
    }

    //Compressor
    //-----------------------------------------------------------------------------------------------------------------
    std::vector<uint8_t> Compression::process(const std::vector<uint8_t> &input) {
        std::vector<uint8_t> output;
        output.resize(compressBound(input.size())); // Get the maximum possible size of the compressed data
        uLongf compressedSize = output.size();
        int result = compress2(output.data(), &compressedSize, input.data(), input.size(), Z_BEST_COMPRESSION);
        if (result != Z_OK) {
            output.clear();
        } else {
            output.resize(compressedSize);
        }
        return output;
    }

    std::vector<uint8_t> Compression::reverseProcess(const std::vector<uint8_t> &input) {
        std::vector<uint8_t> output;
        output.resize(MAX_CHUNK_COUNT*kChunkSize); // Initial guess at the uncompressed size, max is 33 chunks
        uLongf uncompressedSize = output.size();
        int result = uncompress(output.data(), &uncompressedSize, input.data(), input.size());
        if (result != Z_OK) {
            output.clear();
        } else {
            output.resize(uncompressedSize);
        }
        return output;
    }

} //namespace ECE141