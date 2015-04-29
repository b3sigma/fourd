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

#ifndef FD_SIMPLE_FILE_HEADER
#define FD_SIMPLE_FILE_HEADER

#ifdef FD_SIMPLE_FILE_STATIC
#define FDSFIDEF static
#else
#define FDSFIDEF extern
#endif // FD_SIMPLE_FILE_STATIC

//////////////////////
// Function list
FDSFIDEF std::string fd_file_to_string(const char* filename);

#endif // FD_SIMPLE_FILE_HEADER

#ifdef FD_SIMPLE_FILE_IMPLEMENTATION

bool fd_file_to_string(const char* filename, std::string& buffer) {
  if (NULL == filename) return false;
  std::ifstream file(filename, std::ios::in | std::ios::binary);
  if (!file || !file.is_open()) return false;
  
  file.seekg(0, std::ios::end);
  buffer.resize(static_cast<unsigned int>(file.tellg()));
  file.seekg(0, std::ios::beg);
  file.read(&buffer[0], buffer.size());
  file.close();
  return true;
}

#endif // FD_SIMPLE_FILE_IMPLEMENTATION
