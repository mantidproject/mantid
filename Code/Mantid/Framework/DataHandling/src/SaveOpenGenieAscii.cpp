//---------------------------------------------------
// Includes
//---------------------------------------------------
#include "MantidDataHandling/SaveOpenGenieAscii.h"
#include "MantidAPI/FileProperty.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/Exception.h"
#include <Poco/File.h>
#include <Poco/Path.h>
#include <fstream>
#include <exception>


namespace Mantid {
namespace DatHandling {

using namespace Kernel;
using namespace Mantid::API;


// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SaveOpenGenieAscii)

/// Empty constructor
SaveOpenGenieAscii::SaveOpenGenieAscii() : Mantid::API::Algorithm() {}


//---------------------------------------------------
// Private member functions
//---------------------------------------------------
/**
 * Initialise the algorithm
 */

void SaveOpenGenieAscii::init() {
  declareProperty(
      new API::WorkspaceProperty<>("InputWorkspace", "",
                                   Kernel::Direction::Input),
      "The name of the workspace containing the data you wish to save");

  declareProperty(
      new API::FileProperty("Filename", "", API::FileProperty::Save),
      "The filename to use for the saved data");

  declareProperty(
      "Append", false,
      "If true and Filename already exists, append, else overwrite");
}


void SaveOpenGenieAscii::exec() {
  // Process properties

  // Retrieve the input workspace
  MatrixWorkspace_sptr inputWorkspace = getProperty("InputWorkspace");

  // Retrieve the filename from the properties
  const std::string filename = getProperty("Filename");



//up to here @shahroz


  // Check whether it is PointData or Histogram
  if (!inputWorkspace->isHistogramData())
    g_log.warning("Input workspace is NOT histogram!  SaveGSS may not work "
                  "well with PointData.");

  // Check the number of histogram/spectra < 99
  const int nHist = static_cast<int>(inputWS->getNumberHistograms());
  if (nHist > 99) {
    std::stringstream errss;
    errss << "Number of Spectra (" << nHist
          << ") cannot be larger than 99 for GSAS file";
    g_log.error(errss.str());
    throw new std::invalid_argument(errss.str());
  }

  // Check whether append or not
  if (inputWorkspace) {
    const std::string file(filename);
    Poco::File fileobj(file);
    if (fileobj.exists()) {
      // Non-append mode and will be overwritten
      g_log.warning() << "Target GSAS file " << filename
                      << " exists and will be overwritten. "
                      << "\n";
    } else if (!fileobj.exists()) {
      // File does not exist but in append mode
      g_log.warning() << "Target GSAS file " << filename
                      << " does not exist.  Append mode is set to false "
                      << "\n";
    }
  }

  return;
};






} // namespace DataHandling
} // namespace Mantid
