// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
//---------------------------------------------------
// Includes
//---------------------------------------------------
#include "MantidDataHandling/LoadRKH.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidAPI/RegisterFileLoader.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataHandling/SaveRKH.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/VectorHelper.h"
#include "MantidKernel/cow_ptr.h"

// clang-format off
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/date_parsing.hpp>
// clang-format on
#include "MantidKernel/StringTokenizer.h"
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

#include <boost/regex.hpp>
#include <istream>
#include <numeric>

namespace {
// Check if we are dealing with a unit line
bool isUnit(const Mantid::Kernel::StringTokenizer &codes) {
  // The unit line needs to have the format of
  //  1. Either 0 or 6 [06]
  //  2. Then several characters [\w]
  //  3. Open bracket
  //  4. Several characters
  //  5. Close bracket
  std::string input = std::accumulate(codes.begin(), codes.end(), std::string(""));
  std::string reg(R"(^[06][\w]+\([/ \w\^-]+\)$)");
  boost::regex baseRegex(reg);
  return boost::regex_match(input, baseRegex);
}
} // namespace

namespace Mantid::DataHandling {
using namespace Mantid::API;
using namespace Mantid::Kernel;

DECLARE_FILELOADER_ALGORITHM(LoadRKH)

/**
 * Read data from a RKH1D file
 *
 * @param stream :: the input stream to read from
 * @param readStart :: the line to start reading from
 * @param readEnd :: the line to stop reading at
 * @param x :: histogram data points to fill
 * @param y :: histogram data counts to fill
 * @param ye :: histogram data count standard deviations to fill
 * @param xe :: histogram data point standard deviations to fill
 * @param prog :: handle to progress bar
 * @param readXError :: whether to read x errors (optional, default: false)
 */
void LoadRKH::readLinesForRKH1D(std::istream &stream, int readStart, int readEnd, HistogramData::Points &x,
                                HistogramData::Counts &y, HistogramData::CountStandardDeviations &ye,
                                HistogramData::PointStandardDeviations &xe, Progress &prog, bool readXError) {

  std::vector<double> xData;
  std::vector<double> yData;
  std::vector<double> xError;
  std::vector<double> yError;

  xData.reserve(readEnd);
  yData.reserve(readEnd);
  xError.reserve(readEnd);
  yError.reserve(readEnd);

  std::string fileline;
  for (int index = 1; index <= readEnd; ++index) {
    getline(stream, fileline);
    if (index < readStart)
      continue;

    double xValue(0.), yValue(0.), yErrorValue(0.);
    std::istringstream datastr(fileline);
    datastr >> xValue >> yValue >> yErrorValue;

    xData.emplace_back(xValue);
    yData.emplace_back(yValue);
    yError.emplace_back(yErrorValue);

    // check if we need to read in x error values
    if (readXError) {
      double xErrorValue(0.);
      datastr >> xErrorValue;
      xError.emplace_back(xErrorValue);
    }

    prog.report();
  }

  x = xData;
  y = yData;
  ye = yError;
  xe = xError;
}

/**
 * Return the confidence with with this algorithm can load the file
 * @param descriptor A descriptor for the file
 * @returns An integer specifying the confidence level. 0 indicates it will not
 * be used
 */
int LoadRKH::confidence(Kernel::FileDescriptor &descriptor) const {
  if (!descriptor.isAscii())
    return 0;

  auto &file = descriptor.data();
  std::string fileline;

  // Header looks something like this where the text inside [] could be anything
  //  LOQ Thu 28-OCT-2004 12:23 [W 26  INST_DIRECT_BEAM]

  // -- First line --
  std::getline(file, fileline);
  // LOQ or SANS2D (case insensitive)
  if (boost::ifind_first(fileline, "loq").empty() && boost::ifind_first(fileline, "sans2d").empty())
    return 0;

  // Next should be date time string
  static const char *MONTHS[12] = {"-JAN-", "-FEB-", "-MAR-", "-APR-", "-MAY-", "-JUN-",
                                   "-JUL-", "-AUG-", "-SEP-", "-OCT-", "-NOV-", "-DEC-"};

  bool foundMonth(false);
  for (auto &month : MONTHS) {
    if (!boost::ifind_first(fileline, month).empty()) {
      foundMonth = true;
      break;
    }
  }
  if (!foundMonth)
    return 0;

  // there are no constraints on the second line
  std::getline(file, fileline);

  // read 3rd line - should contain sequence "0    0    0    1"
  std::getline(file, fileline);
  if (fileline.find("0    0    0    1") == std::string::npos)
    return 0;

  // read 4th line - should contain sequence ""0         0         0         0"
  std::getline(file, fileline);
  if (fileline.find("0         0         0         0") == std::string::npos)
    return 0;

  // read 5th line - should contain sequence "3 (F12.5,2E16.6)"
  std::getline(file, fileline);
  if (fileline.find("3 (F12.5,2E16.6)") == std::string::npos)
    return 0;

  return 20; // Better than LoadAscii
}

/**
 * Initialise the algorithm
 */
void LoadRKH::init() {
  const std::vector<std::string> exts{".txt", ".q", ".dat"};
  declareProperty(std::make_unique<API::FileProperty>("Filename", "", API::FileProperty::Load, exts),
                  "Name of the RKH file to load");
  declareProperty(std::make_unique<API::WorkspaceProperty<>>("OutputWorkspace", "", Kernel::Direction::Output),
                  "The name to use for the output workspace");
  // Get the units registered with the UnitFactory
  std::vector<std::string> propOptions = Kernel::UnitFactory::Instance().getKeys();
  m_unitKeys.insert(propOptions.begin(), propOptions.end());

  // m_RKHKeys will be taken as axis(1) units, the first axis will have only one
  // value
  // and so selection of one of these units will result in a workspace
  // orientated differently
  // from selection of the above
  m_RKHKeys.insert("SpectrumNumber");
  propOptions.insert(propOptions.end(), m_RKHKeys.begin(), m_RKHKeys.end());

  declareProperty("FirstColumnValue", "Wavelength", std::make_shared<Kernel::StringListValidator>(propOptions),
                  "Only used for 1D files, the units of the first column in the RKH "
                  "file (default Wavelength)");
}

/**
 * Execute the algorithm
 */
void LoadRKH::exec() {
  using namespace Mantid::Kernel;
  using namespace Mantid::API;

  // Retrieve filename and try to open the file
  std::string filename = getPropertyValue("Filename");

  m_fileIn.open(filename.c_str());
  if (!m_fileIn) {
    g_log.error("Unable to open file " + filename);
    throw Exception::FileError("Unable to open File: ", filename);
  }
  g_log.information() << "Opened file \"" << filename << "\" for reading\n";

  std::string line;
  // The first line contains human readable information about the original
  // workspace that we don't need
  getline(m_fileIn, line);
  getline(m_fileIn, line);

  // Use one line of the file to diagnose if it is 1D or 2D, this line contains
  // some data required by the 2D data reader
  MatrixWorkspace_sptr result = is2D(line) ? read2D(line) : read1D();

  // all RKH files contain distribution data
  result->setDistribution(true);
  // Set the output workspace
  setProperty("OutputWorkspace", result);
}

/** Determines if the file is 1D or 2D based on the first after the workspace's
 *  title
 *  @param testLine :: the first line in the file after the title
 *  @return true if the file must contain 1D data
 */
bool LoadRKH::is2D(const std::string &testLine) {
  // split the line into words
  const Mantid::Kernel::StringTokenizer codes(
      testLine, " ", Mantid::Kernel::StringTokenizer::TOK_TRIM | Mantid::Kernel::StringTokenizer::TOK_IGNORE_EMPTY);
  return isUnit(codes);
}

/** Read a data file that contains only one spectrum into a workspace
 *  @return the new workspace
 */
const API::MatrixWorkspace_sptr LoadRKH::read1D() {
  g_log.information() << "file appears to contain 1D information, reading in 1D data mode\n";

  // The 3rd line contains information regarding the number of points in the
  // file and
  // start and end reading points
  int totalPoints(0), readStart(0), readEnd(0), buried(0);
  std::string fileline;

  getline(m_fileIn, fileline);
  std::istringstream is(fileline);
  // Get data information
  for (int counter = 1; counter < 8; ++counter) {
    switch (counter) {
    case 1:
      is >> totalPoints;
      break;
    case 5:
      is >> readStart;
      break;
    case 6:
      is >> readEnd;
      break;
    default:
      is >> buried;
      break;
    }
  }

  g_log.information() << "Total number of data points declared to be in the data file: " << totalPoints << "\n";

  // What are we reading?
  std::string firstColVal = getProperty("FirstColumnValue");
  bool colIsUnit(true);
  if (m_RKHKeys.find(firstColVal) != m_RKHKeys.end()) {
    colIsUnit = false;
    readStart = 1;
    readEnd = totalPoints;
  }

  if (readStart < 1 || readEnd < 1 || readEnd < readStart || readStart > totalPoints || readEnd > totalPoints) {
    g_log.error("Invalid data range specfied.");
    m_fileIn.close();
    throw std::invalid_argument("Invalid data range specfied.");
  }

  g_log.information() << "Reading started on data line: " << readStart << "\n";
  g_log.information() << "Reading finished on data line: " << readEnd << "\n";

  // The 4th and 5th line do not contain useful information either
  skipLines(m_fileIn, 2);

  int pointsToRead = readEnd - readStart + 1;
  // Now stream sits at the first line of data
  HistogramData::Points columnOne;
  HistogramData::Counts ydata;
  HistogramData::PointStandardDeviations xError;
  HistogramData::CountStandardDeviations errdata;

  auto hasXError = hasXerror(m_fileIn);

  Progress prog(this, 0.0, 1.0, readEnd);

  readLinesForRKH1D(m_fileIn, readStart, readEnd, columnOne, ydata, errdata, xError, prog, hasXError);

  m_fileIn.close();

  assert(pointsToRead == static_cast<int>(columnOne.size()));
  assert(pointsToRead == static_cast<int>(ydata.size()));
  assert(pointsToRead == static_cast<int>(errdata.size()));

  if (hasXError) {
    assert(pointsToRead == static_cast<int>(xError.size()));
  }

  if (colIsUnit) {
    MatrixWorkspace_sptr localworkspace =
        WorkspaceFactory::Instance().create("Workspace2D", 1, pointsToRead, pointsToRead);
    localworkspace->getSpectrum(0).setDetectorID(static_cast<detid_t>(1));
    localworkspace->getAxis(0)->unit() = UnitFactory::Instance().create(firstColVal);
    localworkspace->setPoints(0, columnOne);
    localworkspace->setCounts(0, ydata);
    localworkspace->setCountStandardDeviations(0, errdata);
    if (hasXError) {
      localworkspace->setPointStandardDeviations(0, xError);
    }
    return localworkspace;
  } else {
    MatrixWorkspace_sptr localworkspace = WorkspaceFactory::Instance().create("Workspace2D", pointsToRead, 1, 1);
    // Set the appropriate values
    for (int index = 0; index < pointsToRead; ++index) {
      localworkspace->getSpectrum(index).setSpectrumNo(static_cast<int>(columnOne[index]));
      localworkspace->getSpectrum(index).setDetectorID(static_cast<detid_t>(index + 1));
      localworkspace->dataY(index)[0] = ydata[index];
      localworkspace->dataE(index)[0] = errdata[index];
    }

    if (hasXError) {
      for (int index = 0; index < pointsToRead; ++index) {
        localworkspace->setPointStandardDeviations(0, 1, xError[index]);
      }
    }
    return localworkspace;
  }
}
/** Reads from the third line of the input file to the end assuming it contains
 *  2D data
 *  @param firstLine :: the second line in the file
 *  @return a workspace containing the loaded data
 *  @throw NotFoundError if there is compulsulary data is missing from the file
 *  @throw invalid_argument if there is an inconsistency in the header
 * information
 */
const MatrixWorkspace_sptr LoadRKH::read2D(const std::string &firstLine) {
  g_log.information() << "file appears to contain 2D information, reading in 2D data mode\n";

  MatrixWorkspace_sptr outWrksp;
  MantidVec axis0Data;
  Progress prog(read2DHeader(firstLine, outWrksp, axis0Data));
  const size_t nAxis1Values = outWrksp->getNumberHistograms();

  // set the X-values to the common bin values we read above
  auto toPass = Kernel::make_cow<HistogramData::HistogramX>(axis0Data);
  for (size_t i = 0; i < nAxis1Values; ++i) {
    outWrksp->setX(i, toPass);

    // now read in the Y values
    MantidVec &YOut = outWrksp->dataY(i);
    for (double &value : YOut) {
      m_fileIn >> value;
    }
    prog.report("Loading Y data");
  } // loop on to the next spectrum

  // the error values form one big block after the Y-values
  for (size_t i = 0; i < nAxis1Values; ++i) {
    MantidVec &EOut = outWrksp->dataE(i);
    for (double &value : EOut) {
      m_fileIn >> value;
    }
    prog.report("Loading error estimates");
  } // loop on to the next spectrum

  return outWrksp;
}
/** Reads the header information from a file containing 2D data
 *  @param[in] initalLine the second line in the file
 *  @param[out] outWrksp the workspace that the data will be writen to
 *  @param[out] axis0Data x-values for the workspace
 *  @return a progress bar object
 *  @throw NotFoundError if there is compulsulary data is missing from the file
 *  @throw invalid_argument if there is an inconsistency in the header
 * information
 */
Progress LoadRKH::read2DHeader(const std::string &initalLine, MatrixWorkspace_sptr &outWrksp, MantidVec &axis0Data) {
  const std::string XUnit(readUnit(initalLine));

  std::string fileLine;
  std::getline(m_fileIn, fileLine);
  const std::string YUnit(readUnit(fileLine));

  std::getline(m_fileIn, fileLine);
  const std::string intensityUnit(readUnit(fileLine));

  // the next line should contain just "1", but I'm not enforcing that
  std::getline(m_fileIn, fileLine);
  std::string title;
  std::getline(m_fileIn, title);

  std::getline(m_fileIn, fileLine);
  boost::trim(fileLine);
  const auto nAxis0Boundaries = boost::lexical_cast<int>(fileLine);
  axis0Data.resize(nAxis0Boundaries);
  readNumEntrys(nAxis0Boundaries, axis0Data);

  std::getline(m_fileIn, fileLine);
  boost::trim(fileLine);
  int nAxis1Boundaries;
  try {
    nAxis1Boundaries = boost::lexical_cast<int>(fileLine);
  } catch (boost::bad_lexical_cast &) {
    // using readNumEntrys() above broke the sequence of getline()s and so try
    // again in case we just read the end of a line
    std::getline(m_fileIn, fileLine);
    boost::trim(fileLine);
    nAxis1Boundaries = boost::lexical_cast<int>(fileLine);
  }
  MantidVec axis1Data(nAxis1Boundaries);
  readNumEntrys(nAxis1Boundaries, axis1Data);

  std::getline(m_fileIn, fileLine);
  // check for the file pointer being left at the end of a line
  if (fileLine.size() < 5) {
    std::getline(m_fileIn, fileLine);
  }
  Mantid::Kernel::StringTokenizer wsDimensions(
      fileLine, " ", Mantid::Kernel::StringTokenizer::TOK_TRIM | Mantid::Kernel::StringTokenizer::TOK_IGNORE_EMPTY);
  if (wsDimensions.count() < 2) {
    throw Exception::NotFoundError("Input file", "dimensions");
  }
  const auto nAxis0Values = boost::lexical_cast<int>(wsDimensions[0]);
  const auto nAxis1Values = boost::lexical_cast<int>(wsDimensions[1]);

  Progress prog(this, 0.05, 1.0, 2 * nAxis1Values);

  // we now have all the data we need to create the output workspace
  outWrksp = WorkspaceFactory::Instance().create("Workspace2D", nAxis1Values, nAxis0Boundaries, nAxis0Values);
  for (int i = 0; i < nAxis1Values; ++i) {
    outWrksp->getSpectrum(i).setDetectorID(static_cast<detid_t>(i + 1));
  }
  outWrksp->getAxis(0)->unit() = UnitFactory::Instance().create(XUnit);
  outWrksp->setYUnitLabel(intensityUnit);

  auto axis1 = std::make_unique<Mantid::API::NumericAxis>(nAxis1Boundaries);
  auto axis1Raw = axis1.get();
  axis1->unit() = Mantid::Kernel::UnitFactory::Instance().create(YUnit);
  outWrksp->replaceAxis(1, std::move(axis1));
  for (int i = 0; i < nAxis1Boundaries; ++i) {
    axis1Raw->setValue(i, axis1Data[i]);
  }

  outWrksp->setTitle(title);
  // move over the next line which is there to help with loading from Fortran
  // routines
  std::getline(m_fileIn, fileLine);

  return prog;
}
/** Read the specified number of entries from input file into the
 *  the array that is passed
 *  @param[in] nEntries the number of numbers to read
 *  @param[out] output the contents of this will be replaced by the data read
 * from the file
 */
void LoadRKH::readNumEntrys(const int nEntries, MantidVec &output) {
  output.resize(nEntries);
  for (int i = 0; i < nEntries; ++i) {
    m_fileIn >> output[i];
  }
}
/** Convert the units specification line from the RKH file into a
 *  Mantid unit name
 *  @param line :: units specification line
 *  @return Mantid unit name
 */
const std::string LoadRKH::readUnit(const std::string &line) {
  // split the line into words
  const Mantid::Kernel::StringTokenizer codes(
      line, " ", Mantid::Kernel::StringTokenizer::TOK_TRIM | Mantid::Kernel::StringTokenizer::TOK_IGNORE_EMPTY);

  if (!isUnit(codes)) {
    return "C++ no unit found";
  }

  if (codes.count() < 1) {
    return "C++ no unit found";
  }

  // the symbol for the quantity q = MomentumTransfer, etc.
  const std::string symbol(codes[0]);
  // this is units used to measure the quantity e.g. angstroms, counts, ...
  auto itUnitsToken = codes.cend() - 1;
  const std::string unit(*itUnitsToken);

  // theQuantity will contain the name of the unit, which can be many words long
  std::string theQuantity;
  for (auto current = codes.cbegin() + 1; current != itUnitsToken; ++current) {
    theQuantity += *current;
  }

  // this is a syntax check the line before returning its data
  if (codes.count() >= 3) {
    // For the next line it is possible to use str.compare instead of str.find,
    // this would be more efficient if the line was very long
    // however to use is safely other checks would be required that would impair
    // readability, therefore in this case the unlikely performance hit is
    // accepted.
    if (unit.find('(') != 0 || unit.find(')') != unit.size()) {
      std::string qCode = std::to_string(SaveRKH::Q_CODE);
      if (symbol == qCode && theQuantity == "q" &&
          (unit == "(1/Angstrom)" || unit == "(Angstrom^-1)")) { // 6 q (1/Angstrom) is the synatx for
                                                                 // MomentumTransfer
        return "MomentumTransfer";
      }

      if (symbol == "0" && theQuantity != "q") { // zero means the unit is not q
                                                 // but something else, which
                                                 // I'm assuming is legal
        return theQuantity + " " + unit;
      }
    }
  }
  // the line doesn't contain a valid 2D data file unit line
  return "C++ no unit found";
}
/**
 * Remove lines from an input stream
 * @param strm :: The input stream to adjust
 * @param nlines :: The number of lines to remove
 */
void LoadRKH::skipLines(std::istream &strm, int nlines) {
  std::string buried;
  for (int i = 0; i < nlines; ++i) {
    getline(strm, buried);
  }
}
/** PAss a vector of bin boundaries and get a vector of bin centers
 *  @param[in] oldBoundaries array of bin boundaries
 *  @param[out] toCenter an array that is one shorter than oldBoundaries, the
 * values of the means of pairs of values from the input
 */
void LoadRKH::binCenter(const MantidVec &oldBoundaries, MantidVec &toCenter) const {
  VectorHelper::convertToBinCentre(oldBoundaries, toCenter);
}

/**
 * Checks if there is an x error present in the data set
 * @param stream:: the stream object
 */
bool LoadRKH::hasXerror(std::ifstream &stream) {
  auto containsXerror = false;
  auto currentPutLocation = stream.tellg();
  std::string line;
  getline(stream, line);

  std::string x, y, yerr, xerr;
  std::istringstream datastr(line);
  datastr >> x >> y >> yerr >> xerr;
  if (!xerr.empty()) {
    containsXerror = true;
  }
  // Reset the original location of the stream
  stream.seekg(currentPutLocation, stream.beg);
  return containsXerror;
}
} // namespace Mantid::DataHandling
