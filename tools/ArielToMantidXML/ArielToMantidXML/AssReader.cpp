// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "AssReader.h"
#include "Component.h"
#include <iostream>

extern std::string G_PATH;

AssReader::AssReader(const std::string& filename)
{
  const std::string assfile = G_PATH + filename + ".ass";
  m_assfile.open(assfile.c_str());
  if ( m_assfile.fail() )
  {
    std::cout << "Unable to open file " << assfile << ". Exiting." << std::endl;
    exit(EXIT_FAILURE);
  }
  m_assfile.exceptions( std::fstream::eofbit );
  //std::cout << "Opened file: " << filename << ".ass" << std::endl;
}

AssReader::~AssReader(void)
{
  m_assfile.close();
}

Component* AssReader::parseFile()
{
  try {
    std::string line;
    while ( getline( m_assfile, line) )
    {
      if ( line.find("*") == 0 )
      {
        std::string name;
        m_assfile >> name;
        std::string type;
        m_assfile >> type;
        double r,theta,phi;
        m_assfile >> phi >> theta >> r;

        Component *comp = new Component(name, type);
        comp->setSpherical(r,theta,phi);
        return comp;
      }
    }
  }
  catch (std::fstream::failure)
  {}

  return NULL;
}

