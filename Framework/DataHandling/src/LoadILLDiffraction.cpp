#include "MantidDataHandling/LoadILLDiffraction.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/RegisterFileLoader.h"
#include "MantidAPI/WorkspaceFactory.h"

#include <nexus/napi.h>

namespace Mantid {
namespace DataHandling {

using namespace API;
using namespace Kernel;
using namespace NeXus;

// Register the algorithm into the AlgorithmFactory
DECLARE_NEXUS_FILELOADER_ALGORITHM(LoadILLDiffraction)

int LoadILLDiffraction::confidence(NexusDescriptor &descriptor) const {

  // fields existent only at the ILL Diffraction
  if (descriptor.pathExists("/entry0/data_scan")) {
    return 80;
  } else {
    return 0;
  }
}

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string LoadILLDiffraction::name() const {
  return "LoadILLDiffraction";
}

/// Algorithm's version for identification. @see Algorithm::version
int LoadILLDiffraction::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string LoadILLDiffraction::category() const {
  return "DataHandling\\Nexus";
}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string LoadILLDiffraction::summary() const {
  return "Loads ILL diffraction nexus files.";
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void LoadILLDiffraction::init() {
  declareProperty(
      make_unique<FileProperty>("Filename", "", FileProperty::Load, ".nxs"),
      "File path of the Data file to load");
  declareProperty(Kernel::make_unique<WorkspaceProperty<API::Workspace>>(
                      "OutputWorkspace", "", Direction::Output),
                  "An output workspace.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void LoadILLDiffraction::exec() {

  // open the root node
  NeXus::NXRoot dataRoot(getPropertyValue("Filename"));
  NXEntry firstEntry = dataRoot.openFirstEntry();

  // read in the data
  NXData dataGroup = firstEntry.openNXData("data_scan/detector_data");
  NXInt data = dataGroup.openIntData();
  data.load();

  m_outWorkspace = WorkspaceFactory::Instance().create(
      "Workspace2D", data.dim0(), data.dim1(), data.dim1());

  // Assign X-Y values
  for (int i = 0; i < data.dim0(); ++i) {
    for (int j = 0; j < data.dim1(); ++j) {
      double y = double(data()[i * data.dim1() + j]);
      m_outWorkspace->mutableX(i)[j] = j;
      m_outWorkspace->mutableY(i)[j] = y;
      m_outWorkspace->mutableE(i)[j] = sqrt(y);
    }
  }

  // Set the output workspace property
  setProperty("OutputWorkspace", m_outWorkspace);
}

} // namespace DataHandling
} // namespace Mantid
