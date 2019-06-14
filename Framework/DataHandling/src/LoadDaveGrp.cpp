// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/LoadDaveGrp.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidAPI/Progress.h"
#include "MantidAPI/RegisterFileLoader.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/Workspace2D.h"
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
  if (extn != ".grp" && extn != ".sqe" && extn != ".txt" && extn != ".dat")
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
  if (curline.substr(0, 1) != "#")
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
  std::vector<std::string> exts{".grp", ".sqe", ".txt", ".dat"};

  declareProperty(std::make_unique<API::FileProperty>(
                      "Filename", "", API::FileProperty::Load, exts),
                  "A DAVE grouped ASCII file");
  declareProperty(std::make_unique<API::WorkspaceProperty<>>(
                      "OutputWorkspace", "", Kernel::Direction::Output),
                  "The name of the workspace that will be created.");

  // Extract the current contents of the UnitFactory to be the allowed values
  // of the X-Axis property
  auto allowedUnits = boost::make_shared<Kernel::StringListValidator>(
      Kernel::UnitFactory::Instance().getKeys());
  declareProperty("XAxisUnits", "DeltaE", allowedUnits,
                  "The name of the units for the X-Axis (must be one of "
                  "those registered in "
                  "the Unit Factory)");
  // Extract the current contents of the UnitFactory to be the allowed values
  // of the Y-Axis property
  declareProperty("YAxisUnits", "MomentumTransfer", allowedUnits,
                  "The name of the units for the Y-Axis (must be one of "
                  "those registered in "
                  "the Unit Factory)");
  declareProperty(std::make_unique<Kernel::PropertyWithValue<bool>>(
                      "IsMicroEV", false, Kernel::Direction::Input),
                  "Original file is in units of micro-eV for DeltaE");
  declareProperty(std::make_unique<Kernel::PropertyWithValue<bool>>(
                      "ConvertToHistogram", false, Kernel::Direction::Input),
                  "Convert output workspace to histogram data.");
}

void LoadDaveGrp::exec() {
  const std::string filename = getProperty("Filename");
  std::vector<double> xAxis;
  std::vector<double> yAxis;
  API::MatrixWorkspace_sptr outputWorkspace;

  ifile.open(filename.c_str());
  if (ifile.is_open()) {
    try {
      // Size of x axis
      getAxisLength(xLength);
      // Size of y axis
      getAxisLength(nGroups);
    } catch (boost::bad_lexical_cast &) {
      throw std::runtime_error(
          "LoadDaveGrp: Failed to parse axis length from file.");
    }

    outputWorkspace = setupWorkspace();
    // Read in the x axis values
    getAxisValues(xAxis, xLength);
    // Read in the y axis values
    getAxisValues(yAxis, nGroups);
    // Read in the data
    getData(outputWorkspace);

    ifile.close();
  } else {
    throw std::runtime_error("LoadDaveGrp: Failed to open file.");
  }

  // Scale the x-axis if it is in micro-eV to get it to meV
  const bool isUeV = getProperty("IsMicroEV");
  if (isUeV) {
    for (auto &value : xAxis)
      value /= 1000.0;
  }

  setWorkspaceAxes(outputWorkspace, xAxis, yAxis);

  // convert output workspace to histogram data
  const bool convertToHistogram = getProperty("ConvertToHistogram");
  if (convertToHistogram)
    outputWorkspace = convertWorkspaceToHistogram(outputWorkspace);

  outputWorkspace->mutableRun().addProperty("Filename", filename);
  setProperty("OutputWorkspace", outputWorkspace);
}

API::MatrixWorkspace_sptr LoadDaveGrp::setupWorkspace() const {
  // Create workspace
  API::MatrixWorkspace_sptr outputWorkspace =
      boost::dynamic_pointer_cast<API::MatrixWorkspace>(
          API::WorkspaceFactory::Instance().create("Workspace2D", nGroups,
                                                   xLength, xLength));
  // Force the workspace to be a distribution
  outputWorkspace->setDistribution(true);
  // Set the x-axis units
  outputWorkspace->getAxis(0)->unit() =
      Kernel::UnitFactory::Instance().create(getProperty("XAxisUnits"));

  API::Axis *const verticalAxis = new API::NumericAxis(nGroups);
  // Set the y-axis units
  verticalAxis->unit() =
      Kernel::UnitFactory::Instance().create(getProperty("YAxisUnits"));

  outputWorkspace->replaceAxis(1, verticalAxis);
  return outputWorkspace;
}

void LoadDaveGrp::setWorkspaceAxes(API::MatrixWorkspace_sptr workspace,
                                   const std::vector<double> &xAxis,
                                   const std::vector<double> &yAxis) const {

  auto verticalAxis = workspace->getAxis(1);
  auto sharedX = Kernel::make_cow<HistogramData::HistogramX>(xAxis);
  for (size_t i = 0; i < nGroups; i++) {
    workspace->setSharedX(i, sharedX);
    verticalAxis->setValue(i, yAxis.at(i));
  }
}

API::MatrixWorkspace_sptr
LoadDaveGrp::convertWorkspaceToHistogram(API::MatrixWorkspace_sptr workspace) {
  auto convert2HistAlg = createChildAlgorithm("ConvertToHistogram");
  convert2HistAlg->setProperty("InputWorkspace", workspace);
  convert2HistAlg->setProperty("OutputWorkspace", workspace);
  convert2HistAlg->execute();
  workspace = convert2HistAlg->getProperty("OutputWorkspace");

  auto convertFromDistAlg = createChildAlgorithm("ConvertFromDistribution");
  convertFromDistAlg->setProperty("Workspace", workspace);
  convertFromDistAlg->execute();

  return workspace;
}

void LoadDaveGrp::readLine() { std::getline(ifile, line); }

void LoadDaveGrp::getAxisLength(size_t &length) {
  // Skip a comment line
  readLine();
  // Get the axis length from the file
  readLine();
  std::istringstream is(line);
  std::string strLength;
  is >> strLength;

  length = boost::lexical_cast<size_t>(strLength);
}

void LoadDaveGrp::getAxisValues(std::vector<double> &axis,
                                const std::size_t length) {
  // Skip a comment line
  readLine();
  // Get the axis values from the file
  double value;
  for (std::size_t i = 0; i < length; i++) {
    readLine();
    std::istringstream is(line);
    is >> value;
    axis.push_back(value);
  }
}

void LoadDaveGrp::getData(API::MatrixWorkspace_sptr workspace) {
  double data_val = 0.0;
  double err_val = 0.0;

  API::Progress progress(this, 0.0, 1.0, nGroups);

  for (size_t j = 0; j < nGroups; j++) {
    // Skip the group comment line
    readLine();
    // Read the data block
    auto &dataY = workspace->mutableY(j);
    auto &dataE = workspace->mutableE(j);

    for (std::size_t k = 0; k < xLength; k++) {
      readLine();
      std::istringstream is(line);
      is >> data_val >> err_val;
      dataY[k] = data_val;
      dataE[k] = err_val;
    }

    progress.report();
  }
}

} // namespace DataHandling
} // namespace Mantid
