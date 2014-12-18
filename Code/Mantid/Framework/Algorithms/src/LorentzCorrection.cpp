#include "MantidAlgorithms/LorentzCorrection.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Progress.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidKernel/MultiThreaded.h"
#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <cmath>

namespace Mantid {
namespace Algorithms {
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;
using Mantid::API::WorkspaceProperty;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(LorentzCorrection)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
LorentzCorrection::LorentzCorrection() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
LorentzCorrection::~LorentzCorrection() {}

//----------------------------------------------------------------------------------------------

/// Algorithm's version for identification. @see Algorithm::version
int LorentzCorrection::version() const { return 1; };

/// Algorithm's category for identification. @see Algorithm::category
const std::string LorentzCorrection::category() const { return "Crystal"; }

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string LorentzCorrection::summary() const {
  return "Performs a white beam Lorentz Correction";
};

const std::string LorentzCorrection::name() const {
  return "LorentzCorrection";
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void LorentzCorrection::init() {

  declareProperty(new WorkspaceProperty<MatrixWorkspace>(
                      "InputWorkspace", "", Direction::Input,
                      PropertyMode::Mandatory,
                      boost::make_shared<WorkspaceUnitValidator>("Wavelength")),
                  "Input workspace to correct in Wavelength.");
  declareProperty(new WorkspaceProperty<MatrixWorkspace>("OutputWorkspace", "",
                                                         Direction::Output),
                  "An output workspace.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void LorentzCorrection::exec() {
  MatrixWorkspace_sptr inWS = this->getProperty("InputWorkspace");

  auto cloneAlg = this->createChildAlgorithm("CloneWorkspace", 0, 0.1);
  cloneAlg->initialize();
  cloneAlg->setProperty("InputWorkspace", inWS);
  cloneAlg->execute();
  Workspace_sptr temp = cloneAlg->getProperty("OutputWorkspace");
  MatrixWorkspace_sptr outWS =
      boost::dynamic_pointer_cast<MatrixWorkspace>(temp);

  const auto numHistos = inWS->getNumberHistograms();
  Progress prog(this, 0, 1, numHistos);
  const bool isHist = inWS->isHistogramData();

  PARALLEL_FOR1(inWS)
  for (int64_t i = 0; i < int64_t(numHistos); ++i) {
    PARALLEL_START_INTERUPT_REGION

    const MantidVec &inY = inWS->readY(i);
    const MantidVec &inX = inWS->readX(i);

    MantidVec &outY = outWS->dataY(i);
    MantidVec &outE = outWS->dataE(i);

    IDetector_const_sptr detector;
    try {
      detector = inWS->getDetector(i);
    } catch (Exception::NotFoundError &) {
      // Catch if no detector. Next line tests whether this happened - test
      // placed
      // outside here because Mac Intel compiler doesn't like 'continue' in a
      // catch
      // in an openmp block.
    }
    // If no detector found, skip onto the next spectrum
    if (!detector)
      continue;

    const double twoTheta = inWS->detectorTwoTheta(detector);
    const double sinTheta = std::sin(twoTheta / 2);
    double sinThetaSq = sinTheta * sinTheta;

    for (size_t j = 0; j < inY.size(); ++j) {
      const double wL = isHist ? (0.5 * (inX[j] + inX[j + 1])) : inX[j];
      if (wL == 0) {
        std::stringstream buffer;
        buffer << "Cannot have zero values Wavelength. At workspace index: "
               << i;
        throw std::runtime_error(buffer.str());
      }

      double weight = sinThetaSq / (wL * wL * wL * wL);
      outY[j] *= weight;
      outE[j] *= weight;
    }

    prog.report();

    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION

  this->setProperty("OutputWorkspace", outWS);
}

} // namespace Algorithms
} // namespace Mantid
