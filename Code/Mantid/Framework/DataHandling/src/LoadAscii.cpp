//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/LoadAscii.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/RegisterFileLoader.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/Strings.h"
#include <fstream>

#include <boost/tokenizer.hpp>
#include <Poco/StringTokenizer.h>
// String utilities
#include <boost/algorithm/string.hpp>

namespace Mantid {
namespace DataHandling {
DECLARE_FILELOADER_ALGORITHM(LoadAscii);

using namespace Kernel;
using namespace API;

/// Empty constructor
LoadAscii::LoadAscii() : m_columnSep(), m_separatorIndex() {}

/**
 * Return the confidence with with this algorithm can load the file
 * @param descriptor A descriptor for the file
 * @returns An integer specifying the confidence level. 0 indicates it will not
 * be used
 */
int LoadAscii::confidence(Kernel::FileDescriptor &descriptor) const {
  const std::string &filePath = descriptor.filename();
  const size_t filenameLength = filePath.size();

  // Avoid some known file types that have different loaders
  int confidence(0);
  if (filePath.compare(filenameLength - 12, 12, "_runinfo.xml") == 0 ||
      filePath.compare(filenameLength - 6, 6, ".peaks") == 0 ||
      filePath.compare(filenameLength - 10, 10, ".integrate") == 0) {
    confidence = 0;
  } else if (descriptor.isAscii()) {
    confidence = 9; // Low so that others may try but not stopping version 2
  }
  return confidence;
}

//--------------------------------------------------------------------------
// Protected methods
//--------------------------------------------------------------------------
/**
* Process the header information. This implementation just skips it entirely.
* @param file :: A reference to the file stream
*/
void LoadAscii::processHeader(std::ifstream &file) const {

  // Most files will have some sort of header. If we've haven't been told how
  // many lines to
  // skip then try and guess
  int numToSkip = getProperty("SkipNumLines");
  if (numToSkip == EMPTY_INT()) {
    const int rowsToMatch(5);
    // Have a guess where the data starts. Basically say, when we have say
    // "rowsToMatch" lines of pure numbers
    // in a row then the line that started block is the top of the data
    int numCols(-1), matchingRows(0), row(0);
    std::string line;
    std::vector<double> values;
    while (getline(file, line)) {
      ++row;
      // int nchars = (int)line.length(); TODO dead code?
      boost::trim(line);
      if (this->skipLine(line)) {
        continue;
      }

      std::list<std::string> columns;
      int lineCols = this->splitIntoColumns(columns, line);
      try {
        fillInputValues(values, columns);
      } catch (boost::bad_lexical_cast &) {
        continue;
      }
      if (numCols < 0)
        numCols = lineCols;
      if (lineCols == numCols) {
        ++matchingRows;
        if (matchingRows == rowsToMatch)
          break;
      } else {
        numCols = lineCols;
        matchingRows = 1;
      }
    }
    // if the file does not have more than rowsToMatch + skipped lines, it will
    // stop
    // and raise the EndOfFile, this may cause problems for small workspaces.
    // In this case clear the flag
    if (file.eof()) {
      file.clear(file.eofbit);
    }
    // Seek the file pointer back to the start.
    // NOTE: Originally had this as finding the stream position of the data and
    // then moving the file pointer
    // back to the start of the data. This worked when a file was read on the
    // same platform it was written
    // but failed when read on a different one due to underlying differences in
    // the stream translation.
    file.seekg(0, std::ios::beg);
    // We've read the header plus the number of rowsToMatch
    numToSkip = row - rowsToMatch;
  }
  int i(0);
  std::string line;
  while (i < numToSkip && getline(file, line)) {
    ++i;
  }
  g_log.information() << "Skipped " << numToSkip
                      << " line(s) of header information()\n";
}

/**
* Reads the data from the file. It is assumed that the provided file stream has
* its position
* set such that the first call to getline will be give the first line of data
* @param file :: A reference to a file stream
* @returns A pointer to a new workspace
*/
API::Workspace_sptr LoadAscii::readData(std::ifstream &file) const {
  // Get the first line and find the number of spectra from the number of
  // columns
  std::string line;
  getline(file, line);
  boost::trim(line);

  std::list<std::string> columns;
  const int numCols = splitIntoColumns(columns, line);
  if (numCols < 2) {
    g_log.error() << "Invalid data format found in file \""
                  << getPropertyValue("Filename") << "\"\n";
    throw std::runtime_error(
        "Invalid data format. Fewer than 2 columns found.");
  }
  size_t numSpectra(0);
  bool haveErrors(false);
  bool haveXErrors(false);
  // Assume single data set with no errors
  if (numCols == 2) {
    numSpectra = numCols / 2;
  }
  // Data with errors
  else if ((numCols - 1) % 2 == 0) {
    numSpectra = (numCols - 1) / 2;
    haveErrors = true;
  }
  // Data with errors on both X and Y (4-column file)
  else if (numCols == 4) {
    numSpectra = 1;
    haveErrors = true;
    haveXErrors = true;
  } else {
    g_log.error() << "Invalid data format found in file \""
                  << getPropertyValue("Filename") << "\"\n";
    g_log.error() << "LoadAscii requires the number of columns to be an even "
                     "multiple of either 2 or 3.";
    throw std::runtime_error("Invalid data format.");
  }

  // A quick check at the number of lines won't be accurate enough as
  // potentially there
  // could be blank lines and comment lines
  int numBins(0), lineNo(0);
  std::vector<DataObjects::Histogram1D> spectra(numSpectra);
  std::vector<double> values(numCols, 0.);
  do {
    ++lineNo;
    boost::trim(line);
    if (this->skipLine(line))
      continue;
    columns.clear();
    int lineCols = this->splitIntoColumns(columns, line);
    if (lineCols != numCols) {
      std::ostringstream ostr;
      ostr << "Number of columns changed at line " << lineNo;
      throw std::runtime_error(ostr.str());
    }

    try {
      fillInputValues(values, columns); // ignores nans and replaces them with 0
    } catch (boost::bad_lexical_cast &) {
      g_log.error() << "Invalid value on line " << lineNo << " of \""
                    << getPropertyValue("Filename") << "\"\n";
      throw std::runtime_error("Invalid value encountered.");
    }

    for (size_t i = 0; i < numSpectra; ++i) {
      spectra[i].dataX().push_back(values[0]);
      spectra[i].dataY().push_back(values[i * 2 + 1]);
      if (haveErrors) {
        spectra[i].dataE().push_back(values[i * 2 + 2]);
      }
      if (haveXErrors) {
        // Note: we only have X errors with 4-column files.
        // We are only here when i=0.
        spectra[i].dataDx().push_back(values[3]);
      }
    }
    ++numBins;
  } while (getline(file, line));

  MatrixWorkspace_sptr localWorkspace =
      boost::dynamic_pointer_cast<MatrixWorkspace>(
          WorkspaceFactory::Instance().create("Workspace2D", numSpectra,
                                              numBins, numBins));
  try {
    localWorkspace->getAxis(0)->unit() =
        UnitFactory::Instance().create(getProperty("Unit"));
  } catch (Exception::NotFoundError &) {
    // Asked for dimensionless workspace (obviously not in unit factory)
  }

  for (size_t i = 0; i < numSpectra; ++i) {
    localWorkspace->dataX(i) = spectra[i].dataX();
    localWorkspace->dataY(i) = spectra[i].dataY();
    /* If Y or E errors are not there, DON'T copy across as the 'spectra'
       vectors
       have not been filled above. The workspace will by default have vectors of
       the right length filled with zeroes. */
    if (haveErrors)
      localWorkspace->dataE(i) = spectra[i].dataE();
    if (haveXErrors)
      localWorkspace->dataDx(i) = spectra[i].dataDx();
    // Just have spectrum number start at 1 and count up
    localWorkspace->getSpectrum(i)->setSpectrumNo(static_cast<specid_t>(i) + 1);
  }
  return localWorkspace;
}

/**
* Peek at a line without extracting it from the stream
*/
void LoadAscii::peekLine(std::ifstream &is, std::string &str) const {
  str = Kernel::Strings::peekLine(is);
}

/**
* Return true if the line is to be skipped.
* @param line :: The line to be checked
* @return True if the line should be skipped
*/
bool LoadAscii::skipLine(const std::string &line) const {
  return Kernel::Strings::skipLine(line);
}

/**
* Split the data into columns based on the input separator
* @param[out] columns :: A reference to a list to store the column data
* @param[in] str :: The input string
* @returns The number of columns
*/
int LoadAscii::splitIntoColumns(std::list<std::string> &columns,
                                const std::string &str) const {
  boost::split(columns, str, boost::is_any_of(m_columnSep),
               boost::token_compress_on);
  return static_cast<int>(columns.size());
}

/**
* Fill the given vector with the data values. Its size is assumed to be correct
* @param[out] values :: The data vector fill
* @param columns :: The list of strings denoting columns
*/
void LoadAscii::fillInputValues(std::vector<double> &values,
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
void LoadAscii::init() {
  std::vector<std::string> exts;
  exts.push_back(".dat");
  exts.push_back(".txt");
  exts.push_back(".csv");
  exts.push_back("");

  declareProperty(new FileProperty("Filename", "", FileProperty::Load, exts),
                  "The name of the text file to read, including its full or "
                  "relative path. The file extension must be .tst, .dat, or "
                  ".csv");
  declareProperty(new WorkspaceProperty<Workspace>("OutputWorkspace", "",
                                                   Direction::Output),
                  "The name of the workspace that will be created, filled with "
                  "the read-in data and stored in the [[Analysis Data "
                  "Service]].");

  std::string spacers[6][6] = {{"Automatic", ",\t:; "},
                               {"CSV", ","},
                               {"Tab", "\t"},
                               {"Space", " "},
                               {"Colon", ":"},
                               {"SemiColon", ";"}};
  // For the ListValidator
  std::vector<std::string> sepOptions;
  for (size_t i = 0; i < 5; ++i) {
    std::string option = spacers[i][0];
    m_separatorIndex.insert(
        std::pair<std::string, std::string>(option, spacers[i][1]));
    sepOptions.push_back(option);
  }
  declareProperty(
      "Separator", "Automatic",
      boost::make_shared<StringListValidator>(sepOptions),
      "The separator between data columns in the data file. The possible "
      "values are \"CSV\", \"Tab\", "
      "\"Space\", \"SemiColon\", or \"Colon\" (default: Automatic selection).");

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
void LoadAscii::exec() {
  std::string filename = getProperty("Filename");
  std::ifstream file(filename.c_str());
  if (!file) {
    g_log.error("Unable to open file: " + filename);
    throw Exception::FileError("Unable to open file: ", filename);
  }

  std::string sepOption = getProperty("Separator");
  m_columnSep = m_separatorIndex[sepOption];
  // Process the header information.
  processHeader(file);
  // Read the data
  MatrixWorkspace_sptr outputWS =
      boost::dynamic_pointer_cast<MatrixWorkspace>(readData(file));
  outputWS->mutableRun().addProperty("Filename", filename);
  setProperty("OutputWorkspace", outputWS);
}

} // namespace DataHandling
} // namespace Mantid
