#include "MantidDataHandling/LoadFITS.h"
#include "MantidAPI/MultipleFileProperty.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/RegisterFileLoader.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/UnitFactory.h"
#include <boost/algorithm/string.hpp>
#include <Poco/BinaryReader.h>
#include <fstream>

using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace std;
using namespace boost;
using Poco::BinaryReader;

namespace {
static const std::string BIT_DEPTH_NAME = "BitDepthName";
static const std::string ROTATION_NAME = "RotationName";
static const std::string AXIS_NAMES_NAME = "AxisNames";
static const std::string IMAGE_KEY_NAME = "ImageKeyName";
static const std::string HEADER_MAP_NAME = "HeaderMapFile";

/**
* Used with find_if to check a string isn't a fits file (by checking extension)
* @param s string to check for extension
* @returns bool Value indicating if the string ends with .fits or not
*/
bool IsNotFits(std::string s) {
  std::string tmp = s;
  to_lower(tmp);
  return !ends_with(tmp, ".fits");
}
}

namespace Mantid {
namespace DataHandling {
// Register the algorithm into the AlgorithmFactory
DECLARE_FILELOADER_ALGORITHM(LoadFITS);

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
                  "The input filename of the stored data");

  declareProperty(new API::WorkspaceProperty<API::Workspace>(
      "OutputWorkspace", "", Kernel::Direction::Output));

  declareProperty(
      new PropertyWithValue<int>("ImageKey", -1, Kernel::Direction::Input),
      "Image type to set these files as. 0=data image, 1=flat field, 2=open "
      "field, -1=use the value from FITS header.");

  declareProperty(new PropertyWithValue<string>(BIT_DEPTH_NAME, "BITPIX",
                                                Kernel::Direction::Input),
                  "Name for the pixel bit depth header key.");
  declareProperty(new PropertyWithValue<string>(ROTATION_NAME, "ROTATION",
                                                Kernel::Direction::Input),
                  "Name for the rotation header key.");
  declareProperty(
      new PropertyWithValue<string>(AXIS_NAMES_NAME, "NAXIS1,NAXIS2",
                                    Kernel::Direction::Input),
      "Names for the axis header keys, comma separated string of all axis.");
  declareProperty(new PropertyWithValue<string>(IMAGE_KEY_NAME, "IMAGEKEY",
                                                Kernel::Direction::Input),
                  "Names for the image type, key.");

  declareProperty(
      new FileProperty(HEADER_MAP_NAME, "", FileProperty::OptionalDirectory, "",
                       Kernel::Direction::Input),
      "A file mapping header keys to the ones used by ISIS [line separated "
      "values in the format KEY=VALUE, e.g. BitDepthName=BITPIX ");
}

