#include "MantidDataHandling/LoadPDFgetNFile.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/RegisterFileLoader.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidKernel/Unit.h"
#include "MantidKernel/UnitFactory.h"

#include <fstream>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/trim.hpp>

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace std;
using namespace boost;

// FIXME  Add label and unit to output workspace
// FIXME  Consider to output multiple workspaces if there are multiple column
// data (X, Y1, E1, Y2, E2)

namespace Mantid {
namespace DataHandling {

DECLARE_FILELOADER_ALGORITHM(LoadPDFgetNFile);

//----------------------------------------------------------------------------------------------
/** Constructor
 */
LoadPDFgetNFile::LoadPDFgetNFile() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
LoadPDFgetNFile::~LoadPDFgetNFile() {}

//----------------------------------------------------------------------------------------------
/**
 * Return the confidence with with this algorithm can load the file
 * @param descriptor A descriptor for the file
 * @returns An integer specifying the confidence level. 0 indicates it will not
 * be used
 */
int LoadPDFgetNFile::confidence(Kernel::FileDescriptor &descriptor) const {
  // check the file extension
  const std::string &extn = descriptor.extension();
  // Only allow known file extensions
  if (extn.compare("sq") != 0 && extn.compare("sqa") != 0 &&
      extn.compare("sqb") != 0 && extn.compare("gr") != 0 &&
      extn.compare("ain") != 0 && extn.compare("braw") != 0 &&
      extn.compare("bsmo") != 0) {
    return 0;
  }

  if (!descriptor.isAscii())
    return 0;

  auto &file = descriptor.data();
  std::string str;
  std::getline(file, str); // workspace title first line
  while (!file.eof()) {
    std::getline(file, str);
    if (startsWith(str, "#L")) {
      return 80;
    }
  }

  return 0;
}

//----------------------------------------------------------------------------------------------
/** Define input
  */
void LoadPDFgetNFile::init() {
  std::vector<std::string> exts;
  exts.push_back(".sq");
  exts.push_back(".sqa");
  exts.push_back(".sqb");
  exts.push_back(".gr");
  exts.push_back(".ain");
  exts.push_back(".braw");
  exts.push_back(".bsmo");

  auto fileproperty = new FileProperty("Filename", "", FileProperty::Load, exts,
                                       Kernel::Direction::Input);
  this->declareProperty(fileproperty, "The input filename of the stored data");

  // auto wsproperty = new WorkspaceProperty<Workspace2D>("OutputWorkspace",
  // "Anonymous", Kernel::Direction::Output);
  // this->declareProperty(wsproperty, "Name of output workspace. ");
  declareProperty(new API::WorkspaceProperty<>("OutputWorkspace", "",
                                               Kernel::Direction::Output),
                  "Workspace name to load into.");
}

//----------------------------------------------------------------------------------------------
/** Main executor
  */
void LoadPDFgetNFile::exec() {
  // 1. Parse input file
  std::string inpfilename = getProperty("Filename");

  parseDataFile(inpfilename);

  // 2. Generate output workspace
  generateDataWorkspace();

  setProperty("OutputWorkspace", outWS);

  return;
}

//----------------------------------------------------------------------------------------------
/** Parse PDFgetN data file to
  * 1. a 2D vector for column data
  * 2. a 1D string vector for column name
  */
void LoadPDFgetNFile::parseDataFile(std::string filename) {
  // 1. Open file
  std::ifstream ifile;
  ifile.open((filename.c_str()));

  if (!ifile.is_open()) {
    stringstream errmsg;
    errmsg << "Unable to open file " << filename << ".  Quit!";
    g_log.error() << errmsg.str() << std::endl;
    throw std::runtime_error(errmsg.str());
  } else {
    g_log.notice() << "Open PDFgetN File " << filename << std::endl;
  }

  // 2. Parse
  bool readdata = false;

  char line[256];
  while (ifile.getline(line, 256)) {
    string sline(line);

    if (!readdata && startsWith(sline, "#L")) {
      // a) Find header line for the data segment in the file
      parseColumnNameLine(sline);
      readdata = true;

      // Set up the data structure
      size_t numcols = mColumnNames.size();
      for (size_t i = 0; i < numcols; ++i) {
        std::vector<double> tempvec;
        mData.push_back(tempvec);
      }

    } else if (readdata) {
      // b) Parse data
      parseDataLine(sline);
    } else {
      // c) Do nothing
      ;
    }
  } // ENDWHILE

  if (!readdata) {
    stringstream errmsg;
    errmsg << "Unable to find a line staring with #L as the indicator of data "
              "segment. ";
    g_log.error() << errmsg.str() << std::endl;
    throw std::runtime_error(errmsg.str());
  }

  return;
}

//----------------------------------------------------------------------------------------------
/** Check whether the line starts with some specific character
  */
bool LoadPDFgetNFile::startsWith(const std::string &s,
                                 const std::string &header) const {
  bool answer = true;

  if (s.size() < header.size()) {
    answer = false;
  } else {
    size_t numchars = header.size();
    for (size_t i = 0; i < numchars; ++i) {
      char c0 = s[i];
      char c1 = header[i];
      if (c0 != c1) {
        answer = false;
        break;
      }
    }
  }

  return answer;
}

/** Parse column name line staring with \#L
  */
void LoadPDFgetNFile::parseColumnNameLine(std::string line) {
  // 1. Split
  vector<string> terms;
  boost::split(terms, line, is_any_of(" \t\n"), token_compress_on);

  // 2. Validate
  if (terms.empty()) {
    throw std::runtime_error("There is nothing in the input line!");
  }

  string header = terms[0];
  if (header.compare("#L") != 0) {
    stringstream errmsg;
    errmsg << "Expecting header as #L.  Input line has header as " << header
           << ". Unable to proceed. ";
    g_log.error() << errmsg.str() << endl;
    throw std::runtime_error(errmsg.str());
  }

  // 3. Parse
  size_t numcols = terms.size() - 1;
  stringstream msgss;
  msgss << "Column Names: ";
  for (size_t i = 0; i < numcols; ++i) {
    this->mColumnNames.push_back(terms[i + 1]);
    msgss << setw(-3) << i << ": " << setw(-10) << mColumnNames[i];
  }
  g_log.information() << msgss.str() << endl;

  return;
}

/** Parse data line
  */
void LoadPDFgetNFile::parseDataLine(string line) {
  // 1. Trim (stripg) and Split
  boost::trim(line);
  vector<string> terms;
  boost::split(terms, line, is_any_of(" \t\n"), token_compress_on);

  // 2. Validate
  size_t numcols = mData.size();
  if (line[0] == '#') {
    // Comment/information line to indicate the start of another section of data
    return;
  } else if (terms.size() != numcols) {
    // Data line with incorrect number of columns
    stringstream warnss;
    warnss << "Line (" << line
           << ") has incorrect number of columns other than " << numcols
           << " as expected. ";
    g_log.warning(warnss.str());
    return;
  }

  // 3. Parse
  for (size_t i = 0; i < numcols; ++i) {
    string temps = terms[i];
    double tempvalue;
    if (temps.compare("NaN") == 0) {
      // FIXME:  Need to discuss with Peter about how to treat NaN value
      // tempvalue = DBL_MAX-1.0;
      tempvalue = 0.0;
    } else if (temps.compare("-NaN") == 0) {
      // tempvalue = -DBL_MAX+1.0;
      // FIXME:  Need to discuss with Peter about how to treat NaN value
      tempvalue = 0.0;
    } else {
      tempvalue = atof(temps.c_str());
    }

    mData[i].push_back(tempvalue);
  }

  return;
}

//----------------------------------------------------------------------------------------------
void LoadPDFgetNFile::setUnit(Workspace2D_sptr ws) {
  // 1. Set X
  string xcolname = mColumnNames[0];

  if (xcolname.compare("Q") == 0) {
    string unit = "MomentumTransfer";
    ws->getAxis(0)->setUnit(unit);
  } else if (xcolname.compare("r") == 0) {
    ws->getAxis(0)->unit() = UnitFactory::Instance().create("Label");
    Unit_sptr unit = ws->getAxis(0)->unit();
    boost::shared_ptr<Units::Label> label =
        boost::dynamic_pointer_cast<Units::Label>(unit);
    label->setLabel("AtomicDistance", "Angstrom");
  } else {
    stringstream errss;
    errss << "X axis " << xcolname << " is not supported for unit. " << endl;
    g_log.warning() << errss.str() << endl;
  }

  // 2. Set Y
  string ycolname = mColumnNames[1];
  string ylabel("");
  if (ycolname.compare("G(r)") == 0) {
    ylabel = "PDF";
  } else if (ycolname.compare("S") == 0) {
    ylabel = "S";
  } else {
    ylabel = "Intensity";
  }
  ws->setYUnitLabel(ylabel);

  return;
}

/** Generate output data workspace
  * Assumption.  One data set must contain more than 1 element.
  */
void LoadPDFgetNFile::generateDataWorkspace() {
  // 0. Check
  if (mData.size() == 0) {
    throw runtime_error("Data set has not been initialized. Quit!");
  }

  // 1. Figure out direction of X and number of data set
  bool xascend = true;
  if (mData.size() >= 2 && mData[0][1] < mData[0][0]) {
    xascend = false;
  }

  size_t numsets = 0;
  vector<size_t> numptsvec;
  size_t arraysize = mData[0].size();
  if (arraysize <= 1) {
    throw runtime_error("Number of columns in data is less and equal to 1.  It "
                        "is unphysically too small.");
  }

  double prex = mData[0][0];
  size_t vecsize = 1;
  for (size_t i = 1; i < arraysize; ++i) {
    double curx = mData[0][i];
    if (((xascend) && (curx < prex)) || ((!xascend) && (curx > prex))) {
      // X in ascending order and hit the end of one set of data
      // X in descending order and hit the end of one set of data
      // Record the current data set information and start the next data set
      numsets += 1;
      numptsvec.push_back(vecsize);
      vecsize = 1;
    } else {
      // In the middle of a set of data
      ++vecsize;
    }
    // Loop variable udpate
    prex = curx;
  } // ENDFOR
  // Record the last data set information
  ++numsets;
  numptsvec.push_back(vecsize);

  bool samesize = true;
  for (size_t i = 0; i < numsets; ++i) {
    if (i > 0) {
      if (numptsvec[i] != numptsvec[i - 1]) {
        samesize = false;
      }
    }
    g_log.information() << "Set " << i
                        << ":  Number of Points = " << numptsvec[i]
                        << std::endl;
  }
  if (!samesize) {
    stringstream errmsg;
    errmsg << "Multiple bank (number of banks = " << numsets
           << ") have different size of data array.  Unable to handle this "
              "situation.";
    g_log.error() << errmsg.str() << std::endl;
    throw std::runtime_error(errmsg.str());
  }
  size_t size = numptsvec[0];

  // 2. Generate workspace2D object and set the unit
  outWS = boost::dynamic_pointer_cast<Workspace2D>(
      API::WorkspaceFactory::Instance().create("Workspace2D", numsets, size,
                                               size));

  setUnit(outWS);

  // 3. Set number
  size_t numspec = outWS->getNumberHistograms();
  for (size_t i = 0; i < numspec; ++i) {
    MantidVec &X = outWS->dataX(i);
    MantidVec &Y = outWS->dataY(i);
    MantidVec &E = outWS->dataE(i);

    size_t baseindex = i * size;
    for (size_t j = 0; j < size; ++j) {
      size_t index;
      if (xascend)
        index = j;
      else
        index = (size - 1) - j;

      X[index] = mData[0][baseindex + j];
      Y[index] = mData[1][baseindex + j];
      E[index] = mData[2][baseindex + j];
    }
  }

  return;
}

} // namespace DataHandling
} // namespace Mantid
