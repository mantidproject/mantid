// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidWorkflowAlgorithms/HFIRInstrument.h"
#include "MantidAPI/Run.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/DataService.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/Property.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidKernel/StringTokenizer.h"
#include "Poco/NumberParser.h"

#include <memory>
#include <utility>

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>

namespace Mantid::WorkflowAlgorithms::HFIRInstrument {
/**
    File change history is stored at: <https://github.com/mantidproject/mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/

/*
 * Read a parameter from the instrument description
 */
double readInstrumentParameter(const std::string &parameter, const API::MatrixWorkspace_sptr &dataWS) {
  std::vector<double> pars = dataWS->getInstrument()->getNumberParameter(parameter);
  if (pars.empty())
    throw Kernel::Exception::InstrumentDefinitionError("Unable to find [" + parameter + "] instrument parameter");
  return pars[0];
}

/*
 * Return the detector ID corresponding to the [x,y] pixel coordinates.
 */
int getDetectorFromPixel(const int &pixel_x, const int &pixel_y, const API::MatrixWorkspace_sptr &dataWS) {
  UNUSED_ARG(dataWS);
  return 1000000 + 1000 * pixel_x + pixel_y;
}

/*
 * Returns the real-space coordinates corresponding to the
 * given pixel coordinates [m].
 */
void getCoordinateFromPixel(const double &pixel_x, const double &pixel_y, const API::MatrixWorkspace_sptr &dataWS,
                            double &x, double &y) {
  const int nx_pixels = static_cast<int>(readInstrumentParameter("number-of-x-pixels", dataWS));
  const int ny_pixels = static_cast<int>(readInstrumentParameter("number-of-y-pixels", dataWS));
  const double pixel_size_x = readInstrumentParameter("x-pixel-size", dataWS);
  const double pixel_size_y = readInstrumentParameter("y-pixel-size", dataWS);
  x = (pixel_x - nx_pixels / 2.0 + 0.5) * pixel_size_x / 1000.0;
  y = (pixel_y - ny_pixels / 2.0 + 0.5) * pixel_size_y / 1000.0;
}

/*
 * Returns the pixel coordinates corresponding to the given real-space position.
 * This assumes that the center of the detector is aligned
 * with the beam. An additional offset may need to be applied
 * @param x: real-space x coordinate [m]
 * @param y: real-space y coordinate [m]
 *
 */
void getPixelFromCoordinate(const double &x, const double &y, const API::MatrixWorkspace_sptr &dataWS, double &pixel_x,
                            double &pixel_y) {
  const int nx_pixels = static_cast<int>(readInstrumentParameter("number-of-x-pixels", dataWS));
  const int ny_pixels = static_cast<int>(readInstrumentParameter("number-of-y-pixels", dataWS));
  const double pixel_size_x = readInstrumentParameter("x-pixel-size", dataWS);
  const double pixel_size_y = readInstrumentParameter("y-pixel-size", dataWS);
  pixel_x = x / pixel_size_x * 1000.0 + nx_pixels / 2.0 - 0.5;
  pixel_y = y / pixel_size_y * 1000.0 + ny_pixels / 2.0 - 0.5;
}

/*
 * Returns the default beam center position, or the pixel location
 * of real-space coordinates (0,0).
 */
void getDefaultBeamCenter(const API::MatrixWorkspace_sptr &dataWS, double &pixel_x, double &pixel_y) {
  getPixelFromCoordinate(0.0, 0.0, dataWS, pixel_x, pixel_y);
}

/*
 * Get the source to sample distance (ssd)
 * If "Header/source_distance exists", ssd is this
 * otherwise get the guides distance (based on the number of guides used),
 * defined in instrument parameters file as "aperture-distances"
 * and sums "Header/sample_aperture_to_flange"
 */
double getSourceToSampleDistance(const API::MatrixWorkspace_sptr &dataWS) {

  double sourceToSampleDistance;

  std::vector<double> parsDouble = dataWS->getInstrument()->getNumberParameter("Header_source_distance");
  if (!parsDouble.empty()) {
    // First let's try to get source_distance first:
    sourceToSampleDistance = parsDouble[0] * 1000; // convert to mm
  } else {
    const auto nGuides = dataWS->run().getPropertyValueAsType<int>("Motor_Positions_nguides");
    // aperture-distances: array from the instrument parameters
    std::vector<std::string> parsString = dataWS->getInstrument()->getStringParameter("aperture-distances");
    if (parsString.empty())
      throw Kernel::Exception::InstrumentDefinitionError("Unable to find [aperture-distances] instrument parameter");
    std::string guidesDistances = parsString[0];

    std::vector<std::string> guidesDistancesSplit;
    boost::split(guidesDistancesSplit, guidesDistances, boost::is_any_of("\t ,"), boost::token_compress_on);
    sourceToSampleDistance = boost::lexical_cast<double>(guidesDistancesSplit[nGuides]);

    auto sourceToSampleDistanceOffset =
        dataWS->run().getPropertyValueAsType<double>("Header_sample_aperture_to_flange");

    sourceToSampleDistance -= sourceToSampleDistanceOffset;
  }
  return sourceToSampleDistance;
}

} // namespace Mantid::WorkflowAlgorithms::HFIRInstrument
