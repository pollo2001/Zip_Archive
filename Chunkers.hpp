//
// Created by gennydawg on 2/26/24.
//


#ifndef Chunkers_hpp
#define Chunkers_hpp


#include <iostream>
#include <fstream>
#include <functional>
#include <cstring>
#include <unordered_map>
#include <array>
#include <utility>
#include <cstdint>
#include "Debug.h"

using namespace std;
namespace ECE141 {

    //block size
    constexpr size_t kChunkSize = 1024;
    constexpr size_t maxFileName = 30;


    // '''''''''''''''''''''''''''''''''''''''--------Chunks

    //pack so that spacing is ideal for inc.
    struct __attribute__((packed)) ChunkHeader {
        ChunkHeader():occupied{false},partNum{0},nextBlock{0},checkSum{0},hashNum{0},filesize{0},dateAdded{0},comp_size{0}{
            memset(name, 0, maxFileName); //init
        }

        ~ChunkHeader()=default;

        uint8_t occupied = false;    // 1 byte, used or not
        uint16_t hashNum;    // 2 bytes, reference number to refer to block instead of filename
        char name[maxFileName];
        time_t dateAdded;
        uint16_t partNum;    // 2 bytes, order number of block, where it fits in sequence
        uint32_t filesize; //track file size
        uint32_t comp_size;  //check compression and size of compression
        uint16_t nextBlock;  // 2 bytes, next index of block continuing data of current
        uint32_t checkSum;   // 4 bytes, validate integrity of block

        // Method to calculate checksum based on data in the block
        uint32_t calc_check_sum();
    };

    //what a block is, two types regular block of data, TOC block, holds metadata
        struct Chunk {
            Chunk() {
                memset(this, 0, sizeof(Chunk));
            };
            ~Chunk()=default;

         static  uint16_t calc_hash(const string& filename);    // Method to calculate hash based on filename

            //members
            ChunkHeader meta; //metadat is 13 bytes
             char data[kChunkSize-sizeof(ChunkHeader)]; //a buffer
    };

//------------------------------------Chunking

    using ChunkCallback = std::function<bool(Chunk&)>; //call back to process each chunk individually
    //making blocks
    //chunker chunks indiscriminately
    struct Chunker {

        Chunker(fstream &anInput);
        ~Chunker()=default;

        //chunking algo
        bool chunk_em(ChunkCallback callback);

    protected:
        std::fstream &input;
    };



} //namespace ece 141
#endif //Chunkers_hpp