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

namespace Mantid {
namespace DataHandling {

struct FITSInfo;

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

  /// Algorithm's name for identification overriding a virtual method
  const std::string name() const override { return "LoadFITS"; }

  /// Summary of algorithms purpose
  const std::string summary() const override {
    return "Load FITS files into workspaces of type Workspace2D.";
  }

  /// Algorithm's version for identification overriding a virtual method
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override {
    return {"Load", "SaveFITS"};
  }

  /// Algorithm's category for identification overriding a virtual method
  const std::string category() const override {
    return "DataHandling\\Imaging";
  }

  /// Returns a confidence value that this algorithm can load a file
  int confidence(Kernel::FileDescriptor &descriptor) const override;

  /// Returns a value indicating whether or not loader wants to load multiple
  /// files into a single workspace
  bool loadMutipleAsOne() override { return true; }

private:
  /// Initialisation code
  void init() override;
  /// Execution code
  void exec() override;

  /// Load a block of FITS header(s) at once
  void doLoadHeaders(const std::vector<std::string> &paths,
                     std::vector<FITSInfo> &headers, size_t firstIndex,
                     size_t lastIndex);

  /// Load the FITS header(s) from one fits file into a struct
  void loadHeader(const std::string &filePath, FITSInfo &header);

  /// Once loaded, check against standard and limitations of this algorithm
  void headerSanityCheck(const FITSInfo &hdr, const FITSInfo &hdrFirst);

  /// Loads files into workspace(s)
  void doLoadFiles(const std::vector<std::string> &paths,
                   const std::string &outWSName, bool loadAsRectImg,
                   int binSize, double noiseThresh);

  /// Parses the header values for the FITS file
  void parseHeader(FITSInfo &headerInfo);

  /// Initialises a workspace with IDF and fills it with data
  DataObjects::Workspace2D_sptr makeWorkspace(
      const FITSInfo &fileInfo, size_t &newFileNumber,
      std::vector<char> &buffer, API::MantidImage &imageY,
      API::MantidImage &imageE, const DataObjects::Workspace2D_sptr parent,
      bool loadAsRectImg = false, int binSize = 1, double noiseThresh = false);

  void addAxesInfoAndLogs(DataObjects::Workspace2D_sptr ws, bool loadAsRectImg,
                          const FITSInfo &fileInfo, int binSize, double cmpp);

  // Reads the data from a single FITS file into a workspace (directly, fast)
  void readDataToWorkspace(const FITSInfo &fileInfo, double cmpp,
                           DataObjects::Workspace2D_sptr ws,
                           std::vector<char> &buffer);

  // Reads the data from a single FITS file into image objects (Y and E) that
  // then can/will be copied into a workspace
  void readDataToImgs(const FITSInfo &fileInfo, API::MantidImage &imageY,
                      API::MantidImage &imageE, std::vector<char> &buffer);

  void readInBuffer(const FITSInfo &fileInfo, std::vector<char> &buffer,
                    size_t len);

  /// filter noise pixel by pixel
  void doFilterNoise(double thresh, API::MantidImage &imageY,
                     API::MantidImage &imageE);

  /// rebin the matrix/image
  void doRebin(size_t rebin, API::MantidImage &imageY, API::MantidImage &imageE,
               API::MantidImage &rebinnedY, API::MantidImage &rebinnedE);

  /// identifies fits coming from 'other' cameras by specific headers
  bool isInstrOtherThanIMAT(const FITSInfo &hdr);

  void setupDefaultKeywordNames();

  // Maps the header keys to specified values
  void mapHeaderKeys();

  /// Returns the trailing number from a string minus leading 0's (so 25 from
  /// workspace_000025)
  size_t fetchNumber(const std::string &name);

  // Adds a number of leading 0's to another number up to the totalDigitCount.
  std::string padZeros(const size_t number, const size_t totalDigitCount);

  // Strings used to map header keys
  std::string m_headerScaleKey;
  std::string m_headerOffsetKey;
  std::string m_headerBitDepthKey;
  std::string m_headerRotationKey;
  std::string m_headerImageKeyKey;
  std::string m_headerNAxisNameKey;
  std::vector<std::string> m_headerAxisNameKeys;
  std::string m_mapFile;

  static const std::string g_defaultImgType;

  // names of extension headers
  std::string m_sampleRotation;
  std::string m_imageType;

  size_t m_pixelCount;

  // Number of digits for the fixed width appendix number added to
  // workspace names, i.e. 3=> workspace_001; 5 => workspace_00001
  static const size_t g_DIGIT_SIZE_APPEND = 6;
  /// size of a FITS header block (room for 36 entries, of 80
  /// characters each), in bytes. A FITS header always comes in
  /// multiples of this block.
  static const int g_BASE_HEADER_SIZE = 2880;

  // TODO: in the next round of refactoring of LoadFITS, this should
  // become common between LoadFITS and the new SaveFITS
  // Names for several options that can be given in a "FITS" header
  // setup file
  static const std::string g_END_KEYNAME;
  static const std::string g_COMMENT_KEYNAME;
  static const std::string g_XTENSION_KEYNAME;
  static const std::string g_BIT_DEPTH_NAME;
  static const std::string g_AXIS_NAMES_NAME;
  static const std::string g_ROTATION_NAME;
  static const std::string g_IMAGE_KEY_NAME;
  static const std::string g_HEADER_MAP_NAME;

  // Bits per pixel
  // This must be consistent with how the BITPIX header entries are processed
  static const size_t g_maxBitDepth;
  // max. bytes per pixel, for buffers
  static const size_t g_maxBytesPP;
};

} // namespace DataHandling
} // namespace Mantid

#endif // MANTID_DATAHANDLING_LOADFITS_H_
