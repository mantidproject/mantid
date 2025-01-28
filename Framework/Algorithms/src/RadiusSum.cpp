// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/RadiusSum.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/NumericAxis.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidGeometry/IDetector.h"
#include "MantidGeometry/IObjComponent.h"
#include "MantidGeometry/Objects/BoundingBox.h"
#include "MantidKernel/ArrayLengthValidator.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/Unit.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/VisibleWhenProperty.h"

#include "MantidHistogramData/LinearGenerator.h"

#include <array>
#include <cmath>
#include <limits>
#include <numeric>
#include <sstream>

using namespace Mantid::Kernel;

namespace Mantid::Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(RadiusSum)

/// Algorithm's name for identification.
const std::string RadiusSum::name() const { return "RadiusSum"; }

/// Algorithm's version for identification.
int RadiusSum::version() const { return 1; }

/// Algorithm's category for identification.
const std::string RadiusSum::category() const { return "Transforms\\Grouping"; }

/** Initialize the algorithm's properties.
 */
void RadiusSum::init() {
  declareProperty(
      std::make_unique<API::WorkspaceProperty<API::MatrixWorkspace>>("InputWorkspace", "", Direction::Input),
      "An input workspace.");
  declareProperty(std::make_unique<API::WorkspaceProperty<>>("OutputWorkspace", "", Direction::Output),
                  "An output workspace.");

  auto twoOrThreeElements = std::make_shared<ArrayLengthValidator<double>>(2, 3);
  std::vector<double> myInput(3, 0);
  declareProperty(std::make_unique<ArrayProperty<double>>("Centre", std::move(myInput), std::move(twoOrThreeElements)),
                  "Coordinate of the centre of the ring");

  auto nonNegative = std::make_shared<BoundedValidator<double>>();
  nonNegative->setLower(0);
  declareProperty("MinRadius", 0.0, nonNegative->clone(), "Length of the inner ring. Default=0");
  declareProperty(std::make_unique<PropertyWithValue<double>>("MaxRadius", std::numeric_limits<double>::max(),
                                                              std::move(nonNegative)),
                  "Length of the outer ring. Default=ImageSize.");

  auto nonNegativeInt = std::make_shared<BoundedValidator<int>>();
  nonNegativeInt->setLower(1);
  declareProperty("NumBins", 100, std::move(nonNegativeInt), "Number of slice bins for the output. Default=100");

  const char *normBy = "NormalizeByRadius";
  const char *normOrder = "NormalizationOrder";

  declareProperty(normBy, false,
                  "Divide the sum of each ring by the radius "
                  "powered by Normalization Order");
  declareProperty(normOrder, 1.0,
                  "If 2, the normalization will be divided by "
                  "the quadratic value of the ring for each "
                  "radius.");
  setPropertySettings(normOrder, std::make_unique<VisibleWhenProperty>(normBy, IS_EQUAL_TO, "1"));

  const char *groupNorm = "Normalization";
  setPropertyGroup(normBy, groupNorm);
  setPropertyGroup(normOrder, groupNorm);
}

/** Execute the algorithm.
 */
void RadiusSum::exec() {
  cacheInputPropertyValues();
  inputValidationSanityCheck();

  // this is the main method, that executes the algorithm of radius sum
  std::vector<double> output;
  if (inputWorkspaceHasInstrumentAssociated(inputWS))
    output = processInstrumentRadiusSum();
  else
    output = processNumericImageRadiusSum();

  if (getProperty("NormalizeByRadius"))
    normalizeOutputByRadius(output, getProperty("NormalizationOrder"));

  setUpOutputWorkspace(output);
}

std::vector<double> RadiusSum::processInstrumentRadiusSum() {
  g_log.debug() << "Process Instrument related image\n";

  std::vector<double> accumulator(num_bins, 0);

  g_log.debug() << "For every detector in the image get its position "
                << " and sum up all the counts inside the related spectrum\n";

  const auto &spectrumInfo = inputWS->spectrumInfo();
  for (size_t i = 0; i < inputWS->getNumberHistograms(); i++) {
    if (!spectrumInfo.hasDetectors(i)) {
      // it may occur because there is no detector assigned, but it does not
      // cause problem. Hence, continue the loop.
      g_log.information() << "Spectrum " << i << " has no detector assigned.\n";
      continue;
    }
    if (spectrumInfo.isMonitor(i))
      continue;

    int bin_n = getBinForPixelPos(spectrumInfo.position(i));

    if (bin_n < 0)
      continue; // not in the limits of min_radius and max_radius

    auto &refY = inputWS->getSpectrum(i).y();
    accumulator[bin_n] += std::accumulate(refY.cbegin(), refY.cend(), 0.0);
  }
  return accumulator;
}

