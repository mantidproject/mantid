#ifndef MANTID_CURVEFITTING_CRYSTALFIELDSPECTRUM_H_
#define MANTID_CURVEFITTING_CRYSTALFIELDSPECTRUM_H_

#include "MantidAPI/FunctionGenerator.h"

namespace Mantid {
namespace CurveFitting {
namespace Functions {
/**
Calculates crystal field spectrum.

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
class DLLExport CrystalFieldSpectrum : public API::FunctionGenerator {
public:
  CrystalFieldSpectrum();
  std::string name() const override { return "CrystalFieldSpectrum"; }
  const std::string category() const override { return "General"; }
  void buildTargetFunction() const override;
  std::string asString() const override;

protected:
  void updateTargetFunction() const override;

private:
  /// Number of fitted peaks in the spectrum.
  mutable size_t m_nPeaks;
};

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid

#endif /*MANTID_CURVEFITTING_CRYSTALFIELDSPECTRUM_H_*/
