//---------------------------------------------------
// Includes
//---------------------------------------------------
#include "MantidDataHandling/LoadRKH.h"
#include "MantidDataHandling/SaveRKH.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidAPI/RegisterFileLoader.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/VectorHelper.h"
#include "MantidKernel/ListValidator.h"

#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/date_parsing.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <Poco/StringTokenizer.h>

#include <istream>

namespace Mantid {
namespace DataHandling {
using namespace Mantid::API;
using namespace Mantid::Kernel;

DECLARE_FILELOADER_ALGORITHM(LoadRKH)

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
  std::string fileline("");

  // Header looks something like this where the text inside [] could be anything
  //  LOQ Thu 28-OCT-2004 12:23 [W 26  INST_DIRECT_BEAM]

  // -- First line --
  std::getline(file, fileline);
  // LOQ or SANS2D (case insensitive)
  if (boost::ifind_first(fileline, "loq").empty() &&
      boost::ifind_first(fileline, "sans2d").empty())
    return 0;

  // Next should be date time string
  static const char *MONTHS[12] = {"-JAN-", "-FEB-", "-MAR-", "-APR-",
                                   "-MAY-", "-JUN-", "-JUL-", "-AUG-",
                                   "-SEP-", "-OCT-", "-NOV-", "-DEC-"};

