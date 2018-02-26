#include "MantidDataHandling/LoadShape.h"

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/InstrumentValidator.h"
#include "MantidAPI/WorkspaceFactory.h"

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

void LoadShape::exec() {

}

} // end DataHandling namespace
} // end MantidNamespace
