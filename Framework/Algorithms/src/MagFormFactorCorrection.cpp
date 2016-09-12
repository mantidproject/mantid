//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/MagFormFactorCorrection.h"
#include "MantidKernel/MagneticIon.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"

using namespace Mantid::PhysicalConstants;

namespace Mantid {
namespace Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(MagFormFactorCorrection)

using namespace Kernel;
using namespace API;
using namespace PhysicalConstants;

void MagFormFactorCorrection::init() {
  declareProperty(
      make_unique<WorkspaceProperty<>>("InputWorkspace", "", Direction::Input),
      "Workspace must have one axes with units of Q");
  declareProperty(make_unique<WorkspaceProperty<>>("OutputWorkspace", "",
                                                   Direction::Output),
                  "Output workspace name.");
  declareProperty("IonName", "", "The name of the ion: an element symbol with "
                                 "a number indicating the valence, e.g. Fe2");
  declareProperty("FormFactorWorkspace", "",
                  "If specified the algorithm will create a 1D workspace with "
                  "the form factor vs Q with a name given by this field.");
}

void MagFormFactorCorrection::exec() {
  int64_t iax, numAxes, nQ, iQ;
  bool hasQ = false;
  std::string unitID;
  Axis *QAxis;
  std::vector<double> Qvals, FF;
  double ff;
  MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");
  MatrixWorkspace_sptr outputWS = getProperty("OutputWorkspace");
  const std::string ionNameStr = getProperty("IonName");
  const std::string ffwsStr = getProperty("FormFactorWorkspace");
  const bool isHist = inputWS->isHistogramData();
  const int64_t numHists = inputWS->getNumberHistograms();
  const int64_t specSize = inputWS->blocksize();

  // Checks that there is a |Q| axis.
  numAxes = inputWS->axes();
  for (iax = 0; iax < numAxes; iax++) {
    QAxis = inputWS->getAxis(iax);
    unitID = QAxis->unit()->unitID();
    if (unitID == "MomentumTransfer") {
      hasQ = true;
      // Gets the list of Q values
      if (isHist || iax > 0) {
        nQ = QAxis->length() - 1;
        for (iQ = 0; iQ < nQ; iQ++) {
          Qvals.push_back(0.5 * (QAxis->getValue(static_cast<size_t>(iQ)) +
                                 QAxis->getValue(static_cast<size_t>(iQ + 1))));
        }
      } else {
        nQ = QAxis->length();
        for (iQ = 0; iQ < nQ; iQ++) {
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
  const MagneticIon ion = getMagneticIon(ionNameStr);
  // Gets the vector of form factor values
  FF.reserve(nQ);
  for (iQ = 0; iQ < nQ; iQ++) {
    FF.push_back(ion.analyticalFormFactor(Qvals[iQ] * Qvals[iQ], 0, 0));
  }
  if (!ffwsStr.empty()) {
    MatrixWorkspace_sptr ffws = API::WorkspaceFactory::Instance().create(
        "Workspace2D", 1, Qvals.size(), FF.size());
    ffws->mutableX(0).assign(Qvals.begin(), Qvals.end());
    ffws->mutableY(0).assign(FF.begin(), FF.end());
    API::AnalysisDataService::Instance().addOrReplace(ffwsStr, ffws);
  }

  // Does the actual scaling.
  outputWS = inputWS->clone();
  for (int64_t i = 0; i < numHists; i++) {
    auto &Y = outputWS->mutableY(i);
    auto &E = outputWS->mutableE(i);
    for (int64_t j = 0; j < specSize; j++) {
      ff = (iax == 0) ? FF[j] : FF[i];
      Y[j] /= ff;
      E[j] /= ff;
    }
  }

  setProperty("OutputWorkspace", outputWS);
}

} // namespace Algorithms
} // namespace Mantid
