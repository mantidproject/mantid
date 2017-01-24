#include "MantidDataHandling/SaveOpenGenieAscii.h"

#include "MantidAPI/FileProperty.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/Property.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/VisibleWhenProperty.h"

#include <Poco/File.h>
#include <Poco/Path.h>

#include <fstream>
#include <vector>

namespace Mantid {
namespace DataHandling {

using namespace Kernel;
using namespace Mantid::API;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SaveOpenGenieAscii)

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
  std::vector<std::string> header{"ENGIN-X Format"};
  declareProperty("OpenGenieFormat", "ENGIN-X Format",
                  boost::make_shared<Kernel::StringListValidator>(header),
                  "The format required to successfully load the file to "
                  "OpenGenie: ENGIN-X Format (default)");
}

/**
  * Writes an OpenGenie file in ASCII at the user specified path
  */
void SaveOpenGenieAscii::exec() {
  // Retrieve the input workspace
  m_inputWS = getProperty("InputWorkspace");

  inputValidation();

  // Its better to reserve over and the number of logs gives us a good
  // estimate as some logs are not included whilst some other params are.
  m_outputVector.reserve(m_inputWS->run().getLogData().size());

  // Whilst this doesn't weigh in the processing
  // required it breaks down the algorithm nicely for the moment
  const int numOfSteps = 6;
  Progress progressBar(this, 0, 1, numOfSteps);

  const std::string formatType = getProperty("OpenGenieFormat");
  // If we had a basic format this is where the specialization would go
  if (formatType == "ENGIN-X Format") {
    progressBar.report("Generating ENGINX header");
    applyEnginxFormat();
  }

  // Store empty but required field
  progressBar.report("Storing empty fields");
  storeEmptyFields();

  // store common workspace properties
  progressBar.report("Processing workspace information");
  storeWorkspaceInformation();

  // store x, y, e to vector
  progressBar.report("Processing workspace data");
  parseWorkspaceData();

  progressBar.report("Processing log data");
  getSampleLogs();

  std::ofstream outStream;
  openFileStream(outStream);

  progressBar.report("Writing to file");
  writeDataToFile(outStream);
  outStream.close();

  // Indicate we have finished
  progressBar.report();
}

/**
  * Adds ENGINX specific attributes to the output buffer
  */
void SaveOpenGenieAscii::applyEnginxFormat() {
  // Bank number
  addToOutputBuffer("bank", m_intType, std::to_string(determineEnginXBankId()));

  // Spectrum numbers
  addToOutputBuffer("spec_no", m_stringType, "1");

  // Par file that was used in the calibration
  // This can be set to none at the moment as it does not affect the analysis
  addToOutputBuffer("parameter_file", m_stringType, "None.par");
  addToOutputBuffer("user_name", m_stringType, "NotSet");

  // xunit & xlabel put in OpenGenie format
  const std::string xunitsVal = "Time-of-Flight (\\\\gms)";
  addToOutputBuffer("xunits", m_stringType, xunitsVal);
  addToOutputBuffer("xlabel", m_stringType, xunitsVal);

  // yunit & ylabel put in OpenGenie format
  const std::string yunitsVal = "Neutron counts / \\\\gms";
  addToOutputBuffer("yunits", m_stringType, yunitsVal);
  addToOutputBuffer("ylabel", m_stringType, yunitsVal);
}

/**
  * Calculates the delta in the logged (i.e. not data) X Y Z values by
  * from the minimum and maximum logged values and stores them in the
  * output buffer
  *
  * @param unit :: The axis of this delta (e.g. 'X' or 'Y') as a string
  * @param values :: Pointer to the time series log values to process
  */
void SaveOpenGenieAscii::calculateXYZDelta(const std::string &unit,
                                           const Kernel::Property *values) {
  // Cast to TimeSeries so we can use the min/max value methods
  const auto positionValues =
      static_cast<const TimeSeriesProperty<double> *>(values);

  const double deltaValue =
      positionValues->maxValue() - positionValues->minValue();

  addToOutputBuffer('d' + unit, m_floatType, std::to_string(deltaValue));
}

/**
  * Converts histogram X/Y/E data into a compatible string representation
  * and calculates the number of data points that were parsed
  *
  * @tparam histoData :: The histogram data to parse
  *
  * @return :: An OpenGenie compatible string representation and
  * the number of data points packed into a tuple
  */
template <typename T>
std::tuple<std::string, int>
SaveOpenGenieAscii::convertWorkspaceData(const T &histoData) {
  // Padding to apply after 10 data values
  const std::string newLineStr = "\r\n    ";

  // First 4 spaces for correct padding
  std::string outputString("    ");
  int valueCount(0);

  // Working on primitive type so don't take ref
  for (const auto val : histoData) {
    if (valueCount % 10 == 0) {
      outputString += newLineStr;
    }
    valueCount++;
    outputString += std::to_string(val) + ' ';
  }

  return std::make_tuple(outputString, valueCount);
}

