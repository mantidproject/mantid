#include "MantidDataHandling/LoadDaveGrp.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/UnitFactory.h"

#include <vector>
#include <iostream>

namespace Mantid
{
namespace DataHandling
{
// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(LoadDaveGrp)

LoadDaveGrp::LoadDaveGrp() : ifile(), line(), nGroups(0), xLength(0)
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
  // Extract the current contents of the UnitFactory to be the allowed values
  // of the X-Axis property
  this->declareProperty("X-Axis", "",
      new Kernel::ListValidator(Kernel::UnitFactory::Instance().getKeys()),
    "The name of the units for the X-Axis (must be one of those registered in\n"
    "the Unit Factory)");
  // Extract the current contents of the UnitFactory to be the allowed values
  // of the Y-Axis property
  this->declareProperty("Y-Axis", "MomentumTransfer",
      new Kernel::ListValidator(Kernel::UnitFactory::Instance().getKeys()),
    "The name of the units for the Y-Axis (must be one of those registered in\n"
    "the Unit Factory)");
}

void LoadDaveGrp::exec()
{
  const std::string filename = this->getProperty("Filename");

  int yLength = 0;

  MantidVec *xAxis = new MantidVec();
  MantidVec *yAxis = new MantidVec();

  std::vector<MantidVec *> data;
  std::vector<MantidVec *> errors;

  this->ifile.open(filename.c_str());
  if (this->ifile.is_open())
  {
    // Size of x axis
    this->getAxisLength(this->xLength);
    // Size of y axis
    this->getAxisLength(yLength);
    // This is also the number of groups (spectra)
    this->nGroups = static_cast<std::size_t>(yLength);
    // Read in the x axis values
    this->getAxisValues(xAxis, static_cast<std::size_t>(this->xLength));
    // Read in the y axis values
    this->getAxisValues(yAxis, static_cast<std::size_t>(yLength));
    // Read in the data
    this->getData(data, errors);
  }
  this->ifile.close();

  // Create workspace
  API::MatrixWorkspace_sptr outputWorkspace = \
      boost::dynamic_pointer_cast<API::MatrixWorkspace>\
      (API::WorkspaceFactory::Instance().create("Workspace2D", this->nGroups,
          this->xLength, yLength));
  // Force the workspace to be a distribution
  outputWorkspace->isDistribution(true);

  // Set the x-axis units
  outputWorkspace->getAxis(0)->unit() = Kernel::UnitFactory::Instance().create(this->getProperty("X-Axis"));

  API::Axis* const verticalAxis = new API::NumericAxis(yLength);
  // Set the y-axis units
  verticalAxis->unit() = Kernel::UnitFactory::Instance().create(this->getProperty("Y-Axis"));

  outputWorkspace->replaceAxis(1, verticalAxis);

  for(std::size_t i = 0; i < this->nGroups; i++)
  {
    outputWorkspace->dataX(i) = *xAxis;
    outputWorkspace->dataY(i) = *data[i];
    outputWorkspace->dataE(i) = *errors[i];
    verticalAxis->setValue(static_cast<const int>(i), yAxis->at(i));

    delete data[i];
    delete errors[i];
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

void LoadDaveGrp::getData(std::vector<MantidVec *> &data,
    std::vector<MantidVec *> &errs)
{
  double data_val = 0.0;
  double err_val = 0.0;
  for(std::size_t j = 0; j < this->nGroups; j++)
  {
    // Skip the group comment line
    this->readLine();
    // Read the data block
    MantidVec *d = new MantidVec();
    MantidVec *e = new MantidVec();
    for(std::size_t k = 0; k < static_cast<std::size_t>(this->xLength); k++)
    {
      this->readLine();
      std::istringstream is(this->line);
      is >> data_val >> err_val;
      d->push_back(data_val);
      e->push_back(err_val);
    }
    data.push_back(d);
    errs.push_back(e);
  }
}

} // namespace DataHandling
} // namespace Mantid
