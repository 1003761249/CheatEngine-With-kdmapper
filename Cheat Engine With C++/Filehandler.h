#pragma once
#include <minwindef.h>
#include <string>
#include <sstream> // Add this include for std::stringstream

using namespace std;

BOOL ReadProcessMemoryFile(HANDLE hProcess, LPCVOID lpBaseAddress, LPVOID lpBuffer, DWORD nSize, uintptr_t* lpNumberOfBytesRead);
BOOL WriteProcessMemoryFile(HANDLE hProcess, LPCVOID lpBaseAddress, LPCVOID lpBuffer, DWORD nSize, uintptr_t* lpNumberOfBytesWritten);
DWORD VirtualQueryExFile(HANDLE hProcess, LPCVOID lpAddress, PMEMORY_BASIC_INFORMATION lpBuffer, uintptr_t dwLength);

class FileHandler
{
public:
    string filename;
    fstream* filedata; // Change TMemorystream to stringstream
    uintptr_t* filebaseaddress;

    
    
    FileHandler();
    ~FileHandler();
};