/**
* Execute the algorithm.
*/
void LoadFITS::exec() {
  // Init header info - setup some defaults just in case
  m_headerScaleKey = "BSCALE";
  m_headerOffsetKey = "BZERO";
  m_headerBitDepthKey = "BITPIX";
  m_headerImageKeyKey = "IMAGEKEY";
  m_headerRotationKey = "ROTATION";
  m_mapFile = "";
  m_headerAxisNameKeys.push_back("NAXIS1");
  m_headerAxisNameKeys.push_back("NAXIS2");

  mapHeaderKeys();

  // Create FITS file information for each file selected
  std::vector<std::string> paths;
  string fName = getPropertyValue("Filename");
  boost::split(paths, fName, boost::is_any_of(","));
  m_baseName = "";
  m_spectraCount = 0;

  // If paths contains a non fits file, assume (for now) that it contains
  // information about the rotations
  std::string rotFilePath = "";
  std::vector<std::string>::iterator it =
      std::find_if(paths.begin(), paths.end(), IsNotFits);
  if (it != paths.end()) {
    rotFilePath = *it;
    paths.erase(it);
  }
  vector<FITSInfo> allHeaderInfo;
  allHeaderInfo.resize(paths.size());

  // Check each header is valid for this loader, - standard (no extension to
  // FITS), and has two axis
  bool headerValid = true;

  for (size_t i = 0; i < paths.size(); ++i) {
    allHeaderInfo[i].extension = "";
    allHeaderInfo[i].filePath = paths[i];
    // Get various pieces of information from the file header which are used to
    // create the workspace
    if (parseHeader(allHeaderInfo[i])) {
      // Get and convert specific standard header values which will help when
      // parsing the data
      // BITPIX, NAXIS, NAXISi (where i = 1..NAXIS, e.g. NAXIS2 for two axis),
      // TOF, TIMEBIN, N_COUNTS, N_TRIGS
      try {
        string tmpBitPix = allHeaderInfo[i].headerKeys[m_headerBitDepthKey];
        if (boost::contains(tmpBitPix, "-")) {
          boost::erase_all(tmpBitPix, "-");
          allHeaderInfo[i].isFloat = true;
        } else {
          allHeaderInfo[i].isFloat = false;
        }

        // Add the image key, use the property if it's not -1, otherwise use the
        // header value
        allHeaderInfo[i].imageKey =
            boost::lexical_cast<int>(getPropertyValue("ImageKey"));
        if (allHeaderInfo[i].imageKey == -1) {
          allHeaderInfo[i].imageKey = boost::lexical_cast<int>(
              allHeaderInfo[i].headerKeys[m_headerImageKeyKey]);
        }

        allHeaderInfo[i].bitsPerPixel = lexical_cast<int>(tmpBitPix);
        allHeaderInfo[i].numberOfAxis =
            static_cast<int>(m_headerAxisNameKeys.size());

        for (int j = 0;
             allHeaderInfo.size() > i && j < allHeaderInfo[i].numberOfAxis;
             ++j) {
          allHeaderInfo[i].axisPixelLengths.push_back(lexical_cast<size_t>(
              allHeaderInfo[i].headerKeys[m_headerAxisNameKeys[j]]));
        }

        // m_allHeaderInfo[i].tof =
        // lexical_cast<double>(m_allHeaderInfo[i].headerKeys["TOF"]);
        // m_allHeaderInfo[i].timeBin =
        // lexical_cast<double>(m_allHeaderInfo[i].headerKeys["TIMEBIN"]);
        // m_allHeaderInfo[i].countsInImage = lexical_cast<long
        // int>(m_allHeaderInfo[i].headerKeys["N_COUNTS"]);
        // m_allHeaderInfo[i].numberOfTriggers = lexical_cast<long
        // int>(m_allHeaderInfo[i].headerKeys["N_TRIGS"]);
        allHeaderInfo[i].extension =
            allHeaderInfo[i].headerKeys["XTENSION"]; // Various extensions are
                                                     // available to the FITS
                                                     // format, and must be
                                                     // parsed differently if
                                                     // this is present. Loader
                                                     // doesn't support this.

      } catch (std::exception &) {
        // todo write error and fail this load with invalid data in file.
        throw std::runtime_error("Unable to locate one or more valid BITPIX, "
                                 "NAXIS or IMAGEKEY values in the FITS file "
                                 "header.");
      }

      allHeaderInfo[i].scale =
          (allHeaderInfo[i].headerKeys[m_headerScaleKey] == "")
              ? 1
              : lexical_cast<double>(
                    allHeaderInfo[i].headerKeys[m_headerScaleKey]);
      allHeaderInfo[i].offset =
          (allHeaderInfo[i].headerKeys[m_headerOffsetKey] == "")
              ? 0
              : lexical_cast<int>(
                    allHeaderInfo[i].headerKeys[m_headerOffsetKey]);

      if (allHeaderInfo[i].extension != "")
        headerValid = false;
      if (allHeaderInfo[i].numberOfAxis != 2)
        headerValid = false;

      // Test current item has same axis values as first item.
      if (allHeaderInfo[0].axisPixelLengths[0] !=
          allHeaderInfo[i].axisPixelLengths[0])
        headerValid = false;
      if (allHeaderInfo[0].axisPixelLengths[1] !=
          allHeaderInfo[i].axisPixelLengths[1])
        headerValid = false;
    } else {
      // Unable to parse the header, throw.
      throw std::runtime_error("Unable to open the FITS file.");
    }
  }

  // Check that the files use bit depths of either 8, 16 or 32
  if (allHeaderInfo[0].bitsPerPixel != 8 &&
      allHeaderInfo[0].bitsPerPixel != 16 &&
      allHeaderInfo[0].bitsPerPixel != 32 &&
      allHeaderInfo[0].bitsPerPixel != 64)
    throw std::runtime_error(
        "FITS loader only supports 8, 16, 32 or 64 bits per pixel.");

  // Check the format is correct and create the Workspace
  if (headerValid) {
    // No extension is set, therefore it's the standard format which we can
    // parse.

    if (allHeaderInfo[0].numberOfAxis > 0)
      m_spectraCount += allHeaderInfo[0].axisPixelLengths[0];

    // Presumably 2 axis, but futureproofing.
    for (int i = 1; i < allHeaderInfo[0].numberOfAxis; ++i) {
      m_spectraCount *= allHeaderInfo[0].axisPixelLengths[i];
    }

    MantidImage imageY(allHeaderInfo[0].axisPixelLengths[0],
                       vector<double>(allHeaderInfo[0].axisPixelLengths[1]));
    MantidImage imageE(allHeaderInfo[0].axisPixelLengths[0],
                       vector<double>(allHeaderInfo[0].axisPixelLengths[1]));
    ;

    void *bufferAny = NULL;
    bufferAny = malloc((allHeaderInfo[0].bitsPerPixel / 8) * m_spectraCount);
    if (bufferAny == NULL) {
      throw std::runtime_error(
          "FITS loader couldn't allocate enough memory to run.");
    }

    // Set info in WS log to hold rotational information
    vector<double> rotations;
    if (rotFilePath != "")
      rotations = readRotations(rotFilePath, paths.size());

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
        wsGroup = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(
            groupName);

      std::string latestName = wsGroup->getNames().back();
      // Set next file number
      fileNumberInGroup = fetchNumber(latestName) + 1;
    }

    // Create a progress reporting object
    m_progress = new Progress(this, 0, 1, allHeaderInfo.size() + 1);

    // Create First workspace with instrument definition, also used as a
    // template for creating others
    Workspace2D_sptr latestWS;
    double rot = (rotations.size() > 0) ? rotations[0] : -1;
    map<size_t, Workspace2D_sptr> wsOrdered;

    latestWS = addWorkspace(allHeaderInfo[0], fileNumberInGroup, bufferAny,
                            imageY, imageE, rot, latestWS);
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
    for (int64_t i = 1; i < static_cast<int64_t>(allHeaderInfo.size()); ++i) {
      double rot =
          (static_cast<int64_t>(rotations.size()) > i) ? rotations[i] : -1;
      latestWS = addWorkspace(allHeaderInfo[i], fileNumberInGroup, bufferAny,
                              imageY, imageE, rot, latestWS);
      wsOrdered[i] = latestWS;
    }

    // Add to group - Done here to maintain order
    for (auto it = wsOrdered.begin(); it != wsOrdered.end(); ++it) {
      wsGroup->addWorkspace(it->second);
    }

    free(bufferAny);

    setProperty("OutputWorkspace", wsGroup);
  } else {
    // Invalid files, record error
    throw std::runtime_error("Loader currently doesn't support FITS files with "
                             "non-standard extensions, greater than two axis "
                             "of data, or has detected that all the files are "
                             "not similar.");
  }
}

/**
 * Initialises a workspace with IDF and fills it with data
 * @param fileInfo information for the current file
 * @param newFileNumber number for the new file when added into ws group
 * @param bufferAny Presized buffer to contain data values
 * @param imageY Object to set the Y data values in
 * @param imageE Object to set the E data values in
 * @param rotation Value for the rotation of the current file
 * @param parent A workspace which can be used to copy initialisation
 * information from (size/instrument def etc)
 * @returns A pointer to the workspace created
 */
Workspace2D_sptr LoadFITS::addWorkspace(const FITSInfo &fileInfo,
                                        size_t &newFileNumber, void *&bufferAny,
                                        MantidImage &imageY,
                                        MantidImage &imageE, double rotation,
                                        const Workspace2D_sptr parent) {
  // Create ws
  Workspace2D_sptr ws;
  if (!parent)
    ws = dynamic_pointer_cast<Workspace2D>(WorkspaceFactory::Instance().create(
        "Workspace2D", m_spectraCount, 2, 1));
  else
    ws = dynamic_pointer_cast<Workspace2D>(
        WorkspaceFactory::Instance().create(parent));

  string currNumberS = padZeros(newFileNumber, DIGIT_SIZE_APPEND);
  ++newFileNumber;

  string baseName = m_baseName + currNumberS;

  ws->setTitle(baseName);

  // set data
  readFileToWorkspace(ws, fileInfo, imageY, imageE, bufferAny);

  // Add all header info to log.
  for (auto it = fileInfo.headerKeys.begin(); it != fileInfo.headerKeys.end();
       ++it) {
    ws->mutableRun().removeLogData("_" + it->first, true);
    ws->mutableRun().addLogData(
        new PropertyWithValue<string>("_" + it->first, it->second));
  }

  // Add rotational data to log. Clear first from copied WS
  ws->mutableRun().removeLogData("Rotation", true);
  if (rotation != -1)
    ws->mutableRun().addLogData(
        new PropertyWithValue<double>("Rotation", rotation));

  // Add axis information to log. Clear first from copied WS
  ws->mutableRun().removeLogData("Axis1", true);
  ws->mutableRun().addLogData(new PropertyWithValue<int>(
      "Axis1", static_cast<int>(fileInfo.axisPixelLengths[0])));
  ws->mutableRun().removeLogData("Axis2", true);
  ws->mutableRun().addLogData(new PropertyWithValue<int>(
      "Axis2", static_cast<int>(fileInfo.axisPixelLengths[1])));

  // Add image key data to log. Clear first from copied WS
  ws->mutableRun().removeLogData("ImageKey", true);
  ws->mutableRun().addLogData(new PropertyWithValue<int>(
      "ImageKey", static_cast<int>(fileInfo.imageKey)));

  m_progress->report();

  return ws;
}

