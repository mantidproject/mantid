#ifndef MANTID_DATAHANDLING_LoadPreNexus_H_
#define MANTID_DATAHANDLING_LoadPreNexus_H_

#include <vector>
#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"

namespace Mantid
{
namespace DataHandling
{

  /** LoadPreNexus : TODO: DESCRIPTION
    
    @date 2012-01-30

    Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

    File change history is stored at: <https://svn.mantidproject.org/mantid/trunk/Code/Mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
  */
  class DLLExport LoadPreNexus  :  public API::IDataFileChecker
  {
  public:
    LoadPreNexus();
    virtual ~LoadPreNexus();
    
    virtual const std::string name() const;
    virtual int version() const;
    virtual const std::string category() const;
    virtual const char * filePropertyName() const;
    bool quickFileCheck(const std::string& filePath,size_t nread,const file_header& header);
    int fileCheck(const std::string& filePath);
  private:
    virtual void initDocs();
    void init();
    void exec();
    void parseRuninfo(const std::string &runinfo, std::string &dataDir, std::vector<std::string> &eventFilenames);
    void runLoadNexusLogs(const std::string &runinfo, const std::string &dataDir, Mantid::API::IEventWorkspace_sptr wksp);
    void runLoadMonitors();

  };


} // namespace DataHandling
} // namespace Mantid

#endif  /* MANTID_DATAHANDLING_LoadPreNexus_H_ */
