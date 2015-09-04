//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "MantidAlgorithms/SampleCorrections/SampleMaterialCorrections.h"
#include "MantidAlgorithms/SampleCorrections/MayersSampleCorrectionStrategy.h"
#include "MantidAPI/Progress.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include "MantidKernel/CompositeValidator.h"

namespace Mantid {
namespace Algorithms {

using API::MatrixWorkspace_sptr;
namespace Exception = Kernel::Exception;
using Geometry::IDetector_const_sptr;
using Kernel::Direction;
using Kernel::V3D;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SampleMaterialCorrections)

//------------------------------------------------------------------------------
// Public members
//------------------------------------------------------------------------------

/**
 * Constructor
 */
SampleMaterialCorrections::SampleMaterialCorrections() : API::Algorithm() {}

/// Algorithms name for identification. @see Algorithm::name
const std::string SampleMaterialCorrections::name() const {
  return "SampleMaterialCorrections";
}

/// Algorithm's version for identification. @see Algorithm::version
int SampleMaterialCorrections::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string SampleMaterialCorrections::category() const {
  return "Corrections";
}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string SampleMaterialCorrections::summary() const {
  return "Corrects the input data for the effects of attenuation & multiple "
         "scattering";
}

/** Initialize the algorithm's properties.
 */
void SampleMaterialCorrections::init() {
  using API::WorkspaceProperty;
  // Inputs
  declareProperty(new WorkspaceProperty<>("InputWorkspace", "",
                                          Direction::Input,
                                          createInputWSValidator()),
                  "An input workspace.");

  // Outputs
  declareProperty(
      new WorkspaceProperty<>("OutputWorkspace", "", Direction::Output),
      "An output workspace.");
}

/**
 */
void SampleMaterialCorrections::exec() {
  using API::Progress;
  using API::WorkspaceFactory;
  MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");
  MatrixWorkspace_sptr outputWS = WorkspaceFactory::Instance().create(inputWS);

  // Instrument constants
  auto instrument = inputWS->getInstrument();
  const auto source = instrument->getSource();
  const auto sample = instrument->getSample();
  const auto beamLine = sample->getPos() - source->getPos();
  const auto frame = instrument->getReferenceFrame();
  const double l1 = sample->getDistance(*source);

  // Sample
  const auto &sampleShape = inputWS->sample().getShape();
  // Current Object code computes quite an inaccurate bounding box so we do
  // something better for the time being
  const double big(100.); // seems to be a sweet spot...
  double minX(-big), maxX(big), minY(-big), maxY(big), minZ(-big), maxZ(big);
  sampleShape.getBoundingBox(maxX, maxY, maxZ, minX, minY, minZ);
  V3D boxWidth(maxX - minX, maxY - minY, maxZ - minZ);
  const double radius(0.5 * boxWidth[frame->pointingHorizontal()]),
      height(boxWidth[frame->pointingUp()]);
  const auto &sampleMaterial = sampleShape.material();

  const size_t nhist(inputWS->getNumberHistograms());
  Progress prog(this, 0., 1., nhist);
  prog.setNotifyStep(0.01);
  for (size_t i = 0; i < nhist; ++i) {
    // Copy the X values over
    const auto &inX = inputWS->readX(i);
    outputWS->dataX(i) = inX;
    IDetector_const_sptr det;
    try {
      det = inputWS->getDetector(i);
    } catch (Exception::NotFoundError &) {
      continue;
    }
    if (det->isMonitor() || det->isMasked())
      continue;

    MayersSampleCorrectionStrategy::Parameters params;
    params.l1 = l1;
    params.l2 = det->getDistance(*sample);
    params.twoTheta = det->getTwoTheta(sample->getPos(), beamLine);
    params.phi = det->getPhi();
    params.rho = sampleMaterial.numberDensity();
    params.sigmaAbs = sampleMaterial.absorbXSection();
    params.sigmaSc = sampleMaterial.totalScatterXSection();
    params.cylRadius = radius;
    params.cylHeight = height;
    MayersSampleCorrectionStrategy correction(params, inX, inputWS->readY(i),
                                      inputWS->readE(i));
    correction.apply(outputWS->dataY(i), outputWS->dataE(i));
    prog.report();
  }

  setProperty("OutputWorkspace", outputWS);
}

//------------------------------------------------------------------------------
// Private members
//------------------------------------------------------------------------------
/**
 * @return The validator required for the input workspace
 */
Kernel::IValidator_sptr
SampleMaterialCorrections::createInputWSValidator() const {
  using API::InstrumentValidator;
  using API::SampleValidator;
  using Kernel::CompositeValidator;
  auto validator = boost::make_shared<CompositeValidator>();

  unsigned int requires = (InstrumentValidator::SamplePosition |
                           InstrumentValidator::SourcePosition);
  validator->add<InstrumentValidator, unsigned int>(requires);

  requires = (SampleValidator::Shape | SampleValidator::Material);
  validator->add<SampleValidator, unsigned int>(requires);

  return validator;
}

} // namespace Algorithms
} // namespace Mantid
