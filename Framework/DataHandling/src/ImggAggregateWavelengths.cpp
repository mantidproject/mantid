#include "MantidDataHandling/ImggAggregateWavelengths.h"
#include "MantidAPI/FileProperty.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/make_unique.h"

namespace Mantid {
namespace DataHandling {

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
const std::string PROP_INPUT_FORMAT = "InputImageFormat";
const std::string PROP_OUTPUT_IMAGE_FORMAT = "OutputImageFormat";
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
      Mantid::Kernel::make_unique<API::FileProperty>(
          PROP_OUTPUT_PATH, "", API::FileProperty::Directory),
      "The path (directory) where to generate the output image(s).");

  declareProperty(
      "UniformBands", 1, boost::make_shared<Kernel::MandatoryValidator<int>>(),
      "The number of output bands. The input bands are divided into uniform "
      "non-overlapping blocks and each of the output bands correspond to one "
      "block. This is a convenience particular case of the property " +
          PROP_INDEX_RANGES);

  declareProperty(
      PROP_INDEX_RANGES, "",
      boost::make_shared<Kernel::MandatoryValidator<int>>(),
      "A comma separated list of ranges of indices, where the "
      "indices refer to the input images, counting from 1. For "
      "example: 1-100, 101-200, 201-300. The number of ranges "
      "given will set the number of output bands. If you just need a certain"
      "number of bands split uniformly you can alternatively use "
      "the simple property " +
          PROP_UNIFORM_BANDS);

  // TODO: for later, when we have the required headers to do this
  declareProperty(
      PROP_TOF_RANGES, "",
      boost::make_shared<Kernel::MandatoryValidator<int>>(),
      "A comma separated list of time-of-flight ranges given as for example "
      "5000-10000. These will be the boundaries in ToF that delimit the output "
      "bands. The units are as specified in the input images headers or "
      "metainformation (normally units of microseconds). The algorithm will "
      "produce as many output bands as ranges are "
      "given in this property. The ranges can overlap");

  std::vector<std::string> imgFormat{"FITS"};
  declareProperty(
      "InputImageFormat", "From the input directory(ies) use "
                          "images in this format and ignore any "
                          "other files",
      boost::make_shared<Mantid::Kernel::StringListValidator>(imgFormat));

  declareProperty(
      "OutputImageFormat", "Produce output images in this format",
      boost::make_shared<Mantid::Kernel::StringListValidator>(imgFormat));
}

std::map<std::string, std::string> ImggAggregateWavelengths::validateInputs() {
  std::map<std::string, std::string> result;

  const std::string inputPath = getPropertyValue(PROP_INPUT_PATH);
  const std::string outputPath = getPropertyValue(PROP_OUTPUT_PATH);
  const int uniformBands = getProperty(PROP_UNIFORM_BANDS);
  const std::string indexRanges = getPropertyValue(PROP_INDEX_RANGES);
  const std::string tofRanges = getPropertyValue(PROP_TOF_RANGES);

  return result;
}

Mantid::API::MatrixWorkspace_sptr ImggAggregateWavelengths::processDirectory() {
  Mantid::API::MatrixWorkspace_sptr aggImg;

  return aggImg;
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void ImggAggregateWavelengths::exec() { processDirectory(); }

} // namespace DataHandling
} // namespace Mantid