std::vector<double> RadiusSum::processNumericImageRadiusSum() {
  g_log.debug() << "Process Numeric Image\n";

  std::vector<double> accumulator(num_bins, 0);

  // the position of the pixel in the vertical axis comes from the axis(1)
  // (NumericAxis)
  API::NumericAxis *verticalAxis = dynamic_cast<API::NumericAxis *>(inputWS->getAxis(1));

  // assumption: in a numeric image, all the bins position for every row in the
  // image
  // will be in the same place.
  // The positions in the horizontal axis is the position of X if it is not a
  // histogram,
  // or the center of the bin, if it is a histogram.

  g_log.debug() << "Define the X positions of the pixels\n";
  auto x_pos = inputWS->points(0);

  g_log.debug() << "For every pixel define its bin position and sum them up\n";
  // for each row in the image
  for (size_t i = 0; i < inputWS->getNumberHistograms(); i++) {

    // pixel values
    auto &refY = inputWS->getSpectrum(i).y();

    // for every pixel
    for (size_t j = 0; j < refY.size(); j++) {

      // the position of the pixel is the horizontal position of the pixel
      // and the vertical row position.
      V3D pixelPos(x_pos[j], (*verticalAxis)(i), 0);

      int bin_pos = getBinForPixelPos(pixelPos);

      if (bin_pos < 0)
        continue; // not in the region [min_radius, max_radius]

      accumulator[bin_pos] += refY[j];
    }
  }

  return accumulator;
}

int RadiusSum::getBinForPixelPos(const V3D &pos) {
  double radio, theta, phi;
  V3D diff_vector = pos - centre;
  diff_vector.getSpherical(radio, theta, phi);

  // the distance is the projection of the radio in the plane of the image
  // which is given by radio * sin (theta)
  double effect_distance = radio * sin(theta * M_PI / 180);

  if (effect_distance < min_radius || effect_distance > max_radius)
    return -1; // outside the limits [min_radius, max_radius]

  return fromDistanceToBin(effect_distance);
}

void RadiusSum::cacheInputPropertyValues() {
  g_log.debug() << "Copy the input property values\n";
  inputWS = getProperty("InputWorkspace");

  g_log.debug() << "Extract the center and make it a V3D object\n";
  std::vector<double> centre_aux = getProperty("Centre");
  if (centre_aux.size() == 2)
    centre = V3D(centre_aux[0], centre_aux[1], 0);
  else
    centre = V3D(centre_aux[0], centre_aux[1], centre_aux[2]);

  g_log.debug() << "Copy the remaning properties: MinRadius, MaxRadius and NumBins\n";
  min_radius = getProperty("MinRadius");
  max_radius = getProperty("MaxRadius");
  num_bins = getProperty("NumBins");
}

void RadiusSum::inputValidationSanityCheck() {
  g_log.debug() << "Sanity check\n";

  g_log.debug() << "Check MinRadius < MaxRadius\n";
  if (min_radius >= max_radius) {
    std::stringstream s;
    s << "Wrong definition of the radius min and max. The minimum radius can "
         "not be bigger than maximum. "
      << "\nInputs (" << min_radius << ", " << max_radius << ").\n";
    throw std::invalid_argument(s.str());
  }

  const std::vector<double> boundary_limits = getBoundariesOfInputWorkspace();
  std::stringstream s;
  std::copy(boundary_limits.begin(), std::prev(boundary_limits.end()), std::ostream_iterator<double>(s, ", "));
  s << boundary_limits.back();
  g_log.information() << "Boundary limits are: " << s.str() << '\n';

  g_log.debug() << "Check: centre is defined inside the region defined by the "
                   "image or instrument\n";
  centerIsInsideLimits(getProperty("centre"), boundary_limits);

  g_log.debug() << "Recalculate MaxRadius if default value is given\n";
  if (max_radius > 0.9 * std::numeric_limits<double>::max()) {
    max_radius = getMaxDistance(centre, boundary_limits);
    g_log.notice() << "RadiusMax automatically calculated and set to " << max_radius << '\n';
  }

  g_log.debug() << "Check number of bins to alert user if many bins will end up empty\n";
  numBinsIsReasonable();
}

