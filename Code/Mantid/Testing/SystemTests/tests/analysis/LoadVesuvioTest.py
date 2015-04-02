#pylint: disable=invalid-name,no-init
import stresstesting

from mantid.api import MatrixWorkspace, mtd
from mantid.simpleapi import LoadVesuvio

import unittest

DIFF_PLACES = 12

class VesuvioTests(unittest.TestCase):

    ws_name = "evs_raw"


    def tearDown(self):
        if self.ws_name in mtd:
            mtd.remove(self.ws_name)

    #================== Success cases ================================
    def test_load_with_back_scattering_spectra_produces_correct_workspace(self):
        self._run_load("14188", "3-134", "DoubleDifference")

        # Check some data
        evs_raw = mtd[self.ws_name]
        self.assertAlmostEqual(0.078968412230231877, evs_raw.readY(0)[1], places=DIFF_PLACES)
        self.assertAlmostEqual(0.12162310222873171, evs_raw.readE(0)[1], places=DIFF_PLACES)
        self.assertAlmostEqual(0.018091076761311387, evs_raw.readY(131)[1188], places=DIFF_PLACES)
        self.assertAlmostEqual(0.063175962622448692, evs_raw.readE(131)[1188], places=DIFF_PLACES)

    def test_consecutive_runs_with_back_scattering_spectra_gives_expected_numbers(self):
        self._run_load("14188-14190", "3-134", "DoubleDifference")

        # Check some data
        evs_raw = mtd[self.ws_name]
        self.assertAlmostEqual(0.12812011879757312, evs_raw.readY(0)[1], places=DIFF_PLACES)
        self.assertAlmostEqual(0.07005709042418834, evs_raw.readE(0)[1], places=DIFF_PLACES)
        self.assertAlmostEqual(0.038491709460370394, evs_raw.readY(131)[1188], places=DIFF_PLACES)
        self.assertAlmostEqual(0.036783617369284975, evs_raw.readE(131)[1188], places=DIFF_PLACES)

    def test_non_consecutive_runs_with_back_scattering_spectra_gives_expected_numbers(self):
        self._run_load("14188,14190", "3-134", "DoubleDifference")

        # Check some data
        evs_raw = mtd[self.ws_name]
        self.assertAlmostEqual(0.17509520926405386, evs_raw.readY(0)[1], places=DIFF_PLACES)
        self.assertAlmostEqual(0.085651536076367191, evs_raw.readE(0)[1], places=DIFF_PLACES)
        self.assertAlmostEqual(-0.027855932189430499, evs_raw.readY(131)[1188], places=DIFF_PLACES)
        self.assertAlmostEqual(0.044991428219920804, evs_raw.readE(131)[1188], places=DIFF_PLACES)

    def test_load_with_forward_scattering_spectra_produces_correct_workspace(self):
        self._run_load("14188", "135-198", "SingleDifference")

        # Check some data
        evs_raw = mtd[self.ws_name]
        self.assertAlmostEqual(-0.4421157823659172, evs_raw.readY(0)[1], places=DIFF_PLACES)
        self.assertAlmostEqual(0.23849110331150025, evs_raw.readE(0)[1], places=DIFF_PLACES)
        self.assertAlmostEqual(-0.030129475930755989, evs_raw.readY(63)[1188], places=DIFF_PLACES)
        self.assertAlmostEqual(0.23849110331150025, evs_raw.readE(0)[1], places=DIFF_PLACES)

    def test_consecutive_runs_with_forward_scattering_spectra_gives_expected_numbers(self):
        self._run_load("14188-14190", "135-198", "SingleDifference")

        # Check some data
        evs_raw = mtd[self.ws_name]
        self.assertAlmostEqual(-0.33023675686822429, evs_raw.readY(0)[1], places=DIFF_PLACES)
        self.assertAlmostEqual(0.13839181298987582, evs_raw.readE(0)[1], places=DIFF_PLACES)
        self.assertAlmostEqual(-0.0005762703884557574, evs_raw.readY(63)[1188], places=DIFF_PLACES)
        self.assertAlmostEqual(0.022314627606989094, evs_raw.readE(63)[1188], places=DIFF_PLACES)

    def test_non_consecutive_runs_with_forward_scattering_spectra_gives_expected_numbers(self):
        self._run_load("14188,14190", "135-198", "SingleDifference")

        # Check some data
        evs_raw = mtd[self.ws_name]
        self.assertAlmostEqual(-0.31382658620745474, evs_raw.readY(0)[1], places=DIFF_PLACES)
        self.assertAlmostEqual(0.16935354944452052, evs_raw.readE(0)[1], places=DIFF_PLACES)
        self.assertAlmostEqual(0.0013599866184859088, evs_raw.readY(63)[1188], places=DIFF_PLACES)
        self.assertAlmostEqual(0.16935354944452052, evs_raw.readE(0)[1], places=DIFF_PLACES)

    def test_load_with_spectra_mixed_from_forward_backward_gives_expected_numbers(self):
        self._run_load("14188", "134,135", "DoubleDifference")

        # Check some data
        evs_raw = mtd[self.ws_name]
        self.assertAlmostEqual(0.43816507168120111, evs_raw.readY(0)[1], places=DIFF_PLACES)
        self.assertAlmostEqual(0.23224859590051541, evs_raw.readE(0)[1], places=DIFF_PLACES)
        self.assertAlmostEqual(0.013611354662030284, evs_raw.readY(1)[1188], places=DIFF_PLACES)
        self.assertAlmostEqual(0.031506182465619419, evs_raw.readE(1)[1188], places=DIFF_PLACES)

    def test_foilout_mode_gives_expected_numbers(self):
        self._run_load("14188", "3", "FoilOut")

        evs_raw = mtd[self.ws_name]
        self.assertAlmostEqual(18753.00, evs_raw.readY(0)[1], places=DIFF_PLACES)
        self.assertAlmostEqual(136.94159338929865, evs_raw.readE(0)[1], places=DIFF_PLACES)

    def test_foilin_mode_gives_expected_numbers(self):
        self._run_load("14188", "3", "FoilIn")

        evs_raw = mtd[self.ws_name]
        self.assertAlmostEqual(37594.0, evs_raw.readY(0)[1], places=DIFF_PLACES)
        self.assertAlmostEqual(193.89172236070317, evs_raw.readE(0)[1], places=DIFF_PLACES)

    def test_using_ip_file_adjusts_instrument_and_attaches_parameters(self):
        self._run_load("14188", "3", "SingleDifference","IP0005.dat")

        # Check some data
        evs_raw = mtd[self.ws_name]
        det0 = evs_raw.getDetector(0)
        param = det0.getNumberParameter("t0")
        self.assertEqual(1, len(param))
        self.assertAlmostEqual(-0.4157, param[0],places=4)

    def test_sumspectra_set_to_true_gives_single_spectra_summed_over_all_inputs(self):
        self._run_load("14188", "135-142", "SingleDifference","IP0005.dat",sum=True)
        evs_raw = mtd[self.ws_name]

        # Verify
        self.assertEquals(1, evs_raw.getNumberHistograms())
        self.assertAlmostEqual(5.0, evs_raw.readX(0)[0], places=DIFF_PLACES)
        self.assertAlmostEqual(599.5, evs_raw.readX(0)[-1], places=DIFF_PLACES)
        self.assertAlmostEqual(-1.5288171762918328, evs_raw.readY(0)[0], places=DIFF_PLACES)
        self.assertAlmostEqual(-0.079412793053402098, evs_raw.readY(0)[-1], places=DIFF_PLACES)
        self.assertAlmostEqual(0.52109203357613976, evs_raw.readE(0)[0], places=DIFF_PLACES)
        self.assertAlmostEqual(0.10617318614513051, evs_raw.readE(0)[-1], places=DIFF_PLACES)

    def test_sumspectra_with_multiple_groups_gives_number_output_spectra_as_input_groups(self):
        self._run_load("14188", "135-148;152-165", "SingleDifference","IP0005.dat",sum=True)
        evs_raw = mtd[self.ws_name]

        # Verify
        self.assertEquals(2, evs_raw.getNumberHistograms())
        self.assertAlmostEqual(5.0, evs_raw.readX(0)[0], places=DIFF_PLACES)
        self.assertAlmostEqual(5.0, evs_raw.readX(1)[0], places=DIFF_PLACES)
        self.assertAlmostEqual(599.5, evs_raw.readX(0)[-1], places=DIFF_PLACES)
        self.assertAlmostEqual(599.5, evs_raw.readX(1)[-1], places=DIFF_PLACES)
        self.assertAlmostEqual(-0.713877795283, evs_raw.readY(0)[0], places=DIFF_PLACES)
        self.assertAlmostEqual(-3.00125465604, evs_raw.readY(1)[0], places=DIFF_PLACES)
        self.assertAlmostEqual(0.6219299465, evs_raw.readE(0)[0], places=DIFF_PLACES)
        self.assertAlmostEqual(0.676913729914, evs_raw.readE(1)[0], places=DIFF_PLACES)

    def test_sumspectra_set_to_true_gives_single_spectra_summed_over_all_inputs_with_foil_in(self):
        self._run_load("14188", "3-15", "FoilIn", "IP0005.dat", sum=True)
        evs_raw = mtd[self.ws_name]

        # Verify
        self.assertEquals(1, evs_raw.getNumberHistograms())
        self.assertAlmostEqual(5.0, evs_raw.readX(0)[0], places=DIFF_PLACES)
        self.assertAlmostEqual(19990.0, evs_raw.readX(0)[-1], places=DIFF_PLACES)
        self.assertAlmostEqual(497722.0, evs_raw.readY(0)[0], places=DIFF_PLACES)
        self.assertAlmostEqual(2072.0, evs_raw.readY(0)[-1], places=DIFF_PLACES)
        self.assertAlmostEqual(705.49415305869115, evs_raw.readE(0)[0], places=DIFF_PLACES)
        self.assertAlmostEqual(45.519226706964169, evs_raw.readE(0)[-1], places=DIFF_PLACES)

    def test_sumspectra_with_multiple_groups_gives_number_output_spectra_as_input_groups_with_foil_in(self):
        self._run_load("14188", "3-15;30-50", "FoilIn", "IP0005.dat", sum=True)
        evs_raw = mtd[self.ws_name]

        # Verify
        self.assertEquals(2, evs_raw.getNumberHistograms())
        self.assertAlmostEqual(5.0, evs_raw.readX(0)[0], places=DIFF_PLACES)
        self.assertAlmostEqual(5.0, evs_raw.readX(1)[0], places=DIFF_PLACES)
        self.assertAlmostEqual(19990.0, evs_raw.readX(0)[-1], places=DIFF_PLACES)
        self.assertAlmostEqual(19990.0, evs_raw.readX(1)[-1], places=DIFF_PLACES)
        self.assertAlmostEqual(497722.0, evs_raw.readY(0)[0], places=DIFF_PLACES)
        self.assertAlmostEqual(1332812.0, evs_raw.readY(1)[0], places=DIFF_PLACES)
        self.assertAlmostEqual(705.49415305869115, evs_raw.readE(0)[0], places=DIFF_PLACES)
        self.assertAlmostEqual(1154.4747723532116, evs_raw.readE(1)[0], places=DIFF_PLACES)

    def _run_load(self, runs, spectra, diff_opt, ip_file="", sum=False):
        LoadVesuvio(Filename=runs,OutputWorkspace=self.ws_name,
                    SpectrumList=spectra,Mode=diff_opt,InstrumentParFile=ip_file,
                    SumSpectra=sum)

        self._do_ads_check(self.ws_name)

        def expected_size():
            if sum:
                if ";" in spectra:
                    return 2
                else:
                    return 1
            elif "-" in spectra:
                elements = spectra.split("-")
                min,max=(int(elements[0]), int(elements[1]))
                return max - min + 1
            elif "," in spectra:
                elements = spectra.strip().split(",")
                return len(elements)
            else:
                return 1

        self._do_size_check(self.ws_name, expected_size())
        loaded_data = mtd[self.ws_name]
        if "Difference" in diff_opt:
            self.assertTrue(not loaded_data.isHistogramData())
        else:
            self.assertTrue(loaded_data.isHistogramData())

    def _do_ads_check(self, name):
        self.assertTrue(name in mtd)
        self.assertTrue(type(mtd[name]) == MatrixWorkspace)

    def _do_size_check(self,name, expected_nhist):
        loaded_data = mtd[name]
        self.assertEquals(expected_nhist, loaded_data.getNumberHistograms())

    #================== Failure cases ================================

    def test_missing_spectra_property_raises_error(self):
        self.assertRaises(RuntimeError, LoadVesuvio, Filename="14188",
                          OutputWorkspace=self.ws_name)

    def test_load_with_invalid_spectra_raises_error(self):
        self.assertRaises(RuntimeError, LoadVesuvio, Filename="14188",
                          OutputWorkspace=self.ws_name, SpectrumList="200")

    def test_load_with_spectra_that_are_just_monitors_raises_error(self):
        self.assertRaises(RuntimeError, LoadVesuvio, Filename="14188",
          OutputWorkspace=self.ws_name, SpectrumList="1")
        self.assertRaises(RuntimeError, LoadVesuvio, Filename="14188",
                          OutputWorkspace=self.ws_name, SpectrumList="1-2")

    def test_load_with_invalid_difference_option_raises_error(self):
        self.assertRaises(ValueError, LoadVesuvio, Filename="14188",
          OutputWorkspace=self.ws_name, Mode="Unknown",SpectrumList="3-134")

    def test_load_with_difference_option_not_applicable_to_current_spectra_raises_error(self):
        self.assertRaises(ValueError, LoadVesuvio, Filename="14188",
          OutputWorkspace=self.ws_name, Mode="",SpectrumList="3-134")

    def test_raising_error_removes_temporary_raw_workspaces(self):
        self.assertRaises(RuntimeError, LoadVesuvio, Filename="14188,14199", # Second run is invalid
          OutputWorkspace=self.ws_name, Mode="SingleDifference",SpectrumList="3-134")

        self._do_test_temp_raw_workspaces_not_left_around()

    def _do_test_temp_raw_workspaces_not_left_around(self):
        self.assertTrue("__loadraw_evs" not in mtd)
        self.assertTrue("__loadraw_evs_monitors" not in mtd)


#====================================================================================

class LoadVesuvioTest(stresstesting.MantidStressTest):

    def runTest(self):
        self._success = False
        # Custom code to create and run this single test suite
        suite = unittest.TestSuite()
        suite.addTest( unittest.makeSuite(VesuvioTests, "test") )
        runner = unittest.TextTestRunner()
        # Run using either runner
        res = runner.run(suite)
        if res.wasSuccessful():
            self._success = True
        else:
            self._success = False

    def validate(self):
        return self._success
