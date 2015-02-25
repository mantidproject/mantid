//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/LoadAscii2.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/RegisterFileLoader.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/VisibleWhenProperty.h"
#include "MantidKernel/ListValidator.h"
#include <fstream>

#include <boost/tokenizer.hpp>
#include <Poco/StringTokenizer.h>
// String utilities
#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>

namespace Mantid {
namespace DataHandling {
DECLARE_FILELOADER_ALGORITHM(LoadAscii2);

using namespace Kernel;
using namespace API;

/// Empty constructor
LoadAscii2::LoadAscii2() : m_columnSep(), m_separatorIndex(), m_comment(),
  m_baseCols(0), m_specNo(0), m_lastBins(0), m_curBins(0), m_spectraStart(), 
  m_spectrumIDcount(0), m_lineNo(0), m_spectra(), m_curSpectra(NULL) {
}

/**
* Return the confidence with with this algorithm can load the file
* @param descriptor A descriptor for the file
* @returns An integer specifying the confidence level. 0 indicates it will not
* be used
*/
int LoadAscii2::confidence(Kernel::FileDescriptor &descriptor) const {
  const std::string &filePath = descriptor.filename();
  const size_t filenameLength = filePath.size();

  // Avoid some known file types that have different loaders
  int confidence(0);
  if (filePath.compare(filenameLength - 12, 12, "_runinfo.xml") == 0 ||
      filePath.compare(filenameLength - 6, 6, ".peaks") == 0 ||
      filePath.compare(filenameLength - 10, 10, ".integrate") == 0) {
    confidence = 0;
  } else if (descriptor.isAscii()) {
    confidence = 10; // Low so that others may try
  }
  return confidence;
}

//--------------------------------------------------------------------------
// Protected methods
//--------------------------------------------------------------------------

/**
* Reads the data from the file. It is assumed that the provided file stream has
* its position
* set such that the first call to getline will be give the first line of data
* @param file :: A reference to a file stream
* @returns A pointer to a new workspace
*/
API::Workspace_sptr LoadAscii2::readData(std::ifstream &file) {
  // it's probably more stirct versus version 1, but then this is a format
  // change and we don't want any bad data getting into the workspace
  // there is still flexibility, but the format should just make more sense in
  // general

  m_baseCols = 0;
  m_specNo = 0;
  m_lastBins = 0;
  m_curBins = 0;
  m_spectraStart = true;
  m_spectrumIDcount = 0;

  m_spectra.clear();
  m_curSpectra = new DataObjects::Histogram1D();
  std::string line;

  std::list<std::string> columns;

  setcolumns(file, line, columns);

  while (getline(file, line)) {
    std::string templine = line;
    m_lineNo++;
    boost::trim(templine);
    if (templine.empty()) {
      // the line is empty, treat as a break before a new spectra
      newSpectra();
    } else if (!skipLine(templine)) {
      parseLine(templine, columns);
    }
  }

  newSpectra();

  const size_t numSpectra = m_spectra.size();
  MatrixWorkspace_sptr localWorkspace;
  try {
    localWorkspace = WorkspaceFactory::Instance().create("Workspace2D",
      numSpectra, m_lastBins, m_lastBins);
  } catch(std::exception&) {
    throw std::runtime_error("Failed to create a Workspace2D from the "
                             "data found in this file");
  }

  try {
    writeToWorkspace(localWorkspace, numSpectra);
  } catch(std::exception&) {
    throw std::runtime_error("Failed to write read data into the "
                             "output Workspace2D");
  }
  delete m_curSpectra;
  return localWorkspace;
}

/**
* Check the start of the file for the first data set, then set the number of
* columns that hsould be expected thereafter
* @param[in] line : The current line of data
* @param[in] columns : the columns of values in the current line of data
*/
void LoadAscii2::parseLine(const std::string &line,
                           std::list<std::string> &columns) {
  if (std::isdigit(line.at(0)) || line.at(0) == '-' || line.at(0) == '+') {
    const int cols = splitIntoColumns(columns, line);
    if (cols > 4 || cols < 0) {
      // there were more separators than there should have been, which isn't
      // right, or something went rather wrong
      throw std::runtime_error(
          "Line " + boost::lexical_cast<std::string>(m_lineNo) +
          ": Sets of values must have between 1 and 3 delimiters");
    } else if (cols == 1) {
      // a size of 1 is a spectra ID as long as there are no alphabetic
      // characters in it. Signifies the start of a new spectra if it wasn't
      // preceeded with a blank line
      newSpectra();

      // at this point both vectors should be the same size (or the ID counter
      // should be 0, but as we're here then that's out the window),
      if (m_spectra.size() == m_spectrumIDcount) {
        m_spectrumIDcount++;
      } else {
        // if not then they've ommitted IDs in the the file previously and just
        // decided to include one (which is wrong and confuses everything)
        throw std::runtime_error(
            "Line " + boost::lexical_cast<std::string>(m_lineNo) +
            ": Inconsistent inclusion of spectra IDs. All spectra must have "
            "IDs or all spectra must not have IDs. "
            "Check for blank lines, as they symbolize the end of one spectra "
            "and the start of another. Also check for spectra IDs with no "
            "associated bins.");
      }
      m_curSpectra->setSpectrumNo(boost::lexical_cast<int>(*(columns.begin())));
    } else if (cols != 1) {
      inconsistantIDCheck();

      checkLineColumns(cols);

      addToCurrentSpectra(columns);
    }
  } else if (badLine(line)) {
    throw std::runtime_error(
        "Line " + boost::lexical_cast<std::string>(m_lineNo) +
        ": Unexpected character found at beginning of line. Lines must either "
        "be a single integer, a list of numeric values, blank, or a text line "
        "beginning with the specified comment indicator: " +
        m_comment + ".");
  } else {
    // strictly speaking this should never be hit, but just being sure
    throw std::runtime_error(
        "Line " + boost::lexical_cast<std::string>(m_lineNo) +
        ": Unknown format at line. Lines must either be a single integer, a "
        "list of numeric values, blank, or a text line beginning with the "
        "specified comment indicator: " +
        m_comment + ".");
  }
}

/**
* Construct the workspace
* @param[out] localWorkspace : the workspace beign constructed
* @param[in] numSpectra : The number of spectra found in the file
*/
void LoadAscii2::writeToWorkspace(API::MatrixWorkspace_sptr &localWorkspace,
                                  const size_t &numSpectra) const {
  try {
    localWorkspace->getAxis(0)->unit() =
        UnitFactory::Instance().create(getProperty("Unit"));
  } catch (Exception::NotFoundError &) {
    // Asked for dimensionless workspace (obviously not in unit factory)
  }

  for (size_t i = 0; i < numSpectra; ++i) {
    localWorkspace->dataX(i) = m_spectra[i].readX();
    localWorkspace->dataY(i) = m_spectra[i].readY();
    // if E or DX are ommitted they're implicitly initalised as 0
    if (m_baseCols == 4 || m_baseCols == 3) {
      // E in file
      localWorkspace->dataE(i) = m_spectra[i].readE();
    }
    if (m_baseCols == 4) {
      // DX in file
      localWorkspace->dataDx(i) = m_spectra[i].readDx();
    }
    if (m_spectrumIDcount != 0) {
      localWorkspace->getSpectrum(i)
          ->setSpectrumNo(m_spectra[i].getSpectrumNo());
    } else {
      localWorkspace->getSpectrum(i)
          ->setSpectrumNo(static_cast<specid_t>(i) + 1);
    }
  }
}

/**
* Check the start of the file for the first data set, then set the number of
* columns that should be expected thereafter
* This will also place the file marker at the first spectrum ID or data line,
* inoring any header information at the moment.
* @param[in] file : The file stream
* @param[in] line : The current line of data
* @param[in] columns : the columns of values in the current line of data
*/
void LoadAscii2::setcolumns(std::ifstream &file, std::string &line,
                            std::list<std::string> &columns) {
  m_lineNo = 0;
  std::vector<double> values;
  // processheader will also look for a base number of columns, to save time
  // here if possible
  // but if the user specifies a number of lines to skip that check won't happen
  // in processheader
  processHeader(file);
  if (m_baseCols == 0 || m_baseCols > 4 || m_baseCols < 2) {
    // first find the first data set and set that as the template for the number
    // of data columns we expect from this file
    while (getline(file, line) &&
           (m_baseCols == 0 || m_baseCols > 4 || m_baseCols < 2)) {
      // std::string line = line;
      boost::trim(line);
      if (!line.empty()) {
        if (std::isdigit(line.at(0)) || line.at(0) == '-' ||
            line.at(0) == '+') {
          const int cols = splitIntoColumns(columns, line);
          // we might have the first set of values but there can't be more than
          // 3 commas if it is
          // int values = std::count(line.begin(), line.end(), ',');
          if (cols > 4 || cols < 1) {
            // there were more separators than there should have been, which
            // isn't right, or something went rather wrong
            throw std::runtime_error(
                "Sets of values must have between 1 and 3 delimiters. Found " +
                boost::lexical_cast<std::string>(cols) + ".");
          } else if (cols != 1) {
            try {
              fillInputValues(values, columns);
            } catch (boost::bad_lexical_cast &) {
              continue;
            }
            // a size of 1 is most likely a spectra ID so ignore it, a value of
            // 2, 3 or 4 is a valid data set
            m_baseCols = cols;
          }
        }
      }
    }
    // make sure some valid data has been found to set the amount of columns,
    // and the file isn't at EOF
    if (m_baseCols > 4 || m_baseCols < 2 || file.eof()) {
      throw std::runtime_error("No valid data in file, check separator "
                               "settings or number of columns per bin.");
    }

    // start from the top again, this time filling in the list
    file.seekg(0, std::ios_base::beg);
    for (size_t i = 0; i < m_lineNo; i++) {
      getline(file, line);
    }
  }
}

/**
* Process the header information. This implementation just skips it entirely.
* @param file :: A reference to the file stream
*/
void LoadAscii2::processHeader(std::ifstream &file) {
  // Most files will have some sort of header. If we've haven't been told how
  // many lines to
  // skip then try and guess
  int numToSkip = getProperty("SkipNumLines");
  if (numToSkip == EMPTY_INT()) {
    size_t numCols = 0;
    const size_t rowsToMatch(5);
    // Have a guess where the data starts. Basically say, when we have say
    // "rowsToMatch" lines of pure numbers
    // in a row then the line that started block is the top of the data
    size_t matchingRows = 0;
    int validRows = 0;
    size_t blankRows = 0;
    int row = 0;
    std::string line;
    std::vector<double> values;
    while (getline(file, line) && matchingRows < rowsToMatch) {
      ++row;
      boost::trim(line);

      std::list<std::string> columns;
      size_t lineCols = 0;

      if (!line.empty()) {
        if (badLine(line)) {
          matchingRows = 0;
          validRows = 0;
          continue;
        }

        // a skipped line is a valid non-data line this shouldn't be counted as
        // a matching line
        // but neither should it reset the matching counter
        if (skipLine(line, true)) {
          ++validRows;
          continue;
        }
        if (std::isdigit(line.at(0)) || line.at(0) == '-' ||
            line.at(0) == '+') {
          lineCols = this->splitIntoColumns(columns, line);
          // we might have the first set of values but there can't be more than
          // 3 delimiters if it is
          if (lineCols > 4 || lineCols < 1) {
            // there were more separators than there should have been,
            // which isn't right, or something went rather wrong
            matchingRows = 0;
            validRows = 0;
            continue;
          } else if (lineCols != 1) {
            try {
              fillInputValues(values, columns);
            } catch (boost::bad_lexical_cast &) {
              matchingRows = 0;
              validRows = 0;
              continue;
            }
            // a size of 1 is most likely a spectra ID so ignore it, a value of
            // 2, 3 or 4 is a valid data set
          }
        } else {
          // line wasn't valid
          matchingRows = 0;
          validRows = 0;
          continue;
        }
      } else {
        // an empty line is legitimate but make sure there aren't too many in
        // sucession as we need to see data
        ++matchingRows;
        ++validRows;
        ++blankRows;
        if (blankRows >= rowsToMatch) {
          matchingRows = 1;
          validRows = 1;
        }
        continue;
      }

      if (numCols == 0 && lineCols != 1) {
        numCols = lineCols;
      }

      // to reduce the chance of finding problems later,
      // the concurrent data should also have the same nubmer of columns
      // if the data has a different number of columns to the previous lines
      // start the coutner again assuming those previous lines were header info
      if (lineCols == numCols || lineCols == 1) {
        // line is valid increment the counter
        ++matchingRows;
        ++validRows;
      } else {
        numCols = lineCols;
        matchingRows = 1;
        validRows = 1;
      }
    }
    // if the file does not have more than rowsToMatch + skipped lines, it will
    // stop
    // and raise the EndOfFile, this may cause problems for small workspaces.
    // In this case clear the flag
    if (file.eof()) {
      file.clear(file.eofbit);
    }

    // save some time in setcolumns as we've found the base columns
    if (numCols > 4 || numCols < 2) {
      throw std::runtime_error("No valid data in file, check separator "
                               "settings or number of columns per bin.");
    }
    m_baseCols = numCols;
    // Seek the file pointer back to the start.
    // NOTE: Originally had this as finding the stream position of the data and
    // then moving the file pointer
    // back to the start of the data. This worked when a file was read on the
    // same platform it was written
    // but failed when read on a different one due to underlying differences in
    // the stream translation.
    file.seekg(0, std::ios::beg);
    // We've read the header plus a number of validRows
    numToSkip = row - validRows;
  }
  m_lineNo = 0;
  std::string line;
  while (m_lineNo < static_cast<size_t>(numToSkip) && getline(file, line)) {
    ++m_lineNo;
  }
  g_log.information() << "Skipped " << numToSkip
                      << " line(s) of header information()\n";
}

/**
* Check if the file has been found to inconsistantly include spectra IDs
* @param[in] columns : the columns of values in the current line of data
*/
void LoadAscii2::addToCurrentSpectra(std::list<std::string> &columns) {
  std::vector<double> values(m_baseCols, 0.);
  m_spectraStart = false;
  fillInputValues(values, columns);
  // add X and Y
  m_curSpectra->dataX().push_back(values[0]);
  m_curSpectra->dataY().push_back(values[1]);
  // check for E and DX
  switch (m_baseCols) {
  // if only 2 columns X and Y in file, E = 0 is implicit when constructing
  // workspace, omit DX
  case 3: {
    // E in file, include it, omit DX
    m_curSpectra->dataE().push_back(values[2]);
    break;
  }
  case 4: {
    // E and DX in file, include both
    m_curSpectra->dataE().push_back(values[2]);
    m_curSpectra->dataDx().push_back(values[3]);
    break;
  }
  }
  m_curBins++;
}

/**
* Check if the file has been found to incosistantly include spectra IDs
* @param[in] cols : the number of columns in the current line of data
*/
void LoadAscii2::checkLineColumns(const size_t &cols) const {
  // a size of 2, 3 or 4 is a valid data set, but first see if it's the same as
  // the first observed one
  if (m_baseCols != cols) {
    throw std::runtime_error(
        "Number of data columns not consistent throughout file");
  }
}

/**
* Check if the file has been found to incosistantly include spectra IDs
*/
void LoadAscii2::inconsistantIDCheck() const {
  // we need to do a check regarding spectra ids before doing anything else
  // is this the first bin in the spectra? if not this check has already been
  // done for this spectra
  // If the ID vector is completly empty then it's ok we're assigning them later
  // if there are equal or less IDs than there are spectra, then there's been no
  // ID assigned to this spectra and there should be
  if (m_spectraStart && m_spectrumIDcount != 0 &&
      !(m_spectra.size() < m_spectrumIDcount)) {
    throw std::runtime_error("Inconsistent inclusion of spectra IDs. All "
                             "spectra must have IDs or all spectra must not "
                             "have IDs."
                             " Check for blank lines, as they symbolize the "
                             "end of one spectra and the start of another.");
  }
}

/**
* Check if the file has been found to incosistantly include spectra IDs
*/
void LoadAscii2::newSpectra() {
  if (!m_spectraStart) {
    if (m_lastBins == 0) {
      m_lastBins = m_curBins;
      m_curBins = 0;
    } else if (m_lastBins == m_curBins) {
      m_curBins = 0;
    } else {
      throw std::runtime_error("Number of bins per spectra not consistant.");
    }

    if (m_curSpectra) {
      size_t specSize = m_curSpectra->size();
      if (specSize > 0 && specSize == m_lastBins) {
        m_spectra.push_back(*m_curSpectra);
      }
      delete m_curSpectra;
    }

    m_curSpectra = new DataObjects::Histogram1D();
    m_spectraStart = true;
  }
}

/**
* Return true if the line is to be skipped.
* @param[in] line :: The line to be checked
* @param[in] header :: Flag for if this is header material
* @return True if the line should be skipped
*/
bool LoadAscii2::skipLine(const std::string &line, bool header) const {
  // Comments are skipped, Empty actually means somehting and shouldn't be
  // skipped
  // just checking the comment's first character should be ok as comment
  // cahracters can't be numeric at all, so they can't really be confused
  return ((line.empty() && header) || line.at(0) == m_comment.at(0));
}

/**
* Return true if the line doesn't start wiht a valid character.
* @param[in] line :: The line to be checked
* @return :: True if the line doesn't start with a valid character.
*/
bool LoadAscii2::badLine(const std::string &line) const {
  // Empty or comment
  return (
      !(std::isdigit(line.at(0)) || line.at(0) == '-' || line.at(0) == '+') &&
      line.at(0) != m_comment.at(0));
}

/**
* Split the data into columns based on the input separator
* @param[out] columns :: A reference to a list to store the column data
* @param[in] str :: The input string
* @returns The number of columns
*/
int LoadAscii2::splitIntoColumns(std::list<std::string> &columns,
                                 const std::string &str) const {
  boost::split(columns, str, boost::is_any_of(m_columnSep),
               boost::token_compress_on);
  return static_cast<int>(columns.size());
}

/**
* Fill the given vector with the data values. Its size is assumed to be correct
* @param[out] values :: The data vector fill
* @param[in] columns :: The list of strings denoting columns
*/
void LoadAscii2::fillInputValues(std::vector<double> &values,
                                 const std::list<std::string> &columns) const {
  values.resize(columns.size());
  std::list<std::string>::const_iterator iend = columns.end();
  int i = 0;
  for (std::list<std::string>::const_iterator itr = columns.begin();
       itr != iend; ++itr) {
    std::string value = *itr;
    boost::trim(value);
    boost::to_lower(value);
    if (value == "nan" || value == "1.#qnan") // ignores nans (not a number) and
                                              // replaces them with a nan
    {
      double nan = std::numeric_limits<double>::quiet_NaN(); //(0.0/0.0);
      values[i] = nan;
    } else {
      values[i] = boost::lexical_cast<double>(value);
    }
    ++i;
  }
}

//--------------------------------------------------------------------------
// Private methods
//--------------------------------------------------------------------------
/// Initialisation method.
void LoadAscii2::init() {
  std::vector<std::string> exts;
  exts.push_back(".dat");
  exts.push_back(".txt");
  exts.push_back(".csv");
  exts.push_back("");

  declareProperty(new FileProperty("Filename", "", FileProperty::Load, exts),
                  "The name of the text file to read, including its full or "
                  "relative path. The file extension must be .txt, .dat, or "
                  ".csv");
  declareProperty(new WorkspaceProperty<Workspace>("OutputWorkspace", "",
                                                   Direction::Output),
                  "The name of the workspace that will be created, "
                  "filled with the read-in data and stored in the [[Analysis "
                  "Data Service]].");

  std::string spacers[7][2] = {{"Automatic", ",\t:; "},
                               {"CSV", ","},
                               {"Tab", "\t"},
                               {"Space", " "},
                               {"Colon", ":"},
                               {"SemiColon", ";"},
                               {"UserDefined", "UserDefined"}};
  // For the ListValidator
  std::vector<std::string> sepOptions;
  for (size_t i = 0; i < 7; ++i) {
    std::string option = spacers[i][0];
    m_separatorIndex.insert(
        std::pair<std::string, std::string>(option, spacers[i][1]));
    sepOptions.push_back(option);
  }
  declareProperty("Separator", "Automatic",
                  boost::make_shared<StringListValidator>(sepOptions),
                  "The separator between data columns in the data file. The "
                  "possible values are \"CSV\", \"Tab\", "
                  "\"Space\", \"SemiColon\", \"Colon\" or a user defined "
                  "value. (default: Automatic selection from comma,"
                  " tab, space, semicolon or colon.).");

  declareProperty(
      new PropertyWithValue<std::string>("CustomSeparator", "",
                                         Direction::Input),
      "If present, will override any specified choice given to Separator.");

  setPropertySettings(
      "CustomSeparator",
      new VisibleWhenProperty("Separator", IS_EQUAL_TO, "UserDefined"));

  declareProperty("CommentIndicator", "#", "Character(s) found front of "
                                           "comment lines. Cannot contain "
                                           "numeric characters");

  std::vector<std::string> units = UnitFactory::Instance().getKeys();
  units.insert(units.begin(), "Dimensionless");
  declareProperty("Unit", "Energy",
                  boost::make_shared<StringListValidator>(units),
                  "The unit to assign to the X axis (anything known to the "
                  "[[Unit Factory]] or \"Dimensionless\")");

  auto mustBePosInt = boost::make_shared<BoundedValidator<int>>();
  mustBePosInt->setLower(0);
  declareProperty(
      "SkipNumLines", EMPTY_INT(), mustBePosInt,
      "If given, skip this number of lines at the start of the file.");
}

/**
*   Executes the algorithm.
*/
void LoadAscii2::exec() {
  m_lineNo = 0;
  std::string filename = getProperty("Filename");
  std::ifstream file(filename.c_str());
  if (!file) {
    g_log.error("Unable to open file: " + filename);
    throw Exception::FileError("Unable to open file: ", filename);
  }

  std::string sepOption = getProperty("Separator");
  m_columnSep = m_separatorIndex[sepOption];

  std::string choice = getPropertyValue("Separator");
  std::string custom = getPropertyValue("CustomSeparator");
  std::string sep;
  // If the custom separator property is not empty, then we use that under any
  // circumstance.
  if (custom != "") {
    sep = custom;
  }
  // Else if the separator drop down choice is not UserDefined then we use that.
  else if (choice != "UserDefined") {
    std::map<std::string, std::string>::iterator it =
        m_separatorIndex.find(choice);
    sep = it->second;
  }
  // If we still have nothing, then we are forced to use a default.
  if (sep.empty()) {
    g_log.notice() << "\"UserDefined\" has been selected, but no custom "
                      "separator has been entered."
                      " Using default instead." << std::endl;
    sep = ",";
  }
  m_columnSep = sep;

  // e + and - are included as they're part of the scientific notation
  if (!boost::regex_match(m_columnSep.begin(), m_columnSep.end(),
                          boost::regex("[^0-9e+-]+", boost::regex::perl))) {
    throw std::invalid_argument("Separators cannot contain numeric characters, "
                                "plus signs, hyphens or 'e'");
  }

  std::string tempcomment = getProperty("CommentIndicator");

  if (!boost::regex_match(
          tempcomment.begin(), tempcomment.end(),
          boost::regex("[^0-9e" + m_columnSep + "+-]+", boost::regex::perl))) {
    throw std::invalid_argument("Comment markers cannot contain numeric "
                                "characters, plus signs, hyphens,"
                                " 'e' or the selected separator character");
  }
  m_comment = tempcomment;

  // Process the header information.
  // processHeader(file);
  // Read the data
  API::Workspace_sptr rd;
  try {
    rd = readData(file);
  } catch(std::exception& e) {
    g_log.error() << "Failed to read as ASCII this file: '" << filename <<
      ", error description: " << e.what() << std::endl;
    throw std::runtime_error("Failed to recognize this file as an ASCII file, "
                             "cannot continue.");
  }
  MatrixWorkspace_sptr outputWS =
      boost::dynamic_pointer_cast<MatrixWorkspace>(rd);
  outputWS->mutableRun().addProperty("Filename", filename);
  setProperty("OutputWorkspace", outputWS);
}
} // namespace DataHandling
} // namespace Mantid