/** Differentiate between Instrument related image (where the position of the
 *pixels depend on the instrument
 *  attached to the workspace) and Numeric image (where the position of the
 *pixels depend on the relative position
 *  in the workspace).
 *
 * An instrument related image has the axis 1 defined as spectra (collection of
 *spectrum numbers each one associated to
 * one or more detectors in the instrument).
 *
 * @param inWS the input workspace
 * @return True if it is an instrument related workspace.
 */
bool RadiusSum::inputWorkspaceHasInstrumentAssociated(const API::MatrixWorkspace_sptr &inWS) {
  return inWS->getAxis(1)->isSpectra();
}

std::vector<double> RadiusSum::getBoundariesOfInputWorkspace() {
  if (inputWorkspaceHasInstrumentAssociated(inputWS)) {
    return getBoundariesOfInstrument(inputWS);
  } else {
    return getBoundariesOfNumericImage(inputWS);
  }
}

/** Assuming that the input workspace is a Numeric Image where the pixel
 *positions depend on their
 *  relative position inside the workspace, this function extracts the position
 *of the first and last
 *  pixel of the image.
 *
 *  It is important that the input workspace must be a numeric image, and not an
 *instrument related workspace.
 *  The function will raise exception (std::invalid_argument) if an
 *invalid input is give.
 *
 *  @see RadiusSum::inputWorkspaceHasInstrumentAssociated for reference.
 *
 *  @param inWS reference to the workspace
 *  @return a list of values that defines the limits of the image in this order:
 *Xmin, Xmax, Ymin, Ymax
 */
std::vector<double> RadiusSum::getBoundariesOfNumericImage(const API::MatrixWorkspace_sptr &inWS) {

  // horizontal axis

  // get the pixel position in the horizontal axis from the middle of the image.
  const auto &refX = inWS->x(inWS->getNumberHistograms() / 2);

  double min_x, max_x;

  const double &first_x = refX.front();
  const double &last_x = refX.back();
  if (first_x < last_x) {
    min_x = first_x;
    max_x = last_x;
  } else {
    min_x = last_x;
    max_x = first_x;
  }

  // vertical axis
  const API::NumericAxis *verticalAxis = dynamic_cast<API::NumericAxis *>(inWS->getAxis(1));
  if (!verticalAxis)
    throw std::invalid_argument("Vertical axis is not a numeric axis. Can not "
                                "find the limits of the image.");

  double min_y, max_y;
  min_y = verticalAxis->getMin();
  max_y = verticalAxis->getMax();

  // check the assumption made that verticalAxis will provide the correct
  // answer.
  if (min_y > max_y) {
    throw std::logic_error("Failure to get the boundaries of this image. "
                           "Internal logic error. Please, inform MantidHelp");
  }

  std::vector<double> output(4); // output = {min_x, max_x, min_y, max_y}; not
                                 // supported in all compilers
  output[0] = min_x;
  output[1] = max_x;
  output[2] = min_y;
  output[3] = max_y;
  return output;
}

/** Assuming that the workspace has an instrument associated with it from which
 *the pixel positions has to be taken,
 *  this function extracts the position of the first and last valid pixel
 *(detector) and return a list of values
 *  giving the boundaries of the instrument.
 *
 * @param inWS Input Workspace
 * @return a list of values that defines the limits of the image in this order:
 *Xmin, Xmax, Ymin, Ymax, Zmin, Zmax
 */
