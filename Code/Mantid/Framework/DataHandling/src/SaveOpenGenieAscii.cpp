//---------------------------------------------------
// Includes
//---------------------------------------------------
#include "MantidDataHandling/SaveOpenGenieAscii.h"
#include "MantidAPI/FileProperty.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/Property.h"
#include <Poco/File.h>
#include <Poco/Path.h>
#include <fstream>
#include <exception>

#include <list>

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
}

void SaveOpenGenieAscii::exec() {
  // Process properties

  // Retrieve the input workspace
  /// Workspace
  ws = getProperty("InputWorkspace");
  int nSpectra = static_cast<int>(ws->getNumberHistograms());
  int nBins = static_cast<int>(ws->blocksize());

  // Retrieve the filename from the properties
  std::string filename = getProperty("Filename");

  // Output string variables
  const std::string singleSpc = " ";
  const std::string fourspc = "    ";

  // file
  std::ofstream outfile(filename.c_str());
  if (!outfile) {
    g_log.error("Unable to create file: " + filename);
    throw Exception::FileError("Unable to create file: ", filename);
  }
  if (nBins == 0 || nSpectra == 0)
    throw std::runtime_error("Trying to save an empty workspace");

  // Axis alphabets
  const std::string Alpha[] = {"\"e\"", "\"x\"", "\"y\""};

  bool isHistogram = ws->isHistogramData();
  Progress progress(this, 0, 1, nBins);
  std::string alpha;
  outfile << "# Open Genie ASCII File #" << std::endl
          << "# label " << std::endl
          << "GXWorkspace" << std::endl << nSpectra << std::endl;
  for (int Num = 0; Num < 3; Num++) {
    alpha = Alpha[Num];
    WriteToFile(alpha, outfile, singleSpc, fourspc, nBins, isHistogram,
                nBins);
  }
  progress.report();
  return;
}

//----------------------------------------------------------------------------------------------
/** generates the header for the axis which saves to file
   *  @param alpha ::   onstant string Axis letter that is being used
   *  @param outfile :: File it will save it out to
   *  @param singleSpc :: Constant string for single space
   *  @param fourspc :: Constant string for four spaces
   *  @param nBins ::  Number of bins
   */
void SaveOpenGenieAscii::WriteAxisHeader(const std::string alpha,
                                         std::ofstream &outfile,
                                         const std::string singleSpc,
                                         const std::string fourspc,
                                         int nBins) {
  const std::string GXR = "GXRealarray";
  const std::string banknum = "1";
  const std::string twospc = " ";

  outfile << twospc << singleSpc << alpha << std::endl;
  outfile << fourspc << GXR << std::endl;
  outfile << fourspc << banknum << std::endl;
  outfile << fourspc << nBins << std::endl;
}

//----------------------------------------------------------------------------------------------
/** Uses AxisHeader and WriteAxisValues to write out file
   *  @param alpha ::   Axis letter that is being used
   *  @param outfile :: File it will save it out to
   *  @param singleSpc :: Constant string for single space
   *  @param fourspc :: Constant string for four spaces
   *  @param nBins ::  number of bins
   *  @param isHistogram ::  If its a histogram
   *  @param nSpectra ::  Number of spectrum
   */
void SaveOpenGenieAscii::WriteToFile(const std::string alpha,
                                     std::ofstream &outfile,
                                     const std::string singleSpc,
                                     const std::string fourspc, int nBins,
                                     bool isHistogram, int nSpectra) {

  WriteAxisHeader(alpha, outfile, singleSpc, fourspc, nSpectra);

  for (int bin = 0; bin < nBins; bin++) {
    if (isHistogram) // bin centres
    {
      if (bin == 0) {
        outfile << fourspc;
      }

      WriteAxisValues(alpha, outfile, bin, singleSpc);

      if ((bin + 1) % 10 == 0 && bin != (nBins - 1)) {
        outfile << std::endl << fourspc;
      }
    }
  }
  outfile << std::endl;
}

//----------------------------------------------------------------------------------------------
/** Reads if alpha is e then reads the E values accordingly
   *  @param alpha ::   Axis letter that is being used
   *  @param outfile :: File it will save it out to
   *  @param bin :: bin counter which goes through all the bin
   *  @param singleSpc :: Constant string for single space
   */
void SaveOpenGenieAscii::WriteAxisValues(std::string alpha,
                                         std::ofstream &outfile, int bin,
                                         const std::string singleSpc) {
  if (alpha == "\"e\"") {
    outfile << ws->readE(0)[bin] << singleSpc;
  }
  if (alpha == "\"x\"") {
    outfile << (ws->readX(0)[bin]) << singleSpc;
  }
  if (alpha == "\"y\"") {
    outfile << ws->readY(0)[bin] << singleSpc;
  }
}

} // namespace DataHandling
} // namespace Mantid
