// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/SaveGSS.h"

#include "MantidAPI/AlgorithmHistory.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceHistory.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/ArrayBoundedValidator.h"
#include "MantidKernel/ArrayLengthValidator.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/FloatingPointComparison.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/VisibleWhenProperty.h"

#include <Poco/File.h>
#include <Poco/Path.h>

#include <fstream>

namespace Mantid::DataHandling {

using namespace Mantid::API;
using namespace Mantid::HistogramData;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SaveGSS)

namespace { // Anonymous namespace
const std::string RALF("RALF");
const std::string SLOG("SLOG");
const std::string FXYE("FXYE");
const std::string ALT("ALT");

/// Determines the tolerance when comparing two doubles for equality
const double m_TOLERANCE = 1.e-10;

void assertNumFilesAndSpectraIsValid(size_t numOutFiles, size_t numOutSpectra) {
  // If either numOutFiles or numOutSpectra are not 1 we need to check
  // that we are conforming to the expected storage layout.
  if ((numOutFiles != 1) || (numOutSpectra != 1)) {
    assert((numOutFiles > 1) != (numOutSpectra > 1));
  }
}

bool doesFileExist(const std::string &filePath) {
  auto file = Poco::File(filePath);
  return file.exists();
}

double fixErrorValue(const double value) {
  // Fix error if value is less than zero or infinity
  // Negative errors cannot be read by GSAS
  if (value <= 0. || !std::isfinite(value)) {
    return 0.;
  } else {
    return value;
  }
}

bool isEqual(const double left, const double right) {
  return Kernel::withinRelativeDifference(left, right, m_TOLERANCE);
}

bool isConstantDelta(const HistogramData::BinEdges &xAxis) {
  const double deltaX = (xAxis[1] - xAxis[0]);
  for (std::size_t i = 1; i < xAxis.size(); ++i) {
    if (!isEqual(xAxis[i] - xAxis[i - 1], deltaX)) {
      return false;
    }
  }
  return true;
}

std::unique_ptr<std::stringstream> makeStringStream() {
  // This and all unique_ptrs wrapping streams is a workaround
  // for GCC 4.x. The standard allowing streams to be moved was
  // added after the 4 series was released. Allowing this would
  // break the ABI (hence GCC 5 onwards doesn't have this fault)
  // so there are lots of restrictions in place with stream.
  // Instead we can work around this by using pointers to streams.
  // Tl;dr - This is a workaround for GCC 4.x (RHEL7)
  return std::make_unique<std::stringstream>();
}

//----------------------------------------------------------------------------------------------
/** Write standard bank header in format as
 * BANK bank_id data_size data_size  binning_type
 * This part is same for SLOG and RALF
 * @brief writeBankHeader
 * @param out
 * @param bintype :: GSAS binning type as SLOG or RALF
 * @param banknum
 * @param datasize
 */
void writeBankHeader(std::stringstream &out, const std::string &bintype, const int banknum, const size_t datasize) {
  std::ios::fmtflags fflags(out.flags());
  out << "BANK " << std::fixed << std::setprecision(0)
      << banknum // First bank should be 1 for GSAS; this can be changed
      << std::fixed << " " << datasize << std::fixed << " " << datasize << std::fixed << " " << bintype;
  out.flags(fflags);
}
} // End of anonymous namespace

//----------------------------------------------------------------------------------------------
// Initialise the algorithm
void SaveGSS::init() {
  const std::vector<std::string> exts{".gsa", ".gss", ".gda", ".txt"};
  declareProperty(
      std::make_unique<API::WorkspaceProperty<MatrixWorkspace>>("InputWorkspace", "", Kernel::Direction::Input),
      "The input workspace");

  declareProperty(std::make_unique<API::FileProperty>("Filename", "", API::FileProperty::Save, exts),
                  "The filename to use for the saved data");

  declareProperty("SplitFiles", true,
                  "Whether to save each spectrum into a separate file ('true') "
                  "or not ('false'). Note that this is a string, not a boolean property.");

  declareProperty("Append", true, "If true and Filename already exists, append, else overwrite ");

  declareProperty("Bank", 1,
                  "The bank number to include in the file header for the first spectrum, "
                  "i.e., the starting bank number. "
                  "This will increment for each spectrum or group member.");

  const std::vector<std::string> formats{RALF, SLOG};
  declareProperty("Format", RALF, std::make_shared<Kernel::StringListValidator>(formats), "GSAS format to save as");

  const std::vector<std::string> ralfDataFormats{FXYE, ALT};
  declareProperty("DataFormat", FXYE, std::make_shared<Kernel::StringListValidator>(ralfDataFormats),
                  "Saves RALF data as either FXYE or alternative format");
  setPropertySettings("DataFormat", std::make_unique<Kernel::VisibleWhenProperty>(
                                        "Format", Kernel::ePropertyCriterion::IS_EQUAL_TO, RALF));

  declareProperty("MultiplyByBinWidth", true, "Multiply the intensity (Y) by the bin width; default TRUE.");

  declareProperty("ExtendedHeader", false, "Add information to the header about iparm file and normalization");

  declareProperty("UseSpectrumNumberAsBankID", false,
                  "If true, then each bank's bank ID is equal to the spectrum number; "
                  "otherwise, the continuous bank IDs are applied. ");

  declareProperty(std::make_unique<Kernel::ArrayProperty<std::string>>("UserSpecifiedGSASHeader"),
                  "Each line will be put to the header of the output GSAS file.");

  declareProperty("OverwriteStandardHeader", true,
                  "If true, then the standard header will be replaced "
                  "by the user specified header.  Otherwise, the user "
                  "specified header will be "
                  "inserted before the original header");

  declareProperty(std::make_unique<Kernel::ArrayProperty<std::string>>("UserSpecifiedBankHeader"),
                  "Each string will be used to replaced the standard GSAS bank header."
                  "Number of strings in the give array must be same as number of banks."
                  "And the order is preserved.");

  auto must_be_3 = std::make_shared<Kernel::ArrayLengthValidator<int>>(3);
  auto precision_range = std::make_shared<Kernel::ArrayBoundedValidator<int>>(0, 10);

  auto precision_validator = std::make_shared<Kernel::CompositeValidator>();
  precision_validator->add(must_be_3);
  precision_validator->add(precision_range);

  std::vector<int> default_precision(3, 9);
  declareProperty(std::make_unique<Kernel::ArrayProperty<int>>("SLOGXYEPrecision", std::move(default_precision),
                                                               std::move(precision_validator)),
                  "Enter 3 integers as the precisions of output X, Y and E for SLOG data "
                  "only."
                  "Default is (9, 9, 9) if it is left empty.  Otherwise it is not "
                  "allowed.");
}

// Execute the algorithm
void SaveGSS::exec() {
  // Retrieve the input workspace and associated properties
  m_inputWS = getProperty("InputWorkspace");
  const size_t nHist = m_inputWS->getNumberHistograms();

  // process user special headers
  processUserSpecifiedHeaders();

  // Are we writing one file with n spectra or
  // n files with 1 spectra each
  const bool split = getProperty("SplitFiles");
  const size_t numOfOutFiles{split ? nHist : 1};
  const size_t numOutSpectra{split ? 1 : nHist};

  // Initialise various properties we are going to need within this alg
  m_allDetectorsValid = (isInstrumentValid() && areAllDetectorsValid());
  generateOutFileNames(numOfOutFiles);
  m_outputBuffer.resize(nHist);
  std::generate(m_outputBuffer.begin(), m_outputBuffer.end(), []() { return makeStringStream(); });

  // Progress is 2 * number of histograms. One for generating data
  // one for writing out data
  m_progress = std::make_unique<Progress>(this, 0.0, 1.0, (nHist * 2));

  // Now start executing main part of the code
  generateGSASBuffer(numOfOutFiles, numOutSpectra);
  writeBufferToFile(numOfOutFiles, numOutSpectra);
}

//----------------------------------------------------------------------------------------------
/** process user specified headers
 *@brief SaveGSS::processUserSpecifiedHeaders
 */
