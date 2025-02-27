#include <utility>

#include <utility>

#include <utility>

#include <utility>

#include <utility>

#include <utility>

//
// Created by juanh on 3/23/2019.
//
#include <fstream>
#include <cstdio>
#include <cstring>
#include <iostream>
#include "wad.h"

#define ROOT 0
#define FOLDER_LENGTH 0
#define MAP_MARKER 1
#define NAMESPACE_MARKER 2

Wad::Wad(unordered_map<string, int> _name, unordered_map<int, int> _type,
         unordered_map<int, vector<pair<string, int>>> _content, unordered_map<int, int> _parent,
         unordered_map<int, int> _size, unordered_map<int, int> _offset, string _magic, unsigned long _dCount,
         unsigned long _dOffset, unordered_map<string, int> _fileN, char* __data) {
    folderName = std::move(_name);
    folderType = std::move(_type);
    folderContent = std::move(_content);
    folderParent = std::move(_parent);
    fileSize = std::move(_size);
    fileOffset = std::move(_offset);
    magicType = std::move(_magic);
    descriptorOffset = _dOffset;
    descriptorCount = _dCount;
    fileName = std::move(_fileN);
    rawData = __data;
}

Wad *Wad::loadWad(const std::string &path) {
    // Load file
    std::ifstream wad_binary(path, std::ifstream::binary);

    // Create temporary hash tables to store structured directory
    unordered_map<string, int> _folder_name;
    unordered_map<int, int> _folder_type;
    unordered_map<int, vector<pair<string, int>>> _folder_content;
    unordered_map<int, int> _folder_parent;
    unordered_map<int, int> _file_size;
    unordered_map<int, int> _file_offset;
    unordered_map<string, int> _file_name;

    bool initialized = false;

    // Create Wad object but not initialize it until after the directory is structured
    Wad* _obj;

    if (wad_binary) {
        initialized = true;
        wad_binary.seekg(0, std::ifstream::end);
        auto len = wad_binary.tellg();
        wad_binary.seekg(0, std::ifstream::beg);

        // Allocate space to store raw wad data as char
        char* rawWadBuffer = new char[len];

        // Load the wad binary data into the buffer
        wad_binary.read(rawWadBuffer, len);

        // Retrieve the file type (IWAD OR PWAD)
        std::string magic = "";

        for (int i = 0; i < 4; i++)
            magic += rawWadBuffer[i];

        // Retrieve the number of descriptors in WAD file
        unsigned long _descriptorCount = 0L;
        int shiftAmount = 0;

        for (int i = 4; i < 8; i++) {
            _descriptorCount |=  (unsigned char)rawWadBuffer[i] << shiftAmount;
            shiftAmount += 8;
        }

        // Retrieve descriptor offset (Where I can find this data in the WAD file)
        unsigned long _descriptorOffset = 0L;
        shiftAmount = 0;

        for (int i = 8; i < 12; i++) {
            _descriptorOffset |= (unsigned char)rawWadBuffer[i] << shiftAmount;
            shiftAmount += 8;
        }

        /*// Debug printing to see if file header is correct
        std::cout << "Magic: " << magic << "\nDescriptor count: " << _descriptorCount
        << "\nOffset: " << _descriptorOffset << "\nFile length: " << len << std::endl;
        std::cout << std::endl; */

        // All these variables are to retrieve all map markers and namespace markers
        shiftAmount = 0;
        bool eleOffFlag = false, eleLenFlag = true, eleNameFlag = true,
            isFolder = false, mapMarker = false, addFile = true;
        int letterCount = 0, fileID = 0, folderID = -1, mapMarkerCount = 0, parentID = -1,
            file_type = 0, currentFolderID = 0, _root_ = 0;
        unsigned long eOffset = 0L, eLength = 0L;
        std::string eName = "", folder_name = "", currentFolderName = "", parentName = "";

        // Create root directory
        _folder_name.insert(make_pair("root", ++folderID));
        _folder_type.insert(make_pair(folderID, ROOT));
        _folder_content.insert(make_pair(folderID, vector<pair<string, int>>()));
        currentFolderName = "root";
        currentFolderID = folderID;

        for (int i = (int)_descriptorOffset; i < len; i++) {
            if (!eleOffFlag) {
                eOffset |= (unsigned char) rawWadBuffer[i] << shiftAmount;
                shiftAmount += 8;
                if (shiftAmount == 32) {
                    shiftAmount = 0;
                    eleOffFlag = true;
                    eleLenFlag = false;
                    ++i;
                }
            }
            if (!eleLenFlag) {
                eLength |= (unsigned char) rawWadBuffer[i] << shiftAmount;
                shiftAmount += 8;
                if (shiftAmount == 32) {
                    shiftAmount = 0;
                    eleLenFlag = true;
                    eleNameFlag = false;
                    ++i;
                    if (eLength == FOLDER_LENGTH)
                        isFolder = true;
                }
            }
            if (!eleNameFlag) {
                eName += rawWadBuffer[i];
                ++letterCount;
                if (letterCount == 8) {
                    letterCount = 0;
                    eleNameFlag = true;
                    eleOffFlag = false;

                    if (isFolder) {
                        isFolder = false;
                        if (eName[0] == 'E' && eName[1] >= '0' && eName[1] <= '9' && eName[2] == 'M' && eName[3] >= '0' && eName[3] <= '9') {
                            mapMarker = true;
                            addFile = true;
                            mapMarkerCount = 0;
                            file_type = MAP_MARKER;
                            parentID = currentFolderID;
                            folder_name = eName.substr(0, 4);
                        } else if (eName.find("_START") != string::npos) {
                            addFile = true;
                            file_type = NAMESPACE_MARKER;
                            folder_name = eName.substr(0, eName.find("_START"));
                            parentID = currentFolderID;
                        } else if (eName.find("_END") != string::npos) {
                            addFile = false;
                            folder_name = eName.substr(0, eName.find("_END"));
                            auto parentLookUp = _folder_parent.find(currentFolderID);
                            currentFolderID = parentLookUp->second;
                        }

                        if (addFile) {
                            // Add new folder name to parent's folder content
                            _folder_content[parentID].emplace_back(make_pair(folder_name, ++fileID));

                            // Add file name and file ID to its own hash table for quick random file lookup
                            _file_name.insert(make_pair(folder_name, fileID));

                            // Update parent ID and current folder ID
                            parentID = currentFolderID;
                            currentFolderID = ++folderID;

                            // Add folder name to hash table to create new folder ID
                            _folder_name.insert(make_pair(folder_name, folderID));

                            // fill in new folder type in hash table
                            _folder_type.insert(make_pair(folderID, file_type));

                            // Create new folder content space
                            _folder_content.insert(make_pair(folderID, vector<pair<string, int>>()));

                            // Store new folder's parent ID to associate depth level relation
                            _folder_parent.insert(make_pair(folderID, parentID));

                            // Store folder's size and offset in LUT
                            _file_size.insert(make_pair(fileID, eLength));
                            _file_offset.insert(make_pair(fileID, eOffset));
                        } else {
                            addFile = true;
                        }
                    } else {
                        // creating new file ID and adding its size and offset to hash table
                        _file_size.insert(make_pair(++fileID, eLength));
                        _file_offset.insert(make_pair(fileID, eOffset));

                        // Add file name and file ID to its own hash table for quick random file lookup
                        _file_name.insert(make_pair(eName, fileID));

                        // Add file to current folder's content space
                        _folder_content[currentFolderID].emplace_back(make_pair(eName, fileID));

                        // Check if the file belongs to a map marker.
                        // If it does, update the count for the 10 files it is supposed
                        // to have
                        if (mapMarker) {
                            cout << eOffset << " " << eLength << " " << eName << endl;
                            ++mapMarkerCount;
                            if (mapMarkerCount == 10) {
                                mapMarker = false;
                                // Make current folder be the parent
                                currentFolderID = _root_;
                            }
                        }
                    }

                    eName = "";
                    eOffset = 0;
                    eLength = 0;
                    folder_name = "";
                }
            }
        }
        cout << endl;
        _obj = new Wad(_folder_name, _folder_type, _folder_content,
                       _folder_parent, _file_size, _file_offset, magic, _descriptorCount,
                       _descriptorOffset, _file_name, rawWadBuffer);
    }
    return (initialized) ? _obj : nullptr;
}

