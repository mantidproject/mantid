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

namespace {
const std::string RALF("RALF");
const std::string SLOG("SLOG");
const double TOLERANCE = 1.e-10;
}

SaveGSS::SaveGSS() : Mantid::API::Algorithm(), m_useSpecAsBank(false) {}

bool isEqual(const double left, const double right) {
  if (left == right)
    return true;
  return (2. * std::fabs(left - right) <=
          std::fabs(TOLERANCE * (right + left)));
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

/** Initialise the algorithm
  */
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
  std::vector<std::string> formats;
  formats.push_back(RALF);
  formats.push_back(SLOG);
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
      "otherwise, the continous bank IDs are applied. ");
}

/** Execute the algorithm
  */
void SaveGSS::exec() {
  // Process properties
  // Retrieve the input workspace
  inputWS = getProperty("InputWorkspace");

  // Check whether it is PointData or Histogram
  if (!inputWS->isHistogramData())
    g_log.warning("Input workspace is NOT histogram!  SaveGSS may not work "
                  "well with PointData.");

  // Check the number of histogram/spectra < 99
  const int nHist = static_cast<int>(inputWS->getNumberHistograms());
  if (nHist > 99) {
    std::stringstream errss;
    errss << "Number of Spectra (" << nHist
          << ") cannot be larger than 99 for GSAS file";
    g_log.error(errss.str());
    throw std::invalid_argument(errss.str());
  }

  std::string filename = getProperty("Filename");

  const int bank = getProperty("Bank");
  const bool MultiplyByBinWidth = getProperty("MultiplyByBinWidth");
  bool split = getProperty("SplitFiles");

  std::string outputFormat = getProperty("Format");

  m_useSpecAsBank = getProperty("UseSpectrumNumberAsBankID");

  // Check whether to append to an already existing file or overwrite
  bool append = getProperty("Append");

  // Check whether append or not
  if (!split) {
    Poco::File fileobj(filename);
    if (fileobj.exists() && !append) {
      // Non-append mode and will be overwritten
      g_log.warning() << "Target GSAS file " << filename
                      << " exists and will be overwritten. "
                      << "\n";
    } else if (!fileobj.exists() && append) {
      // File does not exist but in append mode
      g_log.warning() << "Target GSAS file " << filename
                      << " does not exist.  Append mode is set to false "
                      << "\n";
      append = false;
    }
  }

  writeGSASFile(filename, append, bank, MultiplyByBinWidth, split,
                outputFormat);
}

/** Write GSAS file based on user-specified request
  */
