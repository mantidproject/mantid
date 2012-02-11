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
    /// Parses a string consisting of only run number info, into a vector of vector of run numbers.
    MANTID_KERNEL_DLL std::vector<std::vector<unsigned int> > parseMultiRunString(std::string runString);

    /// Regexs used to match / parse various strings.
    namespace Regexs
    {
      extern const std::string INST, UNDERSCORE, SPACE;
      extern const std::string COMMA, PLUS, MINUS, COLON;
      extern const std::string SINGLE, RANGE, STEP_RANGE, ADD_LIST, ADD_RANGE, ADD_STEP_RANGE;
      extern const std::string ANY, LIST;
    }

    /**
      This class takes a string representing multiple files and parses it into
      a vector of vectors of file names.  Filenames to be added are placed in the
      same sub vectors.

      The string to parse should be of the format [dir][inst][under][runs][ext], where:

      [dir]   (Optional) = The OS-specific file directory, e.g. "c:\data\"
      [inst]  (Required) = The instrument name, e.g. "IRS" or "PG3"
      [under] (Optional) = An underscore.
      [runs]  (Required) = The run numbers, e.g. "0102, 0110-0115, 0120, 0130:0140:2"
      [ext]   (Optional) = The file extension, e.g. ".raw"
    */
    class MANTID_KERNEL_DLL Parser
    {
    public:
      /// Constructor
      Parser();
      /// Destructor
      ~Parser();

      /// Parse the given multiFileNameString.
      void parse(const std::string & multiFileName);
      
      /// Return the vector of vectors of parsed file names.
      std::vector<std::vector<unsigned int> > runs() {return m_runs;}
      /// Return the vector of vectors of parsed file names.
      std::vector<std::vector<std::string> > fileNames() {return m_fileNames;}
      /// Return the parsed directory string.
      std::string dirString() {return m_dirString;}
      /// Return the parsed instrument string.
      std::string instString() {return m_instString;}
      /// Return the parsed underscore string.
      std::string underscoreString() {return m_underscoreString;}
      /// Return the parsed run string.
      std::string runString() {return m_runString;}
      /// Return the parsed extension string.
      std::string extString() {return m_extString;}

    private:
      /// Clear all member variables.
      void clear();
      /// Split the string to parse into its component parts.
      void split();
      
      /// A vector of vectors of the parsed runs.
      std::vector<std::vector<unsigned int> > m_runs;
      /// A vector of vectors of the parsed file names.
      std::vector<std::vector<std::string> > m_fileNames;
      /// The given string to parse.
      std::string m_multiFileName;
      /// The various sections of the given string to parse.
      std::string m_dirString, m_instString, m_underscoreString, m_runString, m_extString;
      /// The instrument-specific run zero padding value.
      int m_zeroPadding;
    };

    /**
      A functor that generates a vector of file names from the given vector of runs, and other state
      passed to it when constructed.
     */
    class MANTID_KERNEL_DLL GenerateFileName
    {
    public:
      /// Constructor.
      GenerateFileName(const std::string & prefix, const std::string & suffix, int zeroPadding);

      /// Overloaded function operator that generates a vector of file names from a vector of runs.
      std::vector<std::string> operator()(const std::vector<unsigned int> & runs);
      /// Overloaded function operator that generates a file name from a run.
      std::string operator()(unsigned int run);

    private:
      /// String that prefixes any generated file names.
      std::string m_prefix;
      /// String that suffixes any generated file names.
      std::string m_suffix;
      /// The length of zero padding needed.
      int m_zeroPadding;
    };
    
    /**
     * A convenience function for the cases where we dont use the multi file parser to
     * *add* files - only to list them.  It "flattens" the given vector of vectors
     * into a single vector which is much easier to traverse.  For example a vector of vector of runs:
     *
     * ((1), (2), (30), (31), (32), (100), (102)) becomes (1, 2, 30, 31, 32, 100, 102)
     *
     * Used on a vector of vectors that *has* added filenames, the following behaviour is observed:
     *
     * ((1), (2), (30, 31, 32), (100), (102)) becomes (1, 2, 30, 31, 32, 100, 102)
     *
     * @param vecOfVecs :: a vector of vectors.
     * @return a single vector with the entire contents of vecOfVecs.
     */
    /*template<typename TYPE>
    std::vector<TYPE> flatten(const std::vector<std::vector<TYPE> > & vecOfVecs)
    {
      std::vector<TYPE> flatVector;
      std::vector<std::vector<TYPE> >::const_iterator it = vecOfVecs.begin();

      for(; it != vecOfVecs.end(); ++it)
      {
        flatVector.insert(
          flatVector.end(),
          it->begin(), it->end());
      }

      return flatVector;
    }*/

  } // namespace MultiFileNameParsing

} // namespace Kernel
} // namespace Mantid

#endif
