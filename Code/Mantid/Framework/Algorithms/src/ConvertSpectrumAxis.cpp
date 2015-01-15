//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/ConvertSpectrumAxis.h"
#include "MantidAPI/NumericAxis.h"
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
DECLARE_ALGORITHM(ConvertSpectrumAxis)
using namespace Kernel;
using namespace API;
using namespace Geometry;

void ConvertSpectrumAxis::init() {
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
  std::vector<std::string> targetOptions =
      Mantid::Kernel::UnitFactory::Instance().getKeys();
  targetOptions.push_back("theta");
  targetOptions.push_back("signed_theta");
  declareProperty("Target", "",
                  boost::make_shared<StringListValidator>(targetOptions),
                  "The unit to which the spectrum axis should be converted. "
                  "This can be either \"theta\" (for <math>\\theta</math> "
                  "degrees), or any of the IDs known to the [[Unit Factory]].");
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

void ConvertSpectrumAxis::exec() {
  // Get the input workspace
  MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");
  std::string unitTarget = getProperty("Target");
  // Loop over the original spectrum axis, finding the theta (n.b. not 2theta!)
  // for each spectrum
  // and storing it's corresponding workspace index
  // Map will be sorted on theta, so resulting axis will be ordered as well
  std::multimap<double, size_t> indexMap;
  const size_t nHist = inputWS->getNumberHistograms();
  const size_t nBins = inputWS->blocksize();
  const bool isHist = inputWS->isHistogramData();
  size_t nxBins;
  if (isHist) {
    nxBins = nBins + 1;
  } else {
    nxBins = nBins;
  }
  if (unitTarget != "theta" && unitTarget != "signed_theta") {
    Kernel::Unit_sptr fromUnit = inputWS->getAxis(0)->unit();
    Kernel::Unit_sptr toUnit = UnitFactory::Instance().create(unitTarget);
    IComponent_const_sptr source = inputWS->getInstrument()->getSource();
    IComponent_const_sptr sample = inputWS->getInstrument()->getSample();
    std::vector<double> emptyVector;
    const double l1 = source->getDistance(*sample);
    const std::string emodeStr = getProperty("EMode");
    int emode = 0;
    if (emodeStr == "Direct")
      emode = 1;
    else if (emodeStr == "Indirect")
      emode = 2;
    const double delta = 0.0;
    double efixed;
    for (size_t i = 0; i < nHist; i++) {
      std::vector<double> xval;
      xval.push_back(inputWS->readX(i).front());
      xval.push_back(inputWS->readX(i).back());
      IDetector_const_sptr detector = inputWS->getDetector(i);
      double twoTheta, l1val, l2;
      if (!detector->isMonitor()) {
        twoTheta = inputWS->detectorTwoTheta(detector);
        l2 = detector->getDistance(*sample);
        l1val = l1;
        efixed = getEfixed(detector, inputWS, emode); // get efixed
      } else {
        twoTheta = 0.0;
        l2 = l1;
        l1val = 0.0;
        efixed = DBL_MIN;
      }
      fromUnit->toTOF(xval, emptyVector, l1val, l2, twoTheta, emode, efixed,
                      delta);
      toUnit->fromTOF(xval, emptyVector, l1val, l2, twoTheta, emode, efixed,
                      delta);
      double value = (xval.front() + xval.back()) / 2;
      indexMap.insert(std::make_pair(value, i));
    }
  } else {
    // Set up binding to memeber funtion. Avoids condition as part of loop over
    // nHistograms.
    boost::function<double(IDetector_const_sptr)> thetaFunction;
    if (unitTarget.compare("signed_theta") == 0) {
      thetaFunction =
          boost::bind(&MatrixWorkspace::detectorSignedTwoTheta, inputWS, _1);
    } else {
      thetaFunction =
          boost::bind(&MatrixWorkspace::detectorTwoTheta, inputWS, _1);
    }

    bool warningGiven = false;
    for (size_t i = 0; i < nHist; ++i) {
      try {
        IDetector_const_sptr det = inputWS->getDetector(i);
        // Invoke relevant member function.
        indexMap.insert(std::make_pair(thetaFunction(det) * 180.0 / M_PI, i));
      } catch (Exception::NotFoundError &) {
        if (!warningGiven)
          g_log.warning("The instrument definition is incomplete - spectra "
                        "dropped from output");
        warningGiven = true;
      }
    }
  }
  // Create the output workspace. Can't re-use the input one because we'll be
  // re-ordering the spectra.
  MatrixWorkspace_sptr outputWS = WorkspaceFactory::Instance().create(
      inputWS, indexMap.size(), nxBins, nBins);
  // Now set up a new, numeric axis holding the theta values corresponding to
  // each spectrum
  NumericAxis *const newAxis = new NumericAxis(indexMap.size());
  outputWS->replaceAxis(1, newAxis);
  // The unit of this axis is radians. Use the 'radians' unit defined above.
  if (unitTarget == "theta" || unitTarget == "signed_theta") {
    newAxis->unit() = boost::shared_ptr<Unit>(new Units::Degrees);
  } else {
    newAxis->unit() = UnitFactory::Instance().create(unitTarget);
  }
  std::multimap<double, size_t>::const_iterator it;
  size_t currentIndex = 0;
  for (it = indexMap.begin(); it != indexMap.end(); ++it) {
    // Set the axis value
    newAxis->setValue(currentIndex, it->first);
    // Now copy over the data
    outputWS->dataX(currentIndex) = inputWS->dataX(it->second);
    outputWS->dataY(currentIndex) = inputWS->dataY(it->second);
    outputWS->dataE(currentIndex) = inputWS->dataE(it->second);
    // We can keep the spectrum numbers etc.
    outputWS->getSpectrum(currentIndex)
        ->copyInfoFrom(*inputWS->getSpectrum(it->second));
    ++currentIndex;
  }
  setProperty("OutputWorkspace", outputWS);
}

double ConvertSpectrumAxis::getEfixed(IDetector_const_sptr detector,
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
        Kernel::Property *p = inputWS->run().getProperty("Ei");
        Kernel::PropertyWithValue<double> *doublep =
            dynamic_cast<Kernel::PropertyWithValue<double> *>(p);
        efixed = (*doublep)();
      } else {
        efixed = 0.0;
        g_log.warning() << "Efixed could not be found for detector "
                        << detector->getID() << ", set to 0.0\n";
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
        efixed = 0.0;
        g_log.warning() << "Efixed could not be found for detector "
                        << detector->getID() << ", set to 0.0\n";
      }
    }
  }
  return efixed;
}
} // namespace Algorithms
} // namespace Mantid