/**
  * Determines the current bank from the ENGIN-X detector IDs
  * if the ID is does not match the expected length it will
  * throw.
  *
  * @return :: The focused bank number in this workspace
  */
int SaveOpenGenieAscii::determineEnginXBankId() {
  const auto &detectorIds = m_inputWS->getSpectrum(0).getDetectorIDs();
  const std::string firstDetectorId = std::to_string(*detectorIds.cbegin());

  if (firstDetectorId.length() != 6) {
    throw std::runtime_error("Could not determine bank number as detector ID"
                             " was not of expected length");
  }

  // ENGIN-X format is 1X001, 1X002, 1X003...etc. for detectors
  // where X = 0 is bank 1. X = 1 is bank 2.
  return firstDetectorId[1] == '0' ? 1 : 2;
}

/** Reads the sample logs and converts them from the log name in
  * Mantid to the expected log name in OpenGenie. If any names are
  * unrecognised they are skipped. If the logs have multiple values
  * the time weighted average is taken instead. These strings are
  * then stored in the output buffer.
  */
void SaveOpenGenieAscii::getSampleLogs() {
  // Maps Mantid log names -> Genie save file name / type
  const std::unordered_map<std::string, std::tuple<std::string, std::string>>
      mantidGenieLogMapping = {
          {"x", std::make_tuple("x_pos", m_floatType)},
          {"y", std::make_tuple("y_pos", m_floatType)},
          {"z", std::make_tuple("z_pos", m_floatType)},
          {"gd_prtn_chrg", std::make_tuple("microamps", m_floatType)}};

  const std::vector<Property *> &logData = m_inputWS->run().getLogData();

  for (const auto &logEntry : logData) {
    const std::string &logName = logEntry->name();

    // Check this log value is known to us
    const auto foundMapping = mantidGenieLogMapping.find(logName);
    if (foundMapping == mantidGenieLogMapping.cend()) {
      continue;
    }

    const std::string outName = std::get<0>(foundMapping->second);
    const std::string outType = std::get<1>(foundMapping->second);
    std::string outValue;

    // Calculate dx/dy/dz
    if (outName == "x_pos" || outName == "y_pos" || outName == "z_pos") {
      calculateXYZDelta(foundMapping->first, logEntry);
    } else if (outName == "microamps") {
      // From reverse engineering the scripts the effective time is
      // the microamps * 50 - what 50 represents is not documented
      const std::string effectiveTime =
          std::to_string(std::stod(logEntry->value()) * 50.);
      addToOutputBuffer("effective_time", m_floatType, effectiveTime);
    }

    if (ITimeSeriesProperty *timeSeries =
            dynamic_cast<ITimeSeriesProperty *>(logEntry)) {
      outValue = std::to_string(timeSeries->timeAverageValue());
    } else {
      outValue = logEntry->value();
    }

    addToOutputBuffer(outName, outType, outValue);
  }
}

/**
  * Checks the workspace has data within it and that the number of spectra
  * is 1. If there is more than one spectra this could indicate that an
  * unfocused workspace is being saved.
  */
void SaveOpenGenieAscii::inputValidation() {
  const size_t nSpectra = m_inputWS->getNumberHistograms();

  if (m_inputWS->blocksize() == 0 || nSpectra == 0)
    throw std::runtime_error("Trying to save an empty workspace");
  else if (nSpectra > 1) {
    throw std::runtime_error("Workspace has multiple spectra. This algorithm "
                             "can only save focused workspaces.");
  } else if (!m_inputWS->isHistogramData()) {
    throw std::runtime_error("This algorithm cannot save workspaces with event "
                             "data, please convert to histogram data first.");
  }
}

/**
  * Attempts to open the file at the user specified path. If this
  * fails an exception is thrown else it returns the handle as a
  * std stream.
  *
  * @return:: The opened file as an file stream
  */
void SaveOpenGenieAscii::openFileStream(std::ofstream &stream) {
  // Retrieve the filename from the properties
  const std::string filename = getProperty("Filename");
  // Open file as binary so it doesn't convert CRLF to LF on UNIX
  stream.open(filename, std::ofstream::binary | std::ofstream::out);
  if (!stream) {
    g_log.error("Unable to create file: " + filename);
    throw Exception::FileError("Unable to create file: ", filename);
  }
}

