// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/MatrixWorkspace_fwd.h"

namespace Mantid {
namespace WorkflowAlgorithms {
namespace EQSANSInstrument {
/**
    File change history is stored at: <https://github.com/mantidproject/mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
const double default_slit_positions[3][8] = {{5.0, 10.0, 10.0, 15.0, 20.0, 20.0, 25.0, 40.0},
                                             {0.0, 10.0, 10.0, 15.0, 15.0, 20.0, 20.0, 40.0},
                                             {0.0, 10.0, 10.0, 15.0, 15.0, 20.0, 20.0, 40.0}};

double readInstrumentParameter(const std::string &parameter, const API::MatrixWorkspace_sptr &dataWS);
int getDetectorFromPixel(const int &pixel_x, const int &pixel_y, const API::MatrixWorkspace_sptr &dataWS);
void getCoordinateFromPixel(const double &pixel_x, const double &pixel_y, const API::MatrixWorkspace_sptr &dataWS,
                            double &x, double &y);
void getPixelFromCoordinate(const double &x, const double &y, const API::MatrixWorkspace_sptr &dataWS, double &pixel_x,
                            double &pixel_y);
void getDefaultBeamCenter(const API::MatrixWorkspace_sptr &dataWS, double &pixel_x, double &pixel_y);

} // namespace EQSANSInstrument
} // namespace WorkflowAlgorithms
} // namespace Mantid
