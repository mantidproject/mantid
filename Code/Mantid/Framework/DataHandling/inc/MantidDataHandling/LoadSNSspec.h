#ifndef MANTID_DATAHANDLING_LOADSNSSPEC_H_
#define MANTID_DATAHANDLING_LOADSNSSPEC_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/IDataFileChecker.h"
namespace Mantid
{
  namespace DataHandling
  {
    /**
    Loads a workspace from an SNS spec file. Spectra must be stored in columns.

    Properties:
    <ul>
    <li>Filename  - the name of the file to read from.</li>
    <li>Workspace - the workspace name that will be created and hold the loaded data.</li>
    <li>Separator - the column separation character: comma (default),tab,space,colon,semi-colon.</li>
    <li>Unit      - the unit to assign to the X axis (default: Energy).</li>
    </ul>

    @author Jean Bilheux, ORNL
    @date 08/27/10

    Copyright &copy; 2007-2010 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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
    class DLLExport LoadSNSspec :  public API::IDataFileChecker 
    {
    public:
      LoadSNSspec();
      ~LoadSNSspec() {}
      virtual const std::string name() const { return "LoadSNSspec"; }
      virtual int version() const { return 1; }
      virtual const std::string category() const { return "DataHandling"; }

     /// do a quick check that this file can be loaded 
      virtual bool quickFileCheck(const std::string& filePath,size_t nread,const file_header& header);
      /// check the structure of the file and  return a value between 0 and 100 of how much this file can be loaded
      virtual int fileCheck(const std::string& filePath);

    private:
      void init();
      void exec();

      /// Allowed values for the cache property
      std::vector<std::string> m_seperator_options;
      std::map<std::string,const char*> m_separatormap; ///<a map of seperators
      typedef std::pair<std::string,const char*> separator_pair; ///<serparator pair type def
    };

  } // namespace DataHandling
} // namespace Mantid

#endif  /*  MANTID_DATAHANDLING_LOADSNSPEC_H_  */
