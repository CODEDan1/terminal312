#include <windows.h>
#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <functional>
#include <fstream>

std::string currentDir = "C:\\";

// Helper: Print last error code & message
void printLastError() {
    DWORD errCode = GetLastError();
    LPVOID errMsg;
    FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, errCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPSTR)&errMsg, 0, NULL);
    std::cout << "Error (" << errCode << "): " << (char*)errMsg << "\n";
    LocalFree(errMsg);
}

void listDirectory(const std::string& path) {
    WIN32_FIND_DATAA findFileData;
    HANDLE hFind = FindFirstFileA((path + "\\*").c_str(), &findFileData);

    if (hFind == INVALID_HANDLE_VALUE) {
        std::cout << "Directory not found.\n";
        printLastError();
        return;
    }

    do {
        std::cout << (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ? "[DIR] " : "      ")
                  << findFileData.cFileName << "\n";
    } while (FindNextFileA(hFind, &findFileData) != 0);

    FindClose(hFind);
}

void printTree(const std::string& path, int depth = 0) {
    WIN32_FIND_DATAA findData;
    HANDLE hFind = FindFirstFileA((path + "\\*").c_str(), &findData);

    if (hFind == INVALID_HANDLE_VALUE)
        return;

    do {
        if (strcmp(findData.cFileName, ".") == 0 || strcmp(findData.cFileName, "..") == 0)
            continue;

        for (int i = 0; i < depth; ++i) std::cout << "  ";
        std::cout << (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ? "[D] " : "- ")
                  << findData.cFileName << "\n";

        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            printTree(path + "\\" + findData.cFileName, depth + 1);
        }

    } while (FindNextFileA(hFind, &findData));

    FindClose(hFind);
}

void changeDirectory(const std::string& path) {
    std::string fullPath = path;

    if (path[0] != '\\' && path.find(':') == std::string::npos) {
        fullPath = currentDir + "\\" + path;
    }

    if (SetCurrentDirectoryA(fullPath.c_str())) {
        char buffer[MAX_PATH];
        GetCurrentDirectoryA(MAX_PATH, buffer);
        currentDir = buffer;
    } else {
        std::cout << "Directory not found.\n";
        printLastError();
    }
}

int main() {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN | FOREGROUND_INTENSITY);

    std::cout << "Terminal312 [Version 1.00]\n";