std::vector<double> RadiusSum::getBoundariesOfInstrument(const API::MatrixWorkspace_sptr &inWS) {

  // This function is implemented based in the following assumption:
  //   - The workspace is composed by spectrum with associated spectrum No which
  //   is associated to one detector or monitor
  //   - The first spectrum No (non monitor) is associated with one detector
  //   while the last spectrum No (non monitor)
  //     is associated with one detector.
  //   - They are in complete oposite direction.
  //
  //   Consider the following 'image' (where the ID is the number and the
  //   position is where it is displayed)
  //
  //    1  2  3
  //    4  5  6
  //    7  8  9
  //   10 11 12
  //
  //    In this image, the assumption is true, because, we can derive the
  //    boundaries of the image looking just to the
  //    ids 1 and 12.
  //
  //    But the following image:
  //
  //   1  2  3       6  5  4
  //   6  5  4       1  2  3
  //   7  8  9      12 11 10
  //  12 11 12       7  8  9
  //
  //   Although valid 'IDF' instrument, fail the assumption, and will return
  //   wrong values.
  //   Bear in mind these words if you face problems with the return of the
  //   boundaries of one instrument
  //

  double first_x, first_y, first_z;
  size_t i = 0;
  const auto &spectrumInfo = inWS->spectrumInfo();
  while (true) {
    i++;
    if (i >= inWS->getNumberHistograms())
      throw std::invalid_argument("Did not find any non monitor detector. "
                                  "Failed to identify the boundaries of this "
                                  "instrument.");

    if (spectrumInfo.isMonitor(i))
      continue;

    const auto pos = spectrumInfo.position(i);
    // get the position of the first valid (non-monitor) detector.
    first_x = pos.X();
    first_y = pos.Y();
    first_z = pos.Z();
    break;
  }

  double last_x, last_y, last_z;
  i = inWS->getNumberHistograms() - 1;
  while (true) {
    i--;
    if (i == 0)
      throw std::invalid_argument("There is no region defined for the "
                                  "instrument of this workspace. Failed to "
                                  "identify the boundaries of this instrument");

    if (spectrumInfo.isMonitor(i))
      continue;

    const auto pos = spectrumInfo.position(i);
    // get the last valid detector position
    last_x = pos.X();
    last_y = pos.Y();
    last_z = pos.Z();
    break;
  }

  // order the values
  double xMax, yMax, zMax;
  double xMin, yMin, zMin;
  xMax = std::max(first_x, last_x);
  yMax = std::max(first_y, last_y);
  zMax = std::max(first_z, last_z);
  xMin = std::min(first_x, last_x);
  yMin = std::min(first_y, last_y);
  zMin = std::min(first_z, last_z);

  std::vector<double> output(6); // output  = {xMin, xMax, yMin, yMax, zMin,
                                 // zMax }; not supported in all compilers
  output[0] = xMin;
  output[1] = xMax;
  output[2] = yMin;
  output[3] = yMax;
  output[4] = zMin;
  output[5] = zMax;

  return output;
}

/** Check if a given position is inside the limits defined by the boundaries.
 *
 *  It assumes that the centre will be given as a vector of
 *
 *  centre = {x1, x2, ..., xn}
 *
 *  and boundaries will be given as:
 *
 *  boundaries = {DomX1, DomX2, ..., DomXn}
 *
 *  for DomX1 (dominium of the first dimension given as [x1_min, x1_max],
 *
 *  Hence, boundaries is given as:
 *
 *  boundaries = {x1_min, x1_max, x2_min, x2_max, ... , xn_min, xn_max}
 *
 *  It will test that all the values of the centre is inside their respective
 *dominium.
 *  If the test fails, it will raise an exception (invalid_argument) to
 *express that the given centre is not inside the boundaries.
 */
void RadiusSum::centerIsInsideLimits(const std::vector<double> &centre, const std::vector<double> &boundaries) {
  // sanity check
  if ((2 * centre.size()) != boundaries.size())
    throw std::invalid_argument("CenterIsInsideLimits: The centre and boundaries were not given in the "
                                "correct as {x1,x2,...} and {x1_min, x1_max, x2_min, x2_max, ... }");

  bool check_passed = true;
  std::stringstream s;

  for (size_t i = 0; i < 2; i++) { // TODO: fix this
    if (centre[i] < boundaries[2 * i] || centre[i] > boundaries[2 * i + 1]) {
      check_passed = false;
      s << "The position for axis " << i + 1 << " (" << centre[i] << ") is outside the limits [" << boundaries[2 * i]
        << ", " << boundaries[2 * i + 1] << "]. \n";
    }
  }

  if (!check_passed)
    throw std::invalid_argument(s.str());
}

void RadiusSum::numBinsIsReasonable() {
  double size_bins = (max_radius - min_radius) / num_bins;

  double min_bin_size;
  if (inputWorkspaceHasInstrumentAssociated(inputWS))
    min_bin_size = getMinBinSizeForInstrument(inputWS);
  else
    min_bin_size = getMinBinSizeForNumericImage(inputWS);

  if (size_bins < min_bin_size)
    g_log.warning() << "The number of bins provided is too big. "
                    << "It corresponds to a separation smaller than the image "
                       "resolution (detector size). "
                    << "A resonable number is smaller than "
                    << static_cast<int>((max_radius - min_radius) / min_bin_size) << '\n';
}

