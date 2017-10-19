#include "MantidAlgorithms/GravityCorrection.h"

#include "MantidAPI/HistogramValidator.h"
#include "MantidAPI/InstrumentValidator.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/PhysicalConstants.h"

#include "MantidAPI/Run.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/ComponentHelper.h"

namespace Mantid {
namespace Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(GravityCorrection)

using namespace API;
using namespace Kernel;

// using namespace API::WorkspaceProperty;
// using namespace API::WorkspaceUnitValidator;
// using namespace Kernel::Direction;
// using namespace Kernel::CompositeValidator;
// using namespace Kernel::BoundedValidator;
// using namespace API::HistogramValidator;
// using namespace API::InstrumentValidator;

void GravityCorrection::init() {
  auto wsValidator = boost::make_shared<CompositeValidator>();
  wsValidator->add<WorkspaceUnitValidator>("TOF");
  wsValidator->add<HistogramValidator>();
  wsValidator->add<InstrumentValidator>();
  declareProperty(make_unique<WorkspaceProperty<>>(
                      "InputWorkspace", "", Direction::Input, wsValidator),
                  "The name of the input Workspace2D. X and Y values must be "
                  "TOF and counts, respectively.");
  declareProperty(make_unique<WorkspaceProperty<>>("OutputWorkspace", "",
                                                   Direction::Output),
                  "The name of the output Workspace2D");
  declareProperty("FirstSlitName", "slit1",
                  "Component name of the first slit.");
  declareProperty("SecondSlitName", "slit2",
                  "Component name of the second slit.");
}

/**
 * Validate inputs
 * @returns a string map containing the error messages
 */
std::map<std::string, std::string> GravityCorrection::validateInputs() {
  std::map<std::string, std::string> result;
  if (!getPointerToProperty("InputWorkspace")->isDefault()) {
    result["InputWorkspace"] = "InputWorkspace not defined.";
  }
  // Detector must be a line in xy plane

  // Check slit positions

  return result;
}

void GravityCorrection::exec() {

  const MatrixWorkspace_sptr inWS = getProperty("InputWorkspace");
  const std::string slit1Name = getProperty("FirstSlitName");
  const std::string slit2Name = getProperty("SecondSlitName");

  Mantid::API::MatrixWorkspace_sptr outWS = getProperty("OutputWorkspace");
  outWS = inWS->clone();

  Mantid::Geometry::Instrument_const_sptr instrument = inWS->getInstrument();
  Mantid::Geometry::IComponent_const_sptr slit1 =
      instrument->getComponentByName(slit1Name);
  Mantid::Geometry::IComponent_const_sptr slit2 =
      instrument->getComponentByName(slit2Name);

  if (!slit1)
    throw std::runtime_error(
        "Could not find instrument component with name: '" + slit1Name + "'");

  if (!slit2)
    throw std::runtime_error(
        "Could not find instrument component with name: '" + slit2Name + "'");

  const auto &spectrumInfo = inWS->spectrumInfo();

  const Kernel::V3D samplePos = spectrumInfo.samplePosition();
  const Kernel::V3D sourcePos = spectrumInfo.sourcePosition();

  setProperty("OutputWorkspace", outWS);
}
}
}
