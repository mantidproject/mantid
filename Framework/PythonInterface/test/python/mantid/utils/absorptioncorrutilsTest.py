# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
import tempfile
from mantid.kernel import PropertyManagerDataService
from mantid.simpleapi import Load, PDLoadCharacterizations, PDDetermineCharacterizations, DeleteWorkspaces, mtd
from mantid.utils import absorptioncorrutils


class AbsorptionCorrUtilsTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        # Load common data used in several tests
        Load(Filename="PG3_46577.nxs.h5", MetaDataOnly=True, OutputWorkspace="data")

        charfile = "PG3_char_2020_01_04_PAC_limit_1.4MW.txt"
        charTable = PDLoadCharacterizations(Filename=charfile)
        char = charTable[0]

        PDDetermineCharacterizations(InputWorkspace=mtd["data"],
                                     Characterizations=char,
                                     ReductionProperties="props")

    @classmethod
    def tearDownClass(cls):
        mtd.clear()
        PropertyManagerDataService.remove('props')

    def test_correction_props(self):
        # must supply the filename
        self.assertRaises(ValueError, absorptioncorrutils.create_absorption_input, '', None)
        # must supply some way of determining wavelength range
        self.assertRaises(ValueError, absorptioncorrutils.create_absorption_input, 'PG3_46577.nxs.h5', None)

        props = PropertyManagerDataService.retrieve("props")

        # Sample only absorption correction
        abs_sample = absorptioncorrutils.calculate_absorption_correction("PG3_46577.nxs.h5",
                                                                         "SampleOnly",
                                                                         props,
                                                                         "Si",
                                                                         1.165,
                                                                         element_size=2)
        self.assertIsNotNone(abs_sample[0])
        DeleteWorkspaces([abs_sample[0]])

    def test_correction_methods(self):
        # no caching
        sample_ws, container_ws = absorptioncorrutils.calculate_absorption_correction(
            '', "None", None, "V", 1.0)

        self.assertIsNone(sample_ws)
        self.assertIsNone(container_ws)

        sample_ws, container_ws = absorptioncorrutils.calculate_absorption_correction(
            '', "None", None, "V", 1.0, cache_dirs=[tempfile.gettempdir()])

        self.assertIsNone(sample_ws)
        self.assertIsNone(container_ws)

    def test_cache(self):
        fname = "PG3_46577.nxs.h5"
        props = PropertyManagerDataService.retrieve("props")

        cachedirs = [tempfile.gettempdir()] * 3
        abs_s, _ = absorptioncorrutils.calculate_absorption_correction(
            fname,
            "SampleOnly",
            props,
            "Si",
            1.165,
            element_size=2,
            cache_dirs=cachedirs,
        )
        self.assertIsNotNone(abs_s)

        abs_s, _ = absorptioncorrutils.calculate_absorption_correction(
            fname,
            "SampleOnly",
            props,
            "Si",
            1.165,
            element_size=2,
            cache_dirs=cachedirs,
            )
        self.assertIsNotNone(abs_s)


if __name__ == '__main__':
    unittest.main()
