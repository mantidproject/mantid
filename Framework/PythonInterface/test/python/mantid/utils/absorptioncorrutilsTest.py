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
        self.assertRaises(RuntimeError, absorptioncorrutils.create_absorption_input, '', None)

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
        instr_name = absorptioncorrutils._getInstrName("fakeinstrument", mtd["data"])
        self.assertEqual("PG3", instr_name)

    def test_cache_filename_prefix(self):
        fname = "PG3_46577.nxs.h5"
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

    def test_cache_sha_prefix(self):
        fname = "PG3_46577.nxs.h5"
        props = PropertyManagerDataService.retrieve("props")

        cachedir = tempfile.gettempdir()
        abs_s, abs_c = absorptioncorrutils.calculate_absorption_correction(fname,
                                                                           "SampleOnly",
                                                                           props,
                                                                           "Si",
                                                                           1.165,
                                                                           element_size=2,
                                                                           cache_dir=cachedir,
                                                                           prefix="SHA")
        self.assertIsNotNone(abs_s)

        # Get what the cache SHA should be - verify the donor WS exists since that is used to make cache SHA
        donorws = "__{}_abs".format(absorptioncorrutils._getBasename(fname))
        self.assertEqual(mtd.doesExist(donorws), True)
        cache_prefix = absorptioncorrutils._getInstrName(fname, mtd[donorws])
        cachefile, sha = absorptioncorrutils._getCacheName(donorws, cache_prefix, cachedir,
                                                           "SampleOnly")

        # Compare against expected name using SHA prefix
        cached_wsname = cache_prefix + "_" + sha + "_abs_correction_ass"
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
                                                                           prefix="SHA")
        self.assertIsNotNone(abs_s)


if __name__ == '__main__':
    unittest.main()