  bool foundMonth(false);
  for (size_t i = 0; i < 12; ++i) {
    if (!boost::ifind_first(fileline, MONTHS[i]).empty()) {
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
  std::vector<std::string> exts;
  exts.push_back(".txt");
  exts.push_back(".q");
  exts.push_back(".dat");
  declareProperty(
      new API::FileProperty("Filename", "", API::FileProperty::Load, exts),
      "Name of the RKH file to load");
  declareProperty(new API::WorkspaceProperty<>("OutputWorkspace", "",
                                               Kernel::Direction::Output),
                  "The name to use for the output workspace");
  // Get the units registered with the UnitFactory
  std::vector<std::string> propOptions =
      Kernel::UnitFactory::Instance().getKeys();
  m_unitKeys.insert(propOptions.begin(), propOptions.end());

  // m_RKHKeys will be taken as axis(1) units, the first axis will have only one
  // value
  // and so selection of one of these units will result in a workspace
  // orientated differently
  // from selection of the above
  m_RKHKeys.insert("SpectrumNumber");
  propOptions.insert(propOptions.end(), m_RKHKeys.begin(), m_RKHKeys.end());

  declareProperty(
      "FirstColumnValue", "Wavelength",
      boost::make_shared<Kernel::StringListValidator>(propOptions),
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
  result->isDistribution(true);
  // Set the output workspace
  setProperty("OutputWorkspace", result);
}
/** Determines if the file is 1D or 2D based on the first after the workspace's
*  title
*  @param testLine :: the first line in the file after the title
*  @return true if the file must contain 1D data
*/
bool LoadRKH::is2D(const std::string &testLine) {
  // check the line part of a valid for 2D data else assume the file is 1D
  return readUnit(testLine) != "C++ no unit found";
}
/** Read a data file that contains only one spectrum into a workspace
*  @return the new workspace
*/
const API::MatrixWorkspace_sptr LoadRKH::read1D() {
  g_log.information()
      << "file appears to contain 1D information, reading in 1D data mode\n";

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

  g_log.information()
      << "Total number of data points declared to be in the data file: "
      << totalPoints << "\n";

  // What are we reading?
  std::string firstColVal = getProperty("FirstColumnValue");
  bool colIsUnit(true);
  if (m_RKHKeys.find(firstColVal) != m_RKHKeys.end()) {
    colIsUnit = false;
    readStart = 1;
    readEnd = totalPoints;
  }

  if (readStart < 1 || readEnd < 1 || readEnd < readStart ||
      readStart > totalPoints || readEnd > totalPoints) {
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
  fileline = "";
  std::vector<double> columnOne, ydata, errdata;
  columnOne.reserve(readEnd);
  ydata.reserve(readEnd);
  errdata.reserve(readEnd);

  Progress prog(this, 0.0, 1.0, readEnd);
  for (int index = 1; index <= readEnd; ++index) {
    getline(m_fileIn, fileline);
    if (index < readStart)
      continue;
    double x(0.), y(0.), yerr(0.);
    std::istringstream datastr(fileline);
    datastr >> x >> y >> yerr;
    columnOne.push_back(x);
    ydata.push_back(y);
    errdata.push_back(yerr);
    prog.report();
  }
  m_fileIn.close();

  assert(pointsToRead == static_cast<int>(columnOne.size()));
  assert(pointsToRead == static_cast<int>(ydata.size()));
  assert(pointsToRead == static_cast<int>(errdata.size()));

  if (colIsUnit) {
    MatrixWorkspace_sptr localworkspace = WorkspaceFactory::Instance().create(
        "Workspace2D", 1, pointsToRead, pointsToRead);
    localworkspace->getAxis(0)->unit() =
        UnitFactory::Instance().create(firstColVal);
    localworkspace->dataX(0) = columnOne;
    localworkspace->dataY(0) = ydata;
    localworkspace->dataE(0) = errdata;

    return localworkspace;
  } else {
    MatrixWorkspace_sptr localworkspace =
        WorkspaceFactory::Instance().create("Workspace2D", pointsToRead, 1, 1);
    // Set the appropriate values
    for (int index = 0; index < pointsToRead; ++index) {
      localworkspace->getSpectrum(index)
          ->setSpectrumNo(static_cast<int>(columnOne[index]));
      localworkspace->dataY(index)[0] = ydata[index];
      localworkspace->dataE(index)[0] = errdata[index];
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
  g_log.information()
      << "file appears to contain 2D information, reading in 2D data mode\n";

  MatrixWorkspace_sptr outWrksp;
  MantidVec axis0Data;
  Progress prog(read2DHeader(firstLine, outWrksp, axis0Data));
  const size_t nAxis1Values = outWrksp->getNumberHistograms();

  for (size_t i = 0; i < nAxis1Values; ++i) {
    // set the X-values to the common bin values we read above
    MantidVecPtr toPass;
    toPass.access() = axis0Data;
    outWrksp->setX(i, toPass);

    // now read in the Y values
    MantidVec &YOut = outWrksp->dataY(i);
    for (MantidVec::iterator it = YOut.begin(), end = YOut.end(); it != end;
         ++it) {
      m_fileIn >> *it;
    }
    prog.report("Loading Y data");
  } // loop on to the next spectrum

  // the error values form one big block after the Y-values
  for (size_t i = 0; i < nAxis1Values; ++i) {
    MantidVec &EOut = outWrksp->dataE(i);
    for (MantidVec::iterator it = EOut.begin(), end = EOut.end(); it != end;
         ++it) {
      m_fileIn >> *it;
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
Progress LoadRKH::read2DHeader(const std::string &initalLine,
                               MatrixWorkspace_sptr &outWrksp,
                               MantidVec &axis0Data) {
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
  const int nAxis0Boundaries = boost::lexical_cast<int>(fileLine);
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
  Poco::StringTokenizer wsDimensions(fileLine, " ",
                                     Poco::StringTokenizer::TOK_TRIM);
  if (wsDimensions.count() < 2) {
    throw Exception::NotFoundError("Input file", "dimensions");
  }
  const int nAxis0Values = boost::lexical_cast<int>(wsDimensions[0]);
  const int nAxis1Values = boost::lexical_cast<int>(wsDimensions[1]);

  Progress prog(this, 0.05, 1.0, 2 * nAxis1Values);

  // we now have all the data we need to create the output workspace
  outWrksp = WorkspaceFactory::Instance().create(
      "Workspace2D", nAxis1Values, nAxis0Boundaries, nAxis0Values);
  outWrksp->getAxis(0)->unit() = UnitFactory::Instance().create(XUnit);
  outWrksp->setYUnitLabel(intensityUnit);

  NumericAxis *const axis1 = new Mantid::API::NumericAxis(nAxis1Boundaries);
  axis1->unit() = Mantid::Kernel::UnitFactory::Instance().create(YUnit);
  outWrksp->replaceAxis(1, axis1);
  for (int i = 0; i < nAxis1Boundaries; ++i) {
    axis1->setValue(i, axis1Data[i]);
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
  const Poco::StringTokenizer codes(line, " ", Poco::StringTokenizer::TOK_TRIM);
  if (codes.count() < 1) {
    return "C++ no unit found";
  }

  // the symbol for the quantity q = MomentumTransfer, etc.
  const std::string symbol(codes[0]);
  // this is units used to measure the quantity e.g. angstroms, counts, ...
  const std::string unit(*(codes.end() - 1));

  // theQuantity will contain the name of the unit, which can be many words long
  std::string theQuantity;
  Poco::StringTokenizer::Iterator current = codes.begin() + 1,
                                  end = codes.end();
  for (; current != end; ++current) {
    if (current != end - 1) {
      theQuantity += *current;
    }
  }

  // this is a syntax check the line before returning its data
  if (codes.count() >= 3) {
    // For the next line it is possible to use str.compare instead of str.find,
    // this would be more efficient if the line was very long
    // however to use is safely other checks would be required that would impair
    // readability, therefore in this case the unlikely performance hit is
    // accepted.
    // cppcheck-suppress stlIfStrFind
    if (unit.find('(') != 0 || unit.find(')') != unit.size()) {
      std::string qCode = boost::lexical_cast<std::string>(SaveRKH::Q_CODE);
      if (symbol == qCode && theQuantity == "q" &&
          unit == "(1/Angstrom)") { // 6 q (1/Angstrom) is the synatx for
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
  std::string buried("");
  for (int i = 0; i < nlines; ++i) {
    getline(strm, buried);
  }
}
/** PAss a vector of bin boundaries and get a vector of bin centers
*  @param[in] oldBoundaries array of bin boundaries
*  @param[out] toCenter an array that is one shorter than oldBoundaries, the
* values of the means of pairs of values from the input
*/
void LoadRKH::binCenter(const MantidVec oldBoundaries,
                        MantidVec &toCenter) const {
  VectorHelper::convertToBinCentre(oldBoundaries, toCenter);
}
}
}
