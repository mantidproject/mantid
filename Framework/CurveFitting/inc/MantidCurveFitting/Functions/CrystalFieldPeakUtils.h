#ifndef MANTID_CURVEFITTING_CRYSTALFIELDPEAKUTILS_H_
#define MANTID_CURVEFITTING_CRYSTALFIELDPEAKUTILS_H_
#include <string>
#include <vector>

namespace Mantid {

namespace API {
class CompositeFunction;
class FunctionValues;
class IPeakFunction;
} // namespace API

namespace CurveFitting {
namespace Functions {
namespace CrystalFieldUtils {
/**
Utility functions to help set up peak functions in a Crystal Field spectrum.

Copyright &copy; 2007-8 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
National Laboratory & European Spallation Source

This file is part of Mantid.

Mantid is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

Mantid is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

File change history is stored at: <https://github.com/mantidproject/mantid>
Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
size_t buildSpectrumFunction(API::CompositeFunction &spectrum,
                             const std::string &peakShape,
                             const API::FunctionValues &centresAndIntensities,
                             const std::vector<double> &xVec,
                             const std::vector<double> &yVec,
                             double fwhmVariation, double defaultFWHM,
                             size_t nRequiredPeaks, bool fixAllPeaks);
size_t updateSpectrumFunction(API::CompositeFunction &spectrum,
                              const std::string &peakShape,
                              const API::FunctionValues &centresAndIntensities,
                              size_t iFirst, const std::vector<double> &xVec,
                              const std::vector<double> &yVec,
                              double fwhmVariation, double defaultFWHM,
                              bool fixAllPeaks);
size_t calculateNPeaks(const API::FunctionValues &centresAndIntensities);
size_t calculateMaxNPeaks(size_t nPeaks);

} // namespace CrystalFieldUtils
} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid

#endif /*MANTID_CURVEFITTING_CRYSTALFIELDPEAKUTILS_H_*/
