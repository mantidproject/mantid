// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/ConvertSpectrumAxis.h"
#include "MantidAPI/InstrumentValidator.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/SpectraAxisValidator.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidGeometry/Instrument.h"
#include "MantidHistogramData/Histogram.h"
#include "MantidHistogramData/HistogramBuilder.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/Unit.h"
#include "MantidKernel/UnitFactory.h"

#include <boost/bind.hpp>
#include <boost/function.hpp>

#include <cfloat>

namespace Mantid {
namespace Algorithms {
// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ConvertSpectrumAxis)
using namespace Kernel;
using namespace API;
using namespace Geometry;
using namespace DataObjects;
using namespace HistogramData;
namespace {
constexpr double rad2deg = 180. / M_PI;
}

void ConvertSpectrumAxis::init() {
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
  std::vector<std::string> targetOptions =
      Mantid::Kernel::UnitFactory::Instance().getKeys();
  targetOptions.emplace_back("theta");
  targetOptions.emplace_back("signed_theta");
  declareProperty("Target", "",
                  boost::make_shared<StringListValidator>(targetOptions),
                  "The unit to which the spectrum axis should be converted. "
                  "This can be either \"theta\" (for <math>\\theta</math> "
                  "degrees), or any of the IDs known to the [[Unit Factory]].");
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
}

void ConvertSpectrumAxis::exec() {
  // Get the input workspace
  MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");
  std::string unitTarget = getProperty("Target");
  // Loop over the original spectrum axis, finding the theta (n.b. not 2theta!)
  // for each spectrum and storing it's corresponding workspace index
  // Map will be sorted on theta, so resulting axis will be ordered as well
  std::multimap<double, size_t> indexMap;
  const size_t nHist = inputWS->getNumberHistograms();
  const size_t nBins = inputWS->blocksize();
  const bool isHist = inputWS->isHistogramData();
  auto &spectrumInfo = inputWS->spectrumInfo();
  size_t nxBins;
  if (isHist) {
    nxBins = nBins + 1;
  } else {
    nxBins = nBins;
  }
  if (unitTarget != "theta" && unitTarget != "signed_theta") {
    Kernel::Unit_sptr fromUnit = inputWS->getAxis(0)->unit();
    Kernel::Unit_sptr toUnit = UnitFactory::Instance().create(unitTarget);
    std::vector<double> emptyVector;
    const double l1 = spectrumInfo.l1();
    const std::string emodeStr = getProperty("EMode");
    int emode = 0;
    if (emodeStr == "Direct")
      emode = 1;
    else if (emodeStr == "Indirect")
      emode = 2;
    const double delta = 0.0;
    double efixed;
    for (size_t i = 0; i < nHist; i++) {
      std::vector<double> xval{inputWS->x(i).front(), inputWS->x(i).back()};
      double twoTheta, l1val, l2;
      if (!spectrumInfo.isMonitor(i)) {
        twoTheta = spectrumInfo.twoTheta(i);
        l2 = spectrumInfo.l2(i);
        l1val = l1;
        efixed =
            getEfixed(spectrumInfo.detector(i), inputWS, emode); // get efixed
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
      indexMap.emplace(value, i);
    }
  } else {
    // Set up binding to memeber funtion. Avoids condition as part of loop over
    // nHistograms.
    boost::function<double(const IDetector &)> thetaFunction;
    if (unitTarget == "signed_theta") {
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
        indexMap.emplace(thetaFunction(*det) * rad2deg, i);
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
  HistogramBuilder builder;
  builder.setX(nxBins);
  builder.setY(nBins);
  builder.setDistribution(inputWS->isDistribution());
  MatrixWorkspace_sptr outputWS =
      create<MatrixWorkspace>(*inputWS, indexMap.size(), builder.build());
  // Now set up a new, numeric axis holding the theta values corresponding to
  // each spectrum
  auto const newAxis = new NumericAxis(indexMap.size());
  outputWS->replaceAxis(1, newAxis);
  // The unit of this axis is radians. Use the 'radians' unit defined above.
  if (unitTarget == "theta" || unitTarget == "signed_theta") {
    newAxis->unit() = boost::make_shared<Units::Degrees>();
  } else {
    newAxis->unit() = UnitFactory::Instance().create(unitTarget);
  }
  std::multimap<double, size_t>::const_iterator it;
  size_t currentIndex = 0;
  for (it = indexMap.begin(); it != indexMap.end(); ++it) {
    // Set the axis value
    newAxis->setValue(currentIndex, it->first);
    // Now copy over the data
    outputWS->setHistogram(currentIndex, inputWS->histogram(it->second));
    // We can keep the spectrum numbers etc.
    outputWS->getSpectrum(currentIndex)
        .copyInfoFrom(inputWS->getSpectrum(it->second));
    ++currentIndex;
  }
  setProperty("OutputWorkspace", outputWS);
}

double
ConvertSpectrumAxis::getEfixed(const Mantid::Geometry::IDetector &detector,
                               MatrixWorkspace_const_sptr inputWS,
                               int emode) const {
  double efixed(0);
  double efixedProp = getProperty("Efixed");
  if (efixedProp != EMPTY_DBL()) {
    efixed = efixedProp;
    g_log.debug() << "Detector: " << detector.getID() << " Efixed: " << efixed
                  << "\n";
  } else {
    if (emode == 1) {
      if (inputWS->run().hasProperty("Ei")) {
        Kernel::Property *p = inputWS->run().getProperty("Ei");
        Kernel::PropertyWithValue<double> *doublep =
            dynamic_cast<Kernel::PropertyWithValue<double> *>(p);
        if (doublep) {
          efixed = (*doublep)();
        } else {
          efixed = 0.0;
          g_log.warning() << "Efixed could not be found for detector "
                          << detector.getID() << ", set to 0.0\n";
        }
      } else {
        efixed = 0.0;
        g_log.warning() << "Efixed could not be found for detector "
                        << detector.getID() << ", set to 0.0\n";
      }
    } else if (emode == 2) {
      std::vector<double> efixedVec = detector.getNumberParameter("Efixed");
      if (efixedVec.empty()) {
        int detid = detector.getID();
        IDetector_const_sptr detectorSingle =
            inputWS->getInstrument()->getDetector(detid);
        efixedVec = detectorSingle->getNumberParameter("Efixed");
      }
      if (!efixedVec.empty()) {
        efixed = efixedVec.at(0);
        g_log.debug() << "Detector: " << detector.getID()
                      << " EFixed: " << efixed << "\n";
      } else {
        efixed = 0.0;
        g_log.warning() << "Efixed could not be found for detector "
                        << detector.getID() << ", set to 0.0\n";
      }
    }
  }
  return efixed;
}
} // namespace Algorithms
} // namespace Mantid