/**
 * Returns the trailing number from a string minus leading 0's (so 25 from
 * workspace_00025)the confidence with with this algorithm can load the file
 * @param name string with a numerical suffix
 * @returns A numerical representation of the string minus leading characters
 * and leading 0's
 */
size_t LoadFITS::fetchNumber(std::string name) {
  string tmpStr = "";
  for (auto it = name.end() - 1; isdigit(*it); --it) {
    tmpStr.insert(0, 1, *it);
  }
  while (tmpStr.length() > 0 && tmpStr[0] == '0') {
    tmpStr.erase(tmpStr.begin());
  }
  return (tmpStr.length() > 0) ? lexical_cast<size_t>(tmpStr) : 0;
}

// Adds 0's to the front of a number to create a string of size totalDigitCount
// including number
std::string LoadFITS::padZeros(size_t number, size_t totalDigitCount) {
  std::ostringstream ss;
  ss << std::setw(static_cast<int>(totalDigitCount)) << std::setfill('0')
     << static_cast<int>(number);

  return ss.str();
}

/**
 * Reads the data from a single FITS file into a workspace
 * @param ws Workspace to populate with the data
 * @param fileInfo information pertaining to the FITS file to load
 * @param imageY Object to set the Y data values in
 * @param imageE Object to set the E data values in
 * @param bufferAny Presized buffer to contain data values
 */
