#ifndef DATREADER_H_
#define DATREADER_H_

#include <fstream>
#include <string>

class DatReader
{
public:
  DatReader(const std::string& filename);
  ~DatReader(void);

  // Flag indicating whether this is a composite type (is it "type::  assembly_list")
  bool isAssembly();
  // Find the primary flight path. Only works if we're looking at 'instrument.dat'
  const std::string findL1();

private:
  std::ifstream m_datfile;
};

#endif
