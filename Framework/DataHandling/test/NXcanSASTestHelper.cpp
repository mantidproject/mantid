// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "NXcanSASTestHelper.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/InstrumentFileFinder.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidAPI/Sample.h"
#include "MantidDataHandling/NXcanSASDefinitions.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/UnitFactory.h"

#include <Poco/File.h>

namespace {
// Create a histogram from a workspace and return it
Mantid::API::MatrixWorkspace_sptr toHistogram(const Mantid::API::MatrixWorkspace_sptr &ws) {
  auto toHistAlg = Mantid::API::AlgorithmManager::Instance().createUnmanaged("ConvertToHistogram");
  toHistAlg->initialize();
  toHistAlg->setChild(true);
  toHistAlg->setProperty("InputWorkspace", ws);
  toHistAlg->setProperty("OutputWorkspace", "unused");
  toHistAlg->execute();
  return toHistAlg->getProperty("OutputWorkspace");
}

// Create point data form a histogram
Mantid::API::MatrixWorkspace_sptr toPointData(const Mantid::API::MatrixWorkspace_sptr &ws) {
  // Convert to Point data
  auto toPointAlg = Mantid::API::AlgorithmManager::Instance().createUnmanaged("ConvertToPointData");
  toPointAlg->initialize();
  toPointAlg->setChild(true);
  toPointAlg->setProperty("InputWorkspace", ws);
  toPointAlg->setProperty("OutputWorkspace", "unused");
  toPointAlg->execute();
  return toPointAlg->getProperty("OutputWorkspace");
}

} // namespace

