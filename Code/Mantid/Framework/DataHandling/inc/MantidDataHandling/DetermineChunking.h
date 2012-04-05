#ifndef MANTID_DATAHANDLING_DetermineChunking_H_
#define MANTID_DATAHANDLING_DetermineChunking_H_

#include <string>
#include <vector>
#include "MantidAPI/IDataFileChecker.h"
#include "MantidKernel/System.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/IEventWorkspace.h"

namespace Mantid
{
namespace DataHandling
{

  /** DetermineChunking : Workflow algorithm to load a collection of preNeXus files.
    
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
/// Make the code clearer by having this an explicit type
typedef int PixelType;
/// Type for the DAS time of flight (data file)
typedef int DasTofType;

/// Structure that matches the form in the binary event list.
#pragma pack(push, 4) //Make sure the structure is 8 bytes.
struct DasEvent
{
    /// Time of flight.
    DasTofType tof;
    /// Pixel identifier as published by the DAS/DAE/DAQ.
    PixelType pid;
};
#pragma pack(pop)

  class DLLExport DetermineChunking  :  public API::IDataFileChecker
  {
  public:
    DetermineChunking();
    virtual ~DetermineChunking();
    
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
    std::string setTopEntryName(std::string m_filename);

  };


} // namespace DataHandling
} // namespace Mantid

#endif  /* MANTID_DATAHANDLING_DetermineChunking_H_ */
