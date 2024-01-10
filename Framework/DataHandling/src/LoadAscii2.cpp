// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/LoadAscii2.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidAPI/RegisterFileLoader.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidHistogramData/HistogramMath.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/StringTokenizer.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/VisibleWhenProperty.h"

// String utilities
#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>
#include <boost/tokenizer.hpp>

#include <fstream>

namespace Mantid::DataHandling {
DECLARE_FILELOADER_ALGORITHM(LoadAscii2)

using namespace Kernel;
using namespace API;

/// Empty constructor
LoadAscii2::LoadAscii2()
    : m_columnSep(), m_separatorIndex(), m_comment(), m_baseCols(0), m_specNo(0), m_lastBins(0), m_curBins(0),
      m_spectraStart(), m_spectrumIDcount(0), m_lineNo(0), m_spectra(), m_curSpectra(nullptr) {}

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
  if (filenameLength > 12            ? (filePath.compare(filenameLength - 12, 12, "_runinfo.xml") == 0)
      : false || filenameLength > 6  ? (filePath.compare(filenameLength - 6, 6, ".peaks") == 0)
      : false || filenameLength > 10 ? (filePath.compare(filenameLength - 10, 10, ".integrate") == 0)
                                     : false) {
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

  // if the file appears to be a table workspace then read it as such
  auto ws = readTable(file);
  if (ws) {
    return ws;
  }

  m_baseCols = 0;
  m_specNo = 0;
  m_lastBins = 0;
  m_curBins = 0;
  m_spectraStart = true;
  m_spectrumIDcount = 0;

  m_spectra.clear();
  m_curSpectra = std::make_unique<DataObjects::Histogram1D>(HistogramData::Histogram::XMode::Points,
                                                            HistogramData::Histogram::YMode::Counts);
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
    localWorkspace = WorkspaceFactory::Instance().create("Workspace2D", numSpectra, m_lastBins, m_lastBins);
  } catch (std::exception &e) {
    std::ostringstream msg;
    msg << "Failed to create a Workspace2D from the data found in this file. "
           "Error: "
        << e.what();
    throw std::runtime_error(msg.str());
  }

  try {
    writeToWorkspace(localWorkspace, numSpectra);
  } catch (std::exception &e) {
    std::ostringstream msg;
    msg << "Failed to write read data into the output Workspace2D. Error: " << e.what();
    throw std::runtime_error(msg.str());
  }
  m_curSpectra.reset();
  localWorkspace->setDistribution(setDistribution(file));
  return localWorkspace;
}

bool LoadAscii2::setDistribution(std::ifstream &file) {
  bool isDistribution = false;
  const bool distributionFlag = getProperty("ForceDistributionTrue");
  if (distributionFlag) {
    isDistribution = true;
  } else {
    std::string newLine;
    // reset to start of ifstream
    file.clear();
    file.seekg(0);
    while (std::getline(file, newLine)) {
      if (newLine.find("Distribution=true") != std::string::npos) {
        isDistribution = true;
        break;
      }
    }
  }
  return isDistribution;
}

