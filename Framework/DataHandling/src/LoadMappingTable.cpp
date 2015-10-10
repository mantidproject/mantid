#include "MantidDataHandling/LoadMappingTable.h"
#include "LoadRaw/isisraw2.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/SpectrumDetectorMapping.h"

namespace Mantid {
namespace DataHandling {

using namespace Kernel;
using namespace API;

DECLARE_ALGORITHM(LoadMappingTable)

LoadMappingTable::LoadMappingTable() : Algorithm() {}

void LoadMappingTable::init() {
  declareProperty(new FileProperty("Filename", "", FileProperty::Load),
                  "The name of the RAW file from which to obtain the mapping "
                  "information, including its full or relative path.");
  declareProperty(
      new WorkspaceProperty<>("Workspace", "Anonymous", Direction::InOut),
      "The name of the input and output workspace on which to perform the "
      "algorithm.");
}

void LoadMappingTable::exec() {
  // Get the raw file name
  m_filename = getPropertyValue("Filename");
  // Get the input workspace
  const MatrixWorkspace_sptr localWorkspace = getProperty("Workspace");

  /// ISISRAW class instance which does raw file reading. Shared pointer to
  /// prevent memory leak when an exception is thrown.
  boost::scoped_ptr<ISISRAW2> iraw(new ISISRAW2);

  if (iraw->readFromFile(m_filename.c_str(), 0) !=
      0) // ReadFrom File with no data
  {
    g_log.error("Unable to open file " + m_filename);
    throw Kernel::Exception::FileError("Unable to open File:", m_filename);
  }
  progress(0.5);
  const int number_spectra =
      iraw->i_det; // Number of entries in the spectra/udet table
  if (number_spectra == 0) {
    g_log.warning("The spectra to detector mapping table is empty");
  }
  // Fill in the mapping in the workspace's ISpectrum objects
  localWorkspace->updateSpectraUsing(
      SpectrumDetectorMapping(iraw->spec, iraw->udet, number_spectra));
  progress(1);

  return;
}

} // Namespace DataHandling
} // Namespace Mantid