char *Wad::getMagic() {
    char* magic = new char[5];

    for (int i = 0; i < magicType.length(); i++)
        magic[i] = magicType[i];
    magic[5] = '\0';

    return magic;
}

bool Wad::isContent(const std::string &path) {
    // check if path is root folder
    if (path.length() == 1)
        return false;

    // Container to use to store words after removing demiliter '/'
    vector<string> names;
    string placeholder = "";

    // Build words located between '/' and add them to the list
    for (char ch : path) {
        if (ch == '/') {
            if (!placeholder.empty()) {
                names.push_back(placeholder);
                placeholder = "";
            }
        } else
            placeholder += ch;

    }
    // Check for lingering word
    if (!placeholder.empty())
        names.push_back(placeholder);


    // Check every word in the list for existence in the hash table
    // If any fail then return -1. Else check if last word is content.
    for (int i = 0; i < names.size(); i++) {
        auto itr = folderName.find(names[i]);
        // Check if word is folder
        if (itr == folderName.end()) {
            if (i == names.size() - 1) {
                auto itrr = fileName.find(names[i]);
                return !(itrr == fileName.end());
            } else
                return false;
        } else {
            if (i == names.size() - 1)
                return false;
        }
    }
    return false;
}

bool Wad::isDirectory(const std::string &path) {
    // Check if path is root folder
    if (path.length() == 1)
        return true;

    // Container to use to store words after removing demiliter '/'
    vector<string> names;
    string placeholder = "";

    // Build words located between '/' and add them to the list
    for (char ch : path) {
        if (ch == '/') {
            if (!placeholder.empty()) {
                names.push_back(placeholder);
                placeholder = "";
            }
        } else
            placeholder += ch;

    }
    // Check for lingering word
    if (!placeholder.empty())
        names.push_back(placeholder);

    // Check every word in the list for existence in the hash table
    // If any fail then return -1. Else check if last word is content.
    for (int i = 0; i < names.size(); i++) {
        auto itr = folderName.find(names[i]);
        // Check if word is folder
        if (itr == folderName.end()) {
            return false;
        } else {
            if (i == names.size() - 1)
                return true;
        }
    }
    return false;
}

