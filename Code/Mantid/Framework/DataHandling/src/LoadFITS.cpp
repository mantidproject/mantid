#include "MantidAPI/FileProperty.h"
#include "MantidAPI/MultipleFileProperty.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidAPI/RegisterFileLoader.h"
#include "MantidDataHandling/LoadFITS.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/BoundedValidator.h"
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
      m_mapFile(), m_baseName(), m_pixelCount(0), m_rebin(1),
      m_noiseThresh(0.0), m_progress(NULL) {
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
                  "The name of the input file (note that you can give "
                  "multiple file names separated by commas).");

  declareProperty(new API::WorkspaceProperty<API::Workspace>(
      "OutputWorkspace", "", Kernel::Direction::Output));

  declareProperty(
      new Kernel::PropertyWithValue<bool>("LoadAsRectImg", false,
                                          Kernel::Direction::Input),
      "If enabled (not by default), the output Workspace2D will have "
      "one histogram per row and one bin per pixel, such that a 2D "
      "color plot (color fill plot) will display an image.");

  auto zeroOrPosDbl = boost::make_shared<BoundedValidator<double>>();
  zeroOrPosDbl->setLower(0.0);
  declareProperty("FilterNoiseLevel", 0.0, zeroOrPosDbl,
                  "Threshold to remove noisy pixels. Try 50 for example.");

  auto posInt = boost::make_shared<BoundedValidator<int>>();
  posInt->setLower(1);
  declareProperty("BinSize", 1, posInt,
                  "Rebunch n*n on both axes, generating pixels with sums of "
                  "blocks of n by n original pixels.");

  auto posDbl = boost::make_shared<BoundedValidator<double>>();
  posDbl->setLower(std::numeric_limits<double>::epsilon());
  declareProperty("Scale", 80.0, posDbl, "Pixels per cm.",
                  Kernel::Direction::Input);

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

  std::string fName = getPropertyValue("Filename");
  std::vector<std::string> paths;
  boost::split(paths, fName, boost::is_any_of(","));

  m_rebin = getProperty("BinSize");
  m_noiseThresh = getProperty("FilterNoiseLevel");
  bool loadAsRectImg = getProperty("LoadAsRectImg");
  const std::string outWSName = getPropertyValue("OutputWorkspace");

  doLoadFiles(paths, outWSName, loadAsRectImg);
}

/**
 * Create FITS file information for each file selected. Loads headers
 * and data from the files and creates and fills the output
 * workspace(s).
 *
 * @param paths File names as given in the algorithm input property
 *
 * @param outWSName name of the output (group) workspace to create
 *
 * @param loadAsRectImg Load files with 1 spectrum per row and 1 bin
 * per column, so a color fill plot displays the image
 *
 * @throw std::runtime_error when load fails (for example a memory
 * allocation issue, wrong rebin requested, etc.)
 */
