//
// Created by gennydawg on 2/26/24.
//

#include "Chunkers.hpp"

using namespace ECE141;

    //Chunk]
//-------------------------------------------------------------------------------------

// Method to calculate hash based on filename

    uint16_t Chunk::calc_hash(const string& filename) {
      static int max_iterations = 100; // Limit the number of iterations
      uint16_t hash = 0; // Start with 0
      for (char c : filename) {
           hash = (hash * 31) + c;
           if (--max_iterations == 0) break; // Limit the number of iterations
    }
    return hash;
    }

//header methods
    uint32_t ChunkHeader::calc_check_sum(){
        // Implement checksum calculation based on block data
        // For demonstration, let's assume a basic checksum calculation
        uint32_t sum = 0;
        const auto* ptr = reinterpret_cast<const uint8_t*>(this);
        for (size_t i = 0; i < sizeof(ChunkHeader) - sizeof(checkSum); ++i) {
            sum += *ptr++;
        }
        return sum;
    }


//Chunker Class
//--------------------------------------------------------------------------------------
    Chunker::Chunker(fstream &anInput):input{anInput}{}


bool Chunker::chunk_em(ChunkCallback callback) {
    size_t BufferSize = kChunkSize - sizeof(ChunkHeader);
    Chunk chunk;

    while (!input.eof() && input.good()) {
        // Read a chunk of data from the input file
        input.read(chunk.data, BufferSize);

        // Call the callback function with the current chunk
        if (!callback(chunk)) {
            return false;  // Return false if the callback indicates to stop processing
        }
    }
    return true;
}

