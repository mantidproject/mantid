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
    def test_correction_props(self):
        self.assertRaises(RuntimeError, absorptioncorrutils.create_absorption_input, '', None)

        charfile = "PG3_char_2020_01_04_PAC_limit_1.4MW.txt"
        charTable = PDLoadCharacterizations(Filename=charfile)
        char = charTable[0]

        data = Load(Filename="PG3_46577.nxs.h5", MetaDataOnly=True)

        PDDetermineCharacterizations(InputWorkspace=data,
                                     Characterizations=char,
                                     ReductionProperties="props")
        props = PropertyManagerDataService.retrieve("props")

        # Sample only absorption correction
        abs_sample = absorptioncorrutils.calculate_absorption_correction("PG3_46577.nxs.h5",
                                                                         "SampleOnly",
                                                                         props,
                                                                         "Si",
                                                                         1.165,
                                                                         element_size=2)
        self.assertIsNotNone(abs_sample[0])

        DeleteWorkspaces([char, data, abs_sample[0]])
        PropertyManagerDataService.remove('props')

    def test_correction_methods(self):
        # no caching
        sample_ws, container_ws = absorptioncorrutils.calculate_absorption_correction(
            '', "None", None, "V", 1.0)

        self.assertIsNone(sample_ws)
        self.assertIsNone(container_ws)

        # with caching
        sample_ws, container_ws = absorptioncorrutils.calculate_absorption_correction(
            '', "None", None, "V", 1.0, cache_dir=tempfile.gettempdir())

        self.assertIsNone(sample_ws)
        self.assertIsNone(container_ws)

    def test_instr_name_helper(self):
        fname = "PG3_46577.nxs.h5"

        # Test if name retrieved from filename
        instr_name = absorptioncorrutils._getInstrName(fname)
        self.assertEqual("PG3", instr_name)

        # Use dummy file name but make sure it is retrieved from workspace
        data = Load(Filename=fname, MetaDataOnly=True, AllowList="SampleFormula")
        instr_name = absorptioncorrutils._getInstrName("fakeinstrument", data)
        self.assertEqual("PG3", instr_name)

    def test_cache_filename_prefix(self):
        fname = "PG3_46577.nxs.h5"
        charfile = "PG3_char_2020_01_04_PAC_limit_1.4MW.txt"
        charTable = PDLoadCharacterizations(Filename=charfile)
        char = charTable[0]

        data = Load(Filename=fname, MetaDataOnly=True)
        PDDetermineCharacterizations(InputWorkspace=data,
                                     Characterizations=char,
                                     ReductionProperties="props")
        props = PropertyManagerDataService.retrieve("props")

        cachedir = tempfile.gettempdir()
        abs_s, abs_c = absorptioncorrutils.calculate_absorption_correction(fname,
                                                                           "SampleOnly",
                                                                           props,
                                                                           "Si",
                                                                           1.165,
                                                                           element_size=2,
                                                                           cache_dir=cachedir,
                                                                           prefix="FILENAME")
        self.assertIsNotNone(abs_s)

        # Compare against expected name using FILENAME prefix
        cached_wsname = absorptioncorrutils._getBasename(fname) + "_abs_correction_ass"
        self.assertEqual(mtd.doesExist(cached_wsname), True)

        # Remove the workspace from ADS and verify it can be found from disk
        DeleteWorkspaces(cached_wsname)

        abs_s, abs_c = absorptioncorrutils.calculate_absorption_correction(fname,
                                                                           "SampleOnly",
                                                                           props,
                                                                           "Si",
                                                                           1.165,
                                                                           element_size=2,
                                                                           cache_dir=cachedir,
                                                                           prefix="FILENAME")
        self.assertIsNotNone(abs_s)


if __name__ == '__main__':
    unittest.main()
