// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/MagFormFactorCorrection.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidHistogramData/Histogram.h"
#include "MantidHistogramData/HistogramBuilder.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/MagneticIon.h"
#include "MantidKernel/Unit.h"
#include "MantidKernel/UnitFactory.h"
using namespace Mantid::PhysicalConstants;

namespace Mantid {
namespace Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(MagFormFactorCorrection)

using namespace Kernel;
using namespace API;
using namespace PhysicalConstants;
using namespace Mantid::DataObjects;
using namespace Mantid::HistogramData;

void MagFormFactorCorrection::init() {
  declareProperty(
      make_unique<WorkspaceProperty<>>("InputWorkspace", "", Direction::Input),
      "Workspace must have one axis with units of Q");
  declareProperty(make_unique<WorkspaceProperty<>>("OutputWorkspace", "",
                                                   Direction::Output),
                  "Output workspace name.");
  std::vector<std::string> keys = getMagneticIonList();
  declareProperty("IonName", "Cu2",
                  boost::make_shared<StringListValidator>(keys),
                  "The name of the ion: an element symbol with a number "
                  "indicating the valence, e.g. Fe2 for Fe2+ / Fe(II)");
  declareProperty(make_unique<WorkspaceProperty<>>("FormFactorWorkspace", "",
                                                   Direction::Output,
                                                   PropertyMode::Optional),
                  "If specified the algorithm will create a 1D workspace with "
                  "the form factor vs Q with a name given by this field.");
}

void MagFormFactorCorrection::exec() {
  MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");
  const std::string ionNameStr = getProperty("IonName");
  const std::string ffwsStr = getProperty("FormFactorWorkspace");
  const bool isHist = inputWS->isHistogramData();
  const int64_t numHists = inputWS->getNumberHistograms();
  const int64_t specSize = inputWS->blocksize();

  // Checks that there is a |Q| axis.
  int64_t numAxes = inputWS->axes();
  bool hasQ = false;
  std::vector<double> Qvals;
  int64_t iax;
  for (iax = 0; iax < numAxes; iax++) {
    Axis *QAxis = inputWS->getAxis(iax);
    std::string unitID = QAxis->unit()->unitID();
    if (unitID == "MomentumTransfer") {
      hasQ = true;
      // Gets the list of Q values
      if (isHist || iax > 0) {
        int64_t nQ = QAxis->length() - 1;
        for (int64_t iQ = 0; iQ < nQ; iQ++) {
          Qvals.push_back(0.5 * (QAxis->getValue(static_cast<size_t>(iQ)) +
                                 QAxis->getValue(static_cast<size_t>(iQ + 1))));
        }
      } else {
        int64_t nQ = QAxis->length();
        for (int64_t iQ = 0; iQ < nQ; iQ++) {
          Qvals.push_back(QAxis->getValue(static_cast<size_t>(iQ)));
        }
      }
      break;
    }
  }
  if (!hasQ) {
    throw std::runtime_error("Expected a MatrixWorkspace with a "
                             "MomentumTransfer axis. Cannot apply form factor "
                             "without knowing Q.");
  }

  // Parses the ion name and get handle to MagneticIon object
  const MagneticIon &ion = getMagneticIon(ionNameStr);
  // Gets the vector of form factor values
  std::vector<double> FF;
  FF.reserve(Qvals.size());
  for (double Qval : Qvals) {
    FF.push_back(ion.analyticalFormFactor(Qval * Qval));
  }
  if (!ffwsStr.empty()) {
    HistogramBuilder builder;
    builder.setX(Qvals.size());
    builder.setY(FF.size());
    MatrixWorkspace_sptr ffws = create<Workspace2D>(1, builder.build());
    ffws->mutableX(0).assign(Qvals.begin(), Qvals.end());
    ffws->mutableY(0).assign(FF.begin(), FF.end());
    ffws->getAxis(0)->unit() =
        UnitFactory::Instance().create("MomentumTransfer");
    ffws->setYUnitLabel("F(Q)");
    setProperty("FormFactorWorkspace", ffws);
  }

  // Does the actual scaling.
  MatrixWorkspace_sptr outputWS = inputWS->clone();
  for (int64_t i = 0; i < numHists; i++) {
    auto &Y = outputWS->mutableY(i);
    auto &E = outputWS->mutableE(i);
    for (int64_t j = 0; j < specSize; j++) {
      double ff = (iax == 0) ? FF[j] : FF[i];
      // Sometimes ff can be negative due to analytical approximation to the
      // exact calculation. Catch these, and also case of ff=0 (where there
      // should be no magnetic scattering).
      if (ff < 0.01) {
        Y[j] = NAN;
        E[j] = NAN;
      } else {
        Y[j] /= (ff * ff); // Magnetic intensity proportional |F(Q)|^2
        E[j] /= (ff * ff);
      }
    }
  }

  setProperty("OutputWorkspace", outputWS);
}

} // namespace Algorithms
} // namespace Mantid
