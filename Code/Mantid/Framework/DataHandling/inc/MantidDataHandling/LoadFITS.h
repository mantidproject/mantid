#ifndef MANTID_DATAHANDLING_LOADFITS_H_
#define MANTID_DATAHANDLING_LOADFITS_H_

//---------------------------------------------------
// Includes
//---------------------------------------------------
#include "MantidAPI/IFileLoader.h"
#include <map>
#include <sstream>
#include <string>
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
  /** 
    LoadFITS : Load a number of FITS files into a histogram Workspace

    File format is described here: http://www.fileformat.info/format/fits/egff.htm
    This loader doesn't support the full specification, caveats are:
      Support for unsigned 8, 16, 32 bit values only
      Support only for 2 data axis
      No support for format extensions

    Loader is designed to work with multiple files, loading into a single workspace.
    At points there are assumptions that all files in a batch use the same number of bits per pixel,
    and that the number of spectra in each file are the same.

    @author John R Hill, RAL 
    @date 29/08/2014
    
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

    /// Returns a value indicating whether or not loader wants to load multiple files into a single workspace
    virtual bool loadMutipleAsOne() { return true; }

  private:
    /// Initialisation code
    void init();
    /// Execution code
    void exec();    
    /// Parses the header values for the FITS file
    bool parseHeader(FITSInfo &headerInfo);
    /// Load data from a number of files into the workspace
    void loadChunkOfBinsFromFile(Mantid::API::MatrixWorkspace_sptr &workspace, vector<vector<double> > &yVals, vector<vector<double> > &eVals, void *&bufferAny, MantidVecPtr &x, size_t spetraCount, int bitsPerPixel, size_t binChunkStartIndex);
    /// Initialises a workspace with IDF and fills it with data
    API::MatrixWorkspace_sptr initAndPopulateHistogramWorkspace();
    /// Creates a comma separated string of rotations from a file
    std::string ReadRotations(std::string rotFilePath, size_t fileCount);
    
    vector<FITSInfo> m_allHeaderInfo;
    size_t m_binChunkSize;
    static const int FIXED_HEADER_SIZE = 2880;    
  };
  

} // namespace DataHandling
} // namespace Mantid

#endif // MANTID_DATAHANDLING_LOADFITS_H_
