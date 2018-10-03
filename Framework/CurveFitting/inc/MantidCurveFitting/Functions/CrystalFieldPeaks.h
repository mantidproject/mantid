#ifndef MANTID_CURVEFITTING_CRYSTALFIELDPEAKS_H_
#define MANTID_CURVEFITTING_CRYSTALFIELDPEAKS_H_

#include "MantidAPI/IFunctionGeneral.h"
#include "MantidCurveFitting/Functions/CrystalFieldPeaksBase.h"

namespace Mantid {
namespace CurveFitting {
namespace Functions {

/**
  CrystalFieldPeaks is a function that calculates crystal field peak
  positions and intensities.

  Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class MANTID_CURVEFITTING_DLL CrystalFieldPeaks : public CrystalFieldPeaksBase,
                                                  public API::IFunctionGeneral {
public:
  CrystalFieldPeaks();
  std::string name() const override;
  size_t getNumberDomainColumns() const override;
  size_t getNumberValuesPerArgument() const override;
  void functionGeneral(const API::FunctionDomainGeneral &generalDomain,
                       API::FunctionValues &values) const override;
  size_t getDefaultDomainSize() const override;

private:
  /// Store the default domain size after first
  /// function evaluation
  mutable size_t m_defaultDomainSize;
};

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid

#endif /* MANTID_CURVEFITTING_CRYSTALFIELDPEAKS_H_ */
