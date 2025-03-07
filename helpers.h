//
// Created by gennydawg on 3/14/24.
//

#ifndef ECE141_ARCHIVE_HELPERS_H
#define ECE141_ARCHIVE_HELPERS_H

#include <string>
#include <filesystem>
#include <Chunkers.hpp>

namespace ECE141 {

//helpers used in archive

    inline bool has_arc_ext(const string &aName) {
        std::string arc = ".arc";
        if (aName.length() < arc.length())
            return false;

        return aName.compare(aName.length() - arc.length(), arc.length(), arc) == 0;
    }

    inline string extractFilename(const std::string &filePath) {
        // Create a path object from the file path
        std::filesystem::path path(filePath);

        // Return the filename component of the path
        return path.filename().string();
    }

//will change to reflect true size
    inline size_t closestMultipleOf1024(size_t value) {
        if (!value) return value; // Special case for 0
        size_t closestMult = 0;
        size_t payload = kChunkSize - sizeof(ChunkHeader); //payload size, precision

        if (value % payload) {
            closestMult = kChunkSize * (1 + value / payload); //need an extra block of space
        } else
            closestMult = kChunkSize * (value / payload);

        return closestMult;
    }
}
#endif //ECE141_ARCHIVE_HELPERS_H