void SaveGSS::writeGSASFile(const std::string &outfilename, bool append,
                            int basebanknumber, bool multiplybybinwidth,
                            bool split, const std::string &outputFormat) {
  // Initialize the file stream
  using std::ios_base;
  ios_base::openmode mode =
      (append ? (ios_base::out | ios_base::app) : ios_base::out);

  // Instrument related
  Geometry::Instrument_const_sptr instrument = inputWS->getInstrument();
  Geometry::IComponent_const_sptr source;
  Geometry::IComponent_const_sptr sample;
  bool has_instrument = false;
  if (instrument != nullptr) {
    source = instrument->getSource();
    sample = instrument->getSample();
    if (source && sample)
      has_instrument = true;
  }

  // Write GSAS file for each histogram (spectrum)
  std::stringstream outbuffer;

  int nHist = static_cast<int>(inputWS->getNumberHistograms());
  Progress p(this, 0.0, 1.0, nHist);

  const auto &spectrumInfo = inputWS->spectrumInfo();
  // Loop over each histogram within the workspace
  for (int histoIndex = 0; histoIndex < nHist; histoIndex++) {
    // Determine whether to skip the spectrum due to being masked
    if (has_instrument) {
      if (!spectrumInfo.hasDetectors(histoIndex)) {
        has_instrument = false;
        g_log.warning() << "There is no detector associated with spectrum "
                        << histoIndex
                        << ". Workspace is treated as NO-INSTRUMENT case. \n";
      } else if (spectrumInfo.isMasked(histoIndex)) {
        continue;
      }
    }

    // Obtain detector information
    double l1, l2, tth, difc;
    if (has_instrument) {
      l1 = spectrumInfo.l1();
      l2 = spectrumInfo.l2(histoIndex);
      tth = spectrumInfo.twoTheta(histoIndex);
      difc =
          ((2.0 * PhysicalConstants::NeutronMass * sin(tth * 0.5) * (l1 + l2)) /
           (PhysicalConstants::h * 1.e4));

    } else {
      l1 = l2 = tth = difc = 0;
    }
    g_log.debug() << "Spectrum " << histoIndex << ": L1 = " << l1
                  << "  L2 = " << l2 << "  2theta = " << tth << "\n";

    std::stringstream tmpbuffer;

    // Header: 2 cases requires header: (1) first bank in non-append mode and
    // (2) split
    g_log.debug() << "[DB9933] Append = " << append << ", split = " << split
                  << "\n";

    bool writeheader = false;
    std::string splitfilename;
    if (!split && histoIndex == 0 && !append) {
      // Non-split mode and first spectrum and in non-append mode
      writeheader = true;
    } else if (split) {
      std::stringstream number;
      number << "-" << histoIndex;

      Poco::Path path(outfilename);
      std::string basename = path.getBaseName(); // Filename minus extension
      std::string ext = path.getExtension();
      // Chop off filename
      path.makeParent();
      path.append(basename + number.str() + "." + ext);
      Poco::File fileobj(path);
      const bool exists = fileobj.exists();
      if (!exists || !append)
        writeheader = true;
      if (fileobj.exists() && !append)
        g_log.warning() << "File " << path.getFileName()
                        << " exists and will be overwritten."
                        << "\n";
      splitfilename = path.toString();
    }

    // Create header
    if (writeheader)
      writeHeaders(outputFormat, tmpbuffer, l1);

    // Write bank header
    if (has_instrument) {
      tmpbuffer << "# Total flight path " << (l1 + l2) << "m, tth "
                << (tth * 180. / M_PI) << "deg, DIFC " << difc << "\n";
    }
    tmpbuffer << "# Data for spectrum :" << histoIndex << "\n";

    // Determine bank number into GSAS file
    int bankid;
    if (m_useSpecAsBank) {
      bankid =
          static_cast<int>(inputWS->getSpectrum(histoIndex).getSpectrumNo());
    } else {
      bankid = basebanknumber + histoIndex;
    }

    // Write data
    if (RALF.compare(outputFormat) == 0) {
      this->writeRALFdata(bankid, multiplybybinwidth, tmpbuffer,
                          inputWS->histogram(histoIndex));
    } else if (SLOG.compare(outputFormat) == 0) {
      this->writeSLOGdata(bankid, multiplybybinwidth, tmpbuffer,
                          inputWS->histogram(histoIndex));
    } else {
      throw std::runtime_error("Cannot write to the unknown " + outputFormat +
                               "output format");
    }

    // Write out to file if necessary
    if (split) {
      // Create a new file name and a new file with ofstream
      std::ofstream out;
      out.open(splitfilename.c_str(), mode);
      out.write(tmpbuffer.str().c_str(), tmpbuffer.str().size());
      out.close();
    } else {
      outbuffer << tmpbuffer.str();
    }

    p.report();

  } // ENDFOR (histoIndex)

  // Write file
  if (!split) {
    std::ofstream out;
    out.open(outfilename.c_str(), mode);
    out.write(outbuffer.str().c_str(), outbuffer.str().length());
    out.close();
  }
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

/** Write value from a RunInfo property (i.e., log) to a stream
    */
void writeLogValue(std::ostream &os, const Run &runinfo,
                   const std::string &name,
                   const std::string &defValue = "UNKNOWN") {
  // Return without property exists
  if (!runinfo.hasProperty(name)) {
    os << defValue;
    return;
  }

  // Get handler of property
  Kernel::Property *prop = runinfo.getProperty(name);

  // Return without a valid pointer to property
  if (prop == nullptr) {
    os << defValue;
    return;
  }

  // Get value
  Kernel::TimeSeriesProperty<double> *log =
      dynamic_cast<Kernel::TimeSeriesProperty<double> *>(prop);
  if (log) {
    // Time series to get mean
    os << log->getStatistics().mean;
  } else {
    // None time series
    os << prop->value();
  }

  // Unit
  std::string units = prop->units();
  if (!units.empty())
    os << " " << units;
}

//--------------------------------------------------------------------------------------------
/** Write the header information, which is independent of bank, from the given
 * workspace
   * @param format :: The string containing the header formatting
   * @param os :: The stream to use to write the information
   * @param primaryflightpath :: Value for the moderator to sample distance
   */
void SaveGSS::writeHeaders(const std::string &format, std::stringstream &os,
                           double primaryflightpath) const {
  const Run &runinfo = inputWS->run();
  std::ios::fmtflags fflags(os.flags());

  // Run number
  if (format.compare(SLOG) == 0) {
    os << "Sample Run: ";
    writeLogValue(os, runinfo, "run_number");
    os << " Vanadium Run: ";
    writeLogValue(os, runinfo, "van_number");
    os << " Wavelength: ";
    writeLogValue(os, runinfo, "LambdaRequest");
    os << "\n";
  }

  if (this->getProperty("ExtendedHeader")) {
    // the instrument parameter file
    if (runinfo.hasProperty("iparm_file")) {
      Kernel::Property *prop = runinfo.getProperty("iparm_file");
      if (prop != nullptr && (!prop->value().empty())) {
        std::stringstream line;
        line << "#Instrument parameter file: " << prop->value();
        os << std::setw(80) << std::left << line.str() << "\n";
      }
    }

    // write out the gsas monitor counts
    os << "Monitor: ";
    if (runinfo.hasProperty("gsas_monitor")) {
      writeLogValue(os, runinfo, "gsas_monitor");
    } else {
      writeLogValue(os, runinfo, "gd_prtn_chrg", "1");
    }
    os << "\n";
  }

  if (format.compare(SLOG) == 0) {
    os << "# "; // make the next line a comment
  }
  os << inputWS->getTitle() << "\n";
  os << "# " << inputWS->getNumberHistograms() << " Histograms\n";
  os << "# File generated by Mantid:\n";
  os << "# Instrument: " << inputWS->getInstrument()->getName() << "\n";
  os << "# From workspace named : " << inputWS->getName() << "\n";
  if (getProperty("MultiplyByBinWidth"))
    os << "# with Y multiplied by the bin widths.\n";
  os << "# Primary flight path " << primaryflightpath << "m \n";
  if (format.compare(SLOG) == 0) {
    os << "# Sample Temperature: ";
    writeLogValue(os, runinfo, "SampleTemp");
    os << " Freq: ";
    writeLogValue(os, runinfo, "SpeedRequest1");
    os << " Guide: ";
    writeLogValue(os, runinfo, "guide");
    os << "\n";

    // print whether it is normalized by monitor or pcharge
    bool norm_by_current = false;
    bool norm_by_monitor = false;
    const Mantid::API::AlgorithmHistories &algohist =
        inputWS->getHistory().getAlgorithmHistories();
    for (const auto &algo : algohist) {
      if (algo->name().compare("NormaliseByCurrent") == 0)
        norm_by_current = true;
      if (algo->name().compare("NormaliseToMonitor") == 0)
        norm_by_monitor = true;
    }
    os << "#";
    if (norm_by_current)
      os << " Normalised to pCharge";
    if (norm_by_monitor)
      os << " Normalised to monitor";
    os << "\n";
  }

  os.flags(fflags);
}

/** Write a single line for bank
  */
inline void writeBankLine(std::stringstream &out, const std::string &bintype,
                          const int banknum, const size_t datasize) {
  std::ios::fmtflags fflags(out.flags());
  out << "BANK " << std::fixed << std::setprecision(0)
      << banknum // First bank should be 1 for GSAS; this can be changed
      << std::fixed << " " << datasize << std::fixed << " " << datasize
      << std::fixed << " " << bintype;
  out.flags(fflags);
}

/** Fix error if value is less than zero or infinity
  */
inline double fixErrorValue(const double value) {
  if (value <= 0. ||
      !std::isfinite(value)) // Negative errors cannot be read by GSAS
    return 0.;
  else
    return value;
}

/**
  */
void SaveGSS::writeRALFdata(const int bank, const bool MultiplyByBinWidth,
                            std::stringstream &out,
                            const HistogramData::Histogram &histo) const {

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
  writeBankLine(out, "RALF", bank, datasize);
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

//--------------------------------------------------------------------------------------------
/** Write data in SLOG format
  */
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
    msg << "While writing SLOG format: Found constant delta-T binning for bank "
        << bank;
    throw std::runtime_error(msg.str());
  }
  const double bc2 =
      0.5 * (*(xVals.rbegin()) +
             *(xVals.rbegin() + 1)); // maximum TOF (in microseconds?)
  const double bc3 = (*(xVals.begin() + 1) - bc1) / bc1; // deltaT/T

  g_log.debug() << "SaveGSS(): Min TOF = " << bc1 << '\n';

  writeBankLine(out, "SLOG", bank, datasize);
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