int Wad::getSize(const std::string &path) {
    // check if path is root folder
    if (path.length() == 1)
        return -1;

    // Container to use to store words after removing demiliter '/'
    vector<string> names;
    string placeholder = "";

    // Build words located between '/' and add them to the list
    for (char ch : path) {
        if (ch == '/') {
            if (!placeholder.empty()) {
                names.push_back(placeholder);
                placeholder = "";
            }
        } else
            placeholder += ch;

    }
    // Check for lingering word
    if (!placeholder.empty())
        names.push_back(placeholder);


    // Check every word in the list for existence in the hash table
    // If any fail then return -1. Else check if last word is content.
    for (int i = 0; i < names.size(); i++) {
        auto itr = folderName.find(names[i]);
        // Check if word is folder
        if (itr == folderName.end()) {
            if (i == names.size() - 1) {
                auto itrr = fileName.find(names[i]);
                if (itrr == fileName.end()) {
                    return -1;
                } else {
                    int _id = itrr->second;
                    auto itrrr = fileSize.find(_id);
                    return itrrr->second;
                }
            } else
                return -1;
        } else {
            if (i == names.size() - 1)
                return -1;
        }
    }
    return -1;
}

int Wad::getContents(const std::string &path, char *buffer, int length, int offset) {
    if (offset > 0) {

        for (int i = offset; i < offset + length; i++)
            buffer += rawData[i];
        return length;
    }

    // Check if path is content. If it is, retrieve list of words and grab last one
    // and look it up on hash table
    if (isContent(path)) {
        // Container to use to store words after removing demiliter '/'
        vector<string> names;
        string placeholder = "";

        // Build words located between '/' and add them to the list
        for (char ch : path) {
            if (ch == '/') {
                if (!placeholder.empty()) {
                    names.push_back(placeholder);
                    placeholder = "";
                }
            } else
                placeholder += ch;

        }
        // Check for lingering word
        if (!placeholder.empty())
            names.push_back(placeholder);

        // Do hash table lookup of last word and retrieve content
        string name = names[names.size() - 1];
        auto idItr = fileName.find(name);
        int _id_ = idItr->second;
        auto sizeItr = fileSize.find(_id_);
        auto _size_ = static_cast<unsigned long>(sizeItr->second);
        auto offsetItr = fileOffset.find(_id_);
        auto _offset_ = static_cast<unsigned long>(offsetItr->second);

        // format offset and size to little-endianness
        _offset_ = (_offset_ << 24) |  ((_offset_ << 8) & 0x00FF0000) |
                ((_offset_ >> 8) & 0x0000FF00) | (_offset_ >> 24);
        _size_ = (_size_ << 24) |  ((_size_ << 8) & 0x00FF0000) |
                 ((_size_ >> 8) & 0x0000FF00) | (_size_ >> 24);

        // Cast it to be able to store it in the buffer
        unsigned char* const offsetArr = reinterpret_cast<unsigned char* const>(&_offset_);
        unsigned char* const sizeArr = reinterpret_cast<unsigned char* const>(&_size_);

        // Store it in the buffer
        int size = static_cast<int>(8 + name.length());
        buffer = new char[size];

        unsigned char temp;
        temp = offsetArr[3];
        offsetArr[3] = offsetArr[0];
        offsetArr[0] = temp;
        temp = offsetArr[2];
        offsetArr[2] = offsetArr[1];
        offsetArr[1] = temp;

        temp = sizeArr[3];
        sizeArr[3] = sizeArr[0];
        sizeArr[0] = temp;
        temp = sizeArr[2];
        sizeArr[2] = sizeArr[1];
        sizeArr[1] = temp;


        for (int i = 0; i < 4; i++) {
            buffer[i] = offsetArr[i];
            buffer[i + 4] = sizeArr[i];
        }

        for (int i = 0; i < name.length(); i++)
            buffer[i + 8] = name[i];

        return size;
    } else
        return -1;
}

