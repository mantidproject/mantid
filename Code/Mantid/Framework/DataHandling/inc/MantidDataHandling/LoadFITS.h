#ifndef MANTID_DATAHANDLING_LOADFITS_H_
#define MANTID_DATAHANDLING_LOADFITS_H_

//---------------------------------------------------
// Includes
//---------------------------------------------------
#include "MantidAPI/IFileLoader.h"
#include "MantidDataObjects/Workspace2D.h"
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
  int offset;
  int headerSizeMultiplier;
  vector<size_t> axisPixelLengths;
  double tof;
  double timeBin;
  double scale;
  int imageKey;
  long int countsInImage;
  long int numberOfTriggers;
  string extension;
  string filePath;
  bool isFloat;
};

namespace Mantid {
namespace DataHandling {
/**
  LoadFITS : Load a number of FITS files into a histogram Workspace

  File format is described here: http://www.fileformat.info/format/fits/egff.htm
  This loader doesn't support the full specification, caveats are:
    Support for unsigned 8, 16, 32 bit values only
    Support only for 2 data axis
    No support for format extensions

  Loader is designed to work with multiple files, loading into a single
  workspace.
  At points there are assumptions that all files in a batch use the same number
  of bits per pixel,
  and that the number of spectra in each file are the same.

  @author John R Hill, RAL
  @date 29/08/2014

  Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
  National Laboratory & European Spallation Source

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

class DLLExport LoadFITS : public API::IFileLoader<Kernel::FileDescriptor> {
public:
  LoadFITS();
  virtual ~LoadFITS() {}

  /// Algorithm's name for identification overriding a virtual method
  virtual const std::string name() const { return "LoadFITS"; }

  /// Summary of algorithms purpose
  virtual const std::string summary() const {
    return "Load data from FITS files.";
  }

  /// Algorithm's version for identification overriding a virtual method
  virtual int version() const { return 1; }

  /// Algorithm's category for identification overriding a virtual method
  virtual const std::string category() const { return "DataHandling"; }

  /// Returns a confidence value that this algorithm can load a file
  virtual int confidence(Kernel::FileDescriptor &descriptor) const;

  /// Returns a value indicating whether or not loader wants to load multiple
  /// files into a single workspace
  virtual bool loadMutipleAsOne() { return true; }

private:
  /// Initialisation code
  void init();
  /// Execution code
  void exec();
  /// Parses the header values for the FITS file
  bool parseHeader(FITSInfo &headerInfo);

  /// Creates a vector of all rotations from a file
  std::vector<double> readRotations(std::string rotFilePath, size_t fileCount);

  /// Initialises a workspace with IDF and fills it with data
  DataObjects::Workspace2D_sptr
  addWorkspace(const FITSInfo &fileInfo, size_t &newFileNumber,
               void *&bufferAny, API::MantidImage &imageY,
               API::MantidImage &imageE, double rotation,
               const DataObjects::Workspace2D_sptr parent);

  /// Returns the trailing number from a string minus leading 0's (so 25 from
  /// workspace_00025)
  size_t fetchNumber(std::string name);

  // Adds a number of leading 0's to another number up to the totalDigitCount.
  std::string padZeros(size_t number, size_t totalDigitCount);

  // Reads the data from a single FITS file into a workspace
  void readFileToWorkspace(DataObjects::Workspace2D_sptr ws,
                           const FITSInfo &fileInfo, API::MantidImage &imageY,
                           API::MantidImage &imageE, void *&bufferAny);

  // Maps the header keys to specified values
  void mapHeaderKeys();

  // Strings used to map header keys
  string m_headerScaleKey;
  string m_headerOffsetKey;
  string m_headerBitDepthKey;
  string m_headerRotationKey;
  string m_headerImageKeyKey;
  string m_mapFile;
  std::vector<std::string> m_headerAxisNameKeys;

  string m_baseName;
  size_t m_spectraCount;
  API::Progress *m_progress;

  // Number of digits which will be appended to a workspace name, i.e. 4 =
  // workspace_0001
  static const size_t DIGIT_SIZE_APPEND = 4;
  static const int BASE_HEADER_SIZE = 2880;
};

} // namespace DataHandling
} // namespace Mantid

#endif // MANTID_DATAHANDLING_LOADFITS_H_
