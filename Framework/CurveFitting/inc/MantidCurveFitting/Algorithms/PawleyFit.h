#ifndef MANTID_CURVEFITTING_PAWLEYFIT_H_
#define MANTID_CURVEFITTING_PAWLEYFIT_H_

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/ITableWorkspace_fwd.h"
#include "MantidAPI/TableRow.h"
#include "MantidCurveFitting/Functions/PawleyFunction.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Unit.h"

namespace Mantid {
namespace CurveFitting {
namespace Algorithms {

/** @class V3DFromHKLColumnExtractor

  Small helper class to extract HKLs as V3D from table columns. The table
  column can either store V3D directly, or a string with various separators:
    , ; [ ] (space)

*/
struct DLLExport V3DFromHKLColumnExtractor {
  Kernel::V3D operator()(const API::Column_const_sptr &hklColumn,
                         size_t i) const;

protected:
  Kernel::V3D getHKLFromV3DColumn(const API::Column_const_sptr &hklColumn,
                                  size_t i) const;
  Kernel::V3D getHKLFromStringColumn(const API::Column_const_sptr &hklColumn,
                                     size_t i) const;

  Kernel::V3D getHKLFromString(const std::string &hklString) const;
};

/** @class PawleyFit

  This algorithm uses the Pawley-method to refine lattice parameters using a
  powder diffractogram and a list of unique Miller indices. From the initial
  lattice parameters, theoretical reflection positions are calculated. Each
  reflection is described by the peak profile function supplied by the user and
  all parameters except the one for location of the reflection are freely
  refined. Available lattice parameters depend on the selected crystal system.

    @author Michael Wedel, Paul Scherrer Institut - SINQ
    @date 15/03/2015

  Copyright © 2015 PSI-NXMM

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
class DLLExport PawleyFit : public API::Algorithm {
public:
  PawleyFit();
  const std::string name() const override { return "PawleyFit"; }
  int version() const override { return 1; }
  const std::vector<std::string> seeAlso() const override {
    return {"PoldiPeakSearch"};
  }
  const std::string summary() const override;
  const std::string category() const override { return "Diffraction\\Fitting"; }

protected:
  double getTransformedCenter(double d, const Kernel::Unit_sptr &unit) const;

  void addHKLsToFunction(Functions::PawleyFunction_sptr &pawleyFn,
                         const API::ITableWorkspace_sptr &tableWs,
                         const Kernel::Unit_sptr &unit, double startX,
                         double endX) const;

  API::ITableWorkspace_sptr
  getLatticeFromFunction(const Functions::PawleyFunction_sptr &pawleyFn) const;
  API::ITableWorkspace_sptr getPeakParametersFromFunction(
      const Functions::PawleyFunction_sptr &pawleyFn) const;

  API::IFunction_sptr
  getCompositeFunction(const Functions::PawleyFunction_sptr &pawleyFn) const;

  void init() override;
  void exec() override;

  Kernel::Unit_sptr m_dUnit;
};

} // namespace Algorithms
} // namespace CurveFitting
} // namespace Mantid

#endif /* MANTID_CURVEFITTING_PAWLEYFIT_H_ */
