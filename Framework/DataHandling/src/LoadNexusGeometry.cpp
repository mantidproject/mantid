#include <iostream>

#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidGeometry/Instrument.h"
#include "MantidDataHandling/LoadNexusGeometry.h"
#include "MantidNexusGeometry/NexusGeometryParser.h"
#include "MantidNexusGeometry/ParsingErrors.h"

#include <memory>
#include <string>

namespace Mantid {
namespace DataHandling {

using namespace API;
using namespace Kernel;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(LoadNexusGeometry)

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string LoadNexusGeometry::name() const { return "LoadNexusGeometry"; }

/// Algorithm's version for identification. @see Algorithm::version
int LoadNexusGeometry::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string LoadNexusGeometry::category() const {
  return "DataHandling\\Nexus";
}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string LoadNexusGeometry::summary() const {
  return "Loads an instrument from OFF nexus geometry file";
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void LoadNexusGeometry::init() {
  declareProperty("Filename" "", "Name of OFF Nexus Geometry File");
  declareProperty("InstrumentName", "", "Name of Instrument");

  declareProperty(
      Kernel::make_unique<WorkspaceProperty<MatrixWorkspace>>("OutputWorkspace", "",
                                                             Direction::Output),
      "An output workspace.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void LoadNexusGeometry::exec() {
    std::string fileName = getProperty("Filename");
    std::string instName = getProperty("InstrumentName");
    auto workspace = WorkspaceFactory::Instance().create("Workspace2D", 1, 2, 1);

    NexusGeometry::iAbstractBuilder_sptr iAbsBuilder_sptr = std::shared_ptr<NexusGeometry::iAbstractBuilder>(new NexusGeometry::iAbstractBuilder(instName));

    NexusGeometry::NexusGeometryParser OFFparser = NexusGeometry::NexusGeometryParser(fileName, iAbsBuilder_sptr);
    //Parse nexus geomtry
    NexusGeometry::ParsingErrors exitStatus = OFFparser.parseNexusGeometry();
    std::cout << exitStatus << std::endl;
    // Add instrument to the workspace
    iAbsBuilder_sptr->_unAbstractInstrument()->parseTreeAndCacheBeamline();
    workspace->setInstrument(iAbsBuilder_sptr->_unAbstractInstrument());
    workspace->populateInstrumentParameters();
    setProperty("OutputWorkspace", workspace);
}

} // namespace DataHandling
} // namespace Mantid
