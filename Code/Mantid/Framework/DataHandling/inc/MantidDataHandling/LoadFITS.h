#ifndef MANTID_DATAHANDLING_LOADFITS_H_
#define MANTID_DATAHANDLING_LOADFITS_H_

//---------------------------------------------------
// Includes
//---------------------------------------------------
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "MantidAPI/IFileLoader.h"
#include "MantidDataObjects/Workspace2D.h"

struct FITSInfo {
  std::vector<std::string> headerItems;
  std::map<std::string, std::string> headerKeys;
  int bitsPerPixel;
  int numberOfAxis;
  int offset;
  int headerSizeMultiplier;
  std::vector<size_t> axisPixelLengths;
  double tof;
  double timeBin;
  double scale;
  std::string imageKey;
  long int countsInImage;
  long int numberOfTriggers;
  std::string extension;
  std::string filePath;
  bool isFloat;
};

namespace Mantid {
namespace DataHandling {
/**
LoadFITS: Load one or more of FITS files into a Workspace2D. The FITS
format, normally used for images, is described for example here:
http://www.fileformat.info/format/fits/egff.htm

At the moment this algorithm only supports 2 data axis and the
following data types: unsigned 8, 16, 32 bits per pixel.

Copyright &copy; 2014,2015 ISIS Rutherford Appleton Laboratory, NScD
Oak Ridge National Laboratory & European Spallation Source

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
    return "Load FITS files into workspaces of type Workspace2D.";
  }

  /// Algorithm's version for identification overriding a virtual method
  virtual int version() const { return 1; }

  /// Algorithm's category for identification overriding a virtual method
  virtual const std::string category() const { return "DataHandling;DataHandling\\Tomography"; }

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

  /// Loads files into workspace(s)
  void doLoadFiles(const std::vector<std::string> &paths,
                   bool loadAsRectImg = false);

  /// Loads the FITS header(s) into a struct
  void doLoadHeaders(const std::vector<std::string> &paths,
                     std::vector<FITSInfo> &headers);

  /// Parses the header values for the FITS file
  void parseHeader(FITSInfo &headerInfo);

  /// Initialises a workspace with IDF and fills it with data
  DataObjects::Workspace2D_sptr
  makeWorkspace(const FITSInfo &fileInfo, size_t &newFileNumber,
                std::vector<char> &buffer, API::MantidImage &imageY,
                API::MantidImage &imageE,
                const DataObjects::Workspace2D_sptr parent,
                bool loadAsRectImg = false);

  // Reads the data from a single FITS file into a workspace
  void readDataToWorkspace2D(DataObjects::Workspace2D_sptr ws,
                             const FITSInfo &fileInfo, API::MantidImage &imageY,
                             API::MantidImage &imageE,
                             std::vector<char> &buffer, bool loadAsRectImg,
                             double scale_1);

  /// Once loaded, check against standard and limitations of this algorithm
  void headerSanityCheck(const FITSInfo &hdr, const FITSInfo &hdrFirst);

  /// filter noise pixel by pixel
  void doFilterNoise(double thresh, API::MantidImage &imageY,
                     API::MantidImage &imageE);

  /// rebin the matrix/image
  void doRebin(int rebin, API::MantidImage &imageX, API::MantidImage &imageY);

  /// identifies fits coming from 'other' cameras by specific headers
  bool isInstrOtherThanIMAT(FITSInfo &hdr);

  void setupDefaultKeywordNames();

  /// Returns the trailing number from a string minus leading 0's (so 25 from
  /// workspace_00025)
  size_t fetchNumber(const std::string &name);

  // Adds a number of leading 0's to another number up to the totalDigitCount.
  std::string padZeros(const size_t number, const size_t totalDigitCount);

  // Maps the header keys to specified values
  void mapHeaderKeys();

  // Strings used to map header keys
  std::string m_headerScaleKey;
  std::string m_headerOffsetKey;
  std::string m_headerBitDepthKey;
  std::string m_headerRotationKey;
  std::string m_headerImageKeyKey;
  std::string m_headerNAxisNameKey;
  std::vector<std::string> m_headerAxisNameKeys;
  std::string m_mapFile;

  static const std::string m_defaultImgType;

  // names of extension headers
  std::string m_sampleRotation;
  std::string m_imageType;

  std::string m_baseName;
  size_t m_pixelCount;
  API::Progress *m_progress;

  // Number of digits for the fixed width appendix number added to
  // workspace names, i.e. 3=> workspace_001; 5 => workspace_00001
  static const size_t DIGIT_SIZE_APPEND = 5;
  /// size of a FITS header block (room for 36 entries, of 80
  /// characters each), in bytes. A FITS header always comes in
  /// multiples of this block.
  static const int BASE_HEADER_SIZE = 2880;

  // names for several options that can be given in a "FITS" header
  // setup file
  static const std::string m_BIT_DEPTH_NAME;
  static const std::string m_AXIS_NAMES_NAME;
  static const std::string m_ROTATION_NAME;
  static const std::string m_IMAGE_KEY_NAME;
  static const std::string m_HEADER_MAP_NAME;
};

} // namespace DataHandling
} // namespace Mantid

#endif // MANTID_DATAHANDLING_LOADFITS_H_
