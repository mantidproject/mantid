#include "MantidDataHandling/SaveGSS.h"

#include "MantidAPI/AlgorithmHistory.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceHistory.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidHistogramData/Histogram.h"

#include <Poco/File.h>
#include <Poco/Path.h>

#include <cmath>
#include <fstream>

namespace Mantid {
namespace DataHandling {

using namespace Mantid::API;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SaveGSS)

namespace { // Anonymous namespace
const std::string RALF("RALF");
const std::string SLOG("SLOG");

/// Determines the tolerance when comparing two doubles for equality
const double m_TOLERANCE = 1.e-10;

bool doesFileExist(const std::string &filePath) {
  auto file = Poco::File(filePath);
  return file.exists();
}

bool isEqual(const double left, const double right) {
  if (left == right)
    return true;
  return (2. * std::fabs(left - right) <=
          std::fabs(m_TOLERANCE * (right + left)));
}

bool isConstantDelta(const HistogramData::HistogramX &xAxis) {
  const double deltaX = (xAxis[1] - xAxis[0]);
  for (std::size_t i = 1; i < xAxis.size(); ++i) {
    if (!isEqual(xAxis[i] - xAxis[i - 1], deltaX)) {
      return false;
    }
  }
  return true;
}

double fixErrorValue(const double value) {
  // Fix error if value is less than zero or infinity
  // Negative errors cannot be read by GSAS
  if (value <= 0. || !std::isfinite(value))
    return 0.;
  else
    return value;
}

std::stringstream writeBankLine(const std::string &bintype, const int banknum,
                                const size_t datasize) {
  std::stringstream out;
  std::ios::fmtflags fflags(out.flags());
  out << "BANK " << std::fixed << std::setprecision(0)
      << banknum // First bank should be 1 for GSAS; this can be changed
      << std::fixed << " " << datasize << std::fixed << " " << datasize
      << std::fixed << " " << bintype;
  out.flags(fflags);
  return out;
}
} // End of anonymous namespace

// Constructor
SaveGSS::SaveGSS() : Mantid::API::Algorithm() {}

// Initialise the algorithm
void SaveGSS::init() {
  // Data must be in TOF
  declareProperty(Kernel::make_unique<API::WorkspaceProperty<>>(
                      "InputWorkspace", "", Kernel::Direction::Input,
                      boost::make_shared<API::WorkspaceUnitValidator>("TOF")),
                  "The input workspace, which must be in time-of-flight");
  declareProperty(Kernel::make_unique<API::FileProperty>(
                      "Filename", "", API::FileProperty::Save),
                  "The filename to use for the saved data");
  declareProperty("SplitFiles", true,
                  "Whether to save each spectrum into a separate file ('true') "
                  "or not ('false'). "
                  "Note that this is a string, not a boolean property.");
  declareProperty(
      "Append", true,
      "If true and Filename already exists, append, else overwrite ");
  declareProperty(
      "Bank", 1,
      "The bank number to include in the file header for the first spectrum, "
      "i.e., the starting bank number. "
      "This will increment for each spectrum or group member. ");
  const std::vector<std::string> formats{RALF, SLOG};
  declareProperty("Format", RALF,
                  boost::make_shared<Kernel::StringListValidator>(formats),
                  "GSAS format to save as");
  declareProperty("MultiplyByBinWidth", true,
                  "Multiply the intensity (Y) by the bin width; default TRUE.");
  declareProperty(
      "ExtendedHeader", false,
      "Add information to the header about iparm file and normalization");

  declareProperty(
      "UseSpectrumNumberAsBankID", false,
      "If true, then each bank's bank ID is equal to the spectrum number; "
      "otherwise, the continuous bank IDs are applied. ");
}

// Execute the algorithm
void SaveGSS::exec() {
  // Retrieve the input workspace and associated properties
  m_inputWS = getProperty("InputWorkspace");
  const size_t nHist = m_inputWS->getNumberHistograms();

  // Are we writing one file with n spectra or
  // n files with 1 spectra each
  const bool split = getProperty("SplitFiles");
  const size_t numOfOutFiles{split ? nHist : 1};
  const size_t numOutSpectra{split ? 1 : nHist};

  // Initialise various properties we are going to need within this alg
  m_allDetectorsValid = (isInstrumentValid() && areAllDetectorsValid());
  generateOutFileNames(numOfOutFiles);
  m_outputBuffer.resize(nHist);

  // Check the user input
  validateInputs();

  // Progress is 2 * number of histograms. One for generating data
  // one for writing out data
  m_progress = Kernel::make_unique<Progress>(this, 0.0, 1.0, (nHist * 2));

  // For the way we are using the vector we must have N number files
  // OR N number spectra. Otherwise we would need a vector of vector
  assert(numOutSpectra + numOfOutFiles == nHist + 1);
  assert(m_outputBuffer.size() == nHist);

  // Now start executing main part of the code
  generateGSASFile(numOfOutFiles, numOutSpectra);
  writeBufferToFile(numOfOutFiles, numOutSpectra);
}

/**
  * Returns if each spectra contains a valid detector
  * and implicitly if the instrument is valid
  *
  * @return :: True if every spectra has a detector. Else false
  */
bool SaveGSS::areAllDetectorsValid() const {
  const auto &spectrumInfo = m_inputWS->spectrumInfo();
  const size_t numHist = m_inputWS->getNumberHistograms();

  if (!isInstrumentValid()) {
    g_log.warning("No valid instrument found with this workspace"
                  " Treating as NO-INSTRUMENT CASE");
    return false;
  }

  bool allValid = true;
  // TODO parallel
  for (size_t histoIndex = 0; histoIndex < numHist; histoIndex++) {
    if (allValid == false) {
      break;
    }
    // Check this spectra has detectors
    if (!spectrumInfo.hasDetectors(histoIndex)) {
      allValid = false;
      g_log.warning() << "There is no detector associated with spectrum "
                      << histoIndex
                      << ". Workspace is treated as NO-INSTRUMENT case. \n";
    }
  }
  return allValid;
}

/**
  * Generates a string stream in GSAS format containing the
  * data for the specified bank from the workspace. This
  * can either be in RALF or SLOG format.
  *
  * @param specIndex :: The index of the bank to convert into
  * a string stream
  * @return :: A string stream containing the bank data in GSAS format
  */
std::stringstream SaveGSS::generateBankData(size_t specIndex) const {
  // Determine bank number into GSAS file
  std::stringstream outBuffer;
  const bool useSpecAsBank = getProperty("UseSpectrumNumberAsBankID");
  const bool multiplyByBinWidth = getProperty("MultiplyByBinWidth");
  const int userStartingBankNumber = getProperty("Bank");
  const std::string outputFormat = getPropertyValue("Format");
  int bankid;
  if (useSpecAsBank) {
    bankid =
        static_cast<int>(m_inputWS->getSpectrum(specIndex).getSpectrumNo());
  } else {
    bankid = userStartingBankNumber + static_cast<int>(specIndex);
  }

  // Write data
  if (outputFormat == RALF) {
    this->writeRALFdata(bankid, multiplyByBinWidth, outBuffer,
                        m_inputWS->histogram(specIndex));
  } else if (outputFormat == SLOG) {
    this->writeSLOGdata(bankid, multiplyByBinWidth, outBuffer,
                        m_inputWS->histogram(specIndex));
  } else {
    throw std::runtime_error("Cannot write to the unknown " + outputFormat +
                             "output format");
  }
  return outBuffer;
}

/**
  * Generates the bank header (which precedes bank data)
  * for the spectra index specified.
  *
  * @param spectrumInfo :: The input workspace spectrum info object
  * @param specIndex :: The bank index to generate a header for
  * @return :: String stream containing a bank header in GSAS format
  */
std::stringstream
SaveGSS::generateBankHeader(const API::SpectrumInfo &spectrumInfo,
                            size_t specIndex) const {
  std::stringstream outBuffer;
  double l1{0}, l2{0}, twoTheta{0}, difc{0};
  // If we have all valid detectors get these properties else use 0
  if (m_allDetectorsValid) {
    l1 = spectrumInfo.l1();
    l2 = spectrumInfo.l2(specIndex);
    twoTheta = spectrumInfo.twoTheta(specIndex);
    difc = (2.0 * PhysicalConstants::NeutronMass * sin(twoTheta * 0.5) *
            (l1 + l2)) /
           (PhysicalConstants::h * 1.e4);
  }

  if (m_allDetectorsValid) {
    outBuffer << "# Total flight path " << (l1 + l2) << "m, tth "
              << (twoTheta * 180. / M_PI) << "deg, DIFC " << difc << "\n";
  }

  outBuffer << "# Data for spectrum :" << specIndex << "\n";
  return outBuffer;
}

/**
  * Generates the GSAS file and populates the output buffer
  * with the data to be written to the file(s) in subsequent methods
  *
  * @param :: numOutFiles The number of file to be written
  * @param :: numOutSpectra The number of spectra per file to be written
  */
void SaveGSS::generateGSASFile(size_t numOutFiles, size_t numOutSpectra) {
  // Generate the output buffer for each histogram (spectrum)
  const auto &spectrumInfo = m_inputWS->spectrumInfo();
  const bool append = getProperty("Append");

  // Check if the caller has reserved space in our output buffer
  assert(m_outputBuffer.size() > 0);

  // If all detectors are not valid we use the no instrument case and
  // set l1 to 0
  const double l1{m_allDetectorsValid ? spectrumInfo.l1() : 0};

  // Create the various output files we will need in a loop
  for (size_t fileIndex = 0; fileIndex < numOutFiles; fileIndex++) {
    // Add header to new files (e.g. doesn't exist or overwriting)
    if (!doesFileExist(m_outFileNames[fileIndex]) || !append) {
      m_outputBuffer[fileIndex] = generateInstrumentHeader(l1);
    }

    // Then add each spectra to buffer
    for (size_t specIndex = 0; specIndex < numOutSpectra; specIndex++) {
      // Determine whether to skip the spectrum due to being masked
      if (m_allDetectorsValid && spectrumInfo.isMasked(specIndex)) {
        continue;
      }
      // Add bank header and details to buffer
      m_outputBuffer[specIndex]
          << generateBankHeader(spectrumInfo, specIndex).rdbuf();
      // Add data to buffer
      m_outputBuffer[specIndex] << generateBankData(specIndex).rdbuf();
      m_progress->report();
    }
  }
}

/**
  * Creates the file header information, which should be at the top of
  * each GSAS output file from the given workspace.
  *
  * @param l1 :: Value for the moderator to sample distance
  * @return :: A string stream containing the bank header details to be
  * written to the start of the file
*/
std::stringstream SaveGSS::generateInstrumentHeader(double l1) const {
  const Run &runinfo = m_inputWS->run();
  const std::string format = getPropertyValue("Format");
  std::stringstream outBuffer;

  // Run number
  if (format.compare(SLOG) == 0) {
    outBuffer << "Sample Run: ";
    outBuffer << getLogValue(runinfo, "run_number").rdbuf();
    outBuffer << " Vanadium Run: ";
    outBuffer << getLogValue(runinfo, "van_number").rdbuf();
    outBuffer << " Wavelength: ";
    outBuffer << getLogValue(runinfo, "LambdaRequest").rdbuf();
    outBuffer << "\n";
  }

  if (this->getProperty("ExtendedHeader")) {
    // the instrument parameter file
    if (runinfo.hasProperty("iparm_file")) {
      Kernel::Property *prop = runinfo.getProperty("iparm_file");
      if (prop != nullptr && (!prop->value().empty())) {
        std::stringstream line;
        line << "#Instrument parameter file: " << prop->value();
        outBuffer << std::setw(80) << std::left << line.str() << "\n";
      }
    }

    // write out the GSAS monitor counts
    outBuffer << "Monitor: ";
    if (runinfo.hasProperty("gsas_monitor")) {
      outBuffer << getLogValue(runinfo, "gsas_monitor").rdbuf();
    } else {
      outBuffer << getLogValue(runinfo, "gd_prtn_chrg", "1").rdbuf();
    }
    outBuffer << "\n";
  }

  if (format.compare(SLOG) == 0) {
    outBuffer << "# "; // make the next line a comment
  }
  outBuffer << m_inputWS->getTitle() << "\n";
  outBuffer << "# " << m_inputWS->getNumberHistograms() << " Histograms\n";
  outBuffer << "# File generated by Mantid:\n";
  outBuffer << "# Instrument: " << m_inputWS->getInstrument()->getName()
            << "\n";
  outBuffer << "# From workspace named : " << m_inputWS->getName() << "\n";
  if (getProperty("MultiplyByBinWidth"))
    outBuffer << "# with Y multiplied by the bin widths.\n";
  outBuffer << "# Primary flight path " << l1 << "m \n";
  if (format.compare(SLOG) == 0) {
    outBuffer << "# Sample Temperature: ";
    outBuffer << getLogValue(runinfo, "SampleTemp").rdbuf();
    outBuffer << " Freq: ";
    outBuffer << getLogValue(runinfo, "SpeedRequest1").rdbuf();
    outBuffer << " Guide: ";
    outBuffer << getLogValue(runinfo, "guide").rdbuf();
    outBuffer << "\n";

    // print whether it is normalized by monitor or proton charge
    bool norm_by_current = false;
    bool norm_by_monitor = false;
    const Mantid::API::AlgorithmHistories &algohist =
        m_inputWS->getHistory().getAlgorithmHistories();
    for (const auto &algo : algohist) {
      if (algo->name().compare("NormaliseByCurrent") == 0)
        norm_by_current = true;
      if (algo->name().compare("NormaliseToMonitor") == 0)
        norm_by_monitor = true;
    }
    outBuffer << "#";
    if (norm_by_current)
      outBuffer << " Normalised to pCharge";
    if (norm_by_monitor)
      outBuffer << " Normalised to monitor";
    outBuffer << "\n";
  }

  return outBuffer;
}

/**
  * Generates the out filename(s). If only one file is specified
  * this is the user specified filename. However when >1 file
  * is required (in split mode) generates the new file name
  * as 'name-n.ext' (where n is the current bank). This is stored
  * as a member variable
  *
  * @param numberOfOutFiles :: The number of output files required
  */
void Mantid::DataHandling::SaveGSS::generateOutFileNames(
    size_t numberOfOutFiles) {
  const std::string outputFileName = getProperty("Filename");
  assert(numberOfOutFiles > 0);

  if (numberOfOutFiles == 1) {
      // Only add one name and don't generate split filenames
      // when we are not in split mode
      m_outFileNames.push_back(outputFileName);
      return;
    }

  m_outFileNames.resize(numberOfOutFiles);

  Poco::Path path(outputFileName);
  // Filename minus extension
  const std::string basename = path.getBaseName();
  const std::string ext = path.getExtension();

  for (size_t i = 0; i < numberOfOutFiles; i++) {
    // Construct output name of the form 'base name-i.ext'
    std::string newFileName = basename;
    ((newFileName += '-') += std::to_string(i) += ".") += ext;
    // Remove filename from path
    path.makeParent();
    path.append(newFileName);
    m_outFileNames[i].assign(path.toString());
  }
}

/**
  * Gets the value from a RunInfo property (i.e., log) and turns it
  * into a string stream. If the property is unknown by default
  * UNKNOWN is written. However an alternative value can be
  * specified to be written out.
  *
  * @param runInfo :: Reference to the associated runInfo
  * @param name :: Name of the property to use
  * @param failsafeValue :: The value to use if the property cannot be
  * found. Defaults to 'UNKNOWN'
  * @return :: The property as a string stream
*/
std::stringstream SaveGSS::getLogValue(const Run &runinfo,
                                       const std::string &name,
                                       const std::string &failsafeValue) const {
  std::stringstream os;
  // Return without property exists
  if (!runinfo.hasProperty(name)) {
    os << failsafeValue;
    return os;
  }
  // Get handle of property
  auto *prop = runinfo.getProperty(name);

  // Return without a valid pointer to property
  if (!prop) {
    os << failsafeValue;
    return os;
  }

  // Get value
  auto *log = dynamic_cast<Kernel::TimeSeriesProperty<double> *>(prop);
  if (log) {
    // Time series to get mean
    os << log->getStatistics().mean;
  } else {
    // None time series
    os << prop->value();
  }

  // Unit
  std::string units = prop->units();
  if (!units.empty()) {
    os << " " << units;
  }
  return os;
}

/**
  * Returns if the input workspace instrument is valid
  *
  * @return :: True if the instrument is valid, else false
  */
bool SaveGSS::isInstrumentValid() const {
  // Instrument related
  Geometry::Instrument_const_sptr instrument = m_inputWS->getInstrument();
  if (instrument) {
    auto source = instrument->getSource();
    auto sample = instrument->getSample();
    if (source && sample) {
      return true;
    }
  }
  return false;
}

/**
  * Attempts to open an output stream at the user specified path.
  * This uses the append property to determine whether to append
  * or overwrite the file at the given path. The caller is
  * responsible for closing to stream when finished.
  *
  * @param outFilePath :: The full path of the file to open a stream out
  * @throws :: If the fail bit is set on the out stream. Additionally
  * logs the system reported error as a Mantid error.
  * @return :: The opened file stream at the user specified path
  */
std::ofstream SaveGSS::openFileStream(const std::string &outFilePath) {
  const bool append = getProperty("Append");

  // Select to append to current stream or override
  using std::ios_base;
  const ios_base::openmode mode = (append ? (ios_base::out | ios_base::app)
                                          : (ios_base::out | ios_base::trunc));

  auto outStream = std::ofstream(outFilePath, mode);
  if (outStream.fail()) {
    // Get the error message from library and log before throwing
    const std::string error = strerror(errno);
    g_log.error("Failed to open file. Error was: " + error);
    throw std::runtime_error("Could not open the file at the following path: " +
                             outFilePath);
  }
  // Stream is good - return to caller through a std::move
  return std::move(outStream);
}

/** Ensures that when a workspace group is passed as output to this workspace
*  everything is saved to one file and the bank number increments for each
*  group member.
*  @param alg ::           Pointer to the algorithm
*  @param propertyName ::  Name of the property
*  @param propertyValue :: Value  of the property
*  @param periodNum ::     Effectively a counter through the group members
*/
void SaveGSS::setOtherProperties(IAlgorithm *alg,
                                 const std::string &propertyName,
                                 const std::string &propertyValue,
                                 int periodNum) {
  // We want to append subsequent group members to the first one
  if (propertyName == "Append") {
    if (periodNum != 1) {
      alg->setPropertyValue(propertyName, "1");
    } else
      alg->setPropertyValue(propertyName, propertyValue);
  }
  // We want the bank number to increment for each member of the group
  else if (propertyName == "Bank") {
    alg->setProperty("Bank", std::stoi(propertyValue) + periodNum - 1);
  } else
    Algorithm::setOtherProperties(alg, propertyName, propertyValue, periodNum);
}

/**
  * Validates the user input is matches certain constraints
  * in length and type and logs a warning or throws depending on
  * whether we can continue.
  *
  * @throws:: If for any reason we cannot run the algorithm
  */
void SaveGSS::validateInputs() const {
  // Check whether it is PointData or Histogram
  if (!m_inputWS->isHistogramData())
    g_log.warning("Input workspace is NOT histogram! SaveGSS may not work "
                  "well with PointData.");

  // Check the number of histogram/spectra < 99
  const int nHist = static_cast<int>(m_inputWS->getNumberHistograms());
  if (nHist > 99) {
    std::string outError = "Number of Spectra(" + std::to_string(nHist) +
                           ") cannot be larger than 99 for GSAS file";
    g_log.error(outError);
    throw std::invalid_argument(outError);
  }

  // Check we have any output filenames
  assert(m_outFileNames.size() > 0);

  const std::string filename = getProperty("Filename");
  const bool append = getProperty("Append");
  for (const auto &filename : m_outFileNames) {
    if (!append && doesFileExist(filename)) {
      g_log.warning("Target GSAS file " + filename +
                    " exists and will be overwritten.\n");
    } else if (append && !doesFileExist(filename)) {
      g_log.warning("Target GSAS file " + filename +
                    " does not exist but algorithm was set to append.\n");
    }
  }
}

/**
  * Writes all the spectra to the file(s) from the buffer to the
  * list of output file paths.
  *
  * @param numOutFiles :: The number of files to be written
  * @param numSpectra :: The number of spectra per file to write
  * @throws :: If the file writing fails at all
  */
void SaveGSS::writeBufferToFile(size_t numOutFiles, size_t numSpectra) {
  for (size_t fileIndex = 0; fileIndex < numOutFiles; fileIndex++) {
    // Open each file when there are multiple
    // TODO Parallel and reporting
    auto outFile = openFileStream(m_outFileNames[fileIndex]);
    for (size_t specIndex = 0; specIndex < numSpectra; specIndex++) {
      // Write each spectra when there are multiple
      assert(m_outputBuffer[specIndex].str().size() > 0);
      outFile << m_outputBuffer[specIndex].rdbuf();
    }
    outFile.close();
  }
}

void SaveGSS::writeRALFdata(const int bank, const bool MultiplyByBinWidth,
                            std::stringstream &out,
                            const HistogramData::Histogram &histo) const {
  // TODO change this so it returns a SS
  const auto &xVals = histo.x();
  const auto &yVals = histo.y();
  const auto &eVals = histo.e();
  const size_t datasize = yVals.size();
  const double bc1 = xVals[0] * 32;
  const double bc2 = (xVals[1] - xVals[0]) * 32;
  // Logarithmic step
  double bc4 = (xVals[1] - xVals[0]) / xVals[0];
  if (!std::isfinite(bc4))
    bc4 = 0; // If X is zero for BANK

  // Write out the data header
  out << writeBankLine("RALF", bank, datasize).rdbuf();
  out << std::fixed << " " << std::setprecision(0) << std::setw(8) << bc1
      << std::fixed << " " << std::setprecision(0) << std::setw(8) << bc2
      << std::fixed << " " << std::setprecision(0) << std::setw(8) << bc1
      << std::fixed << " " << std::setprecision(5) << std::setw(7) << bc4
      << " FXYE\n";

  // Run over each Y entry
  for (size_t j = 0; j < datasize; j++) {
    // Calculate the error
    double Epos;
    if (MultiplyByBinWidth)
      Epos = eVals[j] * (xVals[j + 1] - xVals[j]); // E[j]*X[j]*bc4;
    else
      Epos = eVals[j];
    Epos = fixErrorValue(Epos);

    // The center of the X bin.
    out << std::fixed << std::setprecision(5) << std::setw(15)
        << 0.5 * (xVals[j] + xVals[j + 1]);

    // The Y value
    if (MultiplyByBinWidth)
      out << std::fixed << std::setprecision(8) << std::setw(18)
          << yVals[j] * (xVals[j + 1] - xVals[j]);
    else
      out << std::fixed << std::setprecision(8) << std::setw(18) << yVals[j];

    // The error
    out << std::fixed << std::setprecision(8) << std::setw(18) << Epos << "\n";
  }
}

void SaveGSS::writeSLOGdata(const int bank, const bool MultiplyByBinWidth,
                            std::stringstream &out,
                            const HistogramData::Histogram &histo) const {
  const auto &xVals = histo.x();
  const auto &yVals = histo.y();
  const auto &eVals = histo.e();

  const size_t datasize = yVals.size();
  const double bc1 = xVals.front(); // minimum TOF in microseconds
  if (bc1 <= 0.) {
    throw std::runtime_error(
        "Cannot write out logarithmic data starting at zero");
  }
  if (isConstantDelta(xVals)) {
    std::stringstream msg;
    msg << "While writing SLOG format: Found constant delta-T binning for "
           "bank " << bank;
    throw std::runtime_error(msg.str());
  }
  const double bc2 =
      0.5 * (*(xVals.rbegin()) +
             *(xVals.rbegin() + 1)); // maximum TOF (in microseconds?)
  const double bc3 = (*(xVals.begin() + 1) - bc1) / bc1; // deltaT/T

  g_log.debug() << "SaveGSS(): Min TOF = " << bc1 << '\n';

  out << writeBankLine("SLOG", bank, datasize).rdbuf();
  out << std::fixed << " " << std::setprecision(0) << std::setw(10) << bc1
      << std::fixed << " " << std::setprecision(0) << std::setw(10) << bc2
      << std::fixed << " " << std::setprecision(7) << std::setw(10) << bc3
      << std::fixed << " 0 FXYE\n";

  for (size_t i = 0; i < datasize; i++) {
    double y = yVals[i];
    double e = eVals[i];
    if (MultiplyByBinWidth) {
      // Multiple by bin width as
      double delta = xVals[i + 1] - xVals[i];
      y *= delta;
      e *= delta;
    }
    e = fixErrorValue(e);

    out << "  " << std::fixed << std::setprecision(9) << std::setw(20)
        << 0.5 * (xVals[i] + xVals[i + 1]) << "  " << std::fixed
        << std::setprecision(9) << std::setw(20) << y << "  " << std::fixed
        << std::setprecision(9) << std::setw(20) << e << std::setw(12) << " "
        << "\n"; // let it flush its own buffer
  }
  out << std::flush;
}

} // namespace DataHandling
} // namespace Mantid