/// Attempts to read a table workspace from the file.
/// Failing early if the format does not match.
/// @param file the file handle to load from
/// @returns a populated table workspace or an empty shared pointer
API::Workspace_sptr LoadAscii2::readTable(std::ifstream &file) {

  DataObjects::TableWorkspace_sptr ws;
  // We need to see two rows commented out
  // the first with column names
  // the second with column types
  // Then we need data, with the same number of columns as the first two lines

  try {
    size_t colNames = 0;
    size_t colTypes = 0;
    std::string line;
    std::list<std::string> names;
    std::list<std::string> types;
    std::list<std::string> data;

    while (getline(file, line)) {
      boost::trim(line);

      std::list<std::string> columns;

      if (!line.empty()) {
        // if line starts with a comment
        if (line.at(0) == m_comment.at(0)) {
          // remove the comment character
          line.erase(0, 1);
          size_t lineCols = this->splitIntoColumns(columns, line);
          if (colNames == 0) {
            colNames = lineCols;
            names = columns;
            continue;
          }
          if (colTypes == 0) {
            colTypes = lineCols;
            types = columns;
            continue;
          }
        }

        if (colTypes != colNames) {
          // no point going further, the types and names differ in quantity
          break;
        }

        size_t colData = this->splitIntoColumns(data, line);
        if (colNames > 0 && colNames == colTypes && colTypes == colData) {
          // we seem to have a table workspace
          // if we have not already created a workspace
          if (!ws) {
            ws = std::make_shared<DataObjects::TableWorkspace>();
            // create the columns
            auto itName = names.begin();
            auto itTypes = types.begin();
            for (size_t i = 0; i < colNames; i++) {
              std::string colName = *itName;
              std::string type = *itTypes;
              // trim the strings
              boost::trim(colName);
              boost::trim(type);
              ws->addColumn(type, colName);
              itName++;
              itTypes++;
            }
          }
          // add the data
          TableRow row = ws->appendRow();
          auto itTypes = types.begin();
          for (auto itData = data.begin(); itData != data.end(); itData++) {
            // direct assignment only works for strings, we ill need to handle
            // the other data types here
            std::string type = *itTypes;
            boost::trim(type);
            if (type == "str") {
              row << *itData;
            } else if (type == "int") {
              int num = boost::lexical_cast<int>(*itData);
              row << num;
            } else if (type == "uint") {
              uint32_t num = boost::lexical_cast<uint32_t>(*itData);
              row << num;
            } else if (type == "long64") {
              auto num = boost::lexical_cast<int64_t>(*itData);
              row << num;
            } else if (type == "size_t") {
              size_t num = boost::lexical_cast<size_t>(*itData);
              row << num;
            } else if (type == "float") {
              float num = boost::lexical_cast<float>(*itData);
              row << num;
            } else if (type == "double") {
              double num = boost::lexical_cast<double>(*itData);
              row << num;
            } else if (type == "bool") {
              bool val = (itData->at(0) == 't');
              row << val;
            } else if (type == "V3D") {
              V3D val;
              std::stringstream ss(*itData);
              val.readPrinted(ss);
              row << val;
            } else {
              throw std::runtime_error("unknown column data type " + type);
            }
            itTypes++;
          }
        }
      }
    }
    // if the file does not have more than rowsToCheck, it will
    // stop
    // and raise the EndOfFile, this may cause problems for small workspaces.
    // In this case clear the flag
    if (file.eof()) {
      file.clear(file.eofbit);
    }
  } catch (std::exception &ex) {
    // log and squash the error, so we can still try to load the file as a
    // matrix workspace
    g_log.warning() << "Error while trying to read ascii file as table, "
                       "continuing to load as matrix workspace.\n"
                    << ex.what() << "\n";
    // clear any workspace that we started loading
    ws.reset();
  }

  // Seek the file pointer back to the start.
  file.seekg(0, std::ios::beg);
  return ws;
}

/**
 * Check the start of the file for the first data set, then set the number of
 * columns that hsould be expected thereafter
 * @param[in] line : The current line of data
 * @param[in] columns : the columns of values in the current line of data
 */
