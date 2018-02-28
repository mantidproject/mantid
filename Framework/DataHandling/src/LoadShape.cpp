#include "MantidDataHandling/LoadShape.h"

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/InstrumentValidator.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"

#include "MantidGeometry/Instrument.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/EnabledWhenProperty.h"
#include "MantidKernel/Exception.h"

#include <Poco/File.h>

namespace Mantid {
namespace DataHandling {
// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(LoadShape)

using namespace Kernel;
using namespace API;
using namespace Geometry;

void LoadShape::init() {
  auto wsValidator = boost::make_shared<CompositeValidator>();
  wsValidator->add<API::InstrumentValidator>();

  // input workspace
  declareProperty(
      make_unique<WorkspaceProperty<>>("InputWorkspace", "", Direction::Input,
                                       wsValidator),
      "The name of the workspace containing the instrument to add the shape");

  // shape file
  const std::vector<std::string> extensions{ ".stl" };
  declareProperty(
       make_unique<FileProperty>(
      "Filename", "", FileProperty::Load, extensions),
      "The name of the file containing the shape. "
      "Extension must be .stl");

  // attach to sample
  declareProperty("Attach to sample", false,
    "If true, the shape will be attached to the sample,"
    "else you need to specify the component to which it is attached.");

  // component name
  declareProperty<std::string>("Component name", "",
    "Name of component, to which to attach shape.");
  setPropertySettings("Component name", make_unique<EnabledWhenProperty>(
    "Attach to Sample", IS_EQUAL_TO, "0"));

  // Output workspace
  declareProperty(
       make_unique<WorkspaceProperty<Workspace>>(
       "OutputWorkspace", "", Direction::Output),
       "The name of the workspace that will be same as"
       "the input workspace but with shape added to it");

}

/**
* Return the confidence with with this algorithm can load the file
* @param descriptor A descriptor for the file
* @returns An integer specifying the confidence level. 0 indicates it will not
* be used
*/
int LoadShape::confidence(Kernel::FileDescriptor &descriptor) const {
  const std::string &filePath = descriptor.filename();
  const size_t filenameLength = filePath.size();

  // Avoid some known file types that have different loaders
  int confidence(0);
  if (filePath.compare(filenameLength - 4, 4, ".stl") == 0 ) {
    confidence = 90;
  }
  return confidence;
}

void LoadShape::exec() {

  MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");

  MatrixWorkspace_sptr outputWS = getProperty("OutputWorkspace");

  if (inputWS != outputWS) {
    outputWS = inputWS->clone();
  }
  Instrument_const_sptr inputInstr = inputWS->getInstrument();
  Instrument* outputInstr = inputInstr->clone();
  if (outputInstr == nullptr) {
    throw std::runtime_error("Unable to obtain instrument to add loaded shape to");
  }

  boost::shared_ptr<Component> component = nullptr;
  bool attachToSample = getProperty("Attach to sample");
  if (!attachToSample) {
    std::string compName = getProperty("Component name");
    component = outputInstr->getComponentByName(compName);
  }
  else {
    component = outputInstr->getSample();
  }

  std::string filename = getProperty("Filename");
  std::ifstream file(filename.c_str());
  if (!file) {
    g_log.error("Unable to open file: " + filename);
    throw Exception::FileError("Unable to open file: ", filename);
  }

  boost::shared_ptr<MeshObject> mShape = getMeshObject(file);

  // Still to figure out how to add shape to instrument at componenent.

}

boost::shared_ptr<MeshObject> LoadShape::getMeshObject(std::ifstream& file) {
  return nullptr;
}

} // end DataHandling namespace
} // end MantidNamespace