void LoadFITS::readFileToWorkspace(Workspace2D_sptr ws,
                                   const FITSInfo &fileInfo,
                                   MantidImage &imageY, MantidImage &imageE,
                                   void *&bufferAny) {
  uint8_t *buffer8 = NULL;

  // create pointer of correct data type to void pointer of the buffer:
  buffer8 = static_cast<uint8_t *>(bufferAny);

  // Read Data
  bool fileErr = false;
  FILE *currFile = fopen(fileInfo.filePath.c_str(), "rb");
  if (currFile == NULL)
    fileErr = true;

  size_t result = 0;
  if (!fileErr) {
    if (fseek(currFile, BASE_HEADER_SIZE * fileInfo.headerSizeMultiplier,
              SEEK_CUR) == 0)
      result = fread(buffer8, 1, m_spectraCount * (fileInfo.bitsPerPixel / 8),
                     currFile);
  }

  if (result != m_spectraCount * (fileInfo.bitsPerPixel / 8))
    fileErr = true;

  if (fileErr)
    throw std::runtime_error("Error reading file; possibly invalid data.");

  char *tmp = new char[fileInfo.bitsPerPixel / 8];

  for (size_t i = 0; i < fileInfo.axisPixelLengths[0]; ++i) {
    for (size_t j = 0; j < fileInfo.axisPixelLengths[1]; ++j) {
      double val = 0;
      size_t start =
          ((i * (fileInfo.bitsPerPixel / 8)) * fileInfo.axisPixelLengths[1]) +
          (j * (fileInfo.bitsPerPixel / 8));

      // Reverse byte order of current value
      std::reverse_copy(buffer8 + start,
                        buffer8 + start + (fileInfo.bitsPerPixel / 8), tmp);

      if (fileInfo.bitsPerPixel == 8)
        val = static_cast<double>(*reinterpret_cast<uint8_t *>(tmp));
      if (fileInfo.bitsPerPixel == 16)
        val = static_cast<double>(*reinterpret_cast<uint16_t *>(tmp));
      if (fileInfo.bitsPerPixel == 32 && !fileInfo.isFloat)
        val = static_cast<double>(*reinterpret_cast<uint32_t *>(tmp));
      if (fileInfo.bitsPerPixel == 64 && !fileInfo.isFloat)
        val = static_cast<double>(*reinterpret_cast<uint64_t *>(tmp));

      // cppcheck doesn't realise that these are safe casts
      // cppcheck-suppress invalidPointerCast
      if (fileInfo.bitsPerPixel == 32 && fileInfo.isFloat)
        val = static_cast<double>(*reinterpret_cast<float *>(tmp));
      // cppcheck-suppress invalidPointerCast
      if (fileInfo.bitsPerPixel == 64 && fileInfo.isFloat)
        val = *reinterpret_cast<double *>(tmp);

      val = fileInfo.scale * val - fileInfo.offset;

      imageY[i][j] = val;
      imageE[i][j] = sqrt(val);
    }
  }

  // Set in WS
  ws->setImageYAndE(imageY, imageE, 0, false);

  // Clear memory associated with the file load
  fclose(currFile);
}

/**
* Read a single files header and populate an object with the information
* @param headerInfo A FITSInfo file object to parse header information into
* @returns A bool specifying succes of the operation
*/
bool LoadFITS::parseHeader(FITSInfo &headerInfo) {
  bool ranSuccessfully = true;
  headerInfo.headerSizeMultiplier = 0;
  try {
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
  } catch (...) {
    // Unable to read the file
    ranSuccessfully = false;
  }

  return ranSuccessfully;
}

