// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidSINQ/DllConfig.h"

#include "MantidGeometry/Crystal/CompositeBraggScatterer.h"
#include "MantidGeometry/Crystal/PointGroup.h"
#include "MantidGeometry/Crystal/SpaceGroup.h"
#include "MantidGeometry/Crystal/UnitCell.h"

namespace Mantid {
namespace Poldi {

/** PoldiCreatePeaksFromCell :

    This algorithm creates a list of reflections with HKL and d-values
    in the form of a TableWorkspace that can be converted to a
    PoldiPeakCollection.

      @author Michael Wedel, Paul Scherrer Institut - SINQ
      @date 16/09/2014
  */
class MANTID_SINQ_DLL PoldiCreatePeaksFromCell : public API::Algorithm {
public:
  const std::string name() const override;
  int version() const override;
  const std::vector<std::string> seeAlso() const override { return {"PoldiCreatePeaksFromFile"}; }
  const std::string category() const override;
  const std::string summary() const override;

  std::map<std::string, std::string> validateInputs() override;

protected:
  Geometry::SpaceGroup_const_sptr getSpaceGroup(const std::string &spaceGroupString) const;

  double getDMaxValue(const Geometry::UnitCell &unitCell) const;

  double getLargestDValue(const Geometry::UnitCell &unitCell) const;

  Geometry::UnitCell getUnitCellFromProperties() const;
  Geometry::UnitCell
  getConstrainedUnitCell(const Geometry::UnitCell &unitCell, const Geometry::PointGroup::CrystalSystem &crystalSystem,
                         const Geometry::Group::CoordinateSystem &coordinateSystem = Geometry::Group::Orthogonal) const;

private:
  void init() override;
  void exec() override;
};

} // namespace Poldi
} // namespace Mantid
