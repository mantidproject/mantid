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
namespace HFIRInstrument {
/**
    File change history is stored at: <https://github.com/mantidproject/mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
*/

double readInstrumentParameter(const std::string &parameter, const API::MatrixWorkspace_sptr &dataWS);
int getDetectorFromPixel(const int &pixel_x, const int &pixel_y, const API::MatrixWorkspace_sptr &dataWS);
void getCoordinateFromPixel(const double &pixel_x, const double &pixel_y, const API::MatrixWorkspace_sptr &dataWS,
                            double &x, double &y);
void getPixelFromCoordinate(const double &x, const double &y, const API::MatrixWorkspace_sptr &dataWS, double &pixel_x,
                            double &pixel_y);
void getDefaultBeamCenter(const API::MatrixWorkspace_sptr &dataWS, double &pixel_x, double &pixel_y);
double getSourceToSampleDistance(const API::MatrixWorkspace_sptr &dataWS);

} // namespace HFIRInstrument
} // namespace WorkflowAlgorithms
} // namespace Mantid
