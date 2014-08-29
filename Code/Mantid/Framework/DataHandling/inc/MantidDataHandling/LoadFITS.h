#ifndef MANTID_DATAHANDLING_LOADFITS_H_
#define MANTID_DATAHANDLING_LOADFITS_H_

#include "MantidAPI/IFileLoader.h"
#include <string>
#include <sstream>
#include <map>
#include <vector>

using namespace std;

struct FITSInfo {  
  vector<string> headerItems;
  map<string, string> headerKeys;
  int bitsPerPixel;
  int numberOfAxis;
  vector<int> axisPixelLengths;
  double tof;
  double timeBin;
  long int countsInImage;
  long int numberOfTriggers;
  string extension;
  string filePath;
}; 

namespace Mantid
{
namespace DataHandling
{
  /** LoadFITS : Load FITS files to TableWorkspace(s)
    
    Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

  class DLLExport LoadFITS : public API::IFileLoader<Kernel::FileDescriptor>
  {
  public:
    LoadFITS() {}
    virtual ~LoadFITS() {}

    /// Algorithm's name for identification overriding a virtual method
    virtual const std::string name() const { return "LoadFITS" ;}

    ///Summary of algorithms purpose
    virtual const std::string summary() const {return "Load data from FITS files.";}

    /// Algorithm's version for identification overriding a virtual method
    virtual int version() const { return 1 ;}

    /// Algorithm's category for identification overriding a virtual method
    virtual const std::string category() const { return "DataHandling";}

    /// Returns a confidence value that this algorithm can load a file
    virtual int confidence(Kernel::FileDescriptor & descriptor) const;

  private:
    /// Initialisation code
    void init();
    /// Execution code
    void exec();
    
    /// Parses the header values for the FITS file
    bool parseHeader(FITSInfo &headerInfo);
    void loadChunkOfBinsFromFile(Mantid::API::MatrixWorkspace_sptr &workspace, vector<vector<double> > &yVals, vector<vector<double> > &eVals, void *&bufferAny, MantidVecPtr &x, long spetraCount, int bitsPerPixel, long binChunkStartIndex);

    API::MatrixWorkspace_sptr initAndPopulateHistogramWorkspace();

    vector<FITSInfo> m_allHeaderInfo;

    int m_binChunkSize;
    static const int FIXED_HEADER_SIZE = 2880;
    
  };
  

} // namespace DataHandling
} // namespace Mantid

#endif // MANTID_DATAHANDLING_LOADFITS_H_
