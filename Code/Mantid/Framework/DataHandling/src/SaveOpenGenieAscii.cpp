//---------------------------------------------------
// Includes
//---------------------------------------------------
#include "MantidAPI/FileProperty.h"
#include "MantidDataHandling/SaveOpenGenieAscii.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/Property.h"

#include <exception>
#include <fstream>
#include <list>
#include <vector>
#include <Poco/File.h>
#include <Poco/Path.h>

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

  // Declare required parameters, filename with ext {.his} and input
  // workspac
  std::vector<std::string> his_exts;
  his_exts.push_back(".his");
  his_exts.push_back(".txt");
  his_exts.push_back("");
  declareProperty(
      new API::FileProperty("Filename", "", API::FileProperty::Save, his_exts),
      "The filename to use for the saved data");
  declareProperty("IncludeHeader", true,
                  "Whether to include the header lines (default: true)");
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

  const bool headers = getProperty("IncludeHeader");
  // if true write file header
  if (headers) {
    writeFileHeader(outfile);
  }

  bool isHistogram = ws->isHistogramData();
  Progress progress(this, 0, 1, nBins);

  // writes out x, y, e to vector
  std::string alpha;
  for (int Num = 0; Num < 3; Num++) {
    alpha = Alpha[Num];
    axisToFile(alpha, singleSpc, fourspc, nBins, isHistogram);
  }

  // get all the sample in workspace
  getSampleLogs(fourspc);

  // add ntc sample log
  addNtc(fourspc, nBins);

  // write out all samples in the vector
  writeSampleLogs(outfile);

  progress.report();
  return;
}

// -----------------------------------------------------------------------------
/** generates the OpenGenie file header
   *  @param outfile :: File it will save it out to
   */
void SaveOpenGenieAscii::writeFileHeader(std::ofstream &outfile) {

  const std::vector<Property *> &logData = ws->run().getLogData();
  auto &log = logData;
  // get total number of sample logs
  auto samplenumber = (&log)->size();
  samplenumber += 3; // x, y, e

  outfile << "# Open Genie ASCII File #" << std::endl
          << "# label " << std::endl
          << "GXWorkspace" << std::endl
          // number of entries
          << samplenumber << std::endl;
}

//------------------------------------------------------------------------------
/** generates the header for the axis which saves to file
   *  @param alpha ::   onstant string Axis letter that is being used
   *  @param singleSpc :: Constant string for single space
   *  @param fourspc :: Constant string for four spaces
   *  @param nBins ::  Number of bins
   *  @return A string of of the header for the x y and e
   */
std::string SaveOpenGenieAscii::getAxisHeader(const std::string alpha,
                                              const std::string singleSpc,
                                              const std::string fourspc,
                                              int nBins) {
  std::string outStr = "";
  const std::string GXR = "GXRealarray";
  const std::string banknum = "1";
  const std::string twospc = " ";

  outStr += twospc + singleSpc + alpha + "\n";
  outStr += fourspc + GXR + "\n";
  outStr += fourspc + banknum + "\n";
  outStr += fourspc + boost::lexical_cast<std::string>(nBins) + "\n";

  return outStr;
}

//-----------------------------------------------------------------------------
/** Uses AxisHeader and WriteAxisValues to write in vector
   *  @param alpha ::   Axis letter that is being used
   *  @param singleSpc :: Constant string for single space
   *  @param fourspc :: Constant string for four spaces
   *  @param nBins ::  number of bins
   *  @param isHistogram ::  If its a histogram
   */
void SaveOpenGenieAscii::axisToFile(const std::string alpha,
                                    const std::string singleSpc,
                                    const std::string fourspc, int nBins,
                                    bool isHistogram) {
  std::string out_str = getAxisHeader(alpha, singleSpc, fourspc, nBins);
  for (int bin = 0; bin < nBins; bin++) {
    if (isHistogram) // bin centres
    {
      if (bin == 0) {
        out_str += fourspc;
      }
      auto axisStr = getAxisValues(alpha, bin, singleSpc);
      out_str += axisStr;

      if ((bin + 1) % 10 == 0 && bin != (nBins - 1)) {
        out_str += "\n" + fourspc;
      }
    }
  }
  out_str += "\n";
  logVector.push_back(out_str);
}

