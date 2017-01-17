#include "MantidDataHandling/SaveOpenGenieAscii.h"

#include "MantidAPI/FileProperty.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidGeometry/Instrument.h"
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

  inputValidation();

  // Its better to reserve over and the number of logs gives us a good
  // estimate as some logs are not included whilst some other params are.
  m_outputVector.reserve(m_inputWS->run().getLogData().size());

  // Progress progress(this, 0, 1, nBins);

  // store common workspace properties
  storeWorkspaceInformation();

  // store x, y, e to vector
  parseWorkspaceData();

  // get all the sample in workspace
  getSampleLogs();

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

void SaveOpenGenieAscii::inputValidation() {
	const size_t nSpectra = m_inputWS->getNumberHistograms();

	if (m_inputWS->blocksize() == 0 || nSpectra == 0)
		throw std::runtime_error("Trying to save an empty workspace");
	else if (nSpectra > 1) {
		throw std::runtime_error("Workspace has multiple spectra. This algorithm can only save focused workspaces.");
	}
	else if (!m_inputWS->isHistogramData()) {
		throw std::runtime_error("This algorithm cannot save workspaces with event "
			"data, please convert to histogram data first.");
	}
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
  outDataStrings.push_back(std::make_tuple(xValsOutput, xCount));
  outDataStrings.push_back(std::make_tuple(yValsOutput, yCount));
  outDataStrings.push_back(std::make_tuple(eValsOutput, eCount));
  return outDataStrings;
}

/**
  * Stores the workspace information such as title or run number
  * in the output buffer
  */
void SaveOpenGenieAscii::storeWorkspaceInformation()
{
	m_outputVector.push_back(outputTuple("run_no", m_stringType, std::to_string(m_inputWS->getRunNumber())));
	m_outputVector.push_back(outputTuple("title", m_stringType, m_inputWS->getTitle()));
	m_outputVector.push_back(outputTuple("inst_name", m_stringType,
		m_inputWS->getInstrument()->getName()));

	// Number of bins 
	m_outputVector.push_back(
		outputTuple("ntc", m_intType, std::to_string(m_inputWS->blocksize())));

	const auto &specInfo = m_inputWS->spectrumInfo();
	m_outputVector.push_back(outputTuple("l1", m_floatType, std::to_string(specInfo.l1())));
	m_outputVector.push_back(outputTuple("l2", m_floatType, std::to_string(specInfo.l2(0))));

	const double two_theta = specInfo.twoTheta(0) * 180 / M_PI; // Convert to deg
	m_outputVector.push_back(outputTuple("twotheta", m_floatType, std::to_string(two_theta)));
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
	const std::unordered_map<std::string, std::string> MantidGenieNameMap = 
	{ { "x", "x_pos" }, {"y", "y_pos"},	{"z", "z_pos"}, {"dae_beam_current", "microamps" },
	{"dur", "effective_time"} };
		
	//"theta_pos", "microamps", "load", "macro_strain", "pos", "eurotherm",
	//"x_pos_error", "y_pos_error", "z_pos_error", "theta_pos_error",
	//"load_error", "macro_strain_error", "pos_error", "eurotherm_error"};

	const std::vector<Property *> &logData = m_inputWS->run().getLogData();

  for (const auto &logEntry : logData) {
    const std::string &logName = logEntry->name();
    const std::string &type = logEntry->type();
	std::string outName;
	std::string outType;
	std::string outValue;

	const auto foundMapping = MantidGenieNameMap.find(logName);
	if (foundMapping == MantidGenieNameMap.cend()) {
		continue;
	}
	outName = foundMapping->second;

	// Calculate dx/dy/dz
	if (outName == "x_pos" || outName == "y_pos" || outName == "z_pos") {
		calculateXYZDelta(foundMapping->first, logEntry);
	}

	if (ITimeSeriesProperty* timeSeries = dynamic_cast<ITimeSeriesProperty*>(logEntry)) {
		outValue = std::to_string(timeSeries->timeAverageValue());
	}
	else {
		outValue = logEntry->value();
	}
	
    if ((type.std::string::find("number") != std::string::npos) ||
        (type.std::string::find("double") != std::string::npos) ||
        (type.std::string::find("dbl list") != std::string::npos)) {
		outType = "Float";
    } else if ((type.std::string::find("TimeValueUnit<bool>") !=
              std::string::npos) ||
             (type.std::string::find("TimeValueUnit<int>") !=
              std::string::npos)) {
		outType = "Integer";
    } else if (type.std::string::find("string") != std::string::npos) {
		outType = "String";
    }

	m_outputVector.push_back(outputTuple(outName, outType, outValue));
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

    const std::string outputType = std::get<1>(outTuple);
    outfile << "    " << outputType << '\n';

    // Then the data values - the formatting depends on data type
    outfile << "    ";
    if (outputType == m_stringType) {
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
  // xunit & xlabel put in OpenGenie format
  const std::string xunits = "xunits";
  const std::string xlabel = "xlabel";
  const std::string xunitsVal = "Time-of-Flight (\\\\gms)";
  m_outputVector.push_back(outputTuple(xunits, m_stringType, xunitsVal));
  m_outputVector.push_back(outputTuple(xlabel, m_stringType, xunitsVal));

  // yunit & ylabel put in OpenGenie format
  const std::string yunits = "yunits";
  const std::string ylabel = "ylabel";
  const std::string yunitsVal = "Neutron counts / \\\\gms";
  m_outputVector.push_back(outputTuple(yunits, m_stringType, yunitsVal));
  m_outputVector.push_back(outputTuple(ylabel, m_stringType, yunitsVal));

  // Spectrum numbers
  const std::string specNumIdentifier = "spec_no";
  const std::string specNoToSave = "1";
  m_outputVector.push_back(
      outputTuple(specNumIdentifier, m_stringType, specNoToSave));

  // Par file that was used
  const std::string parameter_file_label = "parameter_file";
  // This can be set to none at the moment as it does not affect the analysis
  const std::string parameter_value = "None.par";
  m_outputVector.push_back(
      outputTuple(parameter_file_label, m_stringType, parameter_value));

// TODO see if we can query the WS for the user name

  const std::string user_name_label = "user_name";
  const std::string user_name_value = "NotSet";
  m_outputVector.push_back(outputTuple(user_name_label, m_stringType, user_name_value));

}

void SaveOpenGenieAscii::calculateXYZDelta(const std::string &unit, const Kernel::Property *values)
{
	const auto positionValues = dynamic_cast<const TimeSeriesProperty<double> *>(values);
	assert(positionValues);

	const double deltaValue = positionValues->maxValue() - positionValues->minValue();
	m_outputVector.push_back(outputTuple('d' + unit, m_floatType, std::to_string(deltaValue)));
}


} // namespace DataHandling
} // namespace Mantid
