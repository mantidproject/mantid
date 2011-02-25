#include "MantidDataHandling/LoadDaveGrp.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/FileProperty.h"
#include <fstream>
#include <string>
#include <vector>
#include <iostream>

namespace Mantid
{
namespace DataHandling
{
// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(LoadDaveGrp)

    void LoadDaveGrp::init()
{
  std::vector<std::string> exts;
  exts.push_back(".grp");

  this->declareProperty(new API::FileProperty("Filename", "",
      API::FileProperty::Load, exts), "A Dave grouped Ascii file");
  this->declareProperty(new API::WorkspaceProperty<>("OutputWorkspace", "",
      Kernel::Direction::Output),
      "The name of the workspace that will be created.");
}

void LoadDaveGrp::exec()
{
  const std::string filename = this->getProperty("Filename");

  int xLength = 0;
  int yLength = 0;
  int nGroups = 0;

  std::string line;

  std::ifstream ifile(filename.c_str());
  if (ifile.is_open())
  {
    // Skip first line
    //ifile.getline(lineBuffer, lineSize);
    std::getline(ifile, line);
    // Size of x axis
    std::getline(ifile, line);
    this->getAxisLength(line, xLength);
    // Skip next line
    std::getline(ifile, line);
    // Size of y axis
    std::getline(ifile, line);
    this->getAxisLength(line, yLength);
    // This is also the number of groups (spectra)
    nGroups = yLength;
    // Skip next line
    std::getline(ifile, line);
    // Read in the x axis values

  }
  ifile.close();

  // Create workspace
  API::MatrixWorkspace_sptr outputWorkspace = \
      boost::dynamic_pointer_cast<API::MatrixWorkspace>\
      (API::WorkspaceFactory::Instance().create("Workspace2D", nGroups,
          xLength, yLength));

  this->setProperty("OutputWorkspace", outputWorkspace);
}

void LoadDaveGrp::getAxisLength(const std::string &iline, int &length)
{
  std::istringstream is(iline);
  is >> length;
}

} // namespace DataHandling
} // namespace Mantid
