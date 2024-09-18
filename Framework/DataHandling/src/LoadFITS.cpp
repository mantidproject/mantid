// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/LoadFITS.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/MultipleFileProperty.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidAPI/RegisterFileLoader.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/Unit.h"
#include "MantidKernel/UnitFactory.h"

#include <boost/algorithm/string.hpp>
#include <boost/scoped_array.hpp>

#include <Poco/BinaryReader.h>
#include <Poco/Path.h>

using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;
using namespace Mantid::API;
using namespace Mantid::Kernel;

namespace {

/**
 * Reinterpret a byte sequence as InterpretType and cast to double
 * @param Pointer to byte src
 */
template <typename InterpretType> double toDouble(uint8_t *src) {
  // cppcheck-suppress invalidPointerCast
  return static_cast<double>(*reinterpret_cast<InterpretType *>(src));
}
} // namespace

namespace Mantid::DataHandling {

// Register the algorithm into the AlgorithmFactory
DECLARE_FILELOADER_ALGORITHM(LoadFITS)

struct FITSInfo {
  std::vector<std::string> headerItems;
  std::map<std::string, std::string> headerKeys;
  int bitsPerPixel;
  int numberOfAxis;
  int offset;
  int headerSizeMultiplier;
  std::vector<size_t> axisPixelLengths;
  double scale;
  std::string imageKey;
  std::string extension;
  std::string filePath;
  bool isFloat;
};

// Static class constants
const std::string LoadFITS::g_END_KEYNAME = "END";
const std::string LoadFITS::g_COMMENT_KEYNAME = "COMMENT";
const std::string LoadFITS::g_XTENSION_KEYNAME = "XTENSION";
const std::string LoadFITS::g_BIT_DEPTH_NAME = "BitDepthName";
const std::string LoadFITS::g_ROTATION_NAME = "RotationName";
const std::string LoadFITS::g_AXIS_NAMES_NAME = "AxisNames";
const std::string LoadFITS::g_IMAGE_KEY_NAME = "ImageKeyName";
const std::string LoadFITS::g_HEADER_MAP_NAME = "HeaderMapFile";
const std::string LoadFITS::g_defaultImgType = "SAMPLE";

// Bits/bytes per pixel. Values fixed in the FITS standard
const size_t LoadFITS::g_maxBitDepth = 64;
const size_t LoadFITS::g_maxBytesPP = g_maxBitDepth / 8;

/**
 * Constructor. Just initialize everything to prevent issues.
 */
LoadFITS::LoadFITS()
    : m_headerScaleKey(), m_headerOffsetKey(), m_headerBitDepthKey(), m_headerRotationKey(), m_headerImageKeyKey(),
      m_headerAxisNameKeys(), m_mapFile(), m_pixelCount(0) {
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
  return (descriptor.extension() == ".fits" || descriptor.extension() == ".fit") ? 80 : 0;
}

/**
 * Initialise the algorithm. Declare properties which can be set before
 * execution (input) or read from after the execution (output).
 */
void LoadFITS::init() {
  // Specify file extensions which can be associated with a FITS file.
  std::vector<std::string> exts, exts2;

  // Declare the Filename algorithm property. Mandatory. Sets the path to the
  // file to load.
  exts.clear();
  exts.emplace_back(".fits");
  exts.emplace_back(".fit");

  exts2.emplace_back(".*");

  declareProperty(std::make_unique<MultipleFileProperty>("Filename", exts),
                  "The name of the input file (note that you can give "
                  "multiple file names separated by commas).");

  declareProperty(
      std::make_unique<API::WorkspaceProperty<API::Workspace>>("OutputWorkspace", "", Kernel::Direction::Output));

  declareProperty(std::make_unique<Kernel::PropertyWithValue<bool>>("LoadAsRectImg", false, Kernel::Direction::Input),
                  "If enabled (not by default), the output Workspace2D will have "
                  "one histogram per row and one bin per pixel, such that a 2D "
                  "color plot (color fill plot) will display an image.");

  auto zeroOrPosDbl = std::make_shared<BoundedValidator<double>>();
  zeroOrPosDbl->setLower(0.0);
  declareProperty("FilterNoiseLevel", 0.0, zeroOrPosDbl, "Threshold to remove noisy pixels. Try 50 for example.");

  auto posInt = std::make_shared<BoundedValidator<int>>();
  posInt->setLower(1);
  declareProperty("BinSize", 1, posInt,
                  "Rebunch n*n on both axes, generating pixels with sums of "
                  "blocks of n by n original pixels.");

  auto posDbl = std::make_shared<BoundedValidator<double>>();
  posDbl->setLower(std::numeric_limits<double>::epsilon());
  declareProperty("Scale", 80.0, posDbl, "Pixels per cm.", Kernel::Direction::Input);

  declareProperty(std::make_unique<FileProperty>(g_HEADER_MAP_NAME, "", FileProperty::OptionalDirectory, "",
                                                 Kernel::Direction::Input),
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

  int binSize = getProperty("BinSize");
  double noiseThresh = getProperty("FilterNoiseLevel");
  bool loadAsRectImg = getProperty("LoadAsRectImg");
  const std::string outWSName = getPropertyValue("OutputWorkspace");

  doLoadFiles(paths, outWSName, loadAsRectImg, binSize, noiseThresh);
}

void LoadFITS::loadHeader(const std::string &filePath, FITSInfo &header) {
  header.extension = "";
  header.filePath = filePath;

  // Get various pieces of information from the file header which are used
  // to
  // create the workspace
  try {
    parseHeader(header);
  } catch (std::exception &e) {
    // Unable to parse the header, throw.
    throw std::runtime_error("Severe problem found while parsing the header of "
                             "this FITS file (" +
                             filePath + "). This file may not be standard FITS. Error description: " + e.what());
  }

  // Get and convert specific MANDATORY standard header values which
  // will are needed to know how to load the data: BITPIX, NAXIS,
  // NAXISi (where i = 1..NAXIS, e.g. NAXIS2 for two axis).
  try {
    std::string tmpBitPix = header.headerKeys[m_headerBitDepthKey];
    if (boost::contains(tmpBitPix, "-")) {
      boost::erase_all(tmpBitPix, "-");
      header.isFloat = true;
    } else {
      header.isFloat = false;
    }

    try {
      header.bitsPerPixel = boost::lexical_cast<int>(tmpBitPix);
    } catch (std::exception &e) {
      throw std::runtime_error("Coult not interpret the entry number of bits per pixel (" + m_headerBitDepthKey +
                               ") as an integer. Error: " + e.what());
    }
    // Check that the files use valid BITPIX values
    // http://archive.stsci.edu/fits/fits_standard/node39.html#SECTION00941000000000000000
    if (header.bitsPerPixel != 8 && header.bitsPerPixel != 16 && header.bitsPerPixel != 32 && header.bitsPerPixel != 64)
      // this implicitly includes when 'header.bitsPerPixel >
      // g_maxBitDepth'
      throw std::runtime_error("This algorithm only supports 8, 16, 32 or 64 "
                               "bits per pixel as allowed in the FITS standard. The header of "
                               "file '" +
                               filePath + "' says that its bit depth is: " + std::to_string(header.bitsPerPixel));
  } catch (std::exception &e) {
    throw std::runtime_error("Failed to process the '" + m_headerBitDepthKey +
                             "' entry (bits per pixel) in the header of this file: " + filePath +
                             ". Error description: " + e.what());
  }

  try {
    // Add the image key, use the value in the FITS header if found,
    // otherwise default (to SAMPLE).
    auto it = header.headerKeys.find(m_headerImageKeyKey);
    if (header.headerKeys.end() != it) {
      header.imageKey = it->second;
    } else {
      header.imageKey = g_defaultImgType;
    }
  } catch (std::exception &e) {
    throw std::runtime_error("Failed to process the '" + m_headerImageKeyKey +
                             "' entry (type of image: sample, dark, open) in "
                             "the header of this file: " +
                             filePath + ". Error description: " + e.what());
  }

  try {
    header.numberOfAxis = static_cast<int>(m_headerAxisNameKeys.size());

    for (int j = 0; j < header.numberOfAxis; ++j) {
      header.axisPixelLengths.emplace_back(boost::lexical_cast<size_t>(header.headerKeys[m_headerAxisNameKeys[j]]));
      // only debug level, when loading multiple files this is very verbose
      g_log.debug() << "Found axis length header entry: " << m_headerAxisNameKeys[j] << " = "
                    << header.axisPixelLengths.back() << '\n';
    }

    // Various extensions to the FITS format are used elsewhere, and
    // must be parsed differently if used. This loader Loader
    // doesn't support this.
    header.extension = header.headerKeys[g_XTENSION_KEYNAME];
  } catch (std::exception &e) {
    throw std::runtime_error("Failed to process the '" + m_headerNAxisNameKey +
                             "' entries (dimensions) in the header of this file: " + filePath +
                             ". Error description: " + e.what());
  }

  // scale parameter, header BSCALE in the fits standard
  if (header.headerKeys[m_headerScaleKey].empty()) {
    header.scale = 1;
  } else {
    try {
      header.scale = boost::lexical_cast<double>(header.headerKeys[m_headerScaleKey]);
    } catch (std::exception &e) {
      throw std::runtime_error("Coult not interpret the entry number of bits per pixel (" + m_headerBitDepthKey +
                               " = " + header.headerKeys[m_headerScaleKey] +
                               ") as a floating point number (double). Error: " + e.what());
    }
  }

  // data offsset parameter, header BZERO in the fits standard
  if (header.headerKeys[m_headerOffsetKey].empty()) {
    header.offset = 0;
  } else {
    try {
      header.offset = boost::lexical_cast<int>(header.headerKeys[m_headerOffsetKey]);
    } catch (std::exception & /*e*/) {
      // still, second try with floating point format (as used for example
      // by
      // Starlight XPRESS cameras)
      try {
        auto doff = boost::lexical_cast<double>(header.headerKeys[m_headerOffsetKey]);
        double intPart;
        if (0 != modf(doff, &intPart)) {
          // anyway we'll do a cast, but warn if there was a fraction
          g_log.warning() << "The value given in the FITS header entry for the data "
                             "offset (" +
                                 m_headerOffsetKey + " = " + header.headerKeys[m_headerOffsetKey] +
                                 ") has a fractional part, and it will be ignored!\n";
        }
        header.offset = static_cast<int>(doff);
      } catch (std::exception &e) {
        throw std::runtime_error("Coult not interpret the entry number of data offset (" + m_headerOffsetKey + " = " +
                                 header.headerKeys[m_headerOffsetKey] +
                                 ") as an integer number nor a floating point "
                                 "number (double). Error: " +
                                 e.what());
      }
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
void LoadFITS::headerSanityCheck(const FITSInfo &hdr, const FITSInfo &hdrFirst) {
  bool valid = true;
  if (!hdr.extension.empty()) {
    valid = false;
    g_log.error() << "File " << hdr.filePath << ": extensions found in the header.\n";
  }
  if (hdr.numberOfAxis != 2) {
    valid = false;
    g_log.error() << "File " << hdr.filePath << ": the number of axes is not 2 but: " << hdr.numberOfAxis << '\n';
  }

  // Test current item has same axis values as first item.
  if (hdr.axisPixelLengths[0] != hdrFirst.axisPixelLengths[0]) {
    valid = false;
    g_log.error() << "File " << hdr.filePath
                  << ": the number of pixels in the first dimension differs "
                     "from the first file loaded ("
                  << hdrFirst.filePath << "): " << hdr.axisPixelLengths[0] << " != " << hdrFirst.axisPixelLengths[0]
                  << '\n';
  }
  if (hdr.axisPixelLengths[1] != hdrFirst.axisPixelLengths[1]) {
    valid = false;
    g_log.error() << "File " << hdr.filePath
                  << ": the number of pixels in the second dimension differs"
                     "from the first file loaded ("
                  << hdrFirst.filePath << "): " << hdr.axisPixelLengths[0] << " != " << hdrFirst.axisPixelLengths[0]
                  << '\n';
  }

  // Check the format is correct and create the Workspace
  if (!valid) {
    // Invalid files, record error
    throw std::runtime_error("An issue has been found in the header of this FITS file: " + hdr.filePath +
                             ". This algorithm currently doesn't support FITS files with "
                             "non-standard extensions, more than two axis "
                             "of data, or has detected that all the files are "
                             "not similar.");
  }
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
 * @param binSize size to rebin (1 == no re-bin == default)
 *
 * @param noiseThresh threshold for noise filtering
 *
 * @throw std::runtime_error when load fails (for example a memory
 * allocation issue, wrong rebin requested, etc.)
 */
void LoadFITS::doLoadFiles(const std::vector<std::string> &paths, const std::string &outWSName, bool loadAsRectImg,
                           int binSize, double noiseThresh) {
  std::vector<FITSInfo> headers;
  headers.resize(paths.size());

  loadHeader(paths[0], headers[0]);

  // No extension is set -> it's the standard format which we can parse.
  if (headers[0].numberOfAxis > 0)
    m_pixelCount += headers[0].axisPixelLengths[0];

  // Presumably 2 axis, but futureproofing.
  for (int i = 1; i < headers[0].numberOfAxis; ++i) {
    m_pixelCount *= headers[0].axisPixelLengths[i];
  }

  // Check consistency of binSize asap
  for (int i = 0; i < headers[0].numberOfAxis; ++i) {
    if (0 != (headers[0].axisPixelLengths[i] % binSize)) {
      throw std::runtime_error("Cannot rebin this image in blocks of " + std::to_string(binSize) + " x " +
                               std::to_string(binSize) + " pixels as requested because the size of dimension " +
                               std::to_string(i + 1) + " (" + std::to_string(headers[0].axisPixelLengths[i]) +
                               ") is not a multiple of the bin size.");
    }
  }

  MantidImage imageY(headers[0].axisPixelLengths[1], std::vector<double>(headers[0].axisPixelLengths[0]));
  MantidImage imageE(headers[0].axisPixelLengths[1], std::vector<double>(headers[0].axisPixelLengths[0]));

  size_t bytes = (headers[0].bitsPerPixel / 8) * m_pixelCount;
  std::vector<char> buffer;
  try {
    buffer.resize(bytes);
  } catch (std::exception &) {
    throw std::runtime_error("Could not allocate enough memory to run when trying to allocate " +
                             std::to_string(bytes) + " bytes.");
  }

  // Create a group for these new workspaces, if the group already exists, add
  // to it.
  size_t fileNumberInGroup = 0;
  WorkspaceGroup_sptr wsGroup;

  if (auto &ads = AnalysisDataService::Instance(); ads.doesExist(outWSName)) {
    // Get the name of the latest file in group to start numbering from
    wsGroup = ads.retrieveWS<WorkspaceGroup>(outWSName);
    std::string latestName = wsGroup->getNames().back();
    // Set next file number
    fileNumberInGroup = fetchNumber(latestName) + 1;
  } else {
    wsGroup = std::make_shared<WorkspaceGroup>();
    wsGroup->setTitle(outWSName);
  }

  const size_t totalWS = headers.size();
  // Create a progress reporting object
  API::Progress progress(this, 0.0, 1.0, totalWS + 1);
  progress.report(0, "Loading file(s) into workspace(s)");

  // Create first workspace (with instrument definition). This is also used as
  // a template for creating others
  Workspace2D_sptr imgWS;
  imgWS =
      makeWorkspace(headers[0], fileNumberInGroup, buffer, imageY, imageE, imgWS, loadAsRectImg, binSize, noiseThresh);
  progress.report(1, "First file loaded.");

  wsGroup->addWorkspace(imgWS);

  if (isInstrOtherThanIMAT(headers[0])) {
    // For now we assume IMAT except when specific headers are found by
    // isInstrOtherThanIMAT()
    //
    // TODO: do this conditional on INSTR='IMAT' when we have proper IMAT .fits
    // files
    try {
      auto loadInst = createChildAlgorithm("LoadInstrument");
      std::string directoryName = Kernel::ConfigService::Instance().getInstrumentDirectory();
      directoryName = directoryName + "/IMAT_Definition.xml";
      loadInst->setPropertyValue("Filename", directoryName);
      loadInst->setProperty<MatrixWorkspace_sptr>("Workspace", std::dynamic_pointer_cast<MatrixWorkspace>(imgWS));
      loadInst->execute();
    } catch (std::exception &ex) {
      g_log.information("Cannot load the instrument definition. " + std::string(ex.what()));
    }
  }

  // don't feel tempted to parallelize this loop as it is - it uses the same
  // imageY and imageE buffers for all the workspaces
  for (int64_t i = 1; i < static_cast<int64_t>(totalWS); ++i) {
    loadHeader(paths[i], headers[i]);
    // Check each header is valid/supported: standard (no extension to
    // FITS), has two axis, and it is consistent with the first header
    headerSanityCheck(headers[i], headers[0]);

    imgWS = makeWorkspace(headers[i], fileNumberInGroup, buffer, imageY, imageE, imgWS, loadAsRectImg, binSize,
                          noiseThresh);
    progress.report("Loaded file " + std::to_string(i + 1) + " of " + std::to_string(totalWS));
    wsGroup->addWorkspace(imgWS);
  }

  setProperty("OutputWorkspace", wsGroup);
}

/**
 * Read a single files header and populate an object with the information.
 *
 * @param headerInfo A FITSInfo file object to parse header
 * information into. This object must have its field filePath set to
 * the input file
 *
 * A typical simple FITS header looks like this:
@verbatim
SIMPLE  =                    T / file does conform to FITS standard
BITPIX  =                   16 / number of bits per data pixel
NAXIS   =                    2 / number of data axes
NAXIS1  =                  512 / length of data axis 1
NAXIS2  =                  512 / length of data axis 2
EXTEND  =                    T / FITS dataset may contain extensions
COMMENT   FITS (Flexible Image Transport System) format is defined in 'Astronomy
COMMENT   and Astrophysics', volume 376, page 359; bibcode: 2001A&A...376..359H
TOF     =   0.0595897599999995 / Ttime of flight from the external trigger
TIMEBIN =           4.096E-005 / Time width of this image
N_COUNTS=               182976 / Total counts in this image
N_TRIGS =                 4426 / Number of triggers acquired
END
@endverbatim
 *
 * @throws various std::runtime_error etc. on read failure
*/
void LoadFITS::parseHeader(FITSInfo &headerInfo) {
  headerInfo.headerSizeMultiplier = 0;
  std::ifstream istr(headerInfo.filePath.c_str(), std::ios::binary);
  istr.seekg(0, istr.end);
  const std::streampos fileSize = istr.tellg();
  if (fileSize <= 0) {
    throw std::runtime_error("Found a file that is readable but empty (0 bytes size): " + headerInfo.filePath);
  }
  istr.seekg(0, istr.beg);

  Poco::BinaryReader reader(istr);

  // Iterate 80 bytes at a time until header is parsed | 2880 bytes is the
  // fixed header length of FITS
  // 2880/80 = 36 iterations required
  const std::string commentKW = g_COMMENT_KEYNAME;
  bool endFound = false;

  while (!endFound && (g_BASE_HEADER_SIZE * headerInfo.headerSizeMultiplier < fileSize)) {
    headerInfo.headerSizeMultiplier++;
    const int entriesPerHDU = 36;

    for (int i = 0; i < entriesPerHDU; ++i) {
      // Keep vect of each header item, including comments, and also keep a
      // map of individual keys.
      std::string part;
      reader.readRaw(80, part);
      headerInfo.headerItems.emplace_back(part);

      // from the FITS standard about COMMENT: This keyword shall have no
      // associated value; columns 9-80 may contain any ASCII text.
      // That includes '='
      if (boost::iequals(commentKW, part.substr(0, commentKW.size()))) {
        continue;
      }

      // Add non-comment key/values. These hey and value are separated by the
      // character '='. All keys should be unique.
      // This will simply and silenty ignore any entry without the '='
      auto eqPos = part.find('=');
      if (eqPos > 0) {
        std::string key = part.substr(0, eqPos);
        std::string value = part.substr(eqPos + 1);

        // Comments on header entries are added after the value separated by a /
        // symbol. Exclude those comments.
        auto slashPos = value.find('/');
        if (slashPos > 0)
          value = value.substr(0, slashPos);

        boost::trim(key);
        boost::trim(value);

        if (key == g_END_KEYNAME)
          endFound = true;

        if (!key.empty())
          headerInfo.headerKeys[key] = value;
      }
    }
  }

  if (!endFound) {
    throw std::runtime_error("Could not find any valid END entry in the headers of this file after "
                             "scanning the file (" +
                             std::to_string(fileSize) +
                             " bytes). This does not look like a valid FITS file and "
                             "it is not possible to read it correctly as the boundary between "
                             "the headers and the data is undefined.");
  }

  istr.close();
}

/**
 * Creates and initialises a workspace with instrument definition and fills it
 * with data
 *
 * @param fileInfo information for the current file
 *
 * @param newFileNumber sequence number for the new file when added
 * into ws group
 *
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
 * @param binSize size to rebin (1 == no re-bin == default)
 *
 * @param noiseThresh threshold for noise filtering
 *
 * @returns A newly created Workspace2D, as a shared pointer
 */
Workspace2D_sptr LoadFITS::makeWorkspace(const FITSInfo &fileInfo, size_t &newFileNumber, std::vector<char> &buffer,
                                         MantidImage &imageY, MantidImage &imageE, const Workspace2D_sptr &parent,
                                         bool loadAsRectImg, int binSize, double noiseThresh) {
  // Create workspace (taking into account already here if rebinning is
  // going to happen)
  Workspace2D_sptr ws;
  if (!parent) {
    if (!loadAsRectImg) {
      size_t finalPixelCount = m_pixelCount / binSize * binSize;
      ws = std::dynamic_pointer_cast<Workspace2D>(
          WorkspaceFactory::Instance().create("Workspace2D", finalPixelCount, 2, 1));
    } else {
      ws = std::dynamic_pointer_cast<Workspace2D>(
          WorkspaceFactory::Instance().create("Workspace2D",
                                              fileInfo.axisPixelLengths[1] / binSize,     // one bin per column
                                              fileInfo.axisPixelLengths[0] / binSize + 1, // one spectrum per row
                                              fileInfo.axisPixelLengths[0] / binSize));
    }
  } else {
    ws = std::dynamic_pointer_cast<Workspace2D>(WorkspaceFactory::Instance().create(parent));
  }

  // this pixel scale property is used to set the workspace X values
  double cm_1 = getProperty("Scale");
  // amount of width units (e.g. cm) per pixel
  double cmpp = 1; // cm per pixel == bin width
  if (0.0 != cm_1)
    cmpp /= cm_1;
  cmpp *= static_cast<double>(binSize);

  if (loadAsRectImg && 1 == binSize) {
    // set data directly into workspace
    readDataToWorkspace(fileInfo, cmpp, ws, buffer);
  } else {
    readDataToImgs(fileInfo, imageY, imageE, buffer);
    doFilterNoise(noiseThresh, imageY, imageE);

    // Note this can change the sizes of the images and the number of pixels
    if (1 == binSize) {
      ws->setImageYAndE(imageY, imageE, 0, loadAsRectImg, cmpp, false /* no parallel load */);

    } else {
      MantidImage rebinnedY(imageY.size() / binSize, std::vector<double>(imageY[0].size() / binSize));
      MantidImage rebinnedE(imageE.size() / binSize, std::vector<double>(imageE[0].size() / binSize));

      doRebin(binSize, imageY, imageE, rebinnedY, rebinnedE);
      ws->setImageYAndE(rebinnedY, rebinnedE, 0, loadAsRectImg, cmpp, false /* no parallel load */);
    }
  }

  try {
    ws->setTitle(Poco::Path(fileInfo.filePath).getFileName());
  } catch (std::runtime_error &) {
    ws->setTitle(padZeros(newFileNumber, g_DIGIT_SIZE_APPEND));
  }
  ++newFileNumber;

  addAxesInfoAndLogs(ws, loadAsRectImg, fileInfo, binSize, cmpp);

  return ws;
}

/**
 * Add information to the workspace being loaded: labels, units, logs related to
 * the image size, etc.
 *
 * @param ws workspace to manipulate
 *
 * @param loadAsRectImg if true, the workspace has one spectrum per
 * row and one bin per column
 *
 * @param fileInfo information for the current file
 *
 * @param binSize size to rebin (1 == no re-bin == default)
 *
 * @param cmpp centimeters per pixel (already taking into account
 * possible rebinning)
 */
void LoadFITS::addAxesInfoAndLogs(const Workspace2D_sptr &ws, bool loadAsRectImg, const FITSInfo &fileInfo, int binSize,
                                  double cmpp) {
  // add axes
  size_t width = fileInfo.axisPixelLengths[0] / binSize;
  size_t height = fileInfo.axisPixelLengths[1] / binSize;
  if (loadAsRectImg) {
    // width/X axis
    auto axw = std::make_unique<Mantid::API::NumericAxis>(width + 1);
    axw->title() = "width";
    for (size_t i = 0; i < width + 1; i++) {
      axw->setValue(i, static_cast<double>(i) * cmpp);
    }
    ws->replaceAxis(0, std::move(axw));
    // "cm" width label unit
    std::shared_ptr<Kernel::Units::Label> unitLbl =
        std::dynamic_pointer_cast<Kernel::Units::Label>(UnitFactory::Instance().create("Label"));
    unitLbl->setLabel("width", "cm");
    ws->getAxis(0)->unit() = unitLbl;

    // height/Y axis
    auto axh = std::make_unique<Mantid::API::NumericAxis>(height);
    axh->title() = "height";
    for (size_t i = 0; i < height; i++) {
      axh->setValue(i, static_cast<double>(i) * cmpp);
    }
    ws->replaceAxis(1, std::move(axh));
    // "cm" height label unit
    unitLbl = std::dynamic_pointer_cast<Kernel::Units::Label>(UnitFactory::Instance().create("Label"));
    unitLbl->setLabel("height", "cm");
    ws->getAxis(1)->unit() = unitLbl;

    ws->setDistribution(true);
  } else {
    // TODO: what to do when loading 1pixel - 1 spectrum?
  }
  ws->setYUnitLabel("brightness");

  // Add all header info to log.
  for (const auto &headerKey : fileInfo.headerKeys) {
    ws->mutableRun().removeLogData(headerKey.first, true);
    ws->mutableRun().addLogData(new PropertyWithValue<std::string>(headerKey.first, headerKey.second));
  }

  // Add rotational data to log. Clear first from copied WS
  auto it = fileInfo.headerKeys.find(m_sampleRotation);
  ws->mutableRun().removeLogData("Rotation", true);
  if (fileInfo.headerKeys.end() != it) {
    auto rot = boost::lexical_cast<double>(it->second);
    if (rot >= 0) {
      ws->mutableRun().addLogData(new PropertyWithValue<double>("Rotation", rot));
    }
  }

  // Add axis information to log. Clear first from copied WS
  ws->mutableRun().removeLogData("Axis1", true);
  ws->mutableRun().addLogData(new PropertyWithValue<int>("Axis1", static_cast<int>(fileInfo.axisPixelLengths[0])));
  ws->mutableRun().removeLogData("Axis2", true);
  ws->mutableRun().addLogData(new PropertyWithValue<int>("Axis2", static_cast<int>(fileInfo.axisPixelLengths[1])));

  // Add image key data to log. Clear first from copied WS
  ws->mutableRun().removeLogData("ImageKey", true);
  ws->mutableRun().addLogData(new PropertyWithValue<std::string>("ImageKey", fileInfo.imageKey));
}

/**
 * Reads the data (FITS matrix) from a single FITS file into a
 * workspace (directly into the spectra, using one spectrum per image
 * row).
 *
 * @param fileInfo information on the FITS file to load, including its path
 * @param cmpp centimeters per pixel, to scale/normalize values
 * @param ws workspace with the required dimensions
 * @param buffer pre-allocated buffer to read from file
 *
 * @throws std::runtime_error if there are file input issues
 */
void LoadFITS::readDataToWorkspace(const FITSInfo &fileInfo, double cmpp, const Workspace2D_sptr &ws,
                                   std::vector<char> &buffer) {
  const size_t bytespp = (fileInfo.bitsPerPixel / 8);
  const size_t len = m_pixelCount * bytespp;
  readInBuffer(fileInfo, buffer, len);

  const size_t nrows(fileInfo.axisPixelLengths[1]), ncols(fileInfo.axisPixelLengths[0]);
  // Treat buffer as a series of bytes
  auto *buffer8 = reinterpret_cast<uint8_t *>(buffer.data());

  PARALLEL_FOR_NO_WSP_CHECK()
  for (int i = 0; i < static_cast<int>(nrows); ++i) {
    auto &xVals = ws->mutableX(i);
    auto &yVals = ws->mutableY(i);
    auto &eVals = ws->mutableE(i);
    xVals = static_cast<double>(i) * cmpp;

    for (size_t j = 0; j < ncols; ++j) {
      // Map from 2D->1D index
      const size_t start = ((i * (bytespp)) * nrows) + (j * (bytespp));
      uint8_t const *const buffer8Start = buffer8 + start;
      // Reverse byte order of current value. Make sure we allocate enough
      // enough space to hold the size
      uint8_t byteValue[g_maxBytesPP];
      std::reverse_copy(buffer8Start, buffer8Start + bytespp, byteValue);

      double val = 0;
      if (fileInfo.bitsPerPixel == 8) {
        val = toDouble<uint8_t>(byteValue);
      } else if (fileInfo.bitsPerPixel == 16) {
        val = toDouble<uint16_t>(byteValue);
      } else if (fileInfo.bitsPerPixel == 32 && !fileInfo.isFloat) {
        val = toDouble<uint32_t>(byteValue);
      } else if (fileInfo.bitsPerPixel == 64 && !fileInfo.isFloat) {
        val = toDouble<uint32_t>(byteValue);
      } else if (fileInfo.bitsPerPixel == 32 && fileInfo.isFloat) {
        val = toDouble<float>(byteValue);
      } else if (fileInfo.bitsPerPixel == 64 && fileInfo.isFloat) {
        val = toDouble<double>(byteValue);
      }

      val = fileInfo.scale * val - fileInfo.offset;
      yVals[j] = val;
      eVals[j] = sqrt(val);
    }
  }
}

/**
 * Reads the data (FITS matrix) from a single FITS file into image
 * objects (Y and E). E is filled with the sqrt() of Y.
 *
 * @param fileInfo information on the FITS file to load, including its path
 * @param imageY Object to set the Y data values in
 * @param imageE Object to set the E data values in
 * @param buffer pre-allocated buffer to contain data values
 *
 * @throws std::runtime_error if there are file input issues
 */
void LoadFITS::readDataToImgs(const FITSInfo &fileInfo, MantidImage &imageY, MantidImage &imageE,
                              std::vector<char> &buffer) {

  size_t bytespp = (fileInfo.bitsPerPixel / 8);
  size_t len = m_pixelCount * bytespp;
  readInBuffer(fileInfo, buffer, len);

  // create pointer of correct data type to void pointer of the buffer:
  auto *buffer8 = reinterpret_cast<uint8_t *>(&buffer[0]);
  std::vector<char> buf(bytespp);
  char *tmp = buf.data();
  size_t start = 0;

  for (size_t i = 0; i < fileInfo.axisPixelLengths[1]; ++i) {   // width
    for (size_t j = 0; j < fileInfo.axisPixelLengths[0]; ++j) { // height
      // If you wanted to PARALLEL_...ize these loops (which doesn't
      // seem to provide any speed up when loading images one at a
      // time, you cannot use the start+=bytespp at the end of this
      // loop. You'd need something like this:
      //
      // size_t start =
      // ((i * (bytespp)) * fileInfo.axisPixelLengths[1]) +
      // (j * (bytespp));
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
 * Reads the data (FITS matrix) from a single FITS file into a
 * buffer. This simply reads the raw block of data, without doing any
 * re-scaling or adjustment.
 *
 * @param fileInfo information on the FITS file to load, including its path
 * @param buffer pre-allocated buffer where to read data
 * @param len amount of chars/bytes/octets to read
 *
 * @throws std::runtime_error if there are file input issues
 */
void LoadFITS::readInBuffer(const FITSInfo &fileInfo, std::vector<char> &buffer, size_t len) {
  std::string filename = fileInfo.filePath;
  std::ifstream file(filename, std::ios::in | std::ios::binary);
  file.seekg(g_BASE_HEADER_SIZE * fileInfo.headerSizeMultiplier);
  file.read(&buffer[0], len);
  if (!file) {
    throw std::runtime_error("Error while reading file: " + filename + ". Tried to read " + std::to_string(len) +
                             " bytes but got " + std::to_string(file.gcount()) +
                             " bytes. The file and/or its headers may be wrong.");
  }
  // all is loaded
  file.close();
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
void LoadFITS::doFilterNoise(double thresh, MantidImage &imageY, MantidImage &imageE) {
  if (thresh <= 0.0)
    return;

  MantidImage goodY = imageY;
  MantidImage goodE = imageE;

  // TODO: this is not very smart about the edge pixels (leftmost and
  // rightmost columns, topmost and bottom rows)
  for (size_t j = 1; j < (imageY.size() - 1); ++j) {
    for (size_t i = 1; i < (imageY[0].size() - 1); ++i) {

      if (((imageY[j][i] - imageY[j][i - 1]) > thresh) && ((imageY[j][i] - imageY[j][i + 1]) > thresh) &&
          ((imageY[j][i] - imageY[j - 1][i]) > thresh) && ((imageY[j][i] - imageY[j + 1][i]) > thresh))
        goodY[j][i] = 0;
      else
        goodY[j][i] = 1;

      if (((imageE[j][i] - imageE[j][i - 1]) > thresh) && ((imageE[j][i] - imageE[j][i + 1]) > thresh) &&
          ((imageE[j][i] - imageE[j - 1][i]) > thresh) && ((imageE[j][i] - imageE[j + 1][i]) > thresh))
        goodE[j][i] = 0;
      else
        goodE[j][i] = 1;
    }
  }

  for (size_t j = 1; j < (imageY.size() - 1); ++j) {
    for (size_t i = 1; i < (imageY[0].size() - 1); ++i) {
      if (goodY[j][i] == 0.0) {
        if (goodY[j - 1][i] != 0.0 || goodY[j + 1][i] != 0.0 || goodY[j][i - 1] != 0.0 || goodY[j][i + 1] != 0.0) {
          imageY[j][i] = goodY[j - 1][i] * imageY[j - 1][i] + goodY[j + 1][i] * imageY[j + 1][i] +
                         goodY[j][i - 1] * imageY[j][i - 1] + goodY[j][i + 1] * imageY[j][i + 1];
        }
      }

      if (goodE[j][i] == 0.0) {
        if (goodE[j - 1][i] != 0.0 || goodE[j + 1][i] != 0.0 || goodE[j][i - 1] != 0.0 || goodE[j][i + 1] != 0.0) {
          imageE[j][i] = goodE[j - 1][i] * imageE[j - 1][i] + goodE[j + 1][i] * imageE[j + 1][i] +
                         goodE[j][i - 1] * imageE[j][i - 1] + goodE[j][i + 1] * imageE[j][i + 1];
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
void LoadFITS::doRebin(size_t rebin, const MantidImage &imageY, const MantidImage &imageE, MantidImage &rebinnedY,
                       MantidImage &rebinnedE) {
  if (1 >= rebin)
    return;

  for (size_t j = 0; j < (rebinnedY.size() - rebin + 1); ++j) {
    for (size_t i = 0; i < (rebinnedY[0].size() - rebin + 1); ++i) {
      double accumY = 0.0;
      double accumE = 0.0;
      size_t origJ = j * rebin;
      size_t origI = i * rebin;
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
bool LoadFITS::isInstrOtherThanIMAT(const FITSInfo &hdr) {
  bool res = false;

  // Images taken with Starlight camera contain this header entry:
  // INSTRUME='Starlight Xpress CCD'
  auto it = hdr.headerKeys.find("INSTRUME");
  if (hdr.headerKeys.end() != it && boost::contains(it->second, "Starlight")) {
    // For now, do nothing, just tell
    // Cameras used for HiFi and EMU are in principle only used
    // occasionally for calibration
    g_log.information() << "Found this in the file headers: " << it->first << " = " << it->second
                        << ". This file seems to come from a Starlight camera, "
                           "as used for calibration of the instruments HiFi and EMU (and"
                           "possibly others). Note: not "
                           "loading instrument definition.\n";
  }

  return res;
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
  m_headerAxisNameKeys.emplace_back("NAXIS1");
  m_headerAxisNameKeys.emplace_back("NAXIS2");

  m_mapFile = "";

  // extensions
  m_sampleRotation = "HIERARCH Sample/Tomo_Angle";
  m_imageType = "HIERARCH Image/Type";
}

/**
 *  Maps the header keys to specified values
 */
void LoadFITS::mapHeaderKeys() {
  if (getPropertyValue(g_HEADER_MAP_NAME).empty())
    return;

  // If a map file is selected, use that.
  std::string headerMapFileName = getPropertyValue(g_HEADER_MAP_NAME);
  std::ifstream fStream(headerMapFileName.c_str());

  try {
    // Ensure valid file
    if (fStream.good()) {
      // Get lines, split words, verify and add to map.
      std::string line;
      std::vector<std::string> lineSplit;
      while (getline(fStream, line)) {
        boost::split(lineSplit, line, boost::is_any_of("="));

        if (lineSplit[0] == g_ROTATION_NAME && !lineSplit[1].empty())
          m_headerRotationKey = lineSplit[1];

        if (lineSplit[0] == g_BIT_DEPTH_NAME && !lineSplit[1].empty())
          m_headerBitDepthKey = lineSplit[1];

        if (lineSplit[0] == g_AXIS_NAMES_NAME && !lineSplit[1].empty()) {
          m_headerAxisNameKeys.clear();
          boost::split(m_headerAxisNameKeys, lineSplit[1], boost::is_any_of(","));
        }

        if (lineSplit[0] == g_IMAGE_KEY_NAME && !lineSplit[1].empty()) {
          m_headerImageKeyKey = lineSplit[1];
        }
      }

      fStream.close();
    } else {
      throw std::runtime_error("Error while trying to read header keys mapping file: " + headerMapFileName);
    }
  } catch (...) {
    g_log.error("Cannot load specified map file, using property values "
                "and/or defaults.");
  }
}

/**
 * Returns the trailing number from a string minus leading 0's (so 25 from
 * workspace_00025).
 *
 * @param name string with a numerical suffix
 *
 * @returns A numerical representation of the string minus leading characters
 * and leading 0's
 */
size_t LoadFITS::fetchNumber(const std::string &name) {
  std::string tmpStr;
  for (auto it = name.end() - 1; isdigit(*it); --it) {
    tmpStr.insert(0, 1, *it);
  }
  while (tmpStr.length() > 0 && tmpStr[0] == '0') {
    tmpStr.erase(tmpStr.begin());
  }
  return (tmpStr.length() > 0) ? boost::lexical_cast<size_t>(tmpStr) : 0;
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
std::string LoadFITS::padZeros(const size_t number, const size_t totalDigitCount) {
  std::ostringstream ss;
  ss << std::setw(static_cast<int>(totalDigitCount)) << std::setfill('0') << static_cast<int>(number);

  return ss.str();
}
} // namespace Mantid::DataHandling
