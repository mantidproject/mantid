#ifndef ASSREADER_H_
#define ASSREADER_H_

#include <fstream>
#include <string>
#include <vector>

class Component;

class AssReader
{
public:
  AssReader(const std::string& filename);
  virtual ~AssReader(void);

  // Find the next component in the current file
  Component* parseFile();
private:
  std::ifstream m_assfile;
};

#endif
