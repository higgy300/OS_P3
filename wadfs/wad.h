//
// Created by juanh on 3/23/2019.
//
#ifndef OS_P3_WAD_H
#define OS_P3_WAD_H
#include <string>
#include <vector>
#include <list>
#include <unordered_map>

using namespace std;

class Wad {
private:
    Wad(unordered_map<string, int> map, unordered_map<int, int> unorderedMap,
            unordered_map<int, vector<pair<string, int>>> map1, unordered_map<int, int> map2, unordered_map<int, int> map3,
            unordered_map<int, int> map4, string str, unsigned long dCount, unsigned long dOffset,
            unordered_map<string, int>_fN, char* _ddata);

    unordered_map<string, int> folderName;
    unordered_map<int, int> folderType;
    unordered_map<int, vector<pair<string, int>>> folderContent;
    unordered_map<int, int> folderParent;
    unordered_map<int, int> fileSize;
    unordered_map<int, int> fileOffset;
    unordered_map<string, int> fileName;
    char* rawData;
    unsigned long descriptorCount;
    unsigned long descriptorOffset;
    string magicType;

public:

    /* Object allocator; creates a Wad object and loads
     * the WAD file data from path into memory. */
    static Wad* loadWad(const std::string &path);

    // Returns the magic for this WAD data.
    char* getMagic();

    // Returns true if path represents content (data), and false otherwise.
    bool isContent(const std::string &path);

    // Returns true if path represents a directory, and false otherwise.
    bool isDirectory(const std::string &path);

    /* If path represents content, returns the number of
     * bytes in its data; otherwise, returns -1. */
    int getSize(const std::string &path);

    /* If path represents content, copies up to length bytes of
     * its data into buffer. If offset is provided, data
     * should be copied starting from that byte in the content.
     * Returns the number of bytes copied into the buffer, or
     * -1 if path does not represent content (e.g., if it represents
     * a directory). */
    int getContents(const std::string &path, char *buffer, int length, int offset = 0);

    /* If path represents a directory, places entries for contained
     * elements in directory. Returns the number of elements in the
     * directory, or -1 if path does not represent a directory
     * (e.g., if it represents content). */
    int getDirectory(const std::string &path, std::vector<std::string> *directory);
};



#endif //OS_P3_WAD_H