/**
* Parses the X Y E data from the workspace into OpenGENIE compatible
* strings and stores them in the output buffer
*/
void SaveOpenGenieAscii::parseWorkspaceData() {
  // Bank number - force to 1 at the moment
  const std::string outputType = "GXRealarray\r\n    1";

  const auto &xTuple = convertWorkspaceData(m_inputWS->x(0));
  const auto &yTuple = convertWorkspaceData(m_inputWS->y(0));
  const auto &eTuple = convertWorkspaceData(m_inputWS->e(0));

  // Have to put the number of values followed by a space then a new line
  // then the data into a string
  const auto xDataString = std::to_string(std::get<1>(xTuple)) + " \r\n" +
                           std::move(std::get<0>(xTuple));
  addToOutputBuffer("x", outputType, std::move(xDataString));

  const auto yDataString = std::to_string(std::get<1>(yTuple)) + " \r\n" +
                           std::move(std::get<0>(yTuple));
  addToOutputBuffer("y", outputType, std::move(yDataString));

  const auto eDataString = std::to_string(std::get<1>(eTuple)) + " \r\n" +
                           std::move(std::get<0>(eTuple));
  addToOutputBuffer("e", outputType, std::move(eDataString));
}

/**
  * Stores the default value OpenGENIE uses in the fields that
  * aren't present in the input workspace but are required to be present
  * into the output buffer.
  */
void SaveOpenGenieAscii::storeEmptyFields() {
  const std::string floatVal = "999.000";
  addToOutputBuffer("eurotherm", m_floatType, floatVal);
  addToOutputBuffer("eurotherm_error", m_floatType, floatVal);

  addToOutputBuffer("load", m_floatType, floatVal);
  addToOutputBuffer("load_error", m_floatType, floatVal);
  addToOutputBuffer("macro_strain", m_floatType, floatVal);
  addToOutputBuffer("macro_strain_error", m_floatType, floatVal);
  addToOutputBuffer("theta_pos", m_floatType, floatVal);
  addToOutputBuffer("theta_pos_error", m_floatType, floatVal);

  addToOutputBuffer("pos", m_floatType, floatVal);
  addToOutputBuffer("pos_error", m_floatType, floatVal);
  addToOutputBuffer("x_pos_error", m_floatType, floatVal);
  addToOutputBuffer("y_pos_error", m_floatType, floatVal);
  addToOutputBuffer("z_pos_error", m_floatType, floatVal);
}

/**
  * Stores common workspace attributes such as title or run number
  * in the output buffer
  */
void SaveOpenGenieAscii::storeWorkspaceInformation() {
  // Run Number
  addToOutputBuffer("run_no", m_stringType,
                    std::to_string(m_inputWS->getRunNumber()));
  // Workspace title
  addToOutputBuffer("title", m_stringType, m_inputWS->getTitle());
  // Instrument name
  addToOutputBuffer("inst_name", m_stringType,
                    m_inputWS->getInstrument()->getName());
  // Number of bins
  addToOutputBuffer("ntc", m_intType, std::to_string(m_inputWS->blocksize()));
  // L1 (Source to sample distance)
  const auto &specInfo = m_inputWS->spectrumInfo();
  addToOutputBuffer("l1", m_floatType, std::to_string(specInfo.l1()));
  // L2 (Sample to spectrum distance)
  addToOutputBuffer("l2", m_floatType, std::to_string(specInfo.l2(0)));
  // Unsigned scattering angle 2theta
  const double two_theta = specInfo.twoTheta(0) * 180 / M_PI; // Convert to deg
  addToOutputBuffer("twotheta", m_floatType, std::to_string(two_theta));
}

/** Sorts the output buffer alphabetically and writes out the output
  * buffer to the stream specified. It also formats the values to
  * a valid OpenGenie format
  *
  *  @param outfile :: File it will to write the formatted buffer to
  */
void SaveOpenGenieAscii::writeDataToFile(std::ofstream &outfile) {
  // Write header
  if (getProperty("IncludeHeader")) {
    outfile << "# Open Genie ASCII File #\r\n"
            << "# label \r\n"
            << "GXWorkspace\r\n"
            << m_outputVector.size() << "\r\n";
  }

  // Sort by parameter name
  std::sort(m_outputVector.begin(), m_outputVector.end(),
            [](const OutputBufferEntry &t1, const OutputBufferEntry &t2) {
              return std::get<0>(t1) < std::get<0>(t2);
            });

  for (const auto &outTuple : m_outputVector) {
    // Format is 2 spaces followed by parameter name, newline
    // 4 spaces then the type name, newline
    // 4 spaces and value(s), newline

    // The parameter name must be surrounded with quotes
    // If the type is a string the value must be wrapped in quotes

    // First the parameter name
    outfile << "  " << '"' << std::get<0>(outTuple) << '"' << "\r\n";

    const std::string outputType = std::get<1>(outTuple);
    outfile << "    " << outputType << "\r\n";

    // Then the data values - the formatting depends on data type
    outfile << "    ";
    if (outputType == m_stringType) {
      outfile << '"' << std::get<2>(outTuple) << '"';
    } else {
      outfile << std::get<2>(outTuple);
    }
    outfile << "\r\n";
  }
}

} // namespace DataHandling
} // namespace Mantid