void SaveGSS::processUserSpecifiedHeaders() {

  // user specified GSAS
  m_user_specified_gsas_header = getProperty("UserSpecifiedGSASHeader");

  m_overwrite_std_gsas_header = getProperty("OverwriteStandardHeader");
  if (m_user_specified_gsas_header.empty()) {
    m_overwrite_std_gsas_header = false;
  }

  // user specified bank header
  m_user_specified_bank_headers = getProperty("UserSpecifiedBankHeader");
  m_overwrite_std_bank_header = !m_user_specified_bank_headers.empty();

  return;
}

//----------------------------------------------------------------------------------------------
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

  const auto numHistInt64 = static_cast<int64_t>(numHist);

  PARALLEL_FOR_NO_WSP_CHECK()
  for (int64_t histoIndex = 0; histoIndex < numHistInt64; histoIndex++) {
    if (allValid) {
      // Check this spectra has detectors
      if (!spectrumInfo.hasDetectors(histoIndex)) {
        allValid = false;
        g_log.warning() << "There is no detector associated with spectrum " << histoIndex
                        << ". Workspace is treated as NO-INSTRUMENT case. \n";
      }
    }
  }
  return allValid;
}

//----------------------------------------------------------------------------------------------
/**
 * Generates a string stream in GSAS format containing the
 * data for the specified bank from the workspace. This
 * can either be in RALF or SLOG format.
 *
 * @param outBuf :: The string stream to write to
 * @param specIndex :: The index of the bank to convert into
 * @param outputFormat :: output format
 * @param slog_xye_precisions :: the precision of output XYE for SLOG data only
 * a string stream
 */
void SaveGSS::generateBankData(std::stringstream &outBuf, size_t specIndex, const std::string &outputFormat,
                               const std::vector<int> &slog_xye_precisions) const {
  // Determine bank number into GSAS file
  const bool useSpecAsBank = getProperty("UseSpectrumNumberAsBankID");
  const bool multiplyByBinWidth = getProperty("MultiplyByBinWidth");
  const int userStartingBankNumber = getProperty("Bank");
  const std::string ralfDataFormat = getPropertyValue("DataFormat");

  int bankid;
  if (useSpecAsBank) {
    bankid = static_cast<int>(m_inputWS->getSpectrum(specIndex).getSpectrumNo());
  } else {
    bankid = userStartingBankNumber + static_cast<int>(specIndex);
  }

  // Write data
  const auto &histogram = m_inputWS->histogram(specIndex);
  if (outputFormat == RALF) {
    if (ralfDataFormat == FXYE) {
      writeRALF_XYEdata(bankid, multiplyByBinWidth, outBuf, histogram);
    } else if (ralfDataFormat == ALT) {
      writeRALF_ALTdata(outBuf, bankid, histogram);
    } else {
      throw std::runtime_error("Unknown RALF data format" + ralfDataFormat);
    }
  } else if (outputFormat == SLOG) {
    writeSLOGdata(specIndex, bankid, multiplyByBinWidth, outBuf, histogram, slog_xye_precisions);
  } else {
    throw std::runtime_error("Cannot write to the unknown " + outputFormat + "output format");
  }
}

/**
 * Generates the bank header (which precedes bank data)
 * for the spectra index specified.
 *
 * @param out :: The stream to write to
 * @param spectrumInfo :: The input workspace spectrum info object
 * @param specIndex :: The bank index to generate a header for
 */
void SaveGSS::generateBankHeader(std::stringstream &out, const API::SpectrumInfo &spectrumInfo,
                                 size_t specIndex) const {
  // If we have all valid detectors get these properties else use 0
  if (m_allDetectorsValid) {
    const auto l1 = spectrumInfo.l1();
    const auto l2 = spectrumInfo.l2(specIndex);
    const auto twoTheta = spectrumInfo.twoTheta(specIndex);
    auto diffConstants = spectrumInfo.diffractometerConstants(specIndex);
    out << "# Total flight path " << (l1 + l2) << "m, tth " << (twoTheta * 180. / M_PI) << "deg, DIFC "
        << diffConstants[Kernel::UnitParams::difc] << "\n";
  }
  out << "# Data for spectrum :" << specIndex << "\n";
}

