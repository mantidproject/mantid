#include "MantidDataHandling/ImggAggregateWavelengths.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/make_unique.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/StringTokenizer.h"

#include <fstream>
#include <iomanip>

#include <Poco/File.h>
#include <Poco/DirectoryIterator.h>
#include <Poco/Path.h>

namespace Mantid {
namespace DataHandling {

const std::string ImggAggregateWavelengths::outPrefixProjections =
    "sum_projection_";
const std::string ImggAggregateWavelengths::outPrefixBands = "bands_";
const std::string ImggAggregateWavelengths::indexRangesPrefix = "idx_";
const std::string ImggAggregateWavelengths::tofRangesPrefix = "tof_";
const std::string ImggAggregateWavelengths::outSubdirsPrefixUniformBands =
    "bands_uniform_";
const std::string ImggAggregateWavelengths::outSubdirsPrefixIndexBands =
    "bands_by_index_";
const std::string ImggAggregateWavelengths::outSubdirsPrefixToFBands =
    "bands_by_tof_";

using Mantid::Kernel::Direction;
using Mantid::API::WorkspaceProperty;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ImggAggregateWavelengths)

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string ImggAggregateWavelengths::name() const {
  return "ImggAggregateWavelengths";
}

/// Algorithm's version for identification. @see Algorithm::version
int ImggAggregateWavelengths::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string ImggAggregateWavelengths::category() const {
  return "DataHandling\\Imaging";
}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string ImggAggregateWavelengths::summary() const {
  return "Aggregates images from multiple energy bands or wavelengths";
}

namespace {
const std::string PROP_INPUT_PATH = "InputPath";
const std::string PROP_OUTPUT_PATH = "OutputPath";
const std::string PROP_UNIFORM_BANDS = "UniformBands";
const std::string PROP_INDEX_RANGES = "IndexRanges";
const std::string PROP_TOF_RANGES = "ToFRanges";
const std::string PROP_NUM_PROJECTIONS_PROCESSED = "NumProjections";
const std::string PROP_NUM_BANDS_PROCESSED = "NumBands";
const std::string PROP_OUTPUT_PREFIX_PROJECTIONS = "OutputProjectionsPrefix";
const std::string PROP_OUTPUT_PREFIX_BANDS = "OutputBandPrefix";
const std::string PROP_OUTPUT_SUBDIRS_PREFIX_UNIFORM_BANDS =
    "OutputSubdirsPrefixUniformBands";
const std::string PROP_OUTPUT_SUBDIRS_PREFIX_INDEX_BANDS =
    "OutputSubdirsPrefixIndexBands";
const std::string PROP_OUTPUT_SUBDIRS_PREFIX_TOF_BANDS =
    "OutputSubdirsPrefixToFBands";
const std::string PROP_INPUT_IMAGE_FORMAT = "InputImageFormat";
const std::string PROP_OUTPUT_IMAGE_FORMAT = "OutputImageFormat";
const std::string PROP_OUTPUT_BIT_DEPTH = "OutputBitDepth";

// just to compare two Poco::Path objects, used for std algorithms
struct PocoPathComp
    : public std::binary_function<Poco::Path, Poco::Path, bool> {
  bool operator()(const Poco::Path &lhs, const Poco::Path &rhs) const {
    return lhs.toString() < rhs.toString();
  }
};
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void ImggAggregateWavelengths::init() {
  declareProperty(Mantid::Kernel::make_unique<API::FileProperty>(
                      PROP_INPUT_PATH, "", API::FileProperty::Directory),
                  "The path (directory) to the input image files. It must "
                  "contain image files (radiography) or subdirectories "
                  "containing image files (radiography, one directory per "
                  "projection angle)");

  declareProperty(
      Kernel::make_unique<API::FileProperty>(PROP_OUTPUT_PATH, "",
                                             API::FileProperty::Directory),
      "The path (directory) where to generate the output image(s).");

  auto positive = boost::make_shared<Kernel::BoundedValidator<int>>();
  positive->setLower(0);
  declareProperty(
      PROP_UNIFORM_BANDS, 1, positive,
      "The number of output bands. The input bands are divided into uniform "
      "non-overlapping blocks and each of the output bands correspond to one "
      "block. This is a convenience particular case of the property " +
          PROP_INDEX_RANGES,
      Direction::Input);

  declareProperty(
      PROP_INDEX_RANGES, "",
      "A comma separated list of ranges of indices, where the "
      "indices refer to the input images, counting from 1. For "
      "example: 1-100, 101-200, 201-300. The number of ranges "
      "given will set the number of output bands. If you just need a "
      "certain"
      "number of bands split uniformly you can alternatively use "
      "the simple property " +
          PROP_UNIFORM_BANDS);

  // TODO: for later, when we have the required headers to do this
  declareProperty(
      PROP_TOF_RANGES, "",
      "A comma separated list of time-of-flight ranges given as for example "
      "5000-10000. These will be the boundaries in ToF that delimit the output "
      "bands. The units are as specified in the input images headers or "
      "metainformation (normally units of microseconds). The algorithm will "
      "produce as many output bands as ranges are "
      "given in this property. The ranges can overlap");

  declareProperty(PROP_NUM_PROJECTIONS_PROCESSED, 0,
                  "The number of projections (subdirectories with images) "
                  "found in the input path and successfully processed",
                  Direction::Output);

  declareProperty(PROP_NUM_BANDS_PROCESSED, 0,
                  "The number of wavelength or energy bands found in the "
                  "inputs and successfully processed (aggregated) into the "
                  "output(s)",
                  Direction::Output);

  const std::string grpPrefixes = "Naming (prefixes for the outputs)";
  declareProperty(
      PROP_OUTPUT_PREFIX_PROJECTIONS, outPrefixProjections,
      Kernel::make_unique<Kernel::MandatoryValidator<std::string>>(),
      "This prefix is added in the output file names to precede the projection "
      "sequence number (or angle, or simply the input directory index). The "
      "names of the output files then look like: " +
          outPrefixProjections + outPrefixBands + indexRangesPrefix + "1_1200" +
          ", as in addition to this prefix, another prefix is added with the "
          "indices of the input images included (see option " +
          PROP_OUTPUT_PREFIX_BANDS,
      Direction::Input);

  declareProperty(
      PROP_OUTPUT_PREFIX_BANDS, outPrefixBands,
      Kernel::make_unique<Kernel::MandatoryValidator<std::string>>(),
      "This prefix is used for the output file names in addition to the prefix "
      "that specifies the projection or input "
      "directory sequence number, (" +
          PROP_OUTPUT_PREFIX_PROJECTIONS +
          "). "
          "The output bands will be written into subdirectories with names "
          "where the prefix is prepended. In addition to this prefix, a second "
          "prefix that specifies whether the input bands were aggregated by "
          "indices or by time of flight is also appended. For example when "
          "running this algorithm using the property " +
          PROP_UNIFORM_BANDS + " or " + PROP_INDEX_RANGES +
          " the output subdirectoy names will look like '" +
          outPrefixProjections + outPrefixBands + indexRangesPrefix +
          "1_1200' (where the 1 and 1200 derive from the "
          "division into uniform non-overlaping blocks of "
          "input bands, or the index ranges given. When "
          "running the algorithm using the property " +
          PROP_TOF_RANGES + " the output names will look like '" +
          outPrefixBands + tofRangesPrefix +
          "10000_50000' (where the 10000 and 50000 are the time of flight "
          "boundaries of the output band, using the same units as in the image "
          "headers).",
      Direction::Input);

  declareProperty(PROP_OUTPUT_SUBDIRS_PREFIX_UNIFORM_BANDS,
                  outSubdirsPrefixUniformBands,
                  "This prefix will be "
                  "used for the name of the output subdirectories of every "
                  "output band when producing uniform output bands (property " +
                      PROP_UNIFORM_BANDS + ")",
                  Direction::Input);

  declareProperty(
      PROP_OUTPUT_SUBDIRS_PREFIX_INDEX_BANDS, outSubdirsPrefixIndexBands,
      "This prefix will be "
      "used for the name of the output subdirectories of every "
      "output band when producing output bands using index ranges (property  " +
          PROP_INDEX_RANGES + ")",
      Direction::Input);

  declareProperty(
      PROP_OUTPUT_SUBDIRS_PREFIX_TOF_BANDS, outSubdirsPrefixToFBands,
      "This prefix will be "
      "used for the name of the output subdirectories of every "
      "output band when producing output bands using ToF ranges (property  " +
          PROP_TOF_RANGES + ")",
      Direction::Input);

  setPropertyGroup(PROP_OUTPUT_PREFIX_PROJECTIONS, grpPrefixes);
  setPropertyGroup(PROP_OUTPUT_PREFIX_BANDS, grpPrefixes);
  setPropertyGroup(PROP_OUTPUT_SUBDIRS_PREFIX_UNIFORM_BANDS, grpPrefixes);
  setPropertyGroup(PROP_OUTPUT_SUBDIRS_PREFIX_INDEX_BANDS, grpPrefixes);
  setPropertyGroup(PROP_OUTPUT_SUBDIRS_PREFIX_TOF_BANDS, grpPrefixes);

  const std::string grpImgFormat = "Image format";

  std::vector<std::string> imgFormat{"FITS"};
  declareProperty(
      PROP_INPUT_IMAGE_FORMAT, "FITS", "From the input directory(ies) use "
                                       "images in this format and ignore any "
                                       "other files",
      boost::make_shared<Mantid::Kernel::StringListValidator>(imgFormat),
      Direction::Input);

  declareProperty(
      PROP_OUTPUT_IMAGE_FORMAT, "FITS", "Produce output images in this format",
      boost::make_shared<Mantid::Kernel::StringListValidator>(imgFormat),
      Direction::Input);

  const std::vector<int> bitDepths{16};
  declareProperty(PROP_OUTPUT_BIT_DEPTH, 16,
                  boost::make_shared<Kernel::ListValidator<int>>(bitDepths),
                  "The bit depth or number of bits per pixel to use for the "
                  "output image(s). Only 16 bits is supported at the "
                  "moment.",
                  Direction::Input);

  setPropertyGroup(PROP_INPUT_IMAGE_FORMAT, grpImgFormat);
  setPropertyGroup(PROP_OUTPUT_IMAGE_FORMAT, grpImgFormat);
  setPropertyGroup(PROP_OUTPUT_BIT_DEPTH, grpImgFormat);
}

std::map<std::string, std::string> ImggAggregateWavelengths::validateInputs() {
  std::map<std::string, std::string> result;

  size_t optCount = 0;
  const int uniformBands = getProperty(PROP_UNIFORM_BANDS);
  if (uniformBands > 0)
    optCount++;
  const std::string indexRanges = getPropertyValue(PROP_INDEX_RANGES);
  if (!indexRanges.empty())
    optCount++;
  const std::string tofRanges = getPropertyValue(PROP_TOF_RANGES);
  if (!tofRanges.empty())
    optCount++;

  // Prevent for now. When we enable this option, remove this if
  if (!tofRanges.empty())
    result[PROP_TOF_RANGES] =
        "This property is not supported in this version of the algorithm";

  if (1 != optCount) {
    result[PROP_UNIFORM_BANDS] = "One and only one of the options " +
                                 PROP_UNIFORM_BANDS + ", " + PROP_INDEX_RANGES +
                                 ", " + PROP_TOF_RANGES + " has to be set.";
  }

  const std::string inputPath = getPropertyValue(PROP_INPUT_PATH);
  const std::string outputPath = getPropertyValue(PROP_OUTPUT_PATH);
  if (inputPath == outputPath)
    result[PROP_INPUT_PATH] = "The input and output paths should be different.";

  return result;
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void ImggAggregateWavelengths::exec() {
  const std::string inputPath = getPropertyValue(PROP_INPUT_PATH);
  const std::string outputPath = getPropertyValue(PROP_OUTPUT_PATH);
  const std::string indexRanges = getPropertyValue(PROP_INDEX_RANGES);
  const std::string tofRanges = getPropertyValue(PROP_TOF_RANGES);
  const int uniformBands = getProperty(PROP_UNIFORM_BANDS);

  if (uniformBands > 0)
    aggUniformBands(inputPath, outputPath, uniformBands);
  else if (!indexRanges.empty())
    aggIndexBands(inputPath, outputPath, indexRanges);
  else if (!tofRanges.empty())
    aggToFBands(inputPath, outputPath, tofRanges);

  g_log.notice() << "Saved output aggregated images into: " << outputPath
                 << ". They are now ready for further processing. \n";
}

void ImggAggregateWavelengths::aggUniformBands(const std::string &inputPath,
                                               const std::string &outputPath,
                                               size_t bands) {

  Poco::Path path(inputPath);
  auto inputSubDirs = findInputSubdirs(path);

  if (inputSubDirs.empty()) {
    g_log.warning() << "Could not find any input files or directories in "
                    << inputPath
                    << " when trying to split the input bands into a uniform "
                       "number of output bands. Nothing will be produced in "
                       "the output path.\n";
  }

  auto outputSubdirs =
      buildOutputSubdirNamesFromUniformBands(inputSubDirs, bands);

  size_t count = 0;
  for (const auto &subdir : inputSubDirs) {
    processDirectory(subdir, bands, outputPath, outputSubdirs, outPrefixBands,
                     count++);
  }
  setProperty(PROP_NUM_PROJECTIONS_PROCESSED, static_cast<int>(count));
}

void ImggAggregateWavelengths::aggIndexBands(const std::string &inputPath,
                                             const std::string &outputPath,
                                             const std::string &rangesSpec) {

  Poco::Path path(inputPath);
  auto inputSubDirs = findInputSubdirs(path);

  if (inputSubDirs.empty()) {
    g_log.warning() << "Could not find any input files or directories in "
                    << inputPath << " when looking for input bands. Nothing "
                                    "will be produced in the output path.\n";
  }

  auto outRanges = rangesFromStringProperty(rangesSpec, PROP_INDEX_RANGES);

  auto outputSubdirs = buildOutputSubdirNamesFromIndexRangesBands(outRanges);

  size_t count = 0;
  for (const auto &subdir : inputSubDirs) {
    processDirectory(subdir, outRanges, outputPath, outputSubdirs,
                     outPrefixBands, count++);
  }
  setProperty(PROP_NUM_PROJECTIONS_PROCESSED, static_cast<int>(count));
}

void ImggAggregateWavelengths::aggToFBands(const std::string & /*inputPath*/,
                                           const std::string & /*outputPath*/,
                                           const std::string & /*ranges*/) {

  throw std::runtime_error("The property " + PROP_TOF_RANGES +
                           "is not supported at the moment");
}

/**
 * Aggregate the images found in an input directory (can be all the
 * images from a radiography experiment, or all the images
 * corresponding to a single projection angle from a tomography
 * experiment). This is the version that processed the "UniformBands"
 * property, splitting the input images into uniform bands or blocks
 * (as many as given in the property).
 *
 * @param inDir where to find the input images.
 *
 * @param bands number of output bands, or number of bands into
 * which the input bands should be aggregated, non-overlapping and
 * uniformly distributed.
 *
 * @param outDir where to write the output image(s).
 *
 * @param outSubdirs subdirectories for every output range (given in the
 * input parameter rages)
 *
 * @param prefix prefix for the image names. The indices will be
 * appended to the prefix
 *
 * @param outImgIndex index of the directory/angle/projection, for
 * file naming purposes
 */
void ImggAggregateWavelengths::processDirectory(
    const Poco::Path &inDir, size_t bands, const std::string &outDir,
    const std::vector<std::string> &outSubdirs, const std::string &prefix,
    size_t outImgIndex) {
  Mantid::API::MatrixWorkspace_sptr aggImg;

  auto images = findInputImages(inDir);

  if (images.empty()) {
    g_log.warning()
        << "Could not find any input image files in the subdirectory '"
        << inDir.toString() << "'. It will be ignored.\n";
    return;
  }

  auto ranges = splitSizeIntoRanges(images.size(), bands);

  processDirectory(inDir, ranges, outDir, outSubdirs, prefix, outImgIndex);
}

/**
 * Produces the output images (bands) from a directory. The directory
 * may hold the images for radiography data, or one projection angle
 * from tomography data. Multiple output bands (images) can be
 * produced as specified in ranges.
 *
 * This passes through the images one by one (only once), but every
 * images can be aggregated into multiple output bands (as the bands
 * or min-max ranges can overlap)
 *
 * @param inDir where to load images from
 * @param ranges min-max pairs that define the limits of the output bands
 * @param prefix to prepend to all the file names
 * @param outDir where to write the output images/bands
 *
 * @param outSubdirs subdirectories for every output range (given in the
 * input parameter rages)
 *
 * @param outImgIndex an index in the sequence of directories being
 * processed
 */
void ImggAggregateWavelengths::processDirectory(
    const Poco::Path &inDir,
    const std::vector<std::pair<size_t, size_t>> &ranges,
    const std::string outDir, const std::vector<std::string> &outSubdirs,
    const std::string &prefix, size_t outImgIndex) {

  auto imgFiles = findInputImages(inDir);

  const size_t maxProgress = imgFiles.size() + 1;
  API::Progress prog(this, 0, 1, maxProgress);

  const std::string wsName = "__ImggAggregateWavelengths_fits_seq";
  const std::string wsNameFirst = wsName + "_first";

  prog.report(0, "Loading first input image file");
  auto it = std::begin(imgFiles);
  std::vector<API::MatrixWorkspace_sptr> outAccums;
  outAccums.resize(ranges.size());
  outAccums[0] = loadFITS(*it, wsNameFirst);

  prog.report(1, "Preparing workspaces for the output images");
  for (size_t idx = 1; idx < ranges.size(); ++idx) {
    outAccums[idx] = outAccums[0]->clone();
  }
  ++it;

  prog.report(1, "Loading input image files");
  size_t inputIdx = 1;
  for (auto end = std::end(imgFiles); it != end; ++it, ++inputIdx) {
    // load one more
    const API::MatrixWorkspace_sptr img = loadFITS(*it, wsName);

    // add into output
    for (size_t idx = 0; idx < outAccums.size(); ++idx) {
      if (idx >= ranges[idx].first && idx <= ranges[idx].second) {
        aggImage(outAccums[idx], img);
      }
    }

    // clear image/workspace. TODO: This is a big waste of
    // allocations/deallocations
    Mantid::API::AnalysisDataService::Instance().remove(wsName);
    prog.reportIncrement(1);
  }

  prog.report("Saving output image file(s)");
  for (size_t idx = 0; idx < outAccums.size(); ++idx) {
    // call the file like: bands_idx_0_1000...
    const std::string indicesName = indexRangesPrefix +
                                    std::to_string(ranges[idx].first) + "_to_" +
                                    std::to_string(ranges[idx].second);

    Poco::Path outPath(outDir);
    outPath.append(outSubdirs[idx]);
    Poco::File fileOutDir(outPath);
    if (!fileOutDir.exists()) {
      fileOutDir.createDirectory();
    }

    const std::string extendedPrefix = prefix + indicesName;
    saveAggImage(outAccums[idx], outPath.toString(), extendedPrefix,
                 outImgIndex);
  }
  Mantid::API::AnalysisDataService::Instance().remove(wsNameFirst);

  // output just the number of bands founds for the first subdirectory
  int nBands = getProperty(PROP_NUM_BANDS_PROCESSED);
  if (0 == nBands) {
    setProperty(PROP_NUM_BANDS_PROCESSED, static_cast<int>(imgFiles.size()));
  }
  prog.reportIncrement(1, "Finished processing input subdirectory/projection");
}

std::vector<std::pair<size_t, size_t>>
ImggAggregateWavelengths::rangesFromStringProperty(
    const std::string &rangesSpec, const std::string &propName) {
  Mantid::Kernel::StringTokenizer rangeTokens(
      rangesSpec, ",", Mantid::Kernel::StringTokenizer::TOK_IGNORE_EMPTY |
                           Mantid::Kernel::StringTokenizer::TOK_TRIM);

  if (rangeTokens.count() <= 0) {
    throw std::invalid_argument(
        "Could not find any valid range of values in the property " + propName);
  }

  std::vector<std::pair<size_t, size_t>> ranges;
  for (const auto &str : rangeTokens) {
    const std::string sep = "-";
    Mantid::Kernel::StringTokenizer minMaxTokens(
        str, sep, Mantid::Kernel::StringTokenizer::TOK_IGNORE_EMPTY |
                      Mantid::Kernel::StringTokenizer::TOK_TRIM);
    if (2 != minMaxTokens.count()) {
      throw std::invalid_argument(
          std::string(
              "Could not parse a minimum and maximum value separated by '")
              .append(sep)
              .append("' from the string: ")
              .append(str));
    }

    try {
      ranges.emplace_back(boost::lexical_cast<size_t>(minMaxTokens[0]),
                          boost::lexical_cast<size_t>(minMaxTokens[1]));
    } catch (std::runtime_error &rexc) {
      throw std::runtime_error("Failed to parse this index range: " + str +
                               " . Details: " + std::string(rexc.what()));
    }
  }

  return ranges;
}

/**
 * Find the (sub)directories that need processing. Each subdirectory
 * would normally correspond to a projection (angle). If an
 * (apparently) image file is found in the input path, it is assumed
 * that the input path is the only that needs processing (it should
 * contain images from a radiography or single projection) and
 * subdirectories are then ignored. Other files (for example .txt
 * files) are ignored in any case.
 *
 * @param path input path which may contain subdirectories
 *
 * @return subdirectories of the input path to process (or the input
 * path itself if it is the only one that will b processed).
 */
std::vector<Poco::Path>
ImggAggregateWavelengths::findInputSubdirs(const Poco::Path &path) {
  // Note: sorted directory iterators available only in poco >= 1.5.2
  std::vector<Poco::Path> dirsFound;

  Poco::File dir(path);
  if (!dir.isDirectory() || !dir.exists())
    return dirsFound;

  // could also use Poco::Glob to find the files as an alternative
  Poco::DirectoryIterator it(dir);
  Poco::DirectoryIterator end;
  while (it != end) {

    // there is at least one image file: take just the first level directory
    if (it->isFile()) {
      const std::string &name = it.name();
      const std::string extShort = name.substr(name.size() - 3);
      const std::string extLong = name.substr(name.size() - 4);

      if (isSupportedExtension(extShort, extLong)) {
        // the input path contiains image files, take it
        dirsFound = {path};
        break;
      }
    }

    if (it->isDirectory()) {
      dirsFound.push_back(it.path());
    }

    ++it;
  }

  // this assumes the usual sorting of images of a stack (directory): a prefix,
  // and a sequence number (with a fixed number of digits).
  std::sort(dirsFound.begin(), dirsFound.end(), PocoPathComp());

  return dirsFound;
}

/**
 * Find images in a directory. It takes the (image) files that have
 * the expected/supported extension. Anything else (other files,
 * directories) is ignored.
 *
 * @param path directory where to look for images
 */
std::vector<Poco::Path>
ImggAggregateWavelengths::findInputImages(const Poco::Path &path) {
  // Note: sorted directory iterators available only in poco >= 1.5.2
  std::vector<Poco::Path> imgsFound;

  Poco::File dir(path);
  if (!dir.isDirectory() || !dir.exists())
    return imgsFound;

  Poco::DirectoryIterator it(dir);
  Poco::DirectoryIterator end;
  while (it != end) {

    if (it->isFile()) {
      const std::string summedSkipStr = "_SummedImg.";
      const std::string name = it.name();
      if (std::string::npos != name.find(summedSkipStr)) {
        ++it;
        continue;
      }

      const std::string extShort = name.substr(name.size() - 3);
      const std::string extLong = name.substr(name.size() - 4);
      if (isSupportedExtension(extShort, extLong)) {
        imgsFound.emplace_back(it.path());
      }
    }

    if (it->isDirectory()) {
      imgsFound.push_back(it.path());
    }

    ++it;
  }

  // this assumes the usual sorting of images of a stack (directory): a prefix,
  // and a sequence number (with a fixed number of digits).
  std::sort(imgsFound.begin(), imgsFound.end(), PocoPathComp());

  return imgsFound;
}

/**
 * Split into uniform blocks, for when producing multiple output bands
 * from the input images/bands. If the division is not exact, a few
 * images at the end will be ignored, so the number of input bands
 * that will go into every output band is the same for all of
 * them. For example, if 1000 input images/bands are aggregated into 3
 * output bands, From 1000/3: 333 (output band 1), 333 (output band
 * 2), 333 (output band 3), and 1 last image is not considered.
 *
 * @param availableCount how many input elements there are @param
 * bands into how many uniform blocks they should be split
 *
 * @returns pairs of min-max for the output blocks
 */
std::vector<std::pair<size_t, size_t>>
ImggAggregateWavelengths::splitSizeIntoRanges(size_t availableCount,
                                              size_t bands) {
  std::vector<std::pair<size_t, size_t>> ranges;
  size_t inc = availableCount / bands;
  if (inc < 1) {
    throw std::runtime_error(
        "The number of output bands requested (" + std::to_string(bands) +
        ") is bigger than the number of available input images (" +
        std::to_string(availableCount) +
        "). It should be equal or smaller, and "
        "normally it is much smaller. Please "
        "check that you are providing the "
        "correct input parameters");
  }

  for (size_t count = 0; count < availableCount; count += inc) {
    size_t max =
        ((count + inc) > availableCount) ? availableCount : count + inc;
    ranges.emplace_back(count, max);
  }

  return ranges;
}

/**
 * Builds the names of the output subdirectories (bands) given the
 * input subdirectories, when splitting the input bands/images into
 * uniform blocks. It has to look for the number of available input
 * images from the first input subdirectory that has any images.
 *
 * @param inputSubDirs input subdirectories (one or more)
 * @param bands how many output bands will be produced
 *
 * @returns list of names that can be used to create subdirectories
 * for the outputs, derived from the ranges that split the input bands
 * into uniform blocks. For example
 */
std::vector<std::string>
ImggAggregateWavelengths::buildOutputSubdirNamesFromUniformBands(
    const std::vector<Poco::Path> &inputSubDirs, size_t bands) {
  std::vector<std::string> outputSubdirs;
  // get number of available images from first effective subdirectory
  std::vector<Poco::Path> images;
  for (size_t idx = 0; idx < inputSubDirs.size() && images.empty(); ++idx) {
    images = findInputImages(inputSubDirs[idx]);
  }
  auto outRanges = splitSizeIntoRanges(images.size(), bands);

  const std::string subdirsPrefix =
      getProperty(PROP_OUTPUT_SUBDIRS_PREFIX_UNIFORM_BANDS);
  for (const auto &range : outRanges) {
    // one different subdirectory for every output band
    outputSubdirs.emplace_back(subdirsPrefix + indexRangesPrefix +
                               std::to_string(range.first) + "_to_" +
                               std::to_string(range.second));
  }

  return outputSubdirs;
}

/**
 * Builds the names of the output subdirectories (bands) given a
 * sequence of image index ranges, when splitting the input
 * bands/images into a sequence of ranges. It has to look for the
 * number of available input images from the first input subdirectory
 * that has any images.
 *
 * @param outRanges min-max pairs with ranges of image indices to use
 * for the output bands
 *
 * @returns list of names that can be used to create subdirectories
 * for the outputs, derived from the ranges
 */
std::vector<std::string>
ImggAggregateWavelengths::buildOutputSubdirNamesFromIndexRangesBands(
    const std::vector<std::pair<size_t, size_t>> &outRanges) {
  std::vector<std::string> outputSubdirs;
  const std::string subdirsPrefix =
      getProperty(PROP_OUTPUT_SUBDIRS_PREFIX_INDEX_BANDS);
  for (const auto &range : outRanges) {
    // one different subdirectory for every output band
    outputSubdirs.emplace_back(subdirsPrefix + indexRangesPrefix +
                               std::to_string(range.first) + "_to_" +
                               std::to_string(range.second));
  }

  return outputSubdirs;
}

bool ImggAggregateWavelengths::isSupportedExtension(
    const std::string &extShort, const std::string &extLong) {
  const std::vector<std::string> formatExtensionsShort{"fit"};
  const std::vector<std::string> formatExtensionsLong{"fits"};

  bool found = (formatExtensionsShort.cend() !=
                std::find(formatExtensionsShort.cbegin(),
                          formatExtensionsShort.cend(), extShort)) ||
               (formatExtensionsLong.cend() !=
                std::find(formatExtensionsLong.cbegin(),
                          formatExtensionsLong.cend(), extLong));
  return found;
}

/**
 * Add a workspace into an accumulator workspace ('toAdd' into
 * 'accum'). This method blatantly ignores the X values and the E
 * (error) values. We're only interested in the Y values (pixels,
 * counts).
 *
 * @param accum workspace where to add a new workspace
 * @param toAdd workspace to add
 */
void ImggAggregateWavelengths::aggImage(API::MatrixWorkspace_sptr accum,
                                        const API::MatrixWorkspace_sptr toAdd) {
  const size_t sizeX = accum->blocksize();
  const size_t sizeY = accum->getNumberHistograms();

  const size_t sizeXIn = accum->blocksize();
  const size_t sizeYIn = accum->getNumberHistograms();

  if (sizeX != sizeXIn || sizeY != sizeYIn) {
    throw std::runtime_error(
        "Cannot add images of different dimensions. The first image has "
        "dimensions " +
        std::to_string(sizeY) + " rows by " + std::to_string(sizeX) +
        " columns whereas the second image has dimensions " +
        std::to_string(sizeYIn) + " rows by " + std::to_string(sizeXIn) +
        " columns.");
  }

  for (size_t row = 0; row < sizeY; row++) {
    accum->mutableY(row) += toAdd->y(row);
  }
}

/**
 * Save an image workspace into an image file.
 *
 * @param accum workspace with image data
 * @param outDir where the file goes
 * @param prefix prefix to use in the file name
 * @param outImgIndex index of the directory/angle/projection which
 * will be used in the filename
 */
void ImggAggregateWavelengths::saveAggImage(
    const API::MatrixWorkspace_sptr accum, const std::string &outDir,
    const std::string &prefix, size_t outImgIndex) {
  // for example 'sum_projection_00003_bands_indices_0_1000'
  std::ostringstream sstr;
  sstr << std::setw(5) << std::setfill('0') << outImgIndex;
  const std::string outName = outPrefixProjections + sstr.str() + "_" + prefix;

  Poco::Path outPath(outDir);
  Poco::File dirFile(outPath);
  if (!dirFile.isDirectory() || !dirFile.exists()) {
    g_log.information() << "Cannot save output image into '"
                        << outPath.toString()
                        << "'. It is not an existing directory.\n";
    return;
  }

  outPath.append(outName);
  std::string fullName = outPath.toString();
  // only FITS support for now
  fullName += ".fits";
  saveFITS(accum, fullName);
  g_log.information() << "Saved output aggregated image into: " << fullName
                      << '\n';
}

API::MatrixWorkspace_sptr
ImggAggregateWavelengths::loadFITS(const Poco::Path &imgPath,
                                   const std::string &outName) {

  auto loader =
      Mantid::API::AlgorithmManager::Instance().createUnmanaged("LoadFITS");
  try {
    loader->initialize();
    loader->setChild(true);
    loader->setPropertyValue("Filename", imgPath.toString());
    loader->setProperty("OutputWorkspace", outName);
    // this is way faster when loading into a MatrixWorkspace
    loader->setProperty("LoadAsRectImg", true);
  } catch (std::exception &e) {
    throw std::runtime_error("Failed to initialize the algorithm to "
                             "load images. Error description: " +
                             std::string(e.what()));
  }

  try {
    loader->execute();
  } catch (std::exception &e) {
    throw std::runtime_error(
        "Failed to load image. Could not load this file as a "
        "FITS image: " +
        std::string(e.what()));
  }

  if (!loader->isExecuted()) {
    throw std::runtime_error(
        "Failed to load image correctly. Note that even though "
        "the image file has been loaded it seems to contain errors.");
  }

  API::MatrixWorkspace_sptr ws;
  try {
    API::Workspace_sptr outWS = loader->getProperty("OutputWorkspace");
    API::WorkspaceGroup_sptr wsg =
        boost::dynamic_pointer_cast<API::WorkspaceGroup>(outWS);
    ws = boost::dynamic_pointer_cast<API::MatrixWorkspace>(
        wsg->getItem(wsg->size() - 1));
  } catch (std::exception &e) {
    throw std::runtime_error(
        "Could not load image contents for file '" + imgPath.toString() +
        "'. An unrecoverable error "
        "happened when trying to load the image contents. Cannot "
        "display it. Error details: " +
        std::string(e.what()));
  }

  return ws;
}

void ImggAggregateWavelengths::saveFITS(const API::MatrixWorkspace_sptr accum,
                                        const std::string &filename) {
  auto writer =
      Mantid::API::AlgorithmManager::Instance().createUnmanaged("SaveFITS");
  try {
    writer->initialize();
    writer->setChild(true);
    writer->setPropertyValue("Filename", filename);
    writer->setProperty("InputWorkspace", accum);
    // this is way faster when loading into a MatrixWorkspace
    writer->setProperty("BitDepth", 16);
  } catch (std::exception &e) {
    throw std::runtime_error("Failed to initialize the algorithm to "
                             "save images in FITS format. Error description: " +
                             std::string(e.what()));
  }

  try {
    writer->execute();
  } catch (std::exception &e) {
    throw std::runtime_error(
        "Failed to write image. Could not write this file as a"
        "FITS image: " +
        std::string(e.what()));
  }
}

} // namespace DataHandling
} // namespace Mantid
