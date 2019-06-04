// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/ConvertSpectrumAxis2.h"
#include "MantidAPI/InstrumentValidator.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/SpectraAxisValidator.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidHistogramData/Histogram.h"
#include "MantidHistogramData/HistogramBuilder.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/Unit.h"
#include "MantidKernel/UnitConversion.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidTypes/SpectrumDefinition.h"

#include <cfloat>

constexpr double rad2deg = 180.0 / M_PI;

namespace Mantid {
namespace Algorithms {
// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ConvertSpectrumAxis2)
using namespace Kernel;
using namespace API;
using namespace Geometry;
using namespace DataObjects;
using namespace HistogramData;

void ConvertSpectrumAxis2::init() {
  // Validator for Input Workspace
  auto wsVal = boost::make_shared<CompositeValidator>();
  wsVal->add<SpectraAxisValidator>();
  wsVal->add<InstrumentValidator>();

  declareProperty(std::make_unique<WorkspaceProperty<>>("InputWorkspace", "",
                                                   Direction::Input, wsVal),
                  "The name of the input workspace.");
  declareProperty(std::make_unique<WorkspaceProperty<>>("OutputWorkspace", "",
                                                   Direction::Output),
                  "The name to use for the output workspace.");
  std::vector<std::string> targetOptions{
      "Theta", "SignedTheta",  "ElasticQ",       "ElasticQSquared",
      "theta", "signed_theta", "ElasticDSpacing"};
  declareProperty(
      "Target", "", boost::make_shared<StringListValidator>(targetOptions),
      "The unit to which spectrum axis is converted to - \"theta\" (for the "
      "angle in degrees), Q or Q^2, where elastic Q is evaluated at EFixed. "
      "Note that 'theta' and 'signed_theta' are there for compatibility "
      "purposes; they are the same as 'Theta' and 'SignedTheta' respectively");
  std::vector<std::string> eModeOptions;
  eModeOptions.emplace_back("Direct");
  eModeOptions.emplace_back("Indirect");
  declareProperty("EMode", "Direct",
                  boost::make_shared<StringListValidator>(eModeOptions),
                  "Some unit conversions require this value to be set "
                  "(\"Direct\" or \"Indirect\")");
  auto mustBePositive = boost::make_shared<BoundedValidator<double>>();
  mustBePositive->setLower(0.0);
  declareProperty("EFixed", EMPTY_DBL(), mustBePositive,
                  "Value of fixed energy in meV : EI (EMode=Direct) or EF "
                  "(EMode=Indirect))");

  declareProperty("OrderAxis", true,
                  "Whether or not to sort the resulting"
                  " spectrum axis.");
}

void ConvertSpectrumAxis2::exec() {
  // Get the input workspace.
  API::MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");
  // Assign value to the member variable storing the number of histograms.
  const size_t nHist = inputWS->getNumberHistograms();

  // The unit to convert to.
  const std::string unitTarget = getProperty("Target");

  // Whether needs to be ordered
  m_toOrder = getProperty("OrderAxis");

  size_t nProgress = nHist;
  if (m_toOrder) {
    // we will need to loop twice, once to build the indexMap,
    // once to copy over the spectra and set the output
    nProgress *= 2;
  } else {
    m_axis.reserve(nHist);
  }

  Progress progress(this, 0.0, 1.0, nProgress);

  // Call the functions to convert to the different forms of theta or Q.
  if (unitTarget == "theta" || unitTarget == "Theta" ||
      unitTarget == "signed_theta" || unitTarget == "SignedTheta") {
    createThetaMap(progress, unitTarget, inputWS);
  } else if (unitTarget == "ElasticQ" || unitTarget == "ElasticQSquared" ||
             unitTarget == "ElasticDSpacing") {
    createElasticQMap(progress, unitTarget, inputWS);
  }

  // Create an output workspace and set the property for it.
  MatrixWorkspace_sptr outputWS =
      createOutputWorkspace(progress, unitTarget, inputWS);
  setProperty("OutputWorkspace", outputWS);
}

/** Converts X axis to theta representation
 * @param progress :: Progress indicator
 * @param targetUnit :: Target conversion unit
 * @param inputWS :: Input Workspace
 */
void ConvertSpectrumAxis2::createThetaMap(API::Progress &progress,
                                          const std::string &targetUnit,
                                          API::MatrixWorkspace_sptr &inputWS) {
  // Not sure about default, previously there was a call to a null function?
  bool signedTheta = false;
  if (targetUnit == "signed_theta" || targetUnit == "SignedTheta") {
    signedTheta = true;
  } else if (targetUnit == "theta" || targetUnit == "Theta") {
    signedTheta = false;
  }

  bool warningGiven = false;

  const auto &spectrumInfo = inputWS->spectrumInfo();
  for (size_t i = 0; i < spectrumInfo.size(); ++i) {
    if (!spectrumInfo.hasDetectors(i)) {
      if (!warningGiven)
        g_log.warning("The instrument definition is incomplete - spectra "
                      "dropped from output");
      warningGiven = true;
      continue;
    }
    if (!spectrumInfo.isMonitor(i)) {
      if (signedTheta)
        emplaceIndexMap(spectrumInfo.signedTwoTheta(i) * rad2deg, i);
      else
        emplaceIndexMap(spectrumInfo.twoTheta(i) * rad2deg, i);
    } else {
      emplaceIndexMap(0.0, i);
    }

    progress.report("Converting to theta...");
  }
}

/** Convert X axis to Elastic Q representation
 * @param progress :: Progress indicator
 * @param targetUnit :: Target conversion unit
 * @param inputWS :: Input workspace
 */
void ConvertSpectrumAxis2::createElasticQMap(
    API::Progress &progress, const std::string &targetUnit,
    API::MatrixWorkspace_sptr &inputWS) {

  const std::string emodeStr = getProperty("EMode");
  int emode = 0;
  if (emodeStr == "Direct")
    emode = 1;
  else if (emodeStr == "Indirect")
    emode = 2;

  const auto &spectrumInfo = inputWS->spectrumInfo();
  const auto &detectorInfo = inputWS->detectorInfo();
  const size_t nHist = spectrumInfo.size();
  for (size_t i = 0; i < nHist; i++) {
    double theta(0.0), efixed(0.0);
    if (!spectrumInfo.isMonitor(i)) {
      theta = 0.5 * spectrumInfo.twoTheta(i);
      /*
       * Two assumptions made in the following code.
       * 1. Getting the detector index of the first detector in the spectrum
       * definition is enough (this should be completely safe).
       * 2. That the time index is not important (first element of pair only
       * accessed). i.e we are not performing scanning. Step scanning is not
       * supported at the time of writing.
       */
      const auto detectorIndex = spectrumInfo.spectrumDefinition(i)[0].first;
      efixed = getEfixed(detectorIndex, detectorInfo, *inputWS,
                         emode); // get efixed
    } else {
      theta = DBL_MIN;
      efixed = DBL_MIN;
    }

    // Convert to MomentumTransfer
    double elasticQInAngstroms =
        Kernel::UnitConversion::convertToElasticQ(theta, efixed);

    if (targetUnit == "ElasticQ") {
      emplaceIndexMap(elasticQInAngstroms, i);
    } else if (targetUnit == "ElasticQSquared") {
      // The QSquared value.
      double elasticQSquaredInAngstroms =
          elasticQInAngstroms * elasticQInAngstroms;

      emplaceIndexMap(elasticQSquaredInAngstroms, i);
    } else if (targetUnit == "ElasticDSpacing") {
      double elasticDSpacing = 2 * M_PI / elasticQInAngstroms;
      emplaceIndexMap(elasticDSpacing, i);
    }

    progress.report("Converting to " + targetUnit);
  }
}

/** Create the final output workspace after converting the X axis
 * @returns the final output workspace
 *
 * @param progress :: Progress indicator
 * @param targetUnit :: Target conversion unit
 * @param inputWS :: Input workspace
 */
MatrixWorkspace_sptr ConvertSpectrumAxis2::createOutputWorkspace(
    API::Progress &progress, const std::string &targetUnit,
    API::MatrixWorkspace_sptr &inputWS) {

  MatrixWorkspace_sptr outputWorkspace = nullptr;
  NumericAxis *newAxis = nullptr;
  EventWorkspace_sptr eventWS =
      boost::dynamic_pointer_cast<EventWorkspace>(inputWS);
  if (m_toOrder) {
    // Can not re-use the input one because the spectra are re-ordered.
    const Histogram hist =
        eventWS ? Histogram(inputWS->binEdges(0)) : inputWS->histogram(0);
    outputWorkspace =
        create<MatrixWorkspace>(*inputWS, m_indexMap.size(), hist);
    std::vector<double> axis;
    axis.reserve(m_indexMap.size());
    for (const auto &it : m_indexMap) {
      axis.emplace_back(it.first);
    }
    newAxis = new NumericAxis(std::move(axis));
  } else {
    // If there is no reordering we can simply clone.
    outputWorkspace = inputWS->clone();
    newAxis = new NumericAxis(m_axis);
  }
  outputWorkspace->replaceAxis(1, newAxis);

  // Set the units of the axis.
  if (targetUnit == "theta" || targetUnit == "Theta" ||
      targetUnit == "signed_theta" || targetUnit == "SignedTheta") {
    newAxis->unit() = boost::make_shared<Units::Degrees>();
  } else if (targetUnit == "ElasticQ") {
    newAxis->unit() = UnitFactory::Instance().create("MomentumTransfer");
  } else if (targetUnit == "ElasticQSquared") {
    newAxis->unit() = UnitFactory::Instance().create("QSquared");
  } else if (targetUnit == "ElasticDSpacing") {
    newAxis->unit() = UnitFactory::Instance().create("dSpacing");
  }

  // Note that this is needed only for ordered case
  if (m_toOrder) {
    size_t currentIndex = 0;
    std::multimap<double, size_t>::const_iterator it;
    for (it = m_indexMap.begin(); it != m_indexMap.end(); ++it) {
      // Copy over the data.
      outputWorkspace->getSpectrum(currentIndex)
          .copyDataFrom(inputWS->getSpectrum(it->second));
      // We can keep the spectrum numbers etc.
      outputWorkspace->getSpectrum(currentIndex)
          .copyInfoFrom(inputWS->getSpectrum(it->second));
      ++currentIndex;
      progress.report("Setting output spectrum #" +
                      std::to_string(currentIndex));
    }
  }

  return outputWorkspace;
}

double ConvertSpectrumAxis2::getEfixed(
    const size_t detectorIndex, const Geometry::DetectorInfo &detectorInfo,
    const Mantid::API::MatrixWorkspace &inputWS, const int emode) const {
  double efixed(0);
  double efixedProp = getProperty("Efixed");
  Mantid::detid_t detectorID = detectorInfo.detectorIDs()[detectorIndex];
  if (efixedProp != EMPTY_DBL()) {
    efixed = efixedProp;
    g_log.debug() << "Detector: " << detectorID << " Efixed: " << efixed
                  << "\n";
  } else {
    if (emode == 1) {
      if (inputWS.run().hasProperty("Ei")) {
        efixed = inputWS.run().getLogAsSingleValue("Ei");
      } else {
        throw std::invalid_argument("Could not retrieve Efixed from the "
                                    "workspace. Please provide a value.");
      }
    } else if (emode == 2) {

      const auto &detectorSingle = detectorInfo.detector(detectorIndex);

      std::vector<double> efixedVec =
          detectorSingle.getNumberParameter("Efixed");

      if (!efixedVec.empty()) {
        efixed = efixedVec.at(0);
        g_log.debug() << "Detector: " << detectorID << " EFixed: " << efixed
                      << "\n";
      } else {
        g_log.warning() << "Efixed could not be found for detector "
                        << detectorID << ", please provide a value\n";
        throw std::invalid_argument("Could not retrieve Efixed from the "
                                    "detector. Please provide a value.");
      }
    }
  }
  return efixed;
}

/** Emplaces inside the ordered or unordered index registry
 * @param value :: value to insert
 * @param wsIndex :: workspace index
 */
void ConvertSpectrumAxis2::emplaceIndexMap(double value, size_t wsIndex) {
  if (m_toOrder) {
    m_indexMap.emplace(value, wsIndex);
  } else {
    m_axis.emplace_back(value);
  }
}

} // namespace Algorithms
} // namespace Mantid