/**
 * Generates the GSAS file and populates the output buffer
 * with the data to be written to the file(s) in subsequent methods
 *
 * @param numOutFiles :: The number of file to be written
 * @param numOutSpectra :: The number of spectra per file to be written
 */
void SaveGSS::generateGSASBuffer(size_t numOutFiles, size_t numOutSpectra) {
  // Generate the output buffer for each histogram (spectrum)
  const auto &spectrumInfo = m_inputWS->spectrumInfo();
  const bool append = getProperty("Append");

  // Check if the caller has reserved space in our output buffer
  assert(m_outputBuffer.size() > 0);

  // Because of the storage layout we can either handle files > 0
  // XOR spectra per file > 0. Compensate for the fact that is will be
  // 1 thing per thing as far as users are concerned.
  assertNumFilesAndSpectraIsValid(numOutFiles, numOutSpectra);

  // If all detectors are not valid we use the no instrument case and
  // set l1 to 0
  const double l1{m_allDetectorsValid ? spectrumInfo.l1() : 0};

  const auto numOutFilesInt64 = static_cast<int64_t>(numOutFiles);

  const std::string outputFormat = getPropertyValue("Format");

  std::vector<int> slog_xye_precisions = getProperty("SLOGXYEPrecision");

  // Create the various output files we will need in a loop
  PARALLEL_FOR_NO_WSP_CHECK()
  for (int64_t fileIndex = 0; fileIndex < numOutFilesInt64; fileIndex++) {
    // Add header to new files (e.g. doesn't exist or overwriting)
    if (!doesFileExist(m_outFileNames[fileIndex]) || !append) {
      generateInstrumentHeader(*m_outputBuffer[fileIndex], l1);
    }

    // Then add each spectra to buffer
    for (size_t specIndex = 0; specIndex < numOutSpectra; specIndex++) {
      // Determine whether to skip the spectrum due to being masked
      if (m_allDetectorsValid && spectrumInfo.isMasked(specIndex)) {
        continue;
      }
      // Find the index - this is because we are guaranteed at least
      // one of these is 0 from the assertion above and the fact
      // split files is a boolean operator on the input side
      const int64_t index = specIndex + fileIndex;
      // Add bank header and details to buffer
      generateBankHeader(*m_outputBuffer[index], spectrumInfo, index);
      // Add data to buffer
      generateBankData(*m_outputBuffer[index], index, outputFormat, slog_xye_precisions);
      m_progress->report();
    }
  }
}

/**
 * Creates the file header information, which should be at the top of
 * each GSAS output file from the given workspace.
 *
 * @param out :: The stringstream to write the header to
 * @param l1 :: Value for the moderator to sample distance
 */
