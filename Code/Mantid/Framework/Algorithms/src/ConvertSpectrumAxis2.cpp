//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/ConvertSpectrumAxis2.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidKernel/UnitConversion.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidAPI/Run.h"
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <cfloat>
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ListValidator.h"

namespace Mantid {
namespace Algorithms {
// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ConvertSpectrumAxis2)
using namespace Kernel;
using namespace API;
using namespace Geometry;

void ConvertSpectrumAxis2::init() {
  // Validator for Input Workspace
  auto wsVal = boost::make_shared<CompositeValidator>();
  wsVal->add<HistogramValidator>();
  wsVal->add<SpectraAxisValidator>();
  wsVal->add<InstrumentValidator>();

  declareProperty(
      new WorkspaceProperty<>("InputWorkspace", "", Direction::Input, wsVal),
      "The name of the input workspace.");
  declareProperty(
      new WorkspaceProperty<>("OutputWorkspace", "", Direction::Output),
      "The name to use for the output workspace.");
  std::vector<std::string> targetOptions(6);
  targetOptions[0] = "Theta";
  targetOptions[1] = "SignedTheta";
  targetOptions[2] = "ElasticQ";
  targetOptions[3] = "ElasticQSquared";
  targetOptions[4] = "theta";
  targetOptions[5] = "signed_theta";

  declareProperty(
      "Target", "", boost::make_shared<StringListValidator>(targetOptions),
      "The unit to which spectrum axis is converted to - \"theta\" (for the "
      "angle in degrees), Q or Q^2, where elastic Q is evaluated at EFixed. "
      "Note that 'theta' and 'signed_theta' are there for compatibility "
      "purposes; they are the same as 'Theta' and 'SignedTheta' respectively");
  std::vector<std::string> eModeOptions;
  eModeOptions.push_back("Direct");
  eModeOptions.push_back("Indirect");
  declareProperty("EMode", "Direct",
                  boost::make_shared<StringListValidator>(eModeOptions),
                  "Some unit conversions require this value to be set "
                  "(\"Direct\" or \"Indirect\")");
  auto mustBePositive = boost::make_shared<BoundedValidator<double>>();
  mustBePositive->setLower(0.0);
  declareProperty("EFixed", EMPTY_DBL(), mustBePositive,
                  "Value of fixed energy in meV : EI (EMode=Direct) or EF "
                  "(EMode=Indirect))");
}

void ConvertSpectrumAxis2::exec() {
  // Get the input workspace.
  m_inputWS = getProperty("InputWorkspace");
  // Assign value to the member variable storing the number of histograms.
  m_nHist = m_inputWS->getNumberHistograms();

  // Assign values to the member variables to store number of bins.
  m_nBins = m_inputWS->blocksize();

  const bool isHist = m_inputWS->isHistogramData();

  if (isHist) {
    m_nxBins = m_nBins + 1;
  } else {
    m_nxBins = m_nBins;
  }

  // The unit to convert to.
  const std::string unitTarget = getProperty("Target");

  // Call the functions to convert to the different forms of theta or Q.
  if (unitTarget == "theta" || unitTarget == "Theta" ||
      unitTarget == "signed_theta" || unitTarget == "SignedTheta") {
    createThetaMap(unitTarget);
  } else if (unitTarget == "ElasticQ" || unitTarget == "ElasticQSquared") {
    createElasticQMap(unitTarget);
  }

  // Create an output workspace and set the property for it.
  MatrixWorkspace_sptr outputWS = createOutputWorkspace(unitTarget);
  setProperty("OutputWorkspace", outputWS);
}

void ConvertSpectrumAxis2::createThetaMap(const std::string &targetUnit) {
  // Set up binding to member funtion. Avoids condition as part of loop over
  // nHistograms.
  boost::function<double(IDetector_const_sptr)> thetaFunction;
  if (targetUnit.compare("signed_theta") == 0 ||
      targetUnit.compare("SignedTheta") == 0) {
    thetaFunction =
        boost::bind(&MatrixWorkspace::detectorSignedTwoTheta, m_inputWS, _1);
  } else if (targetUnit == "theta" || targetUnit == "Theta") {
    thetaFunction =
        boost::bind(&MatrixWorkspace::detectorTwoTheta, m_inputWS, _1);
  }

  bool warningGiven = false;
  for (size_t i = 0; i < m_nHist; ++i) {
    try {
      IDetector_const_sptr det = m_inputWS->getDetector(i);
      // Invoke relevant member function.
      m_indexMap.insert(std::make_pair(thetaFunction(det) * 180.0 / M_PI, i));
    } catch (Exception::NotFoundError &) {
      if (!warningGiven)
        g_log.warning("The instrument definition is incomplete - spectra "
                      "dropped from output");
      warningGiven = true;
    }
  }
}

void ConvertSpectrumAxis2::createElasticQMap(const std::string &targetUnit) {
  IComponent_const_sptr source = m_inputWS->getInstrument()->getSource();
  IComponent_const_sptr sample = m_inputWS->getInstrument()->getSample();

  const std::string emodeStr = getProperty("EMode");
  int emode = 0;
  if (emodeStr == "Direct")
    emode = 1;
  else if (emodeStr == "Indirect")
    emode = 2;

  for (size_t i = 0; i < m_nHist; i++) {
    IDetector_const_sptr detector = m_inputWS->getDetector(i);
    double twoTheta(0.0), efixed(0.0);
    if (!detector->isMonitor()) {
      twoTheta = m_inputWS->detectorTwoTheta(detector) / 2.0;
      efixed = getEfixed(detector, m_inputWS, emode); // get efixed
    } else {
      twoTheta = 0.0;
      efixed = DBL_MIN;
    }

    // Convert to MomentumTransfer
    double elasticQInAngstroms = Kernel::UnitConversion::run(twoTheta, efixed);

    if (targetUnit == "ElasticQ") {
      m_indexMap.insert(std::make_pair(elasticQInAngstroms, i));
    } else if (targetUnit == "ElasticQSquared") {
      // The QSquared value.
      double elasticQSquaredInAngstroms =
          elasticQInAngstroms * elasticQInAngstroms;

      m_indexMap.insert(std::make_pair(elasticQSquaredInAngstroms, i));
    }
  }
}

MatrixWorkspace_sptr
ConvertSpectrumAxis2::createOutputWorkspace(const std::string &targetUnit) {
  // Create the output workspace. Can not re-use the input one because the
  // spectra are re-ordered.
  MatrixWorkspace_sptr outputWorkspace = WorkspaceFactory::Instance().create(
      m_inputWS, m_indexMap.size(), m_nxBins, m_nBins);

  // Now set up a new numeric axis holding the theta values corresponding to
  // each spectrum.
  NumericAxis *const newAxis = new NumericAxis(m_indexMap.size());
  outputWorkspace->replaceAxis(1, newAxis);

  // Set the units of the axis.
  if (targetUnit == "theta" || targetUnit == "Theta" ||
      targetUnit == "signed_theta" || targetUnit == "SignedTheta") {
    newAxis->unit() = boost::shared_ptr<Unit>(new Units::Degrees);
  } else if (targetUnit == "ElasticQ") {
    newAxis->unit() = UnitFactory::Instance().create("MomentumTransfer");
  } else if (targetUnit == "ElasticQSquared") {
    newAxis->unit() = UnitFactory::Instance().create("QSquared");
  }

  std::multimap<double, size_t>::const_iterator it;
  size_t currentIndex = 0;
  for (it = m_indexMap.begin(); it != m_indexMap.end(); ++it) {
    // Set the axis value.
    newAxis->setValue(currentIndex, it->first);
    // Copy over the data.
    outputWorkspace->dataX(currentIndex) = m_inputWS->dataX(it->second);
    outputWorkspace->dataY(currentIndex) = m_inputWS->dataY(it->second);
    outputWorkspace->dataE(currentIndex) = m_inputWS->dataE(it->second);
    // We can keep the spectrum numbers etc.
    outputWorkspace->getSpectrum(currentIndex)
        ->copyInfoFrom(*m_inputWS->getSpectrum(it->second));
    ++currentIndex;
  }
  return outputWorkspace;
}

double ConvertSpectrumAxis2::getEfixed(IDetector_const_sptr detector,
                                       MatrixWorkspace_const_sptr inputWS,
                                       int emode) const {
  double efixed(0);
  double efixedProp = getProperty("Efixed");
  if (efixedProp != EMPTY_DBL()) {
    efixed = efixedProp;
    g_log.debug() << "Detector: " << detector->getID() << " Efixed: " << efixed
                  << "\n";
  } else {
    if (emode == 1) {
      if (inputWS->run().hasProperty("Ei")) {
        efixed = inputWS->run().getLogAsSingleValue("Ei");
      } else {
        throw std::invalid_argument("Could not retrieve Efixed from the "
                                    "workspace. Please provide a value.");
      }
    } else if (emode == 2) {
      std::vector<double> efixedVec = detector->getNumberParameter("Efixed");
      if (efixedVec.empty()) {
        int detid = detector->getID();
        IDetector_const_sptr detectorSingle =
            inputWS->getInstrument()->getDetector(detid);
        efixedVec = detectorSingle->getNumberParameter("Efixed");
      }
      if (!efixedVec.empty()) {
        efixed = efixedVec.at(0);
        g_log.debug() << "Detector: " << detector->getID()
                      << " EFixed: " << efixed << "\n";
      } else {
        g_log.warning() << "Efixed could not be found for detector "
                        << detector->getID() << ", please provide a value\n";
        throw std::invalid_argument("Could not retrieve Efixed from the "
                                    "detector. Please provide a value.");
      }
    }
  }
  return efixed;
}

} // namespace Algorithms
} // namespace Mantid
