# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init,invalid-name
import systemtesting
from mantid.geometry import PointGroup, SpaceGroupFactory, UnitCell


class SpaceGroupUnitCellTest(systemtesting.MantidSystemTest):
    """
    This test checks that SpaceGroup.isAllowedUnitCell produces the expected result for each registered space group.
    """

    def runTest(self):
        cells = self.getUnitCells()
        metric_compatibility = self.getMetricCompatibility(cells)

        # For monoclinic space groups with unique c-axis the metric and compatible metrics are different
        monoclinic_c_cells = cells.copy()
        monoclinic_c_cells[PointGroup.LatticeSystem.Monoclinic] = UnitCell(5.0, 6.0, 7.0, 90.0, 90.0, 102.0)

        monoclinic_c_compatiblity = metric_compatibility.copy()
        monoclinic_c_compatiblity[PointGroup.LatticeSystem.Monoclinic] = metric_compatibility[PointGroup.LatticeSystem.Monoclinic] + [
            PointGroup.LatticeSystem.Hexagonal
        ]

        for sg_name in SpaceGroupFactory.getAllSpaceGroupSymbols():
            sg = SpaceGroupFactory.createSpaceGroup(sg_name)
            lattice_system = sg.getPointGroup().getLatticeSystem()

            # Currently the c-unique monoclinic space groups are determined from the symbol
            if lattice_system != PointGroup.LatticeSystem.Monoclinic or sg_name[2:5] != "1 1":
                self._check_spacegroup(sg, cells, metric_compatibility[lattice_system])
            else:
                # These are the monoclinic space groups with unique c axis, which need a special treatment
                self._check_spacegroup(sg, monoclinic_c_cells, monoclinic_c_compatiblity[lattice_system])

    def _check_spacegroup(self, sg, cells, compatible_metrics):
        for system, cell in cells.items():
            is_allowed = sg.isAllowedUnitCell(cell)
            should_be_allowed = system in compatible_metrics

            self.assertEqual(
                is_allowed,
                should_be_allowed,
                "Problem in space group {0}: UnitCell with {1} metric is {2}, should be {3}.".format(
                    sg.getHMSymbol(), str(system), str(is_allowed), str(should_be_allowed)
                ),
            )

    def getUnitCells(self):
        # Create a unit cell for each of the lattice systems in a dictionary
        return {
            PointGroup.LatticeSystem.Triclinic: UnitCell(5.0, 6.0, 7.0, 100.0, 102.0, 103.0),
            PointGroup.LatticeSystem.Monoclinic: UnitCell(5.0, 6.0, 7.0, 90.0, 102.0, 90.0),
            PointGroup.LatticeSystem.Orthorhombic: UnitCell(5.0, 6.0, 7.0, 90.0, 90.0, 90.0),
            PointGroup.LatticeSystem.Rhombohedral: UnitCell(5.0, 5.0, 5.0, 80.0, 80.0, 80.0),
            PointGroup.LatticeSystem.Hexagonal: UnitCell(5.0, 5.0, 7.0, 90.0, 90.0, 120.0),
            PointGroup.LatticeSystem.Tetragonal: UnitCell(5.0, 5.0, 7.0, 90.0, 90.0, 90.0),
            PointGroup.LatticeSystem.Cubic: UnitCell(5.0, 5.0, 5.0, 90.0, 90.0, 90.0),
        }

    def getMetricCompatibility(self, cells):
        # This map specifies which metrics are compatible. A cell with cubic metric is for example compatible
        # with triclinic symmetry, but the opposite is not true.
        return {
            PointGroup.LatticeSystem.Triclinic: list(cells.keys()),
            PointGroup.LatticeSystem.Monoclinic: [
                PointGroup.LatticeSystem.Monoclinic,
                PointGroup.LatticeSystem.Orthorhombic,
                PointGroup.LatticeSystem.Tetragonal,
                PointGroup.LatticeSystem.Cubic,
            ],
            PointGroup.LatticeSystem.Orthorhombic: [
                PointGroup.LatticeSystem.Orthorhombic,
                PointGroup.LatticeSystem.Tetragonal,
                PointGroup.LatticeSystem.Cubic,
            ],
            PointGroup.LatticeSystem.Rhombohedral: [PointGroup.LatticeSystem.Rhombohedral, PointGroup.LatticeSystem.Cubic],
            PointGroup.LatticeSystem.Hexagonal: [PointGroup.LatticeSystem.Hexagonal],
            PointGroup.LatticeSystem.Tetragonal: [PointGroup.LatticeSystem.Tetragonal, PointGroup.LatticeSystem.Cubic],
            PointGroup.LatticeSystem.Cubic: [PointGroup.LatticeSystem.Cubic],
        }