void SaveGSS::generateInstrumentHeader(std::stringstream &out, double l1) const {

  // write user header first
  if (m_user_specified_gsas_header.size() > 0) {
    for (const auto &iter : m_user_specified_gsas_header) {
      out << iter << "\n";
    }
  }

  // quit method if user plan to use his own header completely
  if (m_overwrite_std_gsas_header) {
    return;
  }

  // write standard header
  const Run &runinfo = m_inputWS->run();
  const std::string format = getPropertyValue("Format");

  // Run number
  if (format == SLOG) {
    out << "Sample Run: ";
    getLogValue(out, runinfo, "run_number");
    out << " Vanadium Run: ";
    getLogValue(out, runinfo, "van_number");
    out << " Wavelength: ";
    getLogValue(out, runinfo, "LambdaRequest");
    out << "\n";
  }

  if (this->getProperty("ExtendedHeader")) {
    // the instrument parameter file
    if (runinfo.hasProperty("iparm_file")) {
      Kernel::Property *prop = runinfo.getProperty("iparm_file");
      if (prop != nullptr && (!prop->value().empty())) {
        out << std::setw(80) << std::left;
        out << "#Instrument parameter file: " << prop->value() << "\n";
      }
    }

    // write out the GSAS monitor counts
    out << "Monitor: ";
    if (runinfo.hasProperty("gsas_monitor")) {
      getLogValue(out, runinfo, "gsas_monitor");
    } else {
      getLogValue(out, runinfo, "gd_prtn_chrg", "1");
    }
    out << "\n";
  }

  if (format == SLOG) {
    out << "# "; // make the next line a comment
  }
  out << m_inputWS->getTitle() << "\n";
  out << "# " << m_inputWS->getNumberHistograms() << " Histograms\n";
  out << "# File generated by Mantid:\n";
  out << "# Instrument: " << m_inputWS->getInstrument()->getName() << "\n";
  out << "# From workspace named : " << m_inputWS->getName() << "\n";
  if (getProperty("MultiplyByBinWidth"))
    out << "# with Y multiplied by the bin widths.\n";
  out << "# Primary flight path " << l1 << "m \n";
  if (format == SLOG) {
    out << "# Sample Temperature: ";
    getLogValue(out, runinfo, "SampleTemp");
    out << " Freq: ";
    getLogValue(out, runinfo, "SpeedRequest1");
    out << " Guide: ";
    getLogValue(out, runinfo, "guide");
    out << "\n";

    // print whether it is normalized by monitor or proton charge
    bool norm_by_current = false;
    bool norm_by_monitor = false;
    const Mantid::API::AlgorithmHistories &algohist = m_inputWS->getHistory().getAlgorithmHistories();
    for (const auto &algo : algohist) {
      if (algo->name() == "NormaliseByCurrent")
        norm_by_current = true;
      if (algo->name() == "NormaliseToMonitor")
        norm_by_monitor = true;
    }
    out << "#";
    if (norm_by_current)
      out << " Normalised to pCharge";
    if (norm_by_monitor)
      out << " Normalised to monitor";
    out << "\n";
  }
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
void SaveGSS::generateOutFileNames(size_t numberOfOutFiles) {
  const std::string outputFileName = getProperty("Filename");
  assert(numberOfOutFiles > 0);

  if (numberOfOutFiles == 1) {
    // Only add one name and don't generate split filenames
    // when we are not in split mode
    m_outFileNames.emplace_back(outputFileName);
    return;
  }

  m_outFileNames.resize(numberOfOutFiles);

  Poco::Path path(outputFileName);
  // Filename minus extension
  const std::string basename = path.getBaseName();
  const std::string ext = path.getExtension();

  // get file name and check with warning
  const bool append = getProperty("Append");
  for (size_t i = 0; i < numberOfOutFiles; i++) {
    // Construct output name of the form 'base name-i.ext'
    std::string newFileName = basename;
    ((newFileName += '-') += std::to_string(i) += ".") += ext;
    // Remove filename from path
    path.makeParent();
    path.append(newFileName);
    std::string filename = path.toString();
    m_outFileNames[i].assign(filename);
    // check and make some warning
    if (!append && doesFileExist(filename)) {
      g_log.warning("Target GSAS file " + filename + " exists and will be overwritten.\n");
    } else if (append && !doesFileExist(filename)) {
      g_log.warning("Target GSAS file " + filename + " does not exist but algorithm was set to append.\n");
    }
  }

  return;
}

/**
 * Gets the value from a RunInfo property (i.e., log) and turns it
 * into a string stream. If the property is unknown by default
 * UNKNOWN is written. However an alternative value can be
 * specified to be written out.
 *
 * @param out :: The stream to write to
 * @param runInfo :: Reference to the associated runInfo
 * @param name :: Name of the property to use
 * @param failsafeValue :: The value to use if the property cannot be
 * found. Defaults to 'UNKNOWN'
 */
void SaveGSS::getLogValue(std::stringstream &out, const API::Run &runInfo, const std::string &name,
                          const std::string &failsafeValue) const {
  // Return without property exists
  if (!runInfo.hasProperty(name)) {
    out << failsafeValue;
    return;
  }
  // Get handle of property
  auto *prop = runInfo.getProperty(name);

  // Return without a valid pointer to property
  if (!prop) {
    out << failsafeValue;
    return;
  }

  // Get value
  auto *log = dynamic_cast<Kernel::TimeSeriesProperty<double> *>(prop);
  if (log) {
    // Time series to get mean
    out << log->getStatistics().mean;
  } else {
    // No time series - just get the value
    out << prop->value();
  }

  // Unit
  std::string units = prop->units();
  if (!units.empty()) {
    out << " " << units;
  }
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
 * @param outStream :: The stream to open at the specified file
 * @throws :: If the fail bit is set on the out stream. Additionally
 * logs the system reported error as a Mantid error.
 */
void SaveGSS::openFileStream(const std::string &outFilePath, std::ofstream &outStream) {
  // We have to take the ofstream as a parameter instead of returning
  // as GCC 4.X (RHEL7) does not allow a ioStream to be moved or have
  // NRVO applied

  // Select to append to current stream or override
  const bool append = getProperty("Append");
  using std::ios_base;
  const ios_base::openmode mode = (append ? (ios_base::out | ios_base::app) : (ios_base::out | ios_base::trunc));

  // Have to wrap this in a unique pointer as GCC 4.x (RHEL 7) does
  // not support the move operator on iostreams
  outStream.open(outFilePath, mode);
  if (outStream.fail()) {
    // Get the error message from library and log before throwing
    const std::string error = strerror(errno);
    throw Kernel::Exception::FileError("Failed to open file. Error was: " + error, outFilePath);
  }

  // Stream is good at this point
}

//----------------------------------------------------------------------------
/** Ensures that when a workspace group is passed as output to this workspace
 *  everything is saved to one file and the bank number increments for each
 *  group member.
 *  @param alg ::           Pointer to the algorithm
 *  @param propertyName ::  Name of the property
 *  @param propertyValue :: Value  of the property
 *  @param periodNum ::     Effectively a counter through the group members
 */
void SaveGSS::setOtherProperties(IAlgorithm *alg, const std::string &propertyName, const std::string &propertyValue,
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
 * @throws :: If for any reason we cannot run the algorithm
 */
std::map<std::string, std::string> SaveGSS::validateInputs() {
  std::map<std::string, std::string> result;

  API::MatrixWorkspace_const_sptr input_ws = getProperty("InputWorkspace");
  if (!input_ws) {
    result["InputWorkspace"] = "The input workspace cannot be a GroupWorkspace.";
    return result;
  }
  // Check the number of histogram/spectra < 99
  const auto nHist = static_cast<int>(input_ws->getNumberHistograms());
  const bool split = getProperty("SplitFiles");
  if (nHist > 99 && !split) {
    std::string outError = "Number of Spectra(" + std::to_string(nHist) + ") cannot be larger than 99 for GSAS file";
    result["InputWorkspace"] = outError;
    result["SplitFiles"] = outError;
    return result;
  }

  // Check we have any output filenames
  std::string output_file_name = getProperty("Filename");
  if (output_file_name.size() == 0) {
    result["Filename"] = "Filename cannot be left empty.";
    return result;
  }

  // Check about the user specified bank header
  std::vector<std::string> user_header_vec = getProperty("UserSpecifiedBankHeader");
  if (user_header_vec.size() > 0 && user_header_vec.size() != input_ws->getNumberHistograms()) {
    result["UserSpecifiedBankHeader"] = "If user specifies bank header, each bank must "
                                        "have a unique user-specified header.";
    return result;
  }

  return result;
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
  // When there are multiple files we can open them all in parallel
  // Check that either the number of files or spectra is greater than
  // 1 otherwise our storage method is no longer valid
  assertNumFilesAndSpectraIsValid(numOutFiles, numSpectra);

  const auto numOutFilesInt64 = static_cast<int64_t>(numOutFiles);

  PARALLEL_FOR_NO_WSP_CHECK()
  for (int64_t fileIndex = 0; fileIndex < numOutFilesInt64; fileIndex++) {
    PARALLEL_START_INTERRUPT_REGION

    // Open each file when there are multiple
    std::ofstream fileStream;
    openFileStream(m_outFileNames[fileIndex], fileStream);
    for (size_t specIndex = 0; specIndex < numSpectra; specIndex++) {
      // Write each spectra when there are multiple
      const size_t index = specIndex + fileIndex;
      assert(m_outputBuffer[index]->str().size() > 0);
      fileStream << m_outputBuffer[index]->rdbuf();
    }

    fileStream.close();
    if (fileStream.fail()) {
      const std::string error = strerror(errno);
      g_log.error("Failed to close file. Error was: " + error);
      throw std::runtime_error("Failed to close the file at " + m_outFileNames[fileIndex] +
                               " - this file may be empty, corrupted or incorrect.");
    }
    PARALLEL_END_INTERRUPT_REGION
  }
  PARALLEL_CHECK_INTERRUPT_REGION
}

void SaveGSS::writeRALFHeader(std::stringstream &out, int bank, const HistogramData::Histogram &histo) const {
  const auto &xVals = histo.binEdges();
  const size_t datasize = histo.y().size();
  const double bc1 = xVals[0] * 32;
  const double bc2 = (xVals[1] - xVals[0]) * 32;
  // Logarithmic step
  double bc4 = (xVals[1] - xVals[0]) / xVals[0];
  if (!std::isfinite(bc4))
    bc4 = 0; // If X is zero for BANK

  // Write out the data header
  writeBankHeader(out, "RALF", bank, datasize);
  const std::string bankDataType = getPropertyValue("DataFormat");
  out << std::fixed << " " << std::setprecision(0) << std::setw(8) << bc1 << std::fixed << " " << std::setprecision(0)
      << std::setw(8) << bc2 << std::fixed << " " << std::setprecision(0) << std::setw(8) << bc1 << std::fixed << " "
      << std::setprecision(5) << std::setw(7) << bc4 << " " << bankDataType << "\n";
}

void SaveGSS::writeRALF_ALTdata(std::stringstream &out, const int bank, const HistogramData::Histogram &histo) const {
  const size_t datasize = histo.y().size();
  const auto &xPointVals = histo.points();
  const auto &yVals = histo.y();
  const auto &eVals = histo.e();

  writeRALFHeader(out, bank, histo);

  const size_t dataEntriesPerLine = 4;
  // This method calculates the minimum number of lines
  // we need to write to capture all the data for example
  // if we have 6 data entries it will calculate 2 output lines (4 + 2)
  const int64_t numberOfOutLines = (datasize + dataEntriesPerLine - 1) / dataEntriesPerLine;

  std::vector<std::unique_ptr<std::stringstream>> outLines;
  outLines.resize(numberOfOutLines);

  for (int64_t i = 0; i < numberOfOutLines; i++) {
    outLines[i] = makeStringStream();
    auto &outLine = *outLines[i];

    size_t dataPosition = i * dataEntriesPerLine;
    const size_t endPosition = dataPosition + dataEntriesPerLine;

    // 4 Blocks of data (20 chars) per line (80 chars)
    for (; dataPosition < endPosition; dataPosition++) {
      if (dataPosition < datasize) {
        // We have data to append

        const auto epos = static_cast<int>(fixErrorValue(eVals[dataPosition] * 1000));

        outLine << std::fixed << std::setw(8) << static_cast<int>(xPointVals[dataPosition] * 32);
        outLine << std::fixed << std::setw(7) << static_cast<int>(yVals[dataPosition] * 1000);
        outLine << std::fixed << std::setw(5) << epos;
      }
    }
    // Append a newline character at the end of each data block
    outLine << "\n";
  }

  for (const auto &outLine : outLines) {
    // Ensure the output order is preserved
    out << outLine->rdbuf();
  }
}

void SaveGSS::writeRALF_XYEdata(const int bank, const bool MultiplyByBinWidth, std::stringstream &out,
                                const HistogramData::Histogram &histo) const {
  const auto &xVals = histo.binEdges();
  const auto &xPointVals = histo.points();
  const auto &yVals = histo.y();
  const auto &eVals = histo.e();
  const size_t datasize = yVals.size();

  writeRALFHeader(out, bank, histo);

  std::vector<std::unique_ptr<std::stringstream>> outLines;
  outLines.resize(datasize);

  PARALLEL_FOR_NO_WSP_CHECK()
  for (int64_t i = 0; i < static_cast<int64_t>(datasize); i++) {
    outLines[i] = makeStringStream();
    auto &outLine = *outLines[i];
    const double binWidth = xVals[i + 1] - xVals[i];
    const double outYVal{MultiplyByBinWidth ? yVals[i] * binWidth : yVals[i]};
    const double epos = fixErrorValue(MultiplyByBinWidth ? eVals[i] * binWidth : eVals[i]);

    // The center of the X bin.
    outLine << std::fixed << std::setprecision(5) << std::setw(15) << xPointVals[i];
    outLine << std::fixed << std::setprecision(8) << std::setw(18) << outYVal;
    outLine << std::fixed << std::setprecision(8) << std::setw(18) << epos << "\n";
  }

  for (const auto &outLine : outLines) {
    // Ensure the output order is preserved
    out << outLine->rdbuf();
  }
}

//----------------------------------------------------------------------------
/** write slog data
 * @brief SaveGSS::writeSLOGdata
 * @param ws_index :: workspace index for the spectrum to be written.  It is
 * essential for using user-specified bank header
 * @param bank
 * @param MultiplyByBinWidth
 * @param out
 * @param histo
 * @param xye_precision
 */
void SaveGSS::writeSLOGdata(const size_t ws_index, const int bank, const bool MultiplyByBinWidth,
                            std::stringstream &out, const HistogramData::Histogram &histo,
                            const std::vector<int> &xye_precision) const {
  // check inputs
  if (xye_precision.size() != 3)
    throw std::runtime_error("SLOG XYE precisions are not given in a 3-item vector.");

  const auto &xVals = histo.binEdges();
  const auto &xPoints = histo.points();
  const auto &yVals = histo.y();
  const auto &eVals = histo.e();
  const size_t datasize = yVals.size();

  const double bc1 = xVals.front();                      // minimum TOF in microseconds
  const double bc2 = *(xPoints.end() - 1);               // maximum TOF (in microseconds?)
  const double bc3 = (*(xVals.begin() + 1) - bc1) / bc1; // deltaT/T

  if (bc1 <= 0.) {
    throw std::runtime_error("Cannot write out logarithmic data starting at zero or less");
  }
  if (isConstantDelta(xVals)) {
    g_log.error() << "Constant delta - T binning : " << xVals.front() << ", " << *(xVals.begin() + 1) << ", "
                  << *(xVals.begin() + 2) << "... " << std::endl;
    throw std::runtime_error("While writing SLOG format : Found constant "
                             "delta - T binning for bank " +
                             std::to_string(bank));
  }

  g_log.debug() << "SaveGSS(): Min TOF = " << bc1 << '\n';

  // Write bank header
  if (m_overwrite_std_bank_header) {
    // write user header only!
    out << std::fixed << std::setw(80) << m_user_specified_bank_headers[ws_index] << "\n";
  } else {
    // write general bank header part
    writeBankHeader(out, "SLOG", bank, datasize);
    // write the SLOG specific type
    out << std::fixed << " " << std::setprecision(0) << std::setw(10) << bc1 << std::fixed << " "
        << std::setprecision(0) << std::setw(10) << bc2 << std::fixed << " " << std::setprecision(7) << std::setw(10)
        << bc3 << std::fixed << " 0 FXYE\n";
  }

  std::vector<std::unique_ptr<std::stringstream>> outLines;
  outLines.resize(datasize);

  PARALLEL_FOR_NO_WSP_CHECK()
  for (int64_t i = 0; i < static_cast<int64_t>(datasize); i++) {
    outLines[i] = makeStringStream();
    auto &outLine = *outLines[i];
    const double binWidth = xVals[i + 1] - xVals[i];
    const double yValue{MultiplyByBinWidth ? yVals[i] * binWidth : yVals[i]};
    const double eValue{fixErrorValue(MultiplyByBinWidth ? eVals[i] * binWidth : eVals[i])};

    // FIXME - Next step is to make the precision to be flexible from user
    // inputs
    outLine << "  " << std::fixed << std::setprecision(xye_precision[0]) << std::setw(20) << xPoints[i] << "  "
            << std::fixed << std::setprecision(xye_precision[1]) << std::setw(20) << yValue << "  " << std::fixed
            << std::setprecision(xye_precision[2]) << std::setw(20) << eValue << std::setw(12) << " "
            << "\n"; // let it flush its own buffer
  }

  for (const auto &outLine : outLines) {
    out << outLine->rdbuf();
  }
}

} // namespace Mantid::DataHandling
