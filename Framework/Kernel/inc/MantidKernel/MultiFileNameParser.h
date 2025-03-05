// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2010 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------

#include "MantidKernel/DllConfig.h"

#include <set>
#include <string>
#include <utility>
#include <vector>

namespace Mantid {
namespace Kernel {
namespace MultiFileNameParsing {
/// Suggests a workspace name, given a vector of file names. (Which we assume
/// will be added.)
MANTID_KERNEL_DLL std::string suggestWorkspaceName(const std::vector<std::string> &fileNames);

/// Regexs used to match / parse various strings.
namespace Regexs {
extern const std::string INST, UNDERSCORE, SPACE;
extern const std::string COMMA, PLUS, MINUS, COLON;
extern const std::string SINGLE, RANGE, STEP_RANGE, ADD_LIST, ADD_RANGE, ADD_STEP_RANGE;
extern const std::string ANY, LIST;
} // namespace Regexs

/**
 * Comparator for set that holds instrument names in Parser.
 */
class MANTID_KERNEL_DLL ReverseCaselessCompare {
public:
  bool operator()(const std::string &a, const std::string &b) const;
};

/**
  This class takes a string representing multiple files and parses it into
  a vector of vectors of file names.  Filenames to be added are placed in the
  same sub vectors.

  The string to parse should be of the format [dir][inst][under][runs][ext],
  where:

  [dir]   (Optional) = The OS-specific file directory, e.g. "c:\data\"
  [inst]  (Optional) = The instrument name, e.g. "IRS" or "PG3".  If none
  provided then use default.
  [under] (Optional) = An underscore.
  [runs]  (Required) = The run numbers, e.g. "0102, 0110-0115, 0120,
  0130:0140:2"
  [ext]   (Optional) = The file extension, e.g. ".raw"

  NOTE: This parser does not parse strings of the form:
        [dir][inst][under][runs][ext],[dir][inst][under][runs][ext]
*/
class MANTID_KERNEL_DLL Parser {
public:
  /// Constructor
  Parser();
  /// Destructor
  ~Parser() = default;

  /// Parse the given multiFileNameString.
  void parse(const std::string &multiFileName);

  /// Return the vector of vectors of parsed file names.
  const std::vector<std::vector<unsigned int>> &runs() const { return m_runs; }
  /// Return the vector of vectors of parsed file names.
  const std::vector<std::vector<std::string>> &fileNames() const { return m_fileNames; }
  /// Return the parsed directory string.
  const std::string &dirString() const { return m_dirString; }
  /// Return the parsed instrument string.
  const std::string &instString() const { return m_instString; }
  /// Return the parsed underscore string.
  const std::string &underscoreString() const { return m_underscoreString; }
  /// Return the parsed run string.
  const std::string &runString() const { return m_runString; }
  /// Return the parsed extension string.
  const std::string &extString() const { return m_extString; }
  /// Parses a string consisting of only run number info, into a vector of vector
  /// of run numbers.
  std::vector<std::vector<unsigned int>> parseMultiRunString(std::string runString);
  /// Return the setting for trimming whitespaces in run string
  bool trimWhiteSpaces() const;
  /// Set the flag for trimming whitespaces in run string
  void setTrimWhiteSpaces(const bool &setting);

private:
  /// Clear all member variables.
  void clear();
  /// Split the string to parse into its component parts.
  void split();

  /// A vector of vectors of the parsed runs.
  std::vector<std::vector<unsigned int>> m_runs;
  /// A vector of vectors of the parsed file names.
  std::vector<std::vector<std::string>> m_fileNames;
  /// The given string to parse.
  std::string m_multiFileName;
  /// The various sections of the given string to parse.
  std::string m_dirString, m_instString, m_underscoreString, m_runString, m_extString;
  /// All the valid instrument names.
  std::set<std::string, ReverseCaselessCompare> m_validInstNames;
  /// Flag to determine if string input should be trimmed of whitespace
  bool m_trimWhiteSpaces;
};

/**
  A functor that generates a vector of file names from the given vector of runs,
  and other state
  passed to it when constructed.
 */
class MANTID_KERNEL_DLL GenerateFileName {
public:
  /// Constructor.
  GenerateFileName(std::string prefix, std::string suffix, std::string instString);

  /// Overloaded function operator that generates a vector of file names from a
  /// vector of runs.
  std::vector<std::string> operator()(const std::vector<unsigned int> &runs);
  /// Overloaded function operator that generates a file name from a run.
  std::string operator()(unsigned int run);

private:
  /// String that prefixes any generated file names.
  std::string m_prefix;
  /// String that suffixes any generated file names.
  std::string m_suffix;
  /// String that identifies the instrument
  std::string m_instString;
};

/**
 * A class that holds a list of ranges of runs.  Each "range" is just a pair of
 * unsigned ints.
 * Adding ranges to the list will merge them with what is already there.  This
 * is essentially
 * just a wrapper around a std::set<std::pair<unsigned int,unsigned int>>
 * object.
 */
class MANTID_KERNEL_DLL RunRangeList {
public:
  /// Constructor
  RunRangeList();

  // Returns the list of run ranges.
  const std::set<std::pair<unsigned int, unsigned int>> &rangeList() const { return m_rangeList; };

  /// Add a run to the list of run ranges.
  void addRun(const unsigned int run);
  /// Add a range of runs
  void addRunRange(const unsigned int from, const unsigned int to);
  /// Add a range of runs
  void addRunRange(const std::pair<unsigned int, unsigned int> &range);

private:
  /// A set of pairs of unsigned ints, where each pair represents a range of
  /// runs.
  std::set<std::pair<unsigned int, unsigned int>> m_rangeList;
};

} // namespace MultiFileNameParsing

} // namespace Kernel
} // namespace Mantid
