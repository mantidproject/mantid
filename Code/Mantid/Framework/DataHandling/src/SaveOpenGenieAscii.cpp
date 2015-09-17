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

//////////////////////////////////////////////////////////////////////////

void SaveOpenGenieAscii::exec() {
  // Process properties

  // Retrieve the input workspace
  /// Workspace
  API::MatrixWorkspace_const_sptr ws = getProperty("InputWorkspace");
  int nSpectra = static_cast<int>(ws->getNumberHistograms());
  int nBins = static_cast<int>(ws->blocksize());

  // Retrieve the filename from the properties
  std::string filename = getProperty("Filename");

  // Output string variables
  const std::string comment = " ";
  const std::string comstr = " "; // spaces between each number
  const std::string errstr = "E";
  const std::string errstr2 = "";
  const std::string GXR = "GXRealarray";
  const std::string banknum = "1";
  const std::string fourspc = "    ";
  const std::string twospc = " ";

  if (nBins == 0 || nSpectra == 0)
    throw std::runtime_error("Trying to save an empty workspace");

  const std::string Alpha[] = {"`e`", "`x`", "`y`"};

  for (int Num = 0; Num < 3; Num++) {
    std::string alpha = Alpha[Num];
    WriteToFile(alpha, filename, comment, comstr, errstr, errstr2, GXR, banknum,
                fourspc, twospc, ws, nBins);
    return;
  };
}

void SaveOpenGenieAscii::WriteHeader(
    const std::string alpha, std::ofstream &outfile, const std::string comment,
    const std::string GXR, const std::string banknum, const std::string fourspc,
    const std::string twospc, API::MatrixWorkspace_const_sptr ws) {
  outfile << twospc << comment << alpha << std::endl;
  outfile << fourspc << GXR << std::endl;
  outfile << fourspc << banknum << std::endl;
  outfile << fourspc << ws->getNumberHistograms() << std::endl;
}

void SaveOpenGenieAscii::WriteToFile(
    const std::string alpha, std::string filename, const std::string comment,
    const std::string comstr, const std::string errstr,
    const std::string errstr2, const std::string GXR, const std::string banknum,
    const std::string fourspc, const std::string twospc,
    API::MatrixWorkspace_const_sptr ws, int nBins) {

  // File
  std::ofstream outfile(filename.c_str());
  if (!outfile) {
    g_log.error("Unable to create file: " + filename);
    throw Exception::FileError("Unable to create file: ", filename);
  }

  WriteHeader(alpha, outfile, comment, GXR, banknum, fourspc, twospc, ws);

  /* outfile << twospc << comment << alpha << std::endl;
    outfile << fourspc << GXR << std::endl;
    outfile << fourspc << banknum << std::endl;
    outfile << fourspc << ws->getNumberHistograms() << std::endl; */

  bool isHistogram = ws->isHistogramData();
  Progress progress(this, 0, 1, nBins);

  for (int bin = 0; bin < nBins; bin++) {
    if (isHistogram) // bin centres
    {
      if (bin == 0) {
        outfile << fourspc;
      }

      outfile << WriteAxisValues(alpha, bin, ws) << comment;

      if ((bin + 1) % 10 == 0 && bin != (nBins - 1)) {
        outfile << std::endl << fourspc;
      }
    }
    outfile << std::endl;
    progress.report();
  };
}

// Reads if alpha is e then reads the E values accordingly
double SaveOpenGenieAscii::WriteAxisValues(std::string alpha, int bin,
                                    API::MatrixWorkspace_const_sptr ws) {
  if (alpha == "`e`") {
    return ws->readE(0)[bin];
  } if (alpha == "`x`") {
    return ws->readX(0)[bin];
  } if (alpha == "`y`") {
    return ws->readY(0)[bin];
  };
}

} // namespace DataHandling
} // namespace Mantid