int Wad::getDirectory(const std::string &path, std::vector<std::string> *directory) {

    if (!isDirectory(path))
        return -1;
    // Check if path is root folder
    if (path.length() == 1) {
        if (path[0] == '/') {
            vector<pair<string, int>> temp;
            auto itr = folderContent.find(0);
            temp = itr->second;
            for (auto &entry : temp) {
                directory->push_back(entry.first);
            }
            return static_cast<int>(temp.size());
        } else {
            return -1;
        }
    }

    // TODO: Convert from here to end of function to match what this function is supposed to do
    // Container to use to store words after removing demiliter '/'
    vector<string> names;
    string placeholder = "";

    // Build words located between '/' and add them to the list
    for (char ch : path) {
        if (ch == '/') {
            if (!placeholder.empty()) {
                names.push_back(placeholder);
                placeholder = "";
            }
        } else
            placeholder += ch;

    }
    // Check for lingering word
    if (!placeholder.empty())
        names.push_back(placeholder);

    // Check every word in the list for existence in the hash table
    // If any fail then return -1. Else check if last word is content.
    auto itr = folderName.find(names[names.size() - 1]);
    int _id_ = itr->second;
    auto itrr = folderContent.find(_id_);
    vector<pair<string, int>> temp;
    temp = itrr->second;
    for (auto &entry : temp) {
        directory->push_back(entry.first);
    }
    return static_cast<int>(temp.size());
}


