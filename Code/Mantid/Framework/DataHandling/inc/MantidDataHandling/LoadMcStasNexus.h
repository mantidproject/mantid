#ifndef MANTID_DATAHANDLING_LOADMCSTASNEXUS_H_
#define MANTID_DATAHANDLING_LOADMCSTASNEXUS_H_

#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/IDataFileChecker.h"

namespace Mantid
{
namespace DataHandling
{

  /** LoadMcStasNexus : TODO: DESCRIPTION
    
    Copyright &copy; 2013 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

    File change history is stored at: <https://github.com/mantidproject/mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
  */
  class DLLExport LoadMcStasNexus  : public API::IDataFileChecker
  {
  public:
    LoadMcStasNexus();
    virtual ~LoadMcStasNexus();
    
    virtual const std::string name() const;
    virtual int version() const;
    virtual const std::string category() const;

    /// do a quick check that this file can be loaded 
    virtual bool quickFileCheck(const std::string& filePath,size_t nread,const file_header& header);
    /// check the structure of the file and  return a value between 0 and 100 of how much this file can be loaded
    virtual int fileCheck(const std::string& filePath);

  private:
    virtual void initDocs();
    void init();
    void exec();


  };


} // namespace DataHandling
} // namespace Mantid

#endif  /* MANTID_DATAHANDLING_LOADMCSTASNEXUS_H_ */