/**
 * Reads a file containing rotation values for each image into a vector of
 *doubles
 * @param rotFilePath The path to a file containing rotation values
 * @param fileCount number of images which should have corresponding rotation
 *values in the file
 *
 * @returns vector<double> A vector of all the rotation values
 */
std::vector<double> LoadFITS::readRotations(std::string rotFilePath,
                                            size_t fileCount) {
  std::vector<double> allRotations;
  ifstream fStream(rotFilePath.c_str());

  try {
    // Ensure valid file
    if (fStream.good()) {
      // Get lines, split words, verify and add to map.
      string line;
      vector<string> lineSplit;
      size_t ind = -1;
      while (getline(fStream, line)) {
        ind++;
        boost::split(lineSplit, line, boost::is_any_of("\t"));

        if (ind == 0 || lineSplit[0] == "")
          continue; // Skip first iteration or where rotation value is empty

        allRotations.push_back(lexical_cast<double>(lineSplit[1]));
      }

      // Check the number of rotations in file matches number of files
      if (ind != fileCount)
        throw std::runtime_error("File error, throw higher up.");

      fStream.close();
    } else {
      throw std::runtime_error("File error, throw higher up.");
    }
  } catch (...) {
    throw std::runtime_error("Invalid file path or file format: Expected a "
                             "file with a line separated list of rotations "
                             "with the same number of entries as other files.");
  }

  return allRotations;
}

/**
 *  Maps the header keys to specified values
 */
void LoadFITS::mapHeaderKeys() {
  bool useProperties = true;

  // If a map file is selected, use that.
  if (getPropertyValue(HEADER_MAP_NAME) != "") {
    // std::vector<double> allRotations;
    ifstream fStream(getPropertyValue(HEADER_MAP_NAME).c_str());

    try {
      // Ensure valid file
      if (fStream.good()) {
        // Get lines, split words, verify and add to map.
        string line;
        vector<string> lineSplit;
        while (getline(fStream, line)) {
          boost::split(lineSplit, line, boost::is_any_of("="));

          if (lineSplit[0] == ROTATION_NAME && lineSplit[1] != "")
            m_headerRotationKey = lineSplit[1];

          if (lineSplit[0] == BIT_DEPTH_NAME && lineSplit[1] != "")
            m_headerBitDepthKey = lineSplit[1];

          if (lineSplit[0] == AXIS_NAMES_NAME && lineSplit[1] != "") {
            m_headerAxisNameKeys.clear();
            std::string propVal = getProperty(AXIS_NAMES_NAME);
            boost::split(m_headerAxisNameKeys, propVal, boost::is_any_of(","));
          }

          if (lineSplit[0] == IMAGE_KEY_NAME && lineSplit[1] != "") {
            m_headerImageKeyKey = lineSplit[1];
          }
        }

        fStream.close();
        useProperties = false;
      } else {
        throw std::runtime_error("File error, throw higher up.");
      }
    } catch (...) {
      g_log.information("Cannot load specified map file, using property values "
                        "and/or defaults.");
      useProperties = true;
    }
  }

  if (useProperties) {
    // Try and set from the loader properties if present and didn't load map
    // file
    if (getPropertyValue(BIT_DEPTH_NAME) != "")
      m_headerBitDepthKey = getPropertyValue(BIT_DEPTH_NAME);
    if (getPropertyValue(ROTATION_NAME) != "")
      m_headerRotationKey = getPropertyValue(ROTATION_NAME);
    if (getPropertyValue(AXIS_NAMES_NAME) != "") {
      m_headerAxisNameKeys.clear();
      std::string propVal = getProperty(AXIS_NAMES_NAME);
      boost::split(m_headerAxisNameKeys, propVal, boost::is_any_of(","));
    }
    if (getPropertyValue(IMAGE_KEY_NAME) != "")
      m_headerImageKeyKey = getPropertyValue(IMAGE_KEY_NAME);
  }
}
}
}
