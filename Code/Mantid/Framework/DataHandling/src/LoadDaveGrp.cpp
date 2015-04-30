#include "MantidDataHandling/LoadDaveGrp.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidAPI/Progress.h"
#include "MantidAPI/RegisterFileLoader.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidKernel/UnitFactory.h"

#include <vector>

namespace Mantid {
namespace DataHandling {
DECLARE_FILELOADER_ALGORITHM(LoadDaveGrp)

LoadDaveGrp::LoadDaveGrp() : ifile(), line(), nGroups(0), xLength(0) {}

/**
 * Return the confidence with with this algorithm can load the file
 * @param descriptor A descriptor for the file
 * @returns An integer specifying the confidence level. 0 indicates it will not
 * be used
 */
int LoadDaveGrp::confidence(Kernel::FileDescriptor &descriptor) const {
  const std::string &extn = descriptor.extension();
  if (extn.compare(".grp") != 0 && extn.compare(".sqe") != 0 &&
      extn.compare(".txt") != 0 && extn.compare(".dat") != 0)
    return 0;

  if (!descriptor.isAscii())
    return 0;

  bool daveGrp(false);
  std::string curline;

  // First line is a comment: #
  std::getline(descriptor.data(), curline);
  if (curline.substr(0, 1) == "#") {
    daveGrp = true;
  } else
    return 0;

  // Second line is an integer
  std::getline(descriptor.data(), curline);
  unsigned int value;
  std::istringstream is(curline);
  is >> value;
  daveGrp = daveGrp && !is.fail();

  // Third line is a comment: #
  std::getline(descriptor.data(), curline);
  if (curline.substr(0, 1) == "#") {
    daveGrp = daveGrp && true;
  } else
    return 0;

  // Fourth line is an integer
  std::getline(descriptor.data(), curline);
  // Clear all stream bits regardless of what happened before
  is.clear();
  is.str(curline);
  is >> value;
  daveGrp = daveGrp && !is.fail();

  if (daveGrp)
    return 80;
  else
    return 0;
}

void LoadDaveGrp::init() {
  std::vector<std::string> exts;
  exts.push_back(".grp");
  exts.push_back(".sqe");
  exts.push_back(".txt");
  exts.push_back(".dat");

  this->declareProperty(
      new API::FileProperty("Filename", "", API::FileProperty::Load, exts),
      "A DAVE grouped ASCII file");
  this->declareProperty(new API::WorkspaceProperty<>("OutputWorkspace", "",
                                                     Kernel::Direction::Output),
                        "The name of the workspace that will be created.");

  // Extract the current contents of the UnitFactory to be the allowed values
  // of the X-Axis property
  auto allowedUnits = boost::make_shared<Kernel::StringListValidator>(
      Kernel::UnitFactory::Instance().getKeys());
  this->declareProperty("XAxisUnits", "DeltaE", allowedUnits,
                        "The name of the units for the X-Axis (must be one of "
                        "those registered in "
                        "the Unit Factory)");
  // Extract the current contents of the UnitFactory to be the allowed values
  // of the Y-Axis property
  this->declareProperty("YAxisUnits", "MomentumTransfer", allowedUnits,
                        "The name of the units for the Y-Axis (must be one of "
                        "those registered in "
                        "the Unit Factory)");
  this->declareProperty(new Kernel::PropertyWithValue<bool>(
                            "IsMicroEV", false, Kernel::Direction::Input),
                        "Original file is in units of micro-eV for DeltaE");
  this->declareProperty(
      new Kernel::PropertyWithValue<bool>("ConvertToHistogram", false,
                                          Kernel::Direction::Input),
      "Convert output workspace to histogram data.");
}

void LoadDaveGrp::exec() {
  const std::string filename = this->getProperty("Filename");

  int yLength = 0;

  MantidVec *xAxis = new MantidVec();
  MantidVec *yAxis = new MantidVec();

  std::vector<MantidVec *> data;
  std::vector<MantidVec *> errors;

  this->ifile.open(filename.c_str());
  if (this->ifile.is_open()) {
    try {
      // Size of x axis
      this->getAxisLength(this->xLength);
      // Size of y axis
      this->getAxisLength(yLength);
    } catch (boost::bad_lexical_cast &) {
      throw std::runtime_error(
          "LoadDaveGrp: Failed to parse axis length from file.");
    }

    // This is also the number of groups (spectra)
    this->nGroups = yLength;
    // Read in the x axis values
    this->getAxisValues(xAxis, static_cast<std::size_t>(this->xLength));
    // Read in the y axis values
    this->getAxisValues(yAxis, static_cast<std::size_t>(yLength));
    // Read in the data
    this->getData(data, errors);
  }
  this->ifile.close();

  // Scale the x-axis if it is in micro-eV to get it to meV
  const bool isUeV = this->getProperty("IsMicroEV");
  if (isUeV) {
    MantidVec::iterator iter;
    for (iter = xAxis->begin(); iter != xAxis->end(); ++iter) {
      *iter /= 1000.0;
    }
  }

  // Create workspace
  API::MatrixWorkspace_sptr outputWorkspace =
      boost::dynamic_pointer_cast<API::MatrixWorkspace>(
          API::WorkspaceFactory::Instance().create("Workspace2D", this->nGroups,
                                                   this->xLength, yLength));
  // Force the workspace to be a distribution
  outputWorkspace->isDistribution(true);

  // Set the x-axis units
  outputWorkspace->getAxis(0)->unit() =
      Kernel::UnitFactory::Instance().create(this->getProperty("XAxisUnits"));

  API::Axis *const verticalAxis = new API::NumericAxis(yLength);
  // Set the y-axis units
  verticalAxis->unit() =
      Kernel::UnitFactory::Instance().create(this->getProperty("YAxisUnits"));

  outputWorkspace->replaceAxis(1, verticalAxis);

  for (int i = 0; i < this->nGroups; i++) {
    outputWorkspace->dataX(i) = *xAxis;
    outputWorkspace->dataY(i) = *data[i];
    outputWorkspace->dataE(i) = *errors[i];
    verticalAxis->setValue(i, yAxis->at(i));

    delete data[i];
    delete errors[i];
  }

  delete xAxis;
  delete yAxis;

  // convert output workspace to histogram data
  const bool convertToHistogram = this->getProperty("ConvertToHistogram");
  if (convertToHistogram) {
    auto convert2HistAlg = createChildAlgorithm("ConvertToHistogram");
    convert2HistAlg->setProperty("InputWorkspace", outputWorkspace);
    convert2HistAlg->setProperty("OutputWorkspace", outputWorkspace);
    convert2HistAlg->execute();
    outputWorkspace = convert2HistAlg->getProperty("OutputWorkspace");

    auto convertFromDistAlg = createChildAlgorithm("ConvertFromDistribution");
    convertFromDistAlg->setProperty("Workspace", outputWorkspace);
    convertFromDistAlg->execute();
  }

  outputWorkspace->mutableRun().addProperty("Filename", filename);
  this->setProperty("OutputWorkspace", outputWorkspace);
}

void LoadDaveGrp::readLine() { std::getline(this->ifile, this->line); }

void LoadDaveGrp::getAxisLength(int &length) {
  // Skip a comment line
  this->readLine();
  // Get the axis length from the file
  this->readLine();
  std::istringstream is(this->line);
  std::string strLength;
  is >> strLength;

  length = boost::lexical_cast<int>(strLength);
}

void LoadDaveGrp::getAxisValues(MantidVec *axis, const std::size_t length) {
  // Skip a comment line
  this->readLine();
  // Get the axis values from the file
  double value;
  for (std::size_t i = 0; i < length; i++) {
    this->readLine();
    std::istringstream is(this->line);
    is >> value;
    axis->push_back(value);
  }
}

void LoadDaveGrp::getData(std::vector<MantidVec *> &data,
                          std::vector<MantidVec *> &errs) {
  double data_val = 0.0;
  double err_val = 0.0;
  API::Progress progress(this, 0.0, 1.0, this->nGroups);
  for (int j = 0; j < this->nGroups; j++) {
    // Skip the group comment line
    this->readLine();
    // Read the data block
    MantidVec *d = new MantidVec();
    MantidVec *e = new MantidVec();
    for (std::size_t k = 0; k < static_cast<std::size_t>(this->xLength); k++) {
      this->readLine();
      std::istringstream is(this->line);
      is >> data_val >> err_val;
      d->push_back(data_val);
      e->push_back(err_val);
    }
    data.push_back(d);
    errs.push_back(e);
    progress.report();
  }
}

} // namespace DataHandling
} // namespace Mantid
