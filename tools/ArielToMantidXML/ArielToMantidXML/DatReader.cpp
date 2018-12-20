#include "DatReader.h"
#include <iostream>

extern std::string G_PATH;

DatReader::DatReader(const std::string& filename)
{
  const std::string datfile = G_PATH + filename + ".dat";
  m_datfile.open(datfile.c_str());
  if ( m_datfile.fail() )
  {
    std::cout << "Unable to open file " << datfile  << ". Exiting." << std::endl;
    exit(EXIT_FAILURE);
  }
  //std::cout << "Opened file: " << filename << ".dat" << std::endl;
}

DatReader::~DatReader(void)
{
    m_datfile.close();
}

bool DatReader::isAssembly()
{
  m_datfile.seekg(0);
  std::string line;
  while ( getline(m_datfile,line) )
  {
    if ( line.find("assembly_list") != std::string::npos ) return true;
  }
  return false;
}

const std::string DatReader::findL1()
{
  m_datfile.seekg(0);
  std::string line;
  while ( getline(m_datfile,line) )
  {
    if ( line.find("primary_fpath") != std::string::npos )
    {
      for (int i = 0; i < 4; ++i)
      {
        std::string first, second;
        m_datfile >> first >> second;
        if ( first.find("value") != std::string::npos ) return second;
      }
    }
  }

  // Return an empty string if not found
  return "";
}

