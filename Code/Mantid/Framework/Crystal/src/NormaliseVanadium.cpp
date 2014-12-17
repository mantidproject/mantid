//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCrystal/NormaliseVanadium.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidGeometry/Objects/ShapeFactory.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/Fast_Exponential.h"
#include "MantidKernel/VectorHelper.h"
#include "MantidKernel/Unit.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/V3D.h"

/*  Following A.J.Schultz's anvred, scaling the vanadium spectra:
 */

namespace Mantid {
namespace Crystal {

// Register the class into the algorithm factory
DECLARE_ALGORITHM(NormaliseVanadium)

using namespace Kernel;
using namespace Geometry;
using namespace API;
using namespace DataObjects;

NormaliseVanadium::NormaliseVanadium() : API::Algorithm() {}

void NormaliseVanadium::init() {
  // The input workspace must have an instrument and units of wavelength
  auto wsValidator = boost::make_shared<CompositeValidator>();
  wsValidator->add<InstrumentValidator>();

  declareProperty(new WorkspaceProperty<>("InputWorkspace", "",
                                          Direction::Input, wsValidator),
                  "The X values for the input workspace must be in units of "
                  "wavelength or TOF");
  declareProperty(
      new WorkspaceProperty<>("OutputWorkspace", "", Direction::Output),
      "Output workspace name");

  auto mustBePositive = boost::make_shared<BoundedValidator<double>>();
  mustBePositive->setLower(0.0);
  declareProperty("Wavelength", 1.0, mustBePositive,
                  "Normalizes spectra to this wavelength");
}

void NormaliseVanadium::exec() {
  // Retrieve the input workspace
  m_inputWS = getProperty("InputWorkspace");
  std::string unitStr = m_inputWS->getAxis(0)->unit()->unitID();

  // Get the input parameters
  double lambdanorm = getProperty("Wavelength"); // in 1/cm

  MatrixWorkspace_sptr correctionFactors =
      WorkspaceFactory::Instance().create(m_inputWS);

  const int64_t numHists =
      static_cast<int64_t>(m_inputWS->getNumberHistograms());
  const int64_t specSize = static_cast<int64_t>(m_inputWS->blocksize());

  const bool isHist = m_inputWS->isHistogramData();

  // If sample not at origin, shift cached positions.
  const V3D samplePos = m_inputWS->getInstrument()->getSample()->getPos();
  const V3D pos = m_inputWS->getInstrument()->getSource()->getPos() - samplePos;
  double L1 = pos.norm();

  Progress prog(this, 0.0, 1.0, numHists);
  // Loop over the spectra
  PARALLEL_FOR2(m_inputWS, correctionFactors)
  for (int64_t i = 0; i < int64_t(numHists); ++i) {
    //    PARALLEL_START_INTERUPT_REGION //FIXME: Restore

    // Get a reference to the Y's in the output WS for storing the factors
    MantidVec &Y = correctionFactors->dataY(i);
    MantidVec &E = correctionFactors->dataE(i);

    // Copy over bin boundaries
    const ISpectrum *inSpec = m_inputWS->getSpectrum(i);
    inSpec->lockData(); // for MRU-related thread safety

    const MantidVec &Xin = inSpec->readX();
    correctionFactors->dataX(i) = Xin;
    const MantidVec &Yin = inSpec->readY();
    const MantidVec &Ein = inSpec->readE();

    // Get detector position
    IDetector_const_sptr det;
    try {
      det = m_inputWS->getDetector(i);
    } catch (Exception::NotFoundError &) {
      // Catch if no detector. Next line tests whether this happened - test
      // placed
      // outside here because Mac Intel compiler doesn't like 'continue' in a
      // catch
      // in an openmp block.
    }
    // If no detector found, skip onto the next spectrum
    if (!det)
      continue;

    // This is the scattered beam direction
    Instrument_const_sptr inst = m_inputWS->getInstrument();
    V3D dir = det->getPos() - samplePos;
    double L2 = dir.norm();
    // Two-theta = polar angle = scattering angle = between +Z vector and the
    // scattered beam
    double scattering = dir.angle(V3D(0.0, 0.0, 1.0));

    Mantid::Kernel::Units::Wavelength wl;
    std::vector<double> timeflight;

    // Loop through the bins in the current spectrum
    double lambp = 0;
    double lambm = 0;
    double normp = 0;
    double normm = 0;
    for (int64_t j = 0; j < specSize; j++) {
      timeflight.push_back((isHist ? (0.5 * (Xin[j] + Xin[j + 1])) : Xin[j]));
      if (unitStr.compare("TOF") == 0)
        wl.fromTOF(timeflight, timeflight, L1, L2, scattering, 0, 0, 0);
      const double lambda = timeflight[0];
      if (lambda > lambdanorm) {
        lambp = lambda;
        normp = Yin[j];
        break;
      }
      lambm = lambda;
      normm = Yin[j];
      timeflight.clear();
    }
    double normvalue =
        normm + (lambdanorm - lambm) * (normp - normm) / (lambp - lambm);
    for (int64_t j = 0; j < specSize; j++) {
      Y[j] = Yin[j] / normvalue;
      E[j] = Ein[j] / normvalue;
    }

    inSpec->unlockData();

    prog.report();

    //    PARALLEL_END_INTERUPT_REGION
  }
  //  PARALLEL_CHECK_INTERUPT_REGION

  setProperty("OutputWorkspace", correctionFactors);
}

} // namespace Crystal
} // namespace Mantid
