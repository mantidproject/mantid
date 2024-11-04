# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init,invalid-name
import systemtesting
from mantid.simpleapi import PoldiCreatePeaksFromCell
from mantid.geometry import SpaceGroupFactory


class SpaceGroupReflectionConditionsTest(systemtesting.MantidSystemTest):
    """
    This test uses PoldiCreatePeaksFromCell to generate lists of reflections for fake crystal structures, one for each
    registered space group and one atom in the general position. The algorithm uses structure factor calculation for
    determining whether a reflection is present or not, so it also generates additional absences if atoms are on
    special positions.

    Since this is not the case in the examples, space group symmetry must account for all observed reflections, so they
    must be allowed.
    """

    def runTest(self):
        sgTestDict = self.generateReflectionLists()

        for sgName, hkls in sgTestDict.items():
            sg = SpaceGroupFactory.createSpaceGroup(sgName)

            for hkl in hkls:
                self.assertTrue(sg.isAllowedReflection(hkl), "Space group " + sgName + ": problem with hkl: " + str(hkl) + ".")

    def generateReflectionLists(self):
        # Common parameters for PoldiCreatePeaksFromCell
        # Additional lattice parameters are ignored (e.g. all except a in cubic, all except a and c in tetragonal, etc.)
        # so they can be supplied anyway for simplicity.
        parameters = {
            "Atoms": "Fe 0.3421 0.5312 0.7222",
            "a": 5.632,
            "b": 6.121,
            "c": 7.832,
            "Alpha": 101.5,
            "Beta": 102.3,
            "Gamma": 100.75,
            "LatticeSpacingMin": 0.75,
        }

        # Some space groups
        spaceGroups = SpaceGroupFactory.getAllSpaceGroupSymbols()
        sgDict = {}
        for sg in spaceGroups:
            try:
                reflectionsWs = PoldiCreatePeaksFromCell(SpaceGroup=sg, **parameters)

                # extract HKLs
                hkls = [[int(m) for m in x.split()] for x in reflectionsWs.column(0)]
                sgDict[sg] = hkls
            except ValueError:
                print(sg)

        return sgDict
