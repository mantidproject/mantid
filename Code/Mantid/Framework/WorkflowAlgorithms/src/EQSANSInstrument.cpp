//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidWorkflowAlgorithms/EQSANSInstrument.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidKernel/Exception.h"

namespace Mantid {
namespace WorkflowAlgorithms {
namespace EQSANSInstrument {
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
  int ny_pixels = (int)(readInstrumentParameter("number-of-y-pixels", dataWS));
  return ny_pixels * pixel_x + pixel_y;
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
  x = (nx_pixels / 2.0 - 0.5 - pixel_x) * pixel_size_x / 1000.0;
  y = (pixel_y - ny_pixels / 2.0 + 0.5) * pixel_size_y / 1000.0;
}

/*
 * Returns the pixel coordinates corresponding to the given real-space position.
 * This assumes that the center of the detector is aligned
 * with the beam. An additional offset may need to be applied
 * @param x: real-space x coordinate [m]
 * @param y: real-space y coordinate [m]
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
  pixel_x = -x / pixel_size_x * 1000.0 + nx_pixels / 2.0 - 0.5;
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

} // namespace EQSANSInstrument
} // namespace WorkflowAlgorithms
} // namespace Mantid
