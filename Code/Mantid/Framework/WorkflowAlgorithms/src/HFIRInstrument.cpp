//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidWorkflowAlgorithms/HFIRInstrument.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/Property.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidDataObjects/Workspace2D.h"
#include "Poco/StringTokenizer.h"
#include "Poco/NumberParser.h"

namespace Mantid {
namespace WorkflowAlgorithms {
namespace HFIRInstrument {
/**
    File change history is stored at: <https://github.com/mantidproject/mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/

/*
 * Read a parameter from the instrument description
 */
double readInstrumentParameter(const std::string &parameter,
                               API::MatrixWorkspace_sptr dataWS) {
  std::vector<double> pars =
      dataWS->getInstrument()->getNumberParameter(parameter);
  if (pars.empty())
    throw Kernel::Exception::InstrumentDefinitionError(
        "Unable to find [" + parameter + "] instrument parameter");
  return pars[0];
}

/*
 * Return the detector ID corresponding to the [x,y] pixel coordinates.
 */
int getDetectorFromPixel(const int &pixel_x, const int &pixel_y,
                         API::MatrixWorkspace_sptr dataWS) {
  UNUSED_ARG(dataWS);
  return 1000000 + 1000 * pixel_x + pixel_y;
}

/*
 * Returns the real-space coordinates corresponding to the
 * given pixel coordinates [m].
 */
void getCoordinateFromPixel(const double &pixel_x, const double &pixel_y,
                            API::MatrixWorkspace_sptr dataWS, double &x,
                            double &y) {
  const int nx_pixels =
      (int)(readInstrumentParameter("number-of-x-pixels", dataWS));
  const int ny_pixels =
      (int)(readInstrumentParameter("number-of-y-pixels", dataWS));
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
void getPixelFromCoordinate(const double &x, const double &y,
                            API::MatrixWorkspace_sptr dataWS, double &pixel_x,
                            double &pixel_y) {
  const int nx_pixels =
      (int)(readInstrumentParameter("number-of-x-pixels", dataWS));
  const int ny_pixels =
      (int)(readInstrumentParameter("number-of-y-pixels", dataWS));
  const double pixel_size_x = readInstrumentParameter("x-pixel-size", dataWS);
  const double pixel_size_y = readInstrumentParameter("y-pixel-size", dataWS);
  pixel_x = x / pixel_size_x * 1000.0 + nx_pixels / 2.0 - 0.5;
  pixel_y = y / pixel_size_y * 1000.0 + ny_pixels / 2.0 - 0.5;
}

/*
 * Returns the default beam center position, or the pixel location
 * of real-space coordinates (0,0).
 */
void getDefaultBeamCenter(API::MatrixWorkspace_sptr dataWS, double &pixel_x,
                          double &pixel_y) {
  getPixelFromCoordinate(0.0, 0.0, dataWS, pixel_x, pixel_y);
}

double getSourceToSampleDistance(API::MatrixWorkspace_sptr dataWS) {
  Mantid::Kernel::Property *prop =
      dataWS->run().getProperty("number-of-guides");
  Mantid::Kernel::PropertyWithValue<int> *dp =
      dynamic_cast<Mantid::Kernel::PropertyWithValue<int> *>(prop);
  const int nguides = *dp;

  std::vector<std::string> pars =
      dataWS->getInstrument()->getStringParameter("aperture-distances");
  if (pars.empty())
    throw Kernel::Exception::InstrumentDefinitionError(
        "Unable to find [aperture-distances] instrument parameter");

  double SSD = 0;
  Poco::StringTokenizer tok(pars[0], ",",
                            Poco::StringTokenizer::TOK_IGNORE_EMPTY);
  if (tok.count() > 0 && tok.count() < 10 && nguides >= 0 && nguides < 9) {
    const std::string distance_as_string = tok[8 - nguides];
    if (!Poco::NumberParser::tryParseFloat(distance_as_string, SSD))
      throw Kernel::Exception::InstrumentDefinitionError(
          "Bad value for source-to-sample distance");
  } else
    throw Kernel::Exception::InstrumentDefinitionError(
        "Unable to get source-to-sample distance");

  // Check for an offset
  if (dataWS->getInstrument()->hasParameter("source-distance-offset")) {
    const double offset =
        readInstrumentParameter("source-distance-offset", dataWS);
    SSD += offset;
  }
  return SSD;
}
} // namespace HFIRInstrument
} // namespace WorkflowAlgorithms
} // namespace Mantid
