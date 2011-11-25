#ifndef MANTID_KERNEL_MULTIFILENAMEPARSER_H_
#define MANTID_KERNEL_MULTIFILENAMEPARSER_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------

#include "MantidKernel/DllConfig.h"
#include "MantidKernel/UserStringParser.h"

#include <vector>
#include <string>

namespace Mantid
{
namespace Kernel
{

  /**
    This class takes a string representing multiple files and parses it into
    a "VectOfStrings2StringMap" (std::map<std::vector<std::string>, std::string>)
    which contains a map of vectors of fileNames to workspace name. 
    
    Filenames found together in the same vector are to be added - the workspace 
    name reflects this.

    The string to parse will be of the format [dir][inst][runs][ext], where:

    [dir]  (Optional) = The OS-specific file directory, e.g. "c:\data\"
    [inst] (Required) = The instrument name including any underscores, e.g. "IRS" or "PG3_".
    [runs] (Required) = The run numbers, e.g. "0102, 0110-0115, 0120, 0130:0140:2"
    [ext]  (Optional) = The file extension, e.g. ".raw"

    Copyright &copy; 2010-2011 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

    This file is part of Mantid.

    Mantid is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    Mantid is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
 */

  typedef std::map<std::vector<unsigned int>, std::string> VectOfUInt2StringMap;
  typedef std::pair<std::vector<unsigned int>, std::string> VectOfUInt2StringPair;
  typedef std::map<std::vector<std::string>, std::string> VectOfStrings2StringMap;

  class MANTID_KERNEL_DLL MultiFileNameParser
  {
  public:
    /// Constructor
    MultiFileNameParser();
    /// Destructor
    ~MultiFileNameParser();

    /// Parse the multiFileNameString.  Returns error if failed, "" if successful.
    std::string parse(const std::string & multiFileName);
    /// Returns the result of a call to parse.
    VectOfStrings2StringMap getFileNamesToWsNameMap() const;

    /// Regexs
    static const std::string INST;
    static const std::string UNDERSCORE;
    static const std::string SPACE;
    static const std::string COMMA;
    static const std::string PLUS;
    static const std::string MINUS;
    static const std::string COLON;
    static const std::string SINGLE;
    static const std::string RANGE;
    static const std::string STEP_RANGE;
    static const std::string ADD_LIST;
    static const std::string ADD_RANGE;
    static const std::string ADD_STEP_RANGE;
    static const std::string ANY;
    static const std::string LIST;

  private:
    /// Clears all member variables.
    void clear();

    /// Does an initial split of the multiFileName string.
    void split();
    /// Appends run numbers and corresponding wsNames to final map.
    void populateMap(const VectOfUInt2StringPair & pair);
    /// Creates a file name from the given run and currently parsed info 
    /// about file type, etc.
    std::string createFileName(unsigned int run);

    /// Returns the part of the given string that matches the given regex.
    static std::string getMatchingString(const std::string & regex, const std::string & toParse);
    /// Generates a wsName from the given vector of runs.
    static std::string getWorkspaceName(const std::vector<unsigned int> runs);

    std::string m_multiFileName;
    std::string m_dir, m_inst, m_runs, m_ext;

    VectOfStrings2StringMap m_fileNamesToWsNameMap;
    UserStringParser m_parser;
  };
}
}
#endif