namespace NXcanSASTestHelper {

std::string concatenateStringVector(const std::vector<std::string> &stringVector) {
  std::ostringstream os;
  for (const auto &element : stringVector) {
    os << element;
    os << Mantid::DataHandling::NXcanSAS::sasSeparator;
  }

  return os.str();
}

std::string getIDFfromWorkspace(const Mantid::API::MatrixWorkspace_sptr &workspace) {
  auto instrument = workspace->getInstrument();
  auto name = instrument->getFullName();
  auto date = workspace->getWorkspaceStartDate();
  return Mantid::API::InstrumentFileFinder::getInstrumentFilename(name, date);
}

void setXValuesOn1DWorkspace(const Mantid::API::MatrixWorkspace_sptr &workspace, double xmin, double xmax) {
  auto &xValues = workspace->dataX(0);
  const double step = (xmax - xmin) / static_cast<double>(xValues.size() - 1);
  double value(xmin);
  std::generate(std::begin(xValues), std::end(xValues), [&value, &step]() {
    const auto oldValue{value};
    value += step;
    return oldValue;
  });
}

void add_sample_log(const Mantid::API::MatrixWorkspace_sptr &workspace, const std::string &logName,
                    const std::string &logValue) {
  auto logAlg = Mantid::API::AlgorithmManager::Instance().createUnmanaged("AddSampleLog");
  logAlg->initialize();
  logAlg->setChild(true);
  logAlg->setProperty("Workspace", workspace);
  logAlg->setProperty("LogName", logName);
  logAlg->setProperty("LogText", logValue);
  logAlg->execute();
}

void set_logs(const Mantid::API::MatrixWorkspace_sptr &workspace, const std::string &runNumber,
              const std::string &userFile) {
  if (!runNumber.empty()) {
    add_sample_log(workspace, "run_number", runNumber);
  }

  if (!userFile.empty()) {
    add_sample_log(workspace, "UserFile", userFile);
  }
}

void set_instrument(const Mantid::API::MatrixWorkspace_sptr &workspace, const std::string &instrumentName) {
  auto instAlg = Mantid::API::AlgorithmManager::Instance().createUnmanaged("LoadInstrument");
  instAlg->initialize();
  instAlg->setChild(true);
  instAlg->setProperty("Workspace", workspace);
  instAlg->setProperty("InstrumentName", instrumentName);
  instAlg->setProperty("RewriteSpectraMap", "False");
  instAlg->execute();
}

Mantid::API::MatrixWorkspace_sptr provide1DWorkspace(NXcanSASTestParameters &parameters) {
  Mantid::API::MatrixWorkspace_sptr ws;
  if (parameters.hasDx) {
    ws = WorkspaceCreationHelper::create1DWorkspaceConstantWithXerror(parameters.size, parameters.value,
                                                                      parameters.error, parameters.xerror, false);
  } else {
    ws = WorkspaceCreationHelper::create1DWorkspaceConstant(parameters.size, parameters.value, parameters.error, false);
  }
  setXValuesOn1DWorkspace(ws, parameters.xmin, parameters.xmax);

  ws->setTitle(parameters.workspaceTitle);
  ws->getAxis(0)->unit() = Mantid::Kernel::UnitFactory::Instance().create("MomentumTransfer");

  // Add sample logs
  set_logs(ws, parameters.runNumber, parameters.userFile);

  // Set instrument
  set_instrument(ws, parameters.instrumentName);
  ws->getSpectrum(0).setDetectorID(1);

  // Set Sample info
  auto &&sample = ws->mutableSample();
  auto geometry = parameters.geometry;
  boost::to_lower(geometry);
  if (geometry == "cylinder") {
    sample.setGeometryFlag(1);
  } else if (geometry == "flat plate" || geometry == "flatplate") {
    sample.setGeometryFlag(2);
  } else if (geometry == "disc") {
    sample.setGeometryFlag(3);
  }
  sample.setHeight(parameters.beamHeight);
  sample.setWidth(parameters.beamWidth);
  sample.setThickness(parameters.sampleThickness);

  // Set to point data or histogram data
  if (parameters.isHistogram) {
    ws = toHistogram(ws);
  }

  return ws;
}

Mantid::API::MatrixWorkspace_sptr getTransmissionWorkspace(NXcanSASTestTransmissionParameters &parameters) {
  Mantid::API::MatrixWorkspace_sptr ws =
      WorkspaceCreationHelper::create1DWorkspaceConstant(parameters.size, parameters.value, parameters.error, false);
  ws->setTitle(parameters.name);
  ws->getAxis(0)->unit() = Mantid::Kernel::UnitFactory::Instance().create("Wavelength");
  setXValuesOn1DWorkspace(ws, parameters.xmin, parameters.xmax);
  if (parameters.isHistogram)
    ws = toHistogram(ws);
  return ws;
}

Mantid::API::MatrixWorkspace_sptr provide2DWorkspace(NXcanSASTestParameters &parameters) {
  auto ws = provide1DWorkspace(parameters);

  std::string axisBinning = std::to_string(parameters.xmin) + ",1," + std::to_string(parameters.xmax);
  std::string axis2Binning = std::to_string(parameters.ymin) + ",1," + std::to_string(parameters.ymax);

  ws = toHistogram(ws);

  // Convert Spectrum Axis
  auto axisAlg = Mantid::API::AlgorithmManager::Instance().createUnmanaged("ConvertSpectrumAxis");
  std::string toAxisOutputName("toAxisOutput");
  axisAlg->initialize();
  axisAlg->setChild(true);
  axisAlg->setProperty("InputWorkspace", ws);
  axisAlg->setProperty("OutputWorkspace", toAxisOutputName);
  axisAlg->setProperty("Target", "ElasticQ");
  axisAlg->execute();
  ws = axisAlg->getProperty("OutputWorkspace");

  // Rebin 2D
  auto rebin2DAlg = Mantid::API::AlgorithmManager::Instance().createUnmanaged("Rebin2D");
  std::string rebinOutputName("rebinOutput");
  rebin2DAlg->initialize();
  rebin2DAlg->setChild(true);
  rebin2DAlg->setProperty("InputWorkspace", ws);
  rebin2DAlg->setProperty("OutputWorkspace", rebinOutputName);
  rebin2DAlg->setProperty("Axis1Binning", axisBinning);
  rebin2DAlg->setProperty("Axis2Binning", axis2Binning);
  rebin2DAlg->execute();
  ws = rebin2DAlg->getProperty("OutputWorkspace");

  if (!parameters.isHistogram) {
    ws = toPointData(ws);
  }

  // At this point we have a mismatch between the Axis1 elements and the
  // number of histograms.
  if (ws->getAxis(1)->length() != ws->getNumberHistograms()) {
    auto oldAxis = ws->getAxis(1);
    auto newAxis = std::make_unique<Mantid::API::NumericAxis>(ws->getNumberHistograms());
    for (size_t index = 0; index < ws->getNumberHistograms(); ++index) {
      newAxis->setValue(index, oldAxis->getValue(index));
    }
    ws->replaceAxis(1, std::move(newAxis));
  }

  ws->getAxis(1)->unit() = Mantid::Kernel::UnitFactory::Instance().create("MomentumTransfer");
  return ws;
}

void set2DValues(const Mantid::API::MatrixWorkspace_sptr &ws) {
  const auto numberOfHistograms = ws->getNumberHistograms();

  for (size_t index = 0; index < numberOfHistograms; ++index) {
    auto &data = ws->dataY(index);
    data = Mantid::MantidVec(data.size(), static_cast<double>(index));
  }
}

void removeFile(const std::string &filename) {
  if (Poco::File(filename).exists())
    Poco::File(filename).remove();
}
} // namespace NXcanSASTestHelper
