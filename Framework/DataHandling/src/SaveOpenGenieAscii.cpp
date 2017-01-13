#include "MantidDataHandling/SaveOpenGenieAscii.h"

#include "MantidAPI/FileProperty.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/Property.h"
#include "MantidKernel/VisibleWhenProperty.h"

#include <Poco/File.h>
#include <Poco/Path.h>

#include <exception>
#include <fstream>
#include <list>
#include <vector>

namespace Mantid {
namespace DataHandling {

using namespace Kernel;
using namespace Mantid::API;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SaveOpenGenieAscii)

/// Empty constructor
SaveOpenGenieAscii::SaveOpenGenieAscii() : Mantid::API::Algorithm() {}

/**
 * Initialise the algorithm
 */

void SaveOpenGenieAscii::init() {
  declareProperty(
      make_unique<API::WorkspaceProperty<MatrixWorkspace>>(
          "InputWorkspace", "", Kernel::Direction::Input),
      "The name of the workspace containing the data you wish to save");

  // Declare required parameters, filename with ext {.his} and input
  // workspace
  const std::vector<std::string> exts{".his", ".txt", ""};
  declareProperty(Kernel::make_unique<API::FileProperty>(
                      "Filename", "", API::FileProperty::Save, exts),
                  "The filename to use for the saved data");
  declareProperty("IncludeHeader", true,
                  "Whether to include the header lines (default: true)");
  std::vector<std::string> header{"ENGIN-X Format", "Basic Format"};
  declareProperty("OpenGenieFormat", "ENGIN-X Format",
                  boost::make_shared<Kernel::StringListValidator>(header),
                  "The format required to successfully load the file to "
                  "OpenGnie: ENGIN-X Format (default)");
}

void SaveOpenGenieAscii::exec() {
  // Retrieve the input workspace
  m_inputWS = getProperty("InputWorkspace");
  const int nSpectra = static_cast<int>(m_inputWS->getNumberHistograms());
  const int nBins = static_cast<int>(m_inputWS->blocksize());

  if (nBins == 0 || nSpectra == 0)
    throw std::runtime_error("Trying to save an empty workspace");

  if (!m_inputWS->isHistogramData()) {
    throw std::runtime_error("This algorithm cannot save workspaces with event "
                             "data, please convert to histogram data first.");
  }
  // Progress progress(this, 0, 1, nBins);

  // writes out x, y, e to vector
  parseWorkspaceData();

  // get all the sample in workspace
  getSampleLogs();

  // add ntc (number of bins)
  m_outputVector.push_back(
      outputTuple("ntc", "Integer", std::to_string(nBins)));

  // Getting the format property
  std::string formatType = getProperty("OpenGenieFormat");
  if (formatType == "ENGIN-X Format") {
    // Apply EnginX format if selected
    applyEnginxFormat();
  } else {
    std::stringstream msg;
    msg << "Unrecognized format \"" << formatType << "\"";
    throw std::runtime_error(msg.str());
  }

  auto outputStream = openFileStream();

  // write out all data
  writeDataToFile(outputStream);
  outputStream.close();

  // progress.report();
}

/**
  * Determines the spectrum numbers for an ENGINX output file
  * and converts them into an OpenGenie compatible string which
  * is stored into the output buffer
  */
std::string SaveOpenGenieAscii::getSpectrumNumAsString(
    const API::MatrixWorkspace &WSToSave) {
  const auto numHistograms = WSToSave.getNumberHistograms();

  const int lowerSpecNumber = WSToSave.getSpectrum(0).getSpectrumNo();
  const int upperSpecNumber =
      WSToSave.getSpectrum(numHistograms - 1).getSpectrumNo();

  const std::string lowerSpecString(std::to_string(lowerSpecNumber));
  const std::string upperSpecString(std::to_string(upperSpecNumber));

  // The string will either be '0 - 140' or 140 depending on whether the
  // lower and upper numbers are equal or not
  const std::string outputString(lowerSpecNumber == upperSpecNumber
                                     ? upperSpecString
                                     : lowerSpecString + " - " +
                                           upperSpecString);
  return outputString;
}

/**
  * Converts the workspace data into strings which are compatible
  * with OpenGenie and stores them into a tuple with the number
  * of data points for X Y and E
  *
  * @return :: Returns a vector of tuples - the tuples being the
  * data as a string and the number of data points. The first element
  * of the vector is X, second Y, and final element E data
  *
  */
std::vector<std::tuple<std::string, int>>
SaveOpenGenieAscii::convertWorkspaceToStrings() {
  // Build x, y and e strings
  std::string xValsOutput("    "), yValsOutput(xValsOutput),
      eValsOutput(xValsOutput);
  const std::string newlineStr = "\n    ";

  int xCount = 0;
  const auto &x = m_inputWS->x(0);
  for (const auto xVal : x) {
    if (xCount % 10 == 0) {
      xValsOutput += newlineStr;
    }
    xCount++;
    xValsOutput += std::to_string(xVal) + ' ';
  }

  int yCount = 0;
  const auto &y = m_inputWS->y(0);
  for (const auto yVal : y) {
    if (yCount % 10 == 0) {
      yValsOutput += newlineStr;
    }
    yCount++;
    yValsOutput += std::to_string(yVal) + ' ';
  }

  int eCount = 0;
  const auto &e = m_inputWS->e(0);
  for (const auto eVal : e) {
    if (eCount % 10 == 0) {
      eValsOutput += newlineStr;
    }
    eCount++;
    eValsOutput += std::to_string(eVal) + ' ';
  }

  std::vector<std::tuple<std::string, int>> outDataStrings;
  outDataStrings.reserve(3);
  outDataStrings.push_back(std::make_tuple(xValsOutput, xCount));
  outDataStrings.push_back(std::make_tuple(yValsOutput, yCount));
  outDataStrings.push_back(std::make_tuple(eValsOutput, eCount));
  return outDataStrings;
}

//-----------------------------------------------------------------------------
/**
  * Parses the X Y E data from the workspace into OpenGENIE compatible
  * strings and stores them in the output buffer
  */
void SaveOpenGenieAscii::parseWorkspaceData() {
  // 1 is Bank number - force to 1 at the moment
  const std::string outputType = "GXRealarray\n    1";

  const auto xyeTuples = convertWorkspaceToStrings();
  const auto &xTuple = xyeTuples[0];
  const auto &yTuple = xyeTuples[1];
  const auto &eTuple = xyeTuples[2];

  // Have to put the number of values followed by a space then a new line
  // then the data into a string
  const auto xDataString = std::to_string(std::get<1>(xTuple)) + " \n" +
                           std::move(std::get<0>(xTuple));
  m_outputVector.push_back(
      outputTuple("x", outputType, std::move(xDataString)));

  const auto yDataString = std::to_string(std::get<1>(yTuple)) + " \n" +
                           std::move(std::get<0>(yTuple));
  m_outputVector.push_back(
      outputTuple("y", outputType, std::move(yDataString)));

  const auto eDataString = std::to_string(std::get<1>(eTuple)) + " \n" +
                           std::move(std::get<0>(eTuple));
  m_outputVector.push_back(
      outputTuple("e", outputType, std::move(eDataString)));
}

std::ofstream SaveOpenGenieAscii::openFileStream() {
  // Retrieve the filename from the properties
  const std::string filename = getProperty("Filename");
  // file
  std::ofstream outfile(filename.c_str());
  if (!outfile) {
    g_log.error("Unable to create file: " + filename);
    throw Exception::FileError("Unable to create file: ", filename);
  }
  return outfile;
}

//-----------------------------------------------------------------------
/** Reads the sample logs and writes to vector
   */
void SaveOpenGenieAscii::getSampleLogs() {
  const std::vector<Property *> &logData = m_inputWS->run().getLogData();

  return;

  // TODO choose which logs are important to openGENIE
  // DO NOT let this be merged in this state

  for (auto log : logData) {
    std::string name = log->name();
    std::string type = log->type();
    std::string value = log->value();

    if (type.std::string::find("vector") &&
        type.std::string::find("double") != std::string::npos) {

      auto tsp = m_inputWS->run().getTimeSeriesProperty<double>(name);
      value = boost::lexical_cast<std::string>(tsp->timeAverageValue());
    }

    else if (type.std::string::find("vector") &&
             type.std::string::find("int") != std::string::npos) {

      auto tsp = m_inputWS->run().getTimeSeriesProperty<int>(name);
      value = boost::lexical_cast<std::string>(tsp->timeAverageValue());
    }

    else if (type.std::string::find("vector") &&
             type.std::string::find("bool") != std::string::npos) {

      auto tsp = m_inputWS->run().getTimeSeriesProperty<bool>(name);
      value = boost::lexical_cast<std::string>(tsp->timeAverageValue());
    }

    else if (type.std::string::find("vector") &&
             type.std::string::find("char") != std::string::npos) {

      auto tsp = m_inputWS->run().getTimeSeriesProperty<std::string>(name);
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

    if (name == "run_number") {
      name = "run_no";
      value = "\"" + value + "\"";
    }

    if (name == "run_title") {
      name = "title";
      value = "\"" + value + "\"";
    }

    // if name != x y or e push str to vector; to avoid any duplication
    if (name != "x" && name != "y" && name != "e") {

      std::string outStr = ("  \"" + name + "\"" + "\n" + "    " + type + "\n" +
                            "    " + value + "\n");

      // TODO
      // logVector.push_back(outStr);
    }
  }
}

//------------------------------------------------------------------------------
/** Sorts the vector and writes out the output data buffer to file
   *  @param outfile :: File it will save it out to
   */
void SaveOpenGenieAscii::writeDataToFile(std::ofstream &outfile) {
  // Write header
  if (getProperty("IncludeHeader")) {
    outfile << "# Open Genie ASCII File #\n"
            << "# label \n"
            << "GXWorkspace\n"
            << m_outputVector.size() << '\n';
  }

  // Sort by parameter name
  std::sort(m_outputVector.begin(), m_outputVector.end(),
            [](const outputTuple &t1, const outputTuple &t2) {
              return std::get<0>(t1) < std::get<0>(t2);
            });

  for (const auto &outTuple : m_outputVector) {
    // Format is 2 spaces followed by parameter name, newline
    // 4 spaces then the type name, newline
    // 4 spaces and value(s), newline

    // The parameter name must be surrounded with quotes
    // If the type is a string the value must be wrapped in quotes

    // First the parameter name
    outfile << "  " << '"' << std::get<0>(outTuple) << '"' << '\n';

    // Next the parameter type - have to make a copy as we might need to
    // capitalise the first char
    std::string outputType = std::get<1>(outTuple);
    outputType[0] = toupper(outputType[0]);
    outfile << "    " << outputType << '\n';

    // Then the data values - the formatting depends on data type
    outfile << "    ";
    if (outputType == "String") {
      outfile << '"' << std::get<2>(outTuple) << '"';
    } else {
      outfile << std::get<2>(outTuple);
    }
    outfile << '\n';
  }
}

//------------------------------------------------------------------------------
/** Apply enginX format field which is required for OpenGenie
 */
void SaveOpenGenieAscii::applyEnginxFormat() {
  const std::string stringType = "String";

  // xunit & xlabel put in OpenGenie format
  const std::string xunits = "xunits";
  const std::string xlabel = "xlabel";
  const std::string xunitsVal = "Time-of-Flight (\\\\gms)";

  // yunit & ylabel put in OpenGenie format
  const std::string yunits = "yunits";
  const std::string ylabel = "ylabel";
  const std::string yunitsVal = "Neutron counts / \\\\gms";

  const std::string specNumIdentifier = "spec_no";
  const std::string specNoToSave = getSpectrumNumAsString(*m_inputWS);

  m_outputVector.reserve(5);

  m_outputVector.push_back(
      outputTuple(specNumIdentifier, stringType, specNoToSave));

  m_outputVector.push_back(outputTuple(xunits, stringType, xunitsVal));
  m_outputVector.push_back(outputTuple(xlabel, stringType, xunitsVal));
  m_outputVector.push_back(outputTuple(yunits, stringType, yunitsVal));
  m_outputVector.push_back(outputTuple(ylabel, stringType, yunitsVal));
}

} // namespace DataHandling
} // namespace Mantid
