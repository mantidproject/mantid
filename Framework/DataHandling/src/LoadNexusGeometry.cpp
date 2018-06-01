#include <iostream>

#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/FileProperty.h"
#include "MantidGeometry/Instrument.h"
#include "MantidDataHandling/LoadNexusGeometry.h"
#include "MantidNexusGeometry/NexusGeometryParser.h"

namespace Mantid {
namespace DataHandling {

using namespace API;
using namespace Kernel;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(LoadNexusGeometry)

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string LoadNexusGeometry::name() const {
  return "LoadNexusGeometry";
}

/// Algorithm's version for identification. @see Algorithm::version
int LoadNexusGeometry::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string LoadNexusGeometry::category() const {
  return "DataHandling\\Nexus";
}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string LoadNexusGeometry::summary() const {
  return "Loads an instrument from OFF nexus geometry file into an empty "
         "workspace";
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void LoadNexusGeometry::init() {
  const std::vector<std::string> extensions{".nxs", ".hdf5"};
  declareProperty(Kernel::make_unique<FileProperty>(
                      "Filename", "", FileProperty::Load, extensions),
                  "The name of the Nexus file to read geometry from, as a full "
                  "or relative path.");

  declareProperty(Kernel::make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      "OutputWorkspace", "", Direction::Output),
                  "An output workspace.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void LoadNexusGeometry::exec() {
  std::string fileName = getProperty("Filename");
  auto workspace = WorkspaceFactory::Instance().create("Workspace2D", 1, 2, 1);

  auto instrument =
      NexusGeometry::NexusGeometryParser::createInstrument(fileName);
  workspace->setInstrument(std::move(instrument));
  workspace->populateInstrumentParameters();
  setProperty("OutputWorkspace", workspace);
}

} // namespace DataHandling
} // namespace Mantid
