// A single-header file implementation of file utilities in the style of stb,
// except shittier because std::string makes it C++ only.
//
// To use, define FD_SIMPLE_FILE_IMPLEMENTATION in one cpp file,
// then #include in that file.
// In any other cpp file, #include as necessary.
//
// Easy and super-inefficient!
// Uses google style where string inputs are const char* and outputs are string

#include <string>
#include <fstream>
#include <vector>

#ifndef FD_SIMPLE_FILE_HEADER
#define FD_SIMPLE_FILE_HEADER

#ifdef FD_SIMPLE_FILE_STATIC
#define FDSFIDEF static
#else
#define FDSFIDEF extern
#endif // FD_SIMPLE_FILE_STATIC

//////////////////////
// Function list
FDSFIDEF bool fd_file_to_string(const char* filename, std::string& outBuffer);
FDSFIDEF bool fd_file_to_byte(const char* filename,
    unsigned char** outBuffer, size_t& outSize);
FDSFIDEF bool fd_file_to_vec(const char* filename,
    std::vector<unsigned char>& outBuffer);
FDSFIDEF bool fd_file_write_vec(const char* filename,
    std::vector<unsigned char>& buffer);

#endif // FD_SIMPLE_FILE_HEADER

#ifdef FD_SIMPLE_FILE_IMPLEMENTATION

bool fd_file_to_string(const char* filename, std::string& outBuffer) {
  if (NULL == filename) return false;
  std::ifstream file(filename, std::ios::in | std::ios::binary);
  if (!file || !file.is_open()) return false;
  
  file.seekg(0, std::ios::end);
  outBuffer.resize(static_cast<unsigned int>(file.tellg()));
  file.seekg(0, std::ios::beg);
  file.read(&outBuffer[0], outBuffer.size());
  file.close();
  return true;
}

bool fd_file_to_byte(const char* filename,
    unsigned char** outBuffer, unsigned int& outSize) {
  if (NULL == filename) return false;
  std::ifstream file(filename, std::ios::in | std::ios::binary);
  if (!file || !file.is_open()) return false;
  
  file.seekg(0, std::ios::end);
  outSize = (size_t)file.tellg();
  *outBuffer = new unsigned char[outSize];
  file.seekg(0, std::ios::beg);
  file.read((char*)*outBuffer, outSize);
  file.close();
  return true;
}

bool fd_file_to_vec(const char* filename,
    std::vector<unsigned char>& outBuffer) {
  if (NULL == filename) return false;
  std::ifstream file(filename, std::ios::in | std::ios::binary);
  if (!file || !file.is_open()) return false;
  
  file.seekg(0, std::ios::end);
  outBuffer.resize((size_t)file.tellg());
  file.seekg(0, std::ios::beg);
  file.read((char*)&(outBuffer[0]), outBuffer.size());
  file.close();
  return true;
}

bool fd_file_write_vec(const char* filename,
    std::vector<unsigned char>& buffer) {
  if (NULL == filename) return false;

  std::ofstream file(filename, std::ios::out | std::ios::binary);
  if(!file || !file.is_open()) return false;

  file.write((char*)&buffer[0], buffer.size());
  file.close();
  return true;
}


#endif // FD_SIMPLE_FILE_IMPLEMENTATION