std::cout << "(c) abcd312. All rights reserved.\n\n";

    SetConsoleTitleA("Terminal312");
    SetCurrentDirectoryA(currentDir.c_str());

    while (true) {
        std::cout << currentDir << "> ";
        std::string command;
        std::getline(std::cin, command);

        if (command.substr(0, 2) == "cd") {
            std::string path = command.length() > 3 ? command.substr(3) : "";
            if (!path.empty()) changeDirectory(path);

        } else if (command == "dir") {
            listDirectory(currentDir);

        } else if (command == "tree") {
            printTree(currentDir);

        } else if (command.rfind("create", 0) == 0 && command.substr(0, 7) != "createx") {
            std::istringstream iss(command);
            std::string cmd, type, name;
            iss >> cmd >> type >> name;

            if (type == "file") {
                if (name.empty()) {
                    std::cout << "File name missing!\n";
                } else {
                    std::string filePath = currentDir + "\\" + name;
                    HANDLE hFile = CreateFileA(filePath.c_str(), GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
                    if (hFile == INVALID_HANDLE_VALUE) {
                        std::cout << "Failed to create file. It might already exist.\n";
                        printLastError();
                    } else {
                        std::cout << "File created: " << name << "\n";
                        CloseHandle(hFile);
                    }
                }
            } else if (type == "folder") {
                if (name.empty()) {
                    std::cout << "Folder name missing!\n";
                } else {
                    std::string folderPath = currentDir + "\\" + name;
                    if (CreateDirectoryA(folderPath.c_str(), NULL)) {
                        std::cout << "Folder created: " << name << "\n";
                    } else {
                        std::cout << "Failed to create folder. It might already exist.\n";
                        printLastError();
                    }
                }
            } else {
                std::cout << "Invalid type. Use 'file' or 'folder'.\n";
            }

        } else if (command.rfind("createx", 0) == 0) {
            std::istringstream iss(command);
            std::string cmd, type, name, forceArg;
            iss >> cmd >> type >> name >> forceArg;

            bool force = (forceArg == "force");

            if (type == "file") {
                if (name.empty()) {
                    std::cout << "File name missing!\n";
                } else {
                    std::string filePath = currentDir + "\\" + name;
                    if (force) {
                        SetFileAttributesA(filePath.c_str(), FILE_ATTRIBUTE_NORMAL);
                    }
                    if (DeleteFileA(filePath.c_str())) {
                        std::cout << "File deleted: " << name << "\n";
                    } else {
                        std::cout << "Failed to delete file.\n";
                        printLastError();
                    }
                }
            } else if (type == "folder") {
                if (name.empty()) {
                    std::cout << "Folder name missing!\n";
                } else {
                    std::string folderPath = currentDir + "\\" + name;

                    if (force) {
                        // recursive delete helper
                        std::function<bool(const std::string&)> removeFolderRecursive;
                        removeFolderRecursive = [&](const std::string& path) -> bool {
                            WIN32_FIND_DATAA findData;
                            HANDLE hFind = FindFirstFileA((path + "\\*").c_str(), &findData);

                            if (hFind == INVALID_HANDLE_VALUE)
                                return false;

                            bool success = true;

                            do {
                                std::string fname = findData.cFileName;
                                if (fname == "." || fname == "..")
                                    continue;

                                std::string fullPath = path + "\\" + fname;
                                if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                                    success &= removeFolderRecursive(fullPath);
                                } else {
                                    SetFileAttributesA(fullPath.c_str(), FILE_ATTRIBUTE_NORMAL);
                                    success &= DeleteFileA(fullPath.c_str());
                                }
                            } while (FindNextFileA(hFind, &findData));

                            FindClose(hFind);

                            SetFileAttributesA(path.c_str(), FILE_ATTRIBUTE_NORMAL);
                            success &= RemoveDirectoryA(path.c_str());
                            return success;
                        };

                        if (removeFolderRecursive(folderPath)) {
                            std::cout << "Folder and contents force deleted: " << name << "\n";
                        } else {
                            std::cout << "Failed to force delete folder.\n";
                            printLastError();
                        }
                    } else {
                        if (RemoveDirectoryA(folderPath.c_str())) {
                            std::cout << "Folder deleted: " << name << "\n";
                        } else {
                            std::cout << "Failed to delete folder. It may not be empty.\n";
                            printLastError();
                        }
                    }
                }
            } else {
                std::cout << "Invalid type. Use 'file' or 'folder'.\n";
            }

        } else if (command == "exit") {
            break;
            

        } 
        
        else if (command.rfind("input", 0) == 0) {
    std::istringstream iss(command);
    std::string cmd, filename;
    iss >> cmd >> filename;

    if (filename.empty()) {
        std::cout << "Missing filename!\n";
        continue;
    }

    std::string fullPath = currentDir + "\\" + filename;
    std::ofstream file(fullPath, std::ios::app); // append mode

    if (!file.is_open()) {
        std::cout << "Failed to open file for writing.\n";
        printLastError();
        continue;
    }

    std::cout << "Enter your text below (type blank line to finish, use \\\\n for newlines):\n";
    std::string line;
    while (true) {
        std::getline(std::cin, line);
        if (line.empty()) break;

        size_t pos = 0;
        while ((pos = line.find("\\\\n", pos)) != std::string::npos) {
            line.replace(pos, 3, "\n");
            pos += 1; // move forward
        }
        file << line << "\n";
    }

    file.close();
    std::cout << "Text written to " << filename << "\n";


        
} 

else if (command == "aapl") {
    std::cout << "aapl :3" << std::endl;
}

else if (command.rfind("preview", 0) == 0) {
    std::istringstream iss(command);
    std::string cmd, typeOrFile, filename;
    iss >> cmd >> typeOrFile;

    bool isBitmapMode = false;

    // Handle bitmap special case
    if (typeOrFile == "bitmap") {
        iss >> filename;
        isBitmapMode = true;
    } else {
        filename = typeOrFile;
    }

    if (filename.empty()) {
        std::cout << "Missing filename!\n";
        continue;
    }

    std::string fullPath = currentDir + "\\" + filename;
    std::ifstream file(fullPath, std::ios::binary);

    if (!file.is_open()) {
        std::cout << "Failed to open file.\n";
        printLastError();
        continue;
    }

    if (isBitmapMode) {
        // Check BMP header
        char header[2];
        file.read(header, 2);
        if (header[0] != 'B' || header[1] != 'M') {
            std::cout << "This is not a valid bitmap file.\n";
            file.close();
            continue;
        }

        std::cout << "--- Bitmap Detected. Dumping Full Hex ---\n";
        file.seekg(0, std::ios::beg); // rewind to start

        char byte;
        int count = 0;
        while (file.get(byte)) {
            printf("%02X ", (unsigned char)byte);
            if (++count % 16 == 0) std::cout << "\n";
        }
        std::cout << "\n[End of Bitmap Hex Dump]\n";
        file.close();
        continue;
    }

    // Try reading as text
    std::string contents((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    // Check if it's readable text
    bool isText = true;
    for (char c : contents) {
        if ((unsigned char)c < 0x09 || ((unsigned char)c > 0x0D && (unsigned char)c < 0x20)) {
            isText = false;
            break;
        }
    }

    if (isText) {
        std::cout << "--- File Preview (Text) ---\n";
        std::cout << contents << "\n";
    } else {
        std::cout << "--- File Preview (Hex Dump - Capped) ---\n";
        file.open(fullPath, std::ios::binary);
        if (!file) {
            std::cout << "Could not reopen file for hex.\n";
            continue;
        }

        char byte;
        int count = 0;
        while (file.get(byte) && count < 512) {
            printf("%02X ", (unsigned char)byte);
            if (++count % 16 == 0) std::cout << "\n";
        }
        std::cout << "\n[Hex dump capped at 512 bytes]\n";
        file.close();
    }
}

else if (command == "diskpart") {
    std::string confirm;
    std::cout << "Are you sure you want to open DISKPART? (Y/N): ";
    std::getline(std::cin, confirm);
    if (confirm == "Y" || confirm == "y") {
        system("start diskpart");
    } else {
        std::cout << "diskpart cancelled.\n";
    }
}


else if (command == "cat") {
    std::cout << "meow meow meow meow meow :3\n";
}

else if (command == "dog") {
    std::cout << "woof woof woof woof woof :>\n";
}

else if (command == "abcd312") {
    std::cout << "abcd312 is a 12-year-old beginner programmer who created this terminal! :D\n";
}

else if (command.rfind("debunk", 0) == 0) {
    std::string arg = command.substr(7); // grab after "debunk "
    if (arg == "meow meow meow meow meow :3") {
        std::cout << "the creator of this terminal is nothing but a furry femboy and therian X3\n";
    } else if (arg == "woof woof woof woof woof :>") {
        std::cout << "the noise my cute dog makes X3\n";
    } else {
        std::cout << "hmmm... nothing to debunk here.\n";
    }
}


else if (command == "credits") {
    std::cout << "coded by abcd312, running in 312-dos ;3\n";

} else {
            std::cout << "'" << command << "' is not recognized.\n";
        }
    }

    return 0;
}