void LoadFITS::doLoadFiles(const std::vector<std::string> &paths,
                           const std::string &outWSName, bool loadAsRectImg) {
  std::vector<FITSInfo> headers;

  doLoadHeaders(paths, headers);

  // No extension is set -> it's the standard format which we can parse.
  if (headers[0].numberOfAxis > 0)
    m_pixelCount += headers[0].axisPixelLengths[0];

  // Presumably 2 axis, but futureproofing.
  for (int i = 1; i < headers[0].numberOfAxis; ++i) {
    m_pixelCount *= headers[0].axisPixelLengths[i];
  }

  // Check consistency of m_rebin asap
  for (int i = 0; i < headers[0].numberOfAxis; ++i) {
    if (0 != (headers[0].axisPixelLengths[i] % m_rebin)) {
      throw std::runtime_error(
          "Cannot rebin this image in blocks of " +
          boost::lexical_cast<std::string>(m_rebin) + " x " +
          boost::lexical_cast<std::string>(m_rebin) +
          " pixels as requested because the size of dimension " +
          boost::lexical_cast<std::string>(i + 1) + " (" +
          boost::lexical_cast<std::string>(headers[0].axisPixelLengths[i]) +
          ") is not a multiple of the bin size.");
    }
  }

  MantidImage imageY(headers[0].axisPixelLengths[1],
                     vector<double>(headers[0].axisPixelLengths[0]));
  MantidImage imageE(headers[0].axisPixelLengths[1],
                     vector<double>(headers[0].axisPixelLengths[0]));

  size_t bytes = (headers[0].bitsPerPixel / 8) * m_pixelCount;
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
  string groupName = outWSName;

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
                           imageE, latestWS, loadAsRectImg);

  std::map<size_t, Workspace2D_sptr> wsOrdered;
  wsOrdered[0] = latestWS;

  if (isInstrOtherThanIMAT(headers[0])) {
    // For now we assume IMAT except when specific headers are found by
    // isInstrOtherThanIMAT()
    //
    // TODO: do this conditional on INSTR='IMAT' when we have proper IMAT .fits
    // files
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
  }

  PARALLEL_FOR_NO_WSP_CHECK()
  for (int64_t i = 1; i < static_cast<int64_t>(headers.size()); ++i) {
    latestWS = makeWorkspace(headers[i], fileNumberInGroup, buffer, imageY,
                             imageE, latestWS, loadAsRectImg);
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
    // Get various pieces of information from the file header which are used
    // to
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
        g_log.information()
            << "Found axis length header entry: " << m_headerAxisNameKeys[j]
            << " = " << headers[i].axisPixelLengths.back() << std::endl;
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
            m_headerBitDepthKey + " = " +
            headers[i].headerKeys[m_headerScaleKey] +
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
        // still, second try with floating point format (as used for example
        // by
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
                       m_headerOffsetKey + " = " +
                       headers[i].headerKeys[m_headerOffsetKey] +
                       ") has a fractional part, and it will be ignored!"
                << std::endl;
          }
          headers[i].offset = static_cast<int>(doff);
        } catch (std::exception &e) {
          throw std::runtime_error(
              "Coult not interpret the entry number of data offset (" +
              m_headerOffsetKey + " = " +
              headers[i].headerKeys[m_headerOffsetKey] +
              ") as an integer number nor a floating point "
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
 *
 * @param parent A workspace which can be used to copy initialisation
 * information from (size/instrument def etc)
 *
 * @param loadAsRectImg if true, the new workspace will have one
 * spectrum per row and one bin per column, instead of the (default)
 * as many spectra as pixels.
 *
 * @returns A newly created Workspace2D, as a shared pointer
 */
Workspace2D_sptr
LoadFITS::makeWorkspace(const FITSInfo &fileInfo, size_t &newFileNumber,
                        std::vector<char> &buffer, MantidImage &imageY,
                        MantidImage &imageE, const Workspace2D_sptr parent,
                        bool loadAsRectImg) {

  string currNumberS = padZeros(newFileNumber, DIGIT_SIZE_APPEND);
  ++newFileNumber;

  string baseName = m_baseName + currNumberS;

  // set data
  readDataToImgs(fileInfo, imageY, imageE, buffer);

  // Create ws
  Workspace2D_sptr ws;
  if (!parent) {
    if (!loadAsRectImg) {
      size_t finalPixelCount = m_pixelCount / m_rebin * m_rebin;
      ws =
          dynamic_pointer_cast<Workspace2D>(WorkspaceFactory::Instance().create(
              "Workspace2D", finalPixelCount, 2, 1));
    } else {
      ws =
          dynamic_pointer_cast<Workspace2D>(WorkspaceFactory::Instance().create(
              "Workspace2D",
              fileInfo.axisPixelLengths[1] / m_rebin, // one bin per column
              fileInfo.axisPixelLengths[0] / m_rebin +
                  1, // one spectrum per row
              fileInfo.axisPixelLengths[0] / m_rebin));
    }
  } else {
    ws = dynamic_pointer_cast<Workspace2D>(
        WorkspaceFactory::Instance().create(parent));
  }

  doFilterNoise(m_noiseThresh, imageY, imageE);

  // this pixel scale property is used to set the workspace X values
  double cm_1 = getProperty("Scale");
  // amount of width units (e.g. cm) per pixel
  double cmpp = 1; // cm per pixel == bin width
  if (0.0 != cm_1)
    cmpp /= cm_1;
  cmpp *= static_cast<double>(m_rebin);

  // Set in WS
  // Note this can change the sizes of the images and the number of pixels
  if (1 == m_rebin) {
    ws->setImageYAndE(imageY, imageE, 0, loadAsRectImg, cmpp,
                      false /* no parallel load */);
  } else {
    MantidImage rebinnedY(imageY.size() / m_rebin,
                          std::vector<double>(imageY[0].size() / m_rebin));
    MantidImage rebinnedE(imageE.size() / m_rebin,
                          std::vector<double>(imageE[0].size() / m_rebin));

    doRebin(m_rebin, imageY, imageE, rebinnedY, rebinnedE);
    ws->setImageYAndE(rebinnedY, rebinnedE, 0, loadAsRectImg, cmpp,
                      false /* no parallel load */);
  }

  ws->setTitle(baseName);

  // add axes
  size_t width = fileInfo.axisPixelLengths[0] / m_rebin;
  size_t height = fileInfo.axisPixelLengths[1] / m_rebin;
  if (loadAsRectImg) {
    // width/X axis
    auto axw = new Mantid::API::NumericAxis(width + 1);
    axw->title() = "width";
    for (size_t i = 0; i < width + 1; i++) {
      axw->setValue(i, static_cast<double>(i) * cmpp);
    }
    ws->replaceAxis(0, axw);
    // "cm" width label unit
    boost::shared_ptr<Kernel::Units::Label> unitLbl =
        boost::dynamic_pointer_cast<Kernel::Units::Label>(
            UnitFactory::Instance().create("Label"));
    unitLbl->setLabel("width", "cm");
    ws->getAxis(0)->unit() = unitLbl;

    // height/Y axis
    auto axh = new Mantid::API::NumericAxis(height);
    axh->title() = "height";
    for (size_t i = 0; i < height; i++) {
      axh->setValue(i, static_cast<double>(i) * cmpp);
    }
    ws->replaceAxis(1, axh);
    // "cm" height label unit
    unitLbl = boost::dynamic_pointer_cast<Kernel::Units::Label>(
        UnitFactory::Instance().create("Label"));
    unitLbl->setLabel("height", "cm");
    ws->getAxis(1)->unit() = unitLbl;

    ws->isDistribution(true);
  } else {
    // TODO: what to do when loading 1pixel - 1 spectrum?
  }
  ws->setYUnitLabel("brightness");

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
 * Reads the data (matrix) from a single FITS file into image objects.
 *
 * @param fileInfo information on the FITS file to load, including its path
 * @param imageY Object to set the Y data values in
 * @param imageE Object to set the E data values in
 * @param buffer pre-allocated buffer to contain data values
 *
 * @throws std::runtime_error if there are file input issues
 */
void LoadFITS::readDataToImgs(const FITSInfo &fileInfo, MantidImage &imageY,
                              MantidImage &imageE, std::vector<char> &buffer) {
  std::string filename = fileInfo.filePath;
  Poco::FileStream file(filename, std::ios::in);

  size_t bytespp = (fileInfo.bitsPerPixel / 8);
  size_t len = m_pixelCount * bytespp;
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
  for (size_t i = 0; i < fileInfo.axisPixelLengths[1]; ++i) {   // width
    for (size_t j = 0; j < fileInfo.axisPixelLengths[0]; ++j) { // height
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
}

/**
 * Apply a simple noise filter by averaging threshold-filtered
 * neighbor pixels (with 4-neighbohood / 4-connectivity). The
 * filtering is done in place for both imageY and imageE.
 *
 * @param thresh Threshold to apply on pixels
 * @param imageY raw data (Y values)
 * @param imageE raw data (E/error values)
 */
void LoadFITS::doFilterNoise(double thresh, MantidImage &imageY,
                             MantidImage &imageE) {
  if (thresh <= 0.0)
    return;

  MantidImage goodY = imageY;
  MantidImage goodE = imageE;

  // TODO: this is not very smart about the edge pixels (leftmost and
  // rightmost columns, topmost and bottom rows)
  for (size_t j = 1; j < (imageY.size() - 1); ++j) {
    for (size_t i = 1; i < (imageY[0].size() - 1); ++i) {

      if (((imageY[j][i] - imageY[j][i - 1]) > thresh) &&
          ((imageY[j][i] - imageY[j][i + 1]) > thresh) &&
          ((imageY[j][i] - imageY[j - 1][i]) > thresh) &&
          ((imageY[j][i] - imageY[j + 1][i]) > thresh))
        goodY[j][i] = 0;
      else
        goodY[j][i] = 1;

      if (((imageE[j][i] - imageE[j][i - 1]) > thresh) &&
          ((imageE[j][i] - imageE[j][i + 1]) > thresh) &&
          ((imageE[j][i] - imageE[j - 1][i]) > thresh) &&
          ((imageE[j][i] - imageE[j + 1][i]) > thresh))
        goodE[j][i] = 0;
      else
        goodE[j][i] = 1;
    }
  }

  for (size_t j = 1; j < (imageY.size() - 1); ++j) {
    for (size_t i = 1; i < (imageY[0].size() - 1); ++i) {
      if (!goodY[j][i]) {
        if (goodY[j - 1][i] || goodY[j + 1][i] || goodY[j][i - 1] ||
            goodY[j][i + 1]) {
          imageY[j][i] = goodY[j - 1][i] * imageY[j - 1][i] +
                         goodY[j + 1][i] * imageY[j + 1][i] +
                         goodY[j][i - 1] * imageY[j][i - 1] +
                         goodY[j][i + 1] * imageY[j][i + 1];
        }
      }

      if (!goodE[j][i]) {
        if (goodE[j - 1][i] || goodE[j + 1][i] || goodE[j][i - 1] ||
            goodE[j][i + 1]) {
          imageE[j][i] = goodE[j - 1][i] * imageE[j - 1][i] +
                         goodE[j + 1][i] * imageE[j + 1][i] +
                         goodE[j][i - 1] * imageE[j][i - 1] +
                         goodE[j][i + 1] * imageE[j][i + 1];
        }
      }
    }
  }
}

/**
 * Group pixels in blocks of rebin X rebin.
 *
 * @param rebin bin size (n to make blocks of n*n pixels)
 * @param imageY raw data (Y values)
 * @param imageE raw data (E/error values)
 * @param rebinnedY raw data after rebin (Y values)
 * @param rebinnedE raw data after rebin (E/error values)
 */
void LoadFITS::doRebin(size_t rebin, MantidImage &imageY, MantidImage &imageE,
                       MantidImage &rebinnedY, MantidImage &rebinnedE) {
  if (1 >= rebin)
    return;

  for (size_t j = 0; j < (rebinnedY.size() - rebin + 1); ++j) {
    for (size_t i = 0; i < (rebinnedY[0].size() - rebin + 1); ++i) {
      double accumY = 0.0;
      double accumE = 0.0;
      size_t origJ = j*rebin;
      size_t origI = i*rebin;
      for (size_t k = 0; k < rebin; ++k) {
        for (size_t l = 0; l < rebin; ++l) {
          accumY += imageY[origJ + k][origI + l];
          accumE += imageE[origJ + k][origI + l];
        }
      }
      rebinnedY[j][i] = accumY;
      rebinnedE[j][i] = accumE;
    }
  }
}

/**
 * Checks that a FITS header (once loaded) is valid/supported:
 * standard (no extension to FITS), and has two axis with the expected
 * dimensions.
 *
 * @param hdr FITS header struct loaded from a file - to check
 *
 * @param hdrFirst FITS header struct loaded from a (first) reference file -
 *to
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
 * Looks for headers used by specific instruments/cameras, or finds if
 * the instrument does not appear to be IMAT, which is the only one
 * for which we have a camera-instrument definition and because of
 * that is the only one loaded for the moment.
 *
 * @param hdr FITS header information
 *
 * @return whether this file seems to come from 'another' camera such
 * as Starlight Xpress, etc.
 */
bool LoadFITS::isInstrOtherThanIMAT(FITSInfo &hdr) {
  bool res = false;

  // Images taken with Starlight camera contain this header entry:
  // INSTRUME='Starlight Xpress CCD'
  auto it = hdr.headerKeys.find("INSTRUME");
  if (hdr.headerKeys.end() != it && boost::contains(it->second, "Starlight")) {
    // For now, do nothing, just tell
    // Cameras used for HiFi and EMU are in principle only used
    // occasionally for calibration
    g_log.information()
        << "Found this in the file headers: " << it->first << " = "
        << it->second
        << ". This file seems to come from a Starlight camera, "
           "as used for calibration of the instruments HiFi and EMU (and"
           "possibly others). Not "
           "loading instrument definition." << std::endl;
  }

  return res;
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
 * Adds 0's to the front of a number to create a string of size
 * totalDigitCount including number
 *
 * @param number input number to add padding to
 *
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