//------------------------------------------------------------------------
/** Reads if alpha is e then reads the E values accordingly
   *  @param alpha :: Axis letter that is being used
   *  @param bin :: bin counter which goes through all the bin
   *  @param singleSpc :: Constant string for single space
   *  @return A string of either e, x or y
   */
std::string SaveOpenGenieAscii::getAxisValues(std::string alpha, int bin,
                                              const std::string singleSpc) {
  std::string output = "";
  if (alpha == "\"e\"") {
    output += boost::lexical_cast<std::string>(ws->readE(0)[bin]) + singleSpc;
  }
  if (alpha == "\"x\"") {
    output += boost::lexical_cast<std::string>(ws->readX(0)[bin]) + singleSpc;
  }
  if (alpha == "\"y\"") {
    output += boost::lexical_cast<std::string>(ws->readY(0)[bin]) + singleSpc;
  }
  return output;
}

//-----------------------------------------------------------------------
/** Reads the sample logs and writes to vector
   *  @param fourspc :: Constant string for four spaces
   */
void SaveOpenGenieAscii::getSampleLogs(std::string fourspc) {
  const std::vector<Property *> &logData = ws->run().getLogData();

  for (auto log = logData.begin(); log != logData.end(); ++log) {
    std::string name = (*log)->name();
    std::string type = (*log)->type();
    std::string value = (*log)->value();

    if (type.std::string::find("vector") &&
        type.std::string::find("double") != std::string::npos) {

      auto tsp = ws->run().getTimeSeriesProperty<double>(name);
      value = boost::lexical_cast<std::string>(tsp->timeAverageValue());
    }

    else if (type.std::string::find("vector") &&
             type.std::string::find("int") != std::string::npos) {

      auto tsp = ws->run().getTimeSeriesProperty<int>(name);
      value = boost::lexical_cast<std::string>(tsp->timeAverageValue());
    }

    else if (type.std::string::find("vector") &&
             type.std::string::find("bool") != std::string::npos) {

      auto tsp = ws->run().getTimeSeriesProperty<bool>(name);
      value = boost::lexical_cast<std::string>(tsp->timeAverageValue());
    }

    else if (type.std::string::find("vector") &&
             type.std::string::find("char") != std::string::npos) {

      auto tsp = ws->run().getTimeSeriesProperty<std::string>(name);
      value = (tsp->lastValue());
    }

    if ((type.std::string::find("number") != std::string::npos) ||
        (type.std::string::find("double") != std::string::npos) ||
        (type.std::string::find("dbl list") != std::string::npos)) {
      type = "Float";
    }

    else if ((type.std::string::find("TimeValueUnit<bool>") !=
              std::string::npos) ||
             (type.std::string::find("TimeValueUnit<int>") !=
              std::string::npos)) {
      type = "Integer";
    }

    else if (type.std::string::find("string") != std::string::npos) {
      type = "String";
    }

    if (name != "x" && name != "y" && name != "e") {
      std::string outStr = ("  \"" + name + "\"" + "\n" + fourspc + type +
                            "\n" + fourspc + value + "\n");

      logVector.push_back(outStr);
    }
  }
}

//------------------------------------------------------------------------------
/** Sorts the vector and writes outt he sample logs to file
   *  @param outfile :: File it will save it out to
   */
void SaveOpenGenieAscii::writeSampleLogs(std::ofstream &outfile) {
  sort(logVector.begin(), logVector.end());

  for (std::vector<std::string>::const_iterator i = logVector.begin();
       i != logVector.end(); ++i) {
    outfile << *i << ' ';
  }
}

//------------------------------------------------------------------------------
/** Add ntc field (num of bins) required to run open genie
   *  @param fourspc :: Constant string for four spaces
   *  @param nBins ::  Number of bins
   */
void SaveOpenGenieAscii::addNtc(const std::string fourspc, int nBins) {
  std::string outStr = "";
  std::string ntc = "ntc";

  outStr += ("  \"" + ntc + "\"" + "\n" + fourspc + "Integer" + "\n" + fourspc +
             boost::lexical_cast<std::string>(nBins) + "\n");

  logVector.push_back(outStr);
}

} // namespace DataHandling
} // namespace Mantid
