#ifndef MANTID_KERNEL_MULTIFILENAMEPARSER_H_
#define MANTID_KERNEL_MULTIFILENAMEPARSER_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------

#include "MantidKernel/DllConfig.h"
#include "MantidKernel/UserStringParser.h"

#include <set>
#include <vector>
#include <string>
#include <map>
#include <utility>
/**
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

namespace Mantid
{
namespace Kernel
{
  namespace MultiFileNameParsing
  {
    MANTID_KERNEL_DLL std::vector<std::vector<unsigned int> > parseMultiRunString(std::string runString);

    namespace Regexs
    {
      extern const std::string INST, UNDERSCORE, SPACE;
      extern const std::string COMMA, PLUS, MINUS, COLON;
      extern const std::string SINGLE, RANGE, STEP_RANGE, ADD_LIST, ADD_RANGE, ADD_STEP_RANGE;
      extern const std::string ANY, LIST;
    }

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
    */
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
      //std::map<std::vector<std::string>, std::string> getFileNamesToWsNameMap() const;

      /// Returns a vector of all the workspace names.
      std::vector<std::string> getWsNames() const;

      /// Returns a vector of vectors of all the filenames.
      std::vector<std::vector<std::string> > getFileNames() const;

      static std::string getPathDir(const std::string & path);

    private:
      /// Clears all member variables.
      void clear();

      /// Does an initial split of the multiFileName string.
      void split();
      /// Appends run numbers and corresponding wsNames to final map.
      void populateMap(const std::pair<std::vector<unsigned int>, std::string> & pair);
      /// Creates a file name from the given run and currently parsed info 
      /// about file type, etc.
      std::string createFileName(unsigned int run);
      ///
      std::string createZeroPaddedFileName(unsigned int run);
      /// Zero pads the run number used in a file name to required length.
      static std::string pad(std::string run, int count);
      /// Returns the part of the given string that matches the given regex.
      static std::string getMatchingString(const std::string & regexString, const std::string & toParse);
      /// Generates a wsName from the given vector of runs.
      static std::string getWorkspaceName(const std::vector<unsigned int> runs);

      int m_zeroPadding;

      std::string m_multiFileName;
      std::string m_dir, m_inst, m_runs, m_ext;

      std::map<std::vector<std::string>, std::string> m_fileNamesToWsNameMap;
      UserStringParser m_parser;
    };

  }
}
}
#endif