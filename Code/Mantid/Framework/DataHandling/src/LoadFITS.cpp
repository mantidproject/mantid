#include "MantidAPI/MultipleFileProperty.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/RegisterFileLoader.h"
#include "MantidDataHandling/LoadFITS.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/UnitFactory.h"

#include <boost/algorithm/string.hpp>
#include <Poco/BinaryReader.h>
#include <Poco/FileStream.h>

using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace std;
using namespace boost;

namespace {}

namespace Mantid {
namespace DataHandling {
// Register the algorithm into the AlgorithmFactory
DECLARE_FILELOADER_ALGORITHM(LoadFITS)

const std::string LoadFITS::m_BIT_DEPTH_NAME = "BitDepthName";
const std::string LoadFITS::m_ROTATION_NAME = "RotationName";
const std::string LoadFITS::m_AXIS_NAMES_NAME = "AxisNames";
const std::string LoadFITS::m_IMAGE_KEY_NAME = "ImageKeyName";
const std::string LoadFITS::m_HEADER_MAP_NAME = "HeaderMapFile";

const std::string LoadFITS::m_defaultImgType = "SAMPLE";

/**
 * Constructor. Just initialize everything to prevent issues.
 */
LoadFITS::LoadFITS()
    : m_headerScaleKey(), m_headerOffsetKey(), m_headerBitDepthKey(),
      m_headerRotationKey(), m_headerImageKeyKey(), m_headerAxisNameKeys(),
      m_mapFile(), m_baseName(), m_spectraCount(0), m_progress(NULL) {
  setupDefaultKeywordNames();
}

/**
* Return the confidence with with this algorithm can load the file
* @param descriptor A descriptor for the file
* @returns An integer specifying the confidence level. 0 indicates it will not
* be used
*/
int LoadFITS::confidence(Kernel::FileDescriptor &descriptor) const {
  // Should really improve this to check the file header (of first file at
  // least) to make sure it contains the fields wanted
  return (descriptor.extension() == ".fits" || descriptor.extension() == ".fit")
             ? 80
             : 0;
}

/**
 * Sets several keyword names with default (and standard) values. You
 * don't want to change these unless you want to break compatibility
 * with the FITS standard.
 */
void LoadFITS::setupDefaultKeywordNames() {
  // Inits all the absolutely necessary keywords
  // standard headers (If SIMPLE=T)
  m_headerScaleKey = "BSCALE";
  m_headerOffsetKey = "BZERO";
  m_headerBitDepthKey = "BITPIX";
  m_headerImageKeyKey = "IMAGE_TYPE"; // This is a "HIERARCH Image/Type= "
  m_headerRotationKey = "ROTATION";

  m_headerNAxisNameKey = "NAXIS";
  m_headerAxisNameKeys.push_back("NAXIS1");
  m_headerAxisNameKeys.push_back("NAXIS2");

  m_mapFile = "";

  // extensions
  m_sampleRotation = "HIERARCH Sample/Tomo_Angle";
  m_imageType = "HIERARCH Image/Type";
}

/**
* Initialise the algorithm. Declare properties which can be set before execution
* (input) or
* read from after the execution (output).
*/
void LoadFITS::init() {
  // Specify file extensions which can be associated with a FITS file.
  std::vector<std::string> exts, exts2;

  // Declare the Filename algorithm property. Mandatory. Sets the path to the
  // file to load.
  exts.clear();
  exts.push_back(".fits");
  exts.push_back(".fit");

  exts2.push_back(".*");

  declareProperty(new MultipleFileProperty("Filename", exts),
                  "The name of the input file (you can give "
                  "multiple file names separated by commas).");

  declareProperty(new API::WorkspaceProperty<API::Workspace>(
      "OutputWorkspace", "", Kernel::Direction::Output));

  declareProperty(
      new FileProperty(m_HEADER_MAP_NAME, "", FileProperty::OptionalDirectory,
                       "", Kernel::Direction::Input),
      "A file mapping header key names to non-standard names [line separated "
      "values in the format KEY=VALUE, e.g. BitDepthName=BITPIX] - do not use "
      "this if you want to keep compatibility with standard FITS files.");
}

/**
 * Execute the algorithm.
 */
void LoadFITS::exec() {
  // for non-standard headers, by default won't do anything
  mapHeaderKeys();

  string fName = getPropertyValue("Filename");

  std::vector<std::string> paths;
  boost::split(paths, fName, boost::is_any_of(","));
  doLoadFiles(paths);
}

/**
 * Create FITS file information for each file selected. Loads headers
 * and data from the files and fills the output workspace(s).
 *
 * @param paths File names as given in the algorithm input property
 */
void LoadFITS::doLoadFiles(const std::vector<std::string> &paths) {
  std::vector<FITSInfo> headers;

  doLoadHeaders(paths, headers);

  // No extension is set -> it's the standard format which we can parse.
  if (headers[0].numberOfAxis > 0)
    m_spectraCount += headers[0].axisPixelLengths[0];

  // Presumably 2 axis, but futureproofing.
  for (int i = 1; i < headers[0].numberOfAxis; ++i) {
    m_spectraCount *= headers[0].axisPixelLengths[i];
  }

  MantidImage imageY(headers[0].axisPixelLengths[0],
                     vector<double>(headers[0].axisPixelLengths[1]));
  MantidImage imageE(headers[0].axisPixelLengths[0],
                     vector<double>(headers[0].axisPixelLengths[1]));

  size_t bytes = (headers[0].bitsPerPixel / 8) * m_spectraCount;
  std::vector<char> buffer;
  try {
    buffer.resize(bytes);
  } catch (std::exception &) {
    throw std::runtime_error(
        "Could not allocate enough memory to run when trying to allocate " +
        boost::lexical_cast<std::string>(bytes) + " bytes.");
  }

  // Create a group for these new workspaces, if the group already exists, add
  // to it.
  string groupName = getPropertyValue("OutputWorkspace");

  // This forms the name of the group
  m_baseName = getPropertyValue("OutputWorkspace") + "_";

  size_t fileNumberInGroup = 0;
  WorkspaceGroup_sptr wsGroup;

  if (!AnalysisDataService::Instance().doesExist(groupName)) {
    wsGroup = WorkspaceGroup_sptr(new WorkspaceGroup);
    wsGroup->setTitle(groupName);
  } else {
    // Get the name of the latest file in group to start numbering from
    if (AnalysisDataService::Instance().doesExist(groupName))
      wsGroup =
          AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(groupName);

    std::string latestName = wsGroup->getNames().back();
    // Set next file number
    fileNumberInGroup = fetchNumber(latestName) + 1;
  }

  // Create a progress reporting object
  m_progress = new Progress(this, 0, 1, headers.size() + 1);

  // Create first workspace (with instrument definition). This is also used as
  // a template for creating others
  Workspace2D_sptr latestWS;
  latestWS = makeWorkspace(headers[0], fileNumberInGroup, buffer, imageY,
                           imageE, latestWS);

  map<size_t, Workspace2D_sptr> wsOrdered;
  wsOrdered[0] = latestWS;
  try {
    IAlgorithm_sptr loadInst = createChildAlgorithm("LoadInstrument");
    std::string directoryName =
        Kernel::ConfigService::Instance().getInstrumentDirectory();
    directoryName = directoryName + "/IMAT_Definition.xml";
    loadInst->setPropertyValue("Filename", directoryName);
    loadInst->setProperty<MatrixWorkspace_sptr>(
        "Workspace", dynamic_pointer_cast<MatrixWorkspace>(latestWS));
    loadInst->execute();
  } catch (std::exception &ex) {
    g_log.information("Cannot load the instrument definition. " +
                      string(ex.what()));
  }

  PARALLEL_FOR_NO_WSP_CHECK()
  for (int64_t i = 1; i < static_cast<int64_t>(headers.size()); ++i) {
    latestWS = makeWorkspace(headers[i], fileNumberInGroup, buffer, imageY,
                             imageE, latestWS);
    wsOrdered[i] = latestWS;
  }

  // Add to group - done here to maintain sequence
  for (auto it = wsOrdered.begin(); it != wsOrdered.end(); ++it) {
    wsGroup->addWorkspace(it->second);
  }

  setProperty("OutputWorkspace", wsGroup);
}

/**
 * Load header(s) from FITS file(s) into FITSInfo header
 * struct(s). This is usually the first step when loading FITS files
 * into workspaces or anything else. In the simplest case, paths has
 * only one string and only one header struct is added in headers.
 *
 * @param paths File name(s)
 * @param headers Vector where to store the header struct(s)
 *
 * @throws std::runtime_error if issues are found in the headers
 */
void LoadFITS::doLoadHeaders(const std::vector<std::string> &paths,
                             std::vector<FITSInfo> &headers) {
  headers.resize(paths.size());

  for (size_t i = 0; i < paths.size(); ++i) {
    headers[i].extension = "";
    headers[i].filePath = paths[i];
    // Get various pieces of information from the file header which are used to
    // create the workspace
    try {
      parseHeader(headers[i]);
    } catch (std::exception &e) {
      // Unable to parse the header, throw.
      throw std::runtime_error(
          "Severe problem found while parsing the header of "
          "this FITS file (" +
          paths[i] +
          "). This file may not be standard FITS. Error description: " +
          e.what());
    }

    // Get and convert specific standard header values which will are
    // needed to know how to load the data: BITPIX, NAXIS, NAXISi (where i =
    // 1..NAXIS, e.g. NAXIS2 for two axis).
    try {
      std::string tmpBitPix = headers[i].headerKeys[m_headerBitDepthKey];
      if (boost::contains(tmpBitPix, "-")) {
        boost::erase_all(tmpBitPix, "-");
        headers[i].isFloat = true;
      } else {
        headers[i].isFloat = false;
      }

      try {
        headers[i].bitsPerPixel = lexical_cast<int>(tmpBitPix);
      } catch (std::exception &e) {
        throw std::runtime_error(
            "Coult not interpret the entry number of bits per pixel (" +
            m_headerBitDepthKey + ") as an integer. Error: " + e.what());
      }
      // Check that the files use bit depths of either 8, 16 or 32
      if (headers[i].bitsPerPixel != 8 && headers[i].bitsPerPixel != 16 &&
          headers[i].bitsPerPixel != 32 && headers[i].bitsPerPixel != 64)
        throw std::runtime_error(
            "This algorithm only supports 8, 16, 32 or 64 "
            "bits per pixel. The header of file '" +
            paths[i] + "' says that its bit depth is: " +
            boost::lexical_cast<std::string>(headers[i].bitsPerPixel));
    } catch (std::exception &e) {
      throw std::runtime_error(
          "Failed to process the '" + m_headerBitDepthKey +
          "' entry (bits per pixel) in the header of this file: " + paths[i] +
          ". Error description: " + e.what());
    }

    try {
      // Add the image key, use the value in the FITS header if found,
      // otherwise default (to SAMPLE).
      auto it = headers[i].headerKeys.find(m_headerImageKeyKey);
      if (headers[i].headerKeys.end() != it) {
        headers[i].imageKey = it->second;
      } else {
        headers[i].imageKey = m_defaultImgType;
      }
    } catch (std::exception &e) {
      throw std::runtime_error("Failed to process the '" + m_headerImageKeyKey +
                               "' entry (type of image: sample, dark, open) in "
                               "the header of this file: " +
                               paths[i] + ". Error description: " + e.what());
    }

    try {
      headers[i].numberOfAxis = static_cast<int>(m_headerAxisNameKeys.size());

      for (int j = 0; headers.size() > i && j < headers[i].numberOfAxis; ++j) {
        headers[i].axisPixelLengths.push_back(lexical_cast<size_t>(
            headers[i].headerKeys[m_headerAxisNameKeys[j]]));
      }

      // Various extensions to the FITS format are used elsewhere, and
      // must be parsed differently if used. This loader Loader
      // doesn't support this.
      headers[i].extension = headers[i].headerKeys["XTENSION"];
    } catch (std::exception &e) {
      throw std::runtime_error(
          "Failed to process the '" + m_headerNAxisNameKey +
          "' entries (dimensions) in the header of this file: " + paths[i] +
          ". Error description: " + e.what());
    }

    // scale parameter, header BSCALE in the fits standard
    if ("" == headers[i].headerKeys[m_headerScaleKey]) {
      headers[i].scale = 1;
    } else {
      try {
        headers[i].scale =
            lexical_cast<double>(headers[i].headerKeys[m_headerScaleKey]);
      } catch (std::exception &e) {
        throw std::runtime_error(
            "Coult not interpret the entry number of bits per pixel (" +
            m_headerBitDepthKey +
            ") as a floating point number (double). Error: " + e.what());
      }
    }

    // data offsset parameter, header BZERO in the fits standard
    if ("" == headers[i].headerKeys[m_headerOffsetKey]) {
      headers[i].offset = 0;
    } else {
      try {
        headers[i].offset =
            lexical_cast<int>(headers[i].headerKeys[m_headerOffsetKey]);
      } catch (std::exception & /*e*/) {
        // still, second try with floating point format (as used for example by
        // Starlight XPRESS cameras)
        try {
          double doff =
              lexical_cast<double>(headers[i].headerKeys[m_headerOffsetKey]);
          double intPart;
          if (0 != modf(doff, &intPart)) {
            // anyway we'll do a cast, but warn if there was a fraction
            g_log.warning()
                << "The value given in the FITS header entry for the data "
                   "offset (" +
                       m_headerOffsetKey +
                       ") has a fractional part, and it will be ignored!"
                << std::endl;
          }
          headers[i].offset = static_cast<int>(doff);
        } catch (std::exception &e) {
          throw std::runtime_error(
              "Coult not interpret the entry number of data offset (" +
              m_headerOffsetKey + ") as an integer number nor a floating point "
                                  "number (double). Error: " +
              e.what());
        }
      }
    }

    // Check each header is valid/supported: standard (no extension to
    // FITS), and has two axis
    headerSanityCheck(headers[i], headers[0]);
  }
}

/**
 * Read a single files header and populate an object with the information.
 *
 * @param headerInfo A FITSInfo file object to parse header
 * information into. This object must have its field filePath set to
 * the input file
 *
 * @throws various std::runtime_error etc. on read failure
*/
void LoadFITS::parseHeader(FITSInfo &headerInfo) {
  headerInfo.headerSizeMultiplier = 0;
  ifstream istr(headerInfo.filePath.c_str(), ios::binary);
  Poco::BinaryReader reader(istr);

  // Iterate 80 bytes at a time until header is parsed | 2880 bytes is the
  // fixed header length of FITS
  // 2880/80 = 36 iterations required
  bool endFound = false;
  while (!endFound) {
    headerInfo.headerSizeMultiplier++;
    for (int i = 0; i < 36; ++i) {
      // Keep vect of each header item, including comments, and also keep a
      // map of individual keys.
      string part;
      reader.readRaw(80, part);
      headerInfo.headerItems.push_back(part);

      // Add key/values - these are separated by the = symbol.
      // If it doesn't have an = it's a comment to ignore. All keys should be
      // unique
      auto eqPos = part.find('=');
      if (eqPos > 0) {
        string key = part.substr(0, eqPos);
        string value = part.substr(eqPos + 1);

        // Comments are added after the value separated by a / symbol. Remove.
        auto slashPos = value.find('/');
        if (slashPos > 0)
          value = value.substr(0, slashPos);

        boost::trim(key);
        boost::trim(value);

        if (key == "END")
          endFound = true;

        if (key != "")
          headerInfo.headerKeys[key] = value;
      }
    }
  }

  istr.close();
}

/**
 * Creates and initialises a workspace with instrument definition and fills it
 * with data
 *
 * @param fileInfo information for the current file
 * @param newFileNumber number for the new file when added into ws group
 * @param buffer pre-allocated buffer to contain data values
 * @param imageY Object to set the Y data values in
 * @param imageE Object to set the E data values in
 * @param parent A workspace which can be used to copy initialisation
 * information from (size/instrument def etc)
 *
 * @returns A newly created Workspace2D, as a shared pointer
 */
Workspace2D_sptr
LoadFITS::makeWorkspace(const FITSInfo &fileInfo, size_t &newFileNumber,
                        std::vector<char> &buffer, MantidImage &imageY,
                        MantidImage &imageE, const Workspace2D_sptr parent) {
  // Create ws
  Workspace2D_sptr ws;
  if (!parent) {
    ws = dynamic_pointer_cast<Workspace2D>(WorkspaceFactory::Instance().create(
        "Workspace2D", m_spectraCount, 2, 1));
  } else {
    ws = dynamic_pointer_cast<Workspace2D>(
        WorkspaceFactory::Instance().create(parent));
  }

  string currNumberS = padZeros(newFileNumber, DIGIT_SIZE_APPEND);
  ++newFileNumber;

  string baseName = m_baseName + currNumberS;

  ws->setTitle(baseName);

  // set data
  readDataToWorkspace2D(ws, fileInfo, imageY, imageE, buffer);

  // Add all header info to log.
  for (auto it = fileInfo.headerKeys.begin(); it != fileInfo.headerKeys.end();
       ++it) {
    ws->mutableRun().removeLogData(it->first, true);
    ws->mutableRun().addLogData(
        new PropertyWithValue<string>(it->first, it->second));
  }

  // Add rotational data to log. Clear first from copied WS
  auto it = fileInfo.headerKeys.find(m_sampleRotation);
  ws->mutableRun().removeLogData("Rotation", true);
  if (fileInfo.headerKeys.end() != it) {
    double rot = boost::lexical_cast<double>(it->second);
    if (rot >= 0) {
      ws->mutableRun().addLogData(
          new PropertyWithValue<double>("Rotation", rot));
    }
  }

  // Add axis information to log. Clear first from copied WS
  ws->mutableRun().removeLogData("Axis1", true);
  ws->mutableRun().addLogData(new PropertyWithValue<int>(
      "Axis1", static_cast<int>(fileInfo.axisPixelLengths[0])));
  ws->mutableRun().removeLogData("Axis2", true);
  ws->mutableRun().addLogData(new PropertyWithValue<int>(
      "Axis2", static_cast<int>(fileInfo.axisPixelLengths[1])));

  // Add image key data to log. Clear first from copied WS
  ws->mutableRun().removeLogData("ImageKey", true);
  ws->mutableRun().addLogData(
      new PropertyWithValue<std::string>("ImageKey", fileInfo.imageKey));

  m_progress->report();

  return ws;
}

/**
 * Reads the data (matrix) from a single FITS file into a workspace
 *
 * @param ws Workspace to populate with the data
 * @param fileInfo information pertaining to the FITS file to load
 * @param imageY Object to set the Y data values in
 * @param imageE Object to set the E data values in
 * @param buffer pre-allocated buffer to contain data values
 *
 * @throws std::runtime_error if there are file input issues
 */
void LoadFITS::readDataToWorkspace2D(Workspace2D_sptr ws,
                                     const FITSInfo &fileInfo,
                                     MantidImage &imageY, MantidImage &imageE,
                                     std::vector<char> &buffer) {
  std::string filename = fileInfo.filePath;
  Poco::FileStream file(filename, std::ios::in);

  size_t bytespp = (fileInfo.bitsPerPixel / 8);
  size_t len = m_spectraCount * bytespp;
  file.seekg(BASE_HEADER_SIZE * fileInfo.headerSizeMultiplier);
  file.read(&buffer[0], len);
  if (!file) {
    throw std::runtime_error(
        "Error while reading file: " + filename + ". Tried to read " +
        boost::lexical_cast<std::string>(len) + " bytes but got " +
        boost::lexical_cast<std::string>(file.gcount()) +
        " bytes. The file and/or its headers may be wrong.");
  }
  // all is loaded
  file.close();

  // create pointer of correct data type to void pointer of the buffer:
  uint8_t *buffer8 = reinterpret_cast<uint8_t *>(&buffer[0]);

  std::vector<char> buf(bytespp);
  char *tmp = &buf.front();
  size_t start = 0;
  for (size_t i = 0; i < fileInfo.axisPixelLengths[0]; ++i) {
    for (size_t j = 0; j < fileInfo.axisPixelLengths[1]; ++j) {
      // If you wanted to PARALLEL_...ize these loops (which doesn't
      // seem to provide any speed up, you cannot use the
      // start+=bytespp at the end of this loop. You'd need something
      // like this:
      //
      // size_t start =
      //   ((i * (bytespp)) * fileInfo.axisPixelLengths[1]) +
      //  (j * (bytespp));

      // Reverse byte order of current value
      std::reverse_copy(buffer8 + start, buffer8 + start + bytespp, tmp);

      double val = 0;
      if (fileInfo.bitsPerPixel == 8)
        val = static_cast<double>(*reinterpret_cast<uint8_t *>(tmp));
      if (fileInfo.bitsPerPixel == 16)
        val = static_cast<double>(*reinterpret_cast<uint16_t *>(tmp));
      if (fileInfo.bitsPerPixel == 32 && !fileInfo.isFloat)
        val = static_cast<double>(*reinterpret_cast<uint32_t *>(tmp));
      if (fileInfo.bitsPerPixel == 64 && !fileInfo.isFloat)
        val = static_cast<double>(*reinterpret_cast<uint64_t *>(tmp));

      // cppcheck doesn't realise that these are safe casts
      if (fileInfo.bitsPerPixel == 32 && fileInfo.isFloat) {
        // cppcheck-suppress invalidPointerCast
        val = static_cast<double>(*reinterpret_cast<float *>(tmp));
      }
      if (fileInfo.bitsPerPixel == 64 && fileInfo.isFloat) {
        // cppcheck-suppress invalidPointerCast
        val = *reinterpret_cast<double *>(tmp);
      }

      val = fileInfo.scale * val - fileInfo.offset;

      imageY[i][j] = val;
      imageE[i][j] = sqrt(val);

      start += bytespp;
    }
  }

  // Set in WS
  ws->setImageYAndE(imageY, imageE, 0, false);
}

/**
 * Checks that a FITS header (once loaded) is valid/supported:
 * standard (no extension to FITS), and has two axis with the expected
 * dimensions.
 *
 * @param hdr FITS header struct loaded from a file - to check
 *
 * @param hdrFirst FITS header struct loaded from a (first) reference file - to
 * compare against
 *
 * @throws std::exception if there's any issue or unsupported entry in the
 * header
 */
void LoadFITS::headerSanityCheck(const FITSInfo &hdr,
                                 const FITSInfo &hdrFirst) {
  bool valid = true;
  if (hdr.extension != "") {
    valid = false;
    g_log.error() << "File " << hdr.filePath
                  << ": extensions found in the header." << std::endl;
  }
  if (hdr.numberOfAxis != 2) {
    valid = false;
    g_log.error() << "File " << hdr.filePath
                  << ": the number of axes is not 2 but: " << hdr.numberOfAxis
                  << std::endl;
  }

  // Test current item has same axis values as first item.
  if (hdr.axisPixelLengths[0] != hdrFirst.axisPixelLengths[0]) {
    valid = false;
    g_log.error() << "File " << hdr.filePath
                  << ": the number of pixels in the first dimension differs "
                     "from the first file loaded (" << hdrFirst.filePath
                  << "): " << hdr.axisPixelLengths[0]
                  << " != " << hdrFirst.axisPixelLengths[0] << std::endl;
  }
  if (hdr.axisPixelLengths[1] != hdrFirst.axisPixelLengths[1]) {
    valid = false;
    g_log.error() << "File " << hdr.filePath
                  << ": the number of pixels in the second dimension differs"
                     "from the first file loaded (" << hdrFirst.filePath
                  << "): " << hdr.axisPixelLengths[0]
                  << " != " << hdrFirst.axisPixelLengths[0] << std::endl;
  }

  // Check the format is correct and create the Workspace
  if (!valid) {
    // Invalid files, record error
    throw std::runtime_error(
        "An issue has been found in the header of this FITS file: " +
        hdr.filePath +
        ". This algorithm currently doesn't support FITS files with "
        "non-standard extensions, more than two axis "
        "of data, or has detected that all the files are "
        "not similar.");
  }
}

/**
 * Returns the trailing number from a string minus leading 0's (so 25 from
 * workspace_00025)the confidence with with this algorithm can load the file
 *
 * @param name string with a numerical suffix
 *
 * @returns A numerical representation of the string minus leading characters
 * and leading 0's
 */
size_t LoadFITS::fetchNumber(const std::string &name) {
  string tmpStr = "";
  for (auto it = name.end() - 1; isdigit(*it); --it) {
    tmpStr.insert(0, 1, *it);
  }
  while (tmpStr.length() > 0 && tmpStr[0] == '0') {
    tmpStr.erase(tmpStr.begin());
  }
  return (tmpStr.length() > 0) ? lexical_cast<size_t>(tmpStr) : 0;
}

/**
 * Adds 0's to the front of a number to create a string of size totalDigitCount
 * including number
 *
 * @param number input number to add padding to
 * @param totalDigitCount width of the resulting string with 0s followed by
 * number
 *
 * @return A string with the 0-padded number
 */
std::string LoadFITS::padZeros(const size_t number,
                               const size_t totalDigitCount) {
  std::ostringstream ss;
  ss << std::setw(static_cast<int>(totalDigitCount)) << std::setfill('0')
     << static_cast<int>(number);

  return ss.str();
}

/**
 *  Maps the header keys to specified values
 */
void LoadFITS::mapHeaderKeys() {
  if ("" == getPropertyValue(m_HEADER_MAP_NAME))
    return;

  // If a map file is selected, use that.
  std::string name = getPropertyValue(m_HEADER_MAP_NAME);
  ifstream fStream(name.c_str());

  try {
    // Ensure valid file
    if (fStream.good()) {
      // Get lines, split words, verify and add to map.
      std::string line;
      vector<std::string> lineSplit;
      while (getline(fStream, line)) {
        boost::split(lineSplit, line, boost::is_any_of("="));

        if (lineSplit[0] == m_ROTATION_NAME && lineSplit[1] != "")
          m_headerRotationKey = lineSplit[1];

        if (lineSplit[0] == m_BIT_DEPTH_NAME && lineSplit[1] != "")
          m_headerBitDepthKey = lineSplit[1];

        if (lineSplit[0] == m_AXIS_NAMES_NAME && lineSplit[1] != "") {
          m_headerAxisNameKeys.clear();
          boost::split(m_headerAxisNameKeys, lineSplit[1],
                       boost::is_any_of(","));
        }

        if (lineSplit[0] == m_IMAGE_KEY_NAME && lineSplit[1] != "") {
          m_headerImageKeyKey = lineSplit[1];
        }
      }

      fStream.close();
    } else {
      throw std::runtime_error(
          "Error while trying to read header keys mapping file: " + name);
    }
  } catch (...) {
    g_log.error("Cannot load specified map file, using property values "
                "and/or defaults.");
  }
}

} // namespace DataHandling
} // namespace Mantid