double RadiusSum::getMinBinSizeForInstrument(const API::MatrixWorkspace_sptr &inWS) {
  // Assumption made: the detectors are placed one after the other, so the
  // minimum
  // reasonalbe size for the bin is the width of one detector.

  const auto &spectrumInfo = inWS->spectrumInfo();
  for (size_t i = 0; i < inWS->getNumberHistograms(); ++i) {
    if (spectrumInfo.isMonitor(i))
      continue;
    Geometry::BoundingBox bbox;
    spectrumInfo.detector(i).getBoundingBox(bbox);
    return bbox.width().norm();
  }
  // this should never happen because it was done in getBoundariesOfInstrument,
  throw std::invalid_argument("Did not find any non monitor detector position");
}

double RadiusSum::getMinBinSizeForNumericImage(const API::MatrixWorkspace_sptr &inWS) {
  // The pixel dimensions:
  //  - width: image width/ number of pixels in one row
  //  - height: image height/ number of pixels in one column
  //  The minimum bin size is the smallest value between this two values.

  std::vector<double> boundaries = getBoundariesOfNumericImage(inWS);
  const auto &refX = inWS->x(inputWS->getNumberHistograms() / 2);
  auto nX = static_cast<int>(refX.size());
  int nY = static_cast<int>(inWS->getAxis(1)->length());

  // remembering boundaries is defined as { xMin, xMax, yMin, yMax}
  return std::min(((boundaries[1] - boundaries[0]) / nX), ((boundaries[3] - boundaries[2]) / nY));
}

void RadiusSum::normalizeOutputByRadius(std::vector<double> &values, double exp_power) {
  g_log.debug() << "Normalization of the output in relation to the 'radius' (distance)\n";

  // the radius can be defined as:
  // radius_min + bin_size/2 + n * bin_size ; for 0<= n <= num_bins
  double bin_size = (max_radius - min_radius) / num_bins;
  double first_radius = min_radius + bin_size / 2;

  g_log.debug() << "Calculate Output[i] = Counts[i] / (Radius[i] ^ " << exp_power << ") << \n";
  if (exp_power > 1.00001 || exp_power < 0.99999) {
    for (int i = 0; i < static_cast<int>(values.size()); i++) {
      values[i] = values[i] / pow(first_radius + i * bin_size, exp_power);
    }
  } else { // avoid calling pow because exp_power == 1 (for performance)
    for (int i = 0; i < static_cast<int>(values.size()); i++) {
      values[i] = values[i] / (first_radius + i * bin_size);
    }
  }
}

double RadiusSum::getMaxDistance(const V3D &centre, const std::vector<double> &boundary_limits) {

  std::array<double, 2> Xs = {{boundary_limits[0], boundary_limits[1]}};
  std::array<double, 2> Ys = {{boundary_limits[2], boundary_limits[3]}};
  std::array<double, 2> Zs = {{0., 0.}};

  if (boundary_limits.size() == 6) {
    Zs[0] = boundary_limits[4];
    Zs[1] = boundary_limits[5];
  }

  double max_distance = 0;
  for (auto &x : Xs)
    for (auto &y : Ys)
      for (auto &z : Zs) {
        // define all the possible combinations for the limits

        double curr_distance = centre.distance(V3D(x, y, z));

        if (curr_distance > max_distance)
          max_distance = curr_distance; // keep the maximum distance.
      }
  return max_distance;
}

void RadiusSum::setUpOutputWorkspace(const std::vector<double> &values) {

  g_log.debug() << "Output calculated, setting up the output workspace\n";

  API::MatrixWorkspace_sptr outputWS =
      API::WorkspaceFactory::Instance().create(inputWS, 1, values.size() + 1, values.size());

  g_log.debug() << "Set the data\n";
  outputWS->mutableY(0) = values;

  g_log.debug() << "Set the bins limits\n";

  auto xSize = outputWS->mutableX(0).size();
  double bin_size = (max_radius - min_radius) / num_bins;
  outputWS->setBinEdges(0, xSize, HistogramData::LinearGenerator(min_radius, bin_size));

  // configure the axis:
  // for numeric images, the axis are the same as the input workspace, and are
  // copied in the creation.

  // for instrument related, the axis Y (1) continues to be the same.
  // it is necessary to change only the axis X. We have to change it to radius.
  if (inputWorkspaceHasInstrumentAssociated(inputWS)) {
    auto horizontal = std::make_unique<API::NumericAxis>(xSize);
    auto labelX = UnitFactory::Instance().create("Label");
    std::dynamic_pointer_cast<Units::Label>(labelX)->setLabel("Radius");
    horizontal->unit() = labelX;
    outputWS->replaceAxis(0, std::move(horizontal));
  }

  setProperty("OutputWorkspace", outputWS);
}

} // namespace Mantid::Algorithms