void LoadAscii2::parseLine(const std::string &line, std::list<std::string> &columns) {
  if (std::isdigit(static_cast<unsigned char>(line.at(0))) || line.at(0) == '-' || line.at(0) == '+') {
    const int cols = splitIntoColumns(columns, line);
    if (cols > 4 || cols < 0) {
      // there were more separators than there should have been, which isn't
      // right, or something went rather wrong
      throw std::runtime_error("Line " + std::to_string(m_lineNo) +
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
        // if not then they've omitted IDs in the file previously and just
        // decided to include one (which is wrong and confuses everything)
        throw std::runtime_error("Line " + std::to_string(m_lineNo) +
                                 ": Inconsistent inclusion of spectra IDs. All spectra must have "
                                 "IDs or all spectra must not have IDs. "
                                 "Check for blank lines, as they symbolize the end of one spectra "
                                 "and the start of another. Also check for spectra IDs with no "
                                 "associated bins.");
      }
      const std::string singleNumber = columns.front();
      try {
        m_curSpectra->setSpectrumNo(boost::lexical_cast<int>(singleNumber));
      } catch (boost::bad_lexical_cast &) {
        // the single column number is not the spectrum ID, maybe it is the
        // spectrum axis value
        try {
          m_spectrumAxis.emplace_back(boost::lexical_cast<double>(singleNumber));
        } catch (boost::bad_lexical_cast &) {
          throw std::runtime_error("Unable to read as spectrum ID (int) nor as "
                                   "spectrum axis value (double)" +
                                   singleNumber);
        }
      }
    } else {
      inconsistantIDCheck();

      checkLineColumns(cols);

      addToCurrentSpectra(columns);
    }
  } else if (badLine(line)) {
    throw std::runtime_error("Line " + std::to_string(m_lineNo) +
                             ": Unexpected character found at beginning of line. Lines must either "
                             "be a single integer, a list of numeric values, blank, or a text line "
                             "beginning with the specified comment indicator: " +
                             m_comment + ".");
  } else {
    // strictly speaking this should never be hit, but just being sure
    throw std::runtime_error("Line " + std::to_string(m_lineNo) +
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
void LoadAscii2::writeToWorkspace(API::MatrixWorkspace_sptr &localWorkspace, const size_t &numSpectra) const {
  try {
    localWorkspace->getAxis(0)->unit() = UnitFactory::Instance().create(getProperty("Unit"));
  } catch (Exception::NotFoundError &) {
    // Asked for dimensionless workspace (obviously not in unit factory)
  }

  for (size_t i = 0; i < numSpectra; ++i) {
    localWorkspace->setSharedX(i, m_spectra[i].sharedX());
    localWorkspace->setSharedY(i, m_spectra[i].sharedY());
    // if E or DX are omitted they're implicitly initialised as 0
    if (m_baseCols == 4 || m_baseCols == 3) {
      // E in file
      localWorkspace->setSharedE(i, m_spectra[i].sharedE());
    }
    // DX could be NULL
    localWorkspace->setSharedDx(i, m_spectra[i].sharedDx());
    if (m_spectrumIDcount != 0) {
      localWorkspace->getSpectrum(i).setSpectrumNo(m_spectra[i].getSpectrumNo());
    } else {
      localWorkspace->getSpectrum(i).setSpectrumNo(static_cast<specnum_t>(i) + 1);
    }
    if (!m_spectrumAxis.empty()) {
      localWorkspace->replaceAxis(1, std::make_unique<NumericAxis>(m_spectrumAxis));
    }
  }
}

/**
 * Check the start of the file for the first data set, then set the number of
 * columns that should be expected thereafter
 * This will also place the file marker at the first spectrum No or data line,
 * ignoring any header information at the moment.
 * @param[in] file : The file stream
 * @param[in] line : The current line of data
 * @param[in] columns : the columns of values in the current line of data
 */
void LoadAscii2::setcolumns(std::ifstream &file, std::string &line, std::list<std::string> &columns) {
  m_lineNo = 0;
  // processheader will also look for a base number of columns, to save time
  // here if possible
  // but if the user specifies a number of lines to skip that check won't happen
  // in processheader
  processHeader(file);
  if (m_baseCols == 0 || m_baseCols > 4 || m_baseCols < 2) {
    // first find the first data set and set that as the template for the number
    // of data columns we expect from this file
    while (getline(file, line) && (m_baseCols == 0 || m_baseCols > 4 || m_baseCols < 2)) {
      // std::string line = line;
      boost::trim(line);
      if (!line.empty()) {
        if (std::isdigit(static_cast<unsigned char>(line.at(0))) || line.at(0) == '-' || line.at(0) == '+') {
          const int cols = splitIntoColumns(columns, line);
          // we might have the first set of values but there can't be more than
          // 3 commas if it is
          // int values = std::count(line.begin(), line.end(), ',');
          if (cols > 4 || cols < 1) {
            // there were more separators than there should have been, which
            // isn't right, or something went rather wrong
            throw std::runtime_error("Sets of values must have between 1 and 3 delimiters. Found " +
                                     std::to_string(cols) + ".");
          } else if (cols != 1) {
            try {
              std::vector<double> values;
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
        if (std::isdigit(static_cast<unsigned char>(line.at(0))) || line.at(0) == '-' || line.at(0) == '+') {
          std::list<std::string> columns;
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
  g_log.information() << "Skipped " << numToSkip << " line(s) of header information()\n";
}

/**
 * Check if the file has been found to inconsistently include spectra IDs
 * @param[in] columns : the columns of values in the current line of data
 */
void LoadAscii2::addToCurrentSpectra(const std::list<std::string> &columns) {
  std::vector<double> values(m_baseCols, 0.);
  m_spectraStart = false;
  fillInputValues(values, columns);
  // add X and Y
  m_curHistoX.emplace_back(values[0]);
  m_curHistoY.emplace_back(values[1]);

  // check for E and DX
  switch (m_baseCols) {
  // if only 2 columns X and Y in file, E = 0 is implicit when constructing
  // workspace, omit DX
  case 3: {
    // E in file, include it, omit DX
    m_curHistoE.emplace_back(values[2]);
    break;
  }
  case 4: {
    // E and DX in file, include both
    m_curHistoE.emplace_back(values[2]);
    m_curDx.emplace_back(values[3]);
    break;
  }
  }
  m_curBins++;
}

/**
 * Check if the file has been found to inconsistently include spectra IDs
 * @param[in] cols : the number of columns in the current line of data
 */
void LoadAscii2::checkLineColumns(const size_t &cols) const {
  // a size of 2, 3 or 4 is a valid data set, but first see if it's the same as
  // the first observed one
  if (m_baseCols != cols) {
    throw std::runtime_error("Number of data columns not consistent throughout file");
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
  if (m_spectraStart && m_spectrumIDcount != 0 && !(m_spectra.size() < m_spectrumIDcount)) {
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
      auto numXPoints = m_curHistoX.size();
      if (numXPoints > 0) {
        auto hist = m_curSpectra->histogram();
        hist.resize(numXPoints);
        auto &x = hist.mutableX();
        auto &y = hist.mutableY();

        for (size_t i = 0; i < numXPoints; ++i) {
          x[i] = m_curHistoX[i];
          y[i] = m_curHistoY[i];
        }

        if (m_baseCols > 2) {
          auto &e = hist.mutableE();
          for (size_t i = 0; i < numXPoints; ++i)
            e[i] = m_curHistoE[i];
        }
        m_curSpectra->setHistogram(hist);
      }

      size_t specSize = m_curSpectra->size();
      if (specSize > 0 && specSize == m_lastBins) {
        if (m_curSpectra->x().size() == m_curDx.size())
          m_curSpectra->setPointStandardDeviations(std::move(m_curDx));
        m_spectra.emplace_back(*m_curSpectra);
      }

      m_curSpectra.reset();
    }

    m_curSpectra = std::make_unique<DataObjects::Histogram1D>(HistogramData::Histogram::XMode::Points,
                                                              HistogramData::Histogram::YMode::Counts);
    m_curDx.clear();
    m_spectraStart = true;
    m_curHistoX.clear();
    m_curHistoY.clear();
    m_curHistoE.clear();
  }
}

/**
 * Return true if the line is to be skipped.
 * @param[in] line :: The line to be checked
 * @param[in] header :: Flag for if this is header material
 * @return True if the line should be skipped
 */
bool LoadAscii2::skipLine(const std::string &line, bool header) const {
  // Comments are skipped, Empty actually means something and shouldn't be
  // skipped
  // just checking the comment's first character should be ok as comment
  // characters can't be numeric at all, so they can't really be confused
  return ((line.empty() && header) || line.at(0) == m_comment.at(0));
}

/**
 * Return true if the line doesn't start with a valid character.
 * @param[in] line :: The line to be checked
 * @return :: True if the line doesn't start with a valid character.
 */
bool LoadAscii2::badLine(const std::string &line) const {
  // Empty or comment
  return (!(std::isdigit(static_cast<unsigned char>(line.at(0))) || line.at(0) == '-' || line.at(0) == '+') &&
          line.at(0) != m_comment.at(0));
}

/**
 * Split the data into columns based on the input separator
 * @param[out] columns :: A reference to a list to store the column data
 * @param[in] str :: The input string
 * @returns The number of columns
 */
int LoadAscii2::splitIntoColumns(std::list<std::string> &columns, const std::string &str) const {
  boost::split(columns, str, boost::is_any_of(m_columnSep), boost::token_compress_on);
  return static_cast<int>(columns.size());
}

/**
 * Fill the given vector with the data values. Its size is assumed to be correct
 * @param[out] values :: The data vector fill
 * @param[in] columns :: The list of strings denoting columns
 */
void LoadAscii2::fillInputValues(std::vector<double> &values, const std::list<std::string> &columns) const {
  values.resize(columns.size());
  int i = 0;
  for (auto value : columns) {
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
  const std::vector<std::string> exts{".dat", ".txt", ".csv", ""};
  declareProperty(std::make_unique<FileProperty>("Filename", "", FileProperty::Load, exts),
                  "The name of the text file to read, including its full or "
                  "relative path. The file extension must be .txt, .dat, or "
                  ".csv");
  declareProperty(std::make_unique<WorkspaceProperty<Workspace>>("OutputWorkspace", "", Direction::Output),
                  "The name of the workspace that will be created, "
                  "filled with the read-in data and stored in the [[Analysis "
                  "Data Service]].");

  const int numSpacers = 7;
  std::string spacers[numSpacers][2] = {
      {"Automatic", ",\t:; "},       {"CSV", ","}, {"Tab", "\t"}, {"Space", " "}, {"Colon", ":"}, {"SemiColon", ";"},
      {"UserDefined", "UserDefined"}};
  // For the ListValidator
  std::array<std::string, numSpacers> sepOptions;
  int sepOptionsIndex = 0;

  for (const auto &spacer : spacers) {
    const auto &option = spacer[0];
    m_separatorIndex.insert(std::pair<std::string, std::string>(option, spacer[1]));
    sepOptions[sepOptionsIndex++] = option;
  }

  declareProperty("Separator", "Automatic", std::make_shared<StringListValidator>(sepOptions),
                  "The separator between data columns in the data file. The "
                  "possible values are \"CSV\", \"Tab\", "
                  "\"Space\", \"SemiColon\", \"Colon\" or a user defined "
                  "value. (default: Automatic selection from comma,"
                  " tab, space, semicolon or colon.).");

  declareProperty(std::make_unique<PropertyWithValue<std::string>>("CustomSeparator", "", Direction::Input),
                  "If present, will override any specified choice given to Separator.");

  setPropertySettings("CustomSeparator",
                      std::make_unique<VisibleWhenProperty>("Separator", IS_EQUAL_TO, "UserDefined"));

  declareProperty("CommentIndicator", "#",
                  "Character(s) found front of "
                  "comment lines. Cannot contain "
                  "numeric characters");

  std::vector<std::string> units = UnitFactory::Instance().getKeys();
  units.insert(units.begin(), "Dimensionless");
  declareProperty("Unit", "Energy", std::make_shared<StringListValidator>(units),
                  "The unit to assign to the X axis (anything known to the "
                  "[[Unit Factory]] or \"Dimensionless\")");

  auto mustBePosInt = std::make_shared<BoundedValidator<int>>();
  mustBePosInt->setLower(0);
  declareProperty("SkipNumLines", EMPTY_INT(), mustBePosInt,
                  "If given, skip this number of lines at the start of the file.");
  declareProperty("ForceDistributionTrue", false,
                  "(default: false) If true, the loaded workspace is set to Distribution=true. If true, "
                  "the Distribution flag, which may be in the file header, is ignored.");
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
  if (!custom.empty()) {
    sep = custom;
  }
  // Else if the separator drop down choice is not UserDefined then we use that.
  else if (choice != "UserDefined") {
    auto it = m_separatorIndex.find(choice);
    sep = it->second;
  }
  // If we still have nothing, then we are forced to use a default.
  if (sep.empty()) {
    g_log.notice() << "\"UserDefined\" has been selected, but no custom "
                      "separator has been entered."
                      " Using default instead.\n";
    sep = ",";
  }
  m_columnSep = sep;

  // e + and - are included as they're part of the scientific notation
  if (!boost::regex_match(m_columnSep.begin(), m_columnSep.end(), boost::regex("[^0-9e+-]+", boost::regex::perl))) {
    throw std::invalid_argument("Separators cannot contain numeric characters, "
                                "plus signs, hyphens or 'e'");
  }

  std::string tempcomment = getProperty("CommentIndicator");

  if (!boost::regex_match(tempcomment.begin(), tempcomment.end(),
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
  } catch (std::exception &e) {
    g_log.error() << "Failed to read as ASCII this file: '" << filename << ", error description: " << e.what() << '\n';
    throw std::runtime_error("Failed to recognize this file as an ASCII file, "
                             "cannot continue.");
  }
  MatrixWorkspace_sptr outputWS = std::dynamic_pointer_cast<MatrixWorkspace>(rd);
  if (outputWS) {
    outputWS->mutableRun().addProperty("Filename", filename);
  }
  setProperty("OutputWorkspace", rd);
}
} // namespace Mantid::DataHandling
