#include "MantidDataHandling/LoadILLDiffraction.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/RegisterFileLoader.h"

#include <nexus/napi.h>

namespace Mantid {
namespace DataHandling {

using namespace Kernel;
using namespace API;
using namespace NeXus;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(LoadILLDiffraction)

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
const std::string LoadILLDiffraction::name() const { return "LoadILLDiffraction"; }

/// Algorithm's version for identification. @see Algorithm::version
int LoadILLDiffraction::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string LoadILLDiffraction::category() const {
  return "TODO: FILL IN A CATEGORY";
}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string LoadILLDiffraction::summary() const {
  return "TODO: FILL IN A SUMMARY";
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
    // Retrieve filename
    std::string filenameData = getPropertyValue("Filename");

    // open the root node
    NeXus::NXRoot dataRoot(filenameData);
    NXEntry firstEntry = dataRoot.openFirstEntry();

    // read in the data
    NXData dataGroup = firstEntry.openNXData("data_scan/detector_data");
    NXInt data = dataGroup.openIntData();
    // load the counts from the file into memory
    data.load();

    g_log.information("Dimentsions 1: " + std::to_string(data.dim0()));
    g_log.information("Dimentsions 2: " + std::to_string(data.dim1()));
    g_log.information("Dimentsions 3: " + std::to_string(data.dim2()));

    m_localWorkspace =
        WorkspaceFactory::Instance().create("Workspace2D", data.dim0(), data.dim1() + 1, data.dim1());

    // Assign Y values
    for (int i = 0; i < data.dim0(); ++i) {
        for (int j = 0; j<= data.dim1(); ++j ) {
            m_localWorkspace->mutableX(i)[j] = j;
        }
    }

    // Assign Y values
    for (int i = 0; i < data.dim0(); ++i) {
        for (int j = 0; j< data.dim1(); ++j ) {
            m_localWorkspace->mutableY(i)[j] = double(data()[i*data.dim1()+j]);
        }
    }

    // Set the output workspace property
    setProperty("OutputWorkspace", m_localWorkspace);
}

} // namespace DataHandling
} // namespace Mantid
