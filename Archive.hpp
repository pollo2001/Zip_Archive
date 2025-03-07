//
//  Archive.hpp
//

#ifndef Archive_hpp
#define Archive_hpp

#include <cstdio>
#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <cmath>
#include <memory>
#include <optional>
#include <stdexcept>
#include <filesystem>
#include <zlib.h>
#include "Chunkers.hpp"
#include "helpers.h"

namespace ECE141 {

    constexpr size_t MAX_CHUNK_COUNT = 33; //a top limit based on size of XLarge files

    enum class ActionType {added, extracted, removed, listed, dumped, compacted};
    enum class AccessMode {AsNew, AsExisting}; //you can change values (but not names) of this enum

    //observer pattern
    struct ArchiveObserver {
       virtual void operator()(ActionType anAction,const std::string &aName, bool status);
    };

    class IDataProcessor {
    public:
        virtual std::vector<uint8_t> process(const std::vector<uint8_t>& input) = 0;
        virtual std::vector<uint8_t> reverseProcess(const std::vector<uint8_t>& input) = 0;
        virtual ~IDataProcessor()=default;
    };

    /** This is new child class of data processor, use it to compress the if add asks for it*/
    class Compression : public IDataProcessor {
    public:
        std::vector<uint8_t> process(const std::vector<uint8_t>& input) override;
        std::vector<uint8_t> reverseProcess(const std::vector<uint8_t>& input) override ;
        ~Compression()= default ;
    };

    enum class ArchiveErrors {
        noError=0,
        fileNotFound=1, fileExists, fileOpenError, fileReadError, fileWriteError, fileCloseError,
        fileSeekError, fileTellError, fileError, badFilename, badPath, badData, badBlock, badArchive,
        badAction, badMode, badProcessor, badBlockType, badBlockCount, badBlockIndex, badBlockData,
        badBlockHash, badBlockNumber, badBlockLength, badBlockDataLength, badBlockTypeLength
    };

    template<typename T>
    class ArchiveStatus {
    public:
        // Constructor for success case
        explicit ArchiveStatus(const T value)
                : value(value), error(ArchiveErrors::noError) {}

        // Constructor for error case
        explicit ArchiveStatus(ArchiveErrors anError)
                : value(std::nullopt), error(anError) {
            if (anError == ArchiveErrors::noError) {
                throw std::logic_error("Cannot use noError with error constructor");
            }
        }

        // Deleted copy constructor and copy assignment to make ArchiveStatus move-only
        ArchiveStatus(const ArchiveStatus&) = delete;
        ArchiveStatus& operator=(const ArchiveStatus&) = delete;

        // Default move constructor and move assignment
        ArchiveStatus(ArchiveStatus&&) noexcept = default;
        ArchiveStatus& operator=(ArchiveStatus&&) noexcept = default;

        T getValue() const {
            if (!isOK()) {
                throw std::runtime_error("Operation failed with error");
            }
            return *value;
        }

        bool isOK() const { return error == ArchiveErrors::noError && value.has_value(); }
        ArchiveErrors getError() const { return error; }

    private:
        std::optional<T> value;
        ArchiveErrors error;
    };

    //Archive interface
    class Archive {
    protected:
        std::vector<std::shared_ptr<IDataProcessor>> processors;
        std::vector<std::shared_ptr<ArchiveObserver>> observers;

        Archive(const std::string &aFullPath, AccessMode aMode);  //protected on purpose
        string thePath;
        std::fstream theArcFile;



    public:
        ~Archive();

        static ArchiveStatus<std::shared_ptr<Archive>> createArchive(const std::string &anArchiveName);
        static ArchiveStatus<std::shared_ptr<Archive>> openArchive(const std::string &anArchiveName);

        bool addObserver(std::shared_ptr<ArchiveObserver> anObserver);

        ArchiveStatus<bool>      add(const std::string &aFilename, IDataProcessor* aProcessor =nullptr);//add file to archive
        ArchiveStatus<bool>      extract(const std::string &aFilename, const std::string &aFullPath);//Extracting a copy of a file from the archive
        ArchiveStatus<bool>      remove(const std::string &aFilename);//Removing a file from the archive (permanently)

        ArchiveStatus<size_t>    list(std::ostream &aStream);//Listing the names of all files in the archive
        ArchiveStatus<size_t>    debugDump(std::ostream &aStream);//Performing a diagnostic "dump" of all the blocks in the file

        ArchiveStatus<size_t>    compact();
        ArchiveStatus<std::string> getFullPath() const; //get archive path (including .arc extension)

        //notify observer of any change
        void notifyObservers(ActionType action, const std::string& filename, bool success);
        static void assign_meta(Chunk &chunk, size_t aPos,const string &aName,size_t compsize = 0);
        static size_t calculateFileSize(const string& aPath);
        static void read_to_vec(vector<uint8_t> &aVec, fstream &aFile); //reads stream data into vector
        static void writeToFile(const std::vector<uint8_t>& vec, std::fstream& file);
        static void renamefile(const string &old, const string &anew);
    };
}

#endif /* Archive_hpp */
