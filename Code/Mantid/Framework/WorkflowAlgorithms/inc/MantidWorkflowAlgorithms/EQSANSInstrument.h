#ifndef MANTID_ALGORITHMS_EQSANSINSTRUMENT_H_
#define MANTID_ALGORITHMS_EQSANSINSTRUMENT_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/MatrixWorkspace.h"

namespace Mantid {
namespace WorkflowAlgorithms {
namespace EQSANSInstrument {
/**
    File change history is stored at: <https://github.com/mantidproject/mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
const double default_slit_positions[3][8] = {
    {5.0, 10.0, 10.0, 15.0, 20.0, 20.0, 25.0, 40.0},
    {0.0, 10.0, 10.0, 15.0, 15.0, 20.0, 20.0, 40.0},
    {0.0, 10.0, 10.0, 15.0, 15.0, 20.0, 20.0, 40.0}};

double readInstrumentParameter(const std::string &parameter,
                               API::MatrixWorkspace_sptr dataWS);
int getDetectorFromPixel(const int &pixel_x, const int &pixel_y,
                         API::MatrixWorkspace_sptr dataWS);
void getCoordinateFromPixel(const double &pixel_x, const double &pixel_y,
                            API::MatrixWorkspace_sptr dataWS, double &x,
                            double &y);
void getPixelFromCoordinate(const double &x, const double &y,
                            API::MatrixWorkspace_sptr dataWS, double &pixel_x,
                            double &pixel_y);
void getDefaultBeamCenter(API::MatrixWorkspace_sptr dataWS, double &pixel_x,
                          double &pixel_y);

} // namespace EQSANSInstrument
} // namespace WorkflowAlgorithms
} // namespace Mantid

#endif /*MANTID_ALGORITHMS_EQSANSINSTRUMENT_H_*/
