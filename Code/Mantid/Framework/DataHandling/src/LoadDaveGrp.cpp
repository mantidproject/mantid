#include "MantidDataHandling/LoadDaveGrp.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/NumericAxis.h"
#include <vector>
#include <iostream>

namespace Mantid
{
namespace DataHandling
{
// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(LoadDaveGrp)

LoadDaveGrp::LoadDaveGrp() : ifile(), line()
{
}

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

  MantidVec *xAxis = new MantidVec();
  MantidVec *yAxis = new MantidVec();

  //std::vector<MantidVec *> *data = new std::vector<MantidVec *>();
  //std::vector<MantidVec *> *errors = new std::vector<MantidVec *>();

  this->ifile.open(filename.c_str());
  if (this->ifile.is_open())
  {
    // Size of x axis
    this->getAxisLength(xLength);
    // Size of y axis
    this->getAxisLength(yLength);
    // This is also the number of groups (spectra)
    nGroups = yLength;
    // Read in the x axis values
    this->getAxisValues(xAxis, static_cast<std::size_t>(xLength));
    // Read in the y axis values
    this->getAxisValues(yAxis, static_cast<std::size_t>(yLength));
  }
  this->ifile.close();

  // Create workspace
  API::MatrixWorkspace_sptr outputWorkspace = \
      boost::dynamic_pointer_cast<API::MatrixWorkspace>\
      (API::WorkspaceFactory::Instance().create("Workspace2D", nGroups,
          xLength, yLength));

  API::Axis* const verticalAxis = new API::NumericAxis(yLength);
  outputWorkspace->replaceAxis(1, verticalAxis);

  for(std::size_t i = 0; i < static_cast<std::size_t>(nGroups); i++)
  {
    outputWorkspace->dataX(i) = *xAxis;
    verticalAxis->setValue(static_cast<const int>(i), yAxis->at(i));
  }

  this->setProperty("OutputWorkspace", outputWorkspace);
}

void LoadDaveGrp::readLine()
{
  std::getline(this->ifile, this->line);
}

void LoadDaveGrp::getAxisLength(int &length)
{
  // Skip a comment line
  this->readLine();
  // Get the axis length from the file
  this->readLine();
  std::istringstream is(this->line);
  is >> length;
}

void LoadDaveGrp::getAxisValues(MantidVec *axis, const std::size_t length)
{
  // Skip a comment line
  this->readLine();
  // Get the axis values from the file
  double value;
  for(std::size_t i = 0; i < length; i++)
  {
    this->readLine();
    std::istringstream is(this->line);
    is >> value;
    axis->push_back(value);
  }
}

} // namespace DataHandling
} // namespace Mantid
