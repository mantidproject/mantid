# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantid.simpleapi import config
import ISISCommandInterface as i

MASKFILE = "MaskSANS2D.txt"


class Sans2DIsisGuiSettings(unittest.TestCase):
    def setUp(self):
        config["default.instrument"] = "SANS2D"
        i.SANS2D()
        i.MaskFile(MASKFILE)

    def checkFloat(self, f1, f2):
        self.assertAlmostEqual(f1, f2)

    def checkStr(self, s1, s2):
        self.assertEqual(s1, s2, "%s != %s" % (s1, s2))

    def checkObj(self, ob1, ob2):
        self.assertEqual(ob1, ob2, "%s != %s" % (str(ob1), str(ob2)))

    def test_pass(self):
        pass

    def test_read_set_gravity(self):  # GUI: gravity_check
        i.Gravity(True)
        self.assertTrue(i.ReductionSingleton().to_Q.get_gravity())
        i.Gravity(False)
        self.assertTrue(not i.ReductionSingleton().to_Q.get_gravity())

    def test_read_set_radius(self):  # GUI: rad_min, rad_max
        min_value, max_value = 1.2, 18.9
        i.ReductionSingleton().user_settings.readLimitValues("L/R %f %f" % (min_value, max_value), i.ReductionSingleton())
        self.checkFloat(i.ReductionSingleton().mask.min_radius, min_value / 1000)
        self.checkFloat(i.ReductionSingleton().mask.max_radius, max_value / 1000)

    def test_read_set_wav(self):  # GUI: wav_min, wav_max, wav_dw, wav_dw_opt
        min_value, max_value, step, option = 1.2, 4.5, 0.1, "LIN"
        i.LimitsWav(min_value, max_value, step, option)
        self.checkFloat(i.ReductionSingleton().to_wavelen.wav_low, min_value)
        self.checkFloat(i.ReductionSingleton().to_wavelen.wav_high, max_value)
        self.checkFloat(i.ReductionSingleton().to_wavelen.wav_step, step)
        min_value, max_value, step, option = 1.2, 4.5, 0.1, "LOG"
        i.LimitsWav(min_value, max_value, step, option)
        self.checkFloat(i.ReductionSingleton().to_wavelen.wav_low, min_value)
        self.checkFloat(i.ReductionSingleton().to_wavelen.wav_high, max_value)
        self.checkFloat(i.ReductionSingleton().to_wavelen.wav_step, -step)

    def test_wavRanges(self):  # wavRanges, wav_stack, wav_dw_opt
        # it seems to be accessible only through gui, changing the reduction
        # it will use CompWavRanges...
        pass

    def test_qx(self):  # GUI: q_min, q_max, q_dq, q_rebin, q_dq_opt
        def checkvalues(min_value, max_value, step_value, str_values):
            list_values = str_values.split(",")
            self.checkFloat(min_value, float(list_values[0]))
            self.checkFloat(max_value, float(list_values[2]))
            self.checkFloat(step_value, float(list_values[1]))

        def checklistvalues(str1, str2):
            list_v1 = str1.split(",")
            list_v2 = str2.split(",")
            self.checkFloat(len(list_v1), len(list_v2))
            for j in range(len(list_v1)):
                self.checkFloat(float(list_v1[j]), float(list_v2[j]))

        min_value, max_value, step_value = 0.01, 2.8, 0.02
        opt_pattern = "%f %f %f/%s"
        option = "LIN"
        min_max_step_option = opt_pattern % (min_value, max_value, step_value, option)
        i.ReductionSingleton().user_settings.readLimitValues("L/Q " + min_max_step_option, i.ReductionSingleton())
        checkvalues(min_value, max_value, step_value, i.ReductionSingleton().to_Q.binning)

        option = "LOG"
        min_max_step_option = opt_pattern % (min_value, max_value, step_value, option)
        i.ReductionSingleton().user_settings.readLimitValues("L/Q " + min_max_step_option, i.ReductionSingleton())
        checkvalues(min_value, max_value, -step_value, i.ReductionSingleton().to_Q.binning)

        rebining_option = ".001,.001,.0126,-.08,.2"
        i.ReductionSingleton().user_settings.readLimitValues("L/Q " + rebining_option, i.ReductionSingleton())
        checklistvalues(rebining_option, i.ReductionSingleton().to_Q.binning)

    def test_qy(self):  # qy_max, qy_dqy, qy_dqy_opt
        value_max, value_step = 0.06, 0.002
        i.LimitsQXY(0.0, value_max, value_step)
        self.checkFloat(i.ReductionSingleton().QXY2, value_max)
        self.checkFloat(i.ReductionSingleton().DQXY, value_step)

    def test_fit(self):
        def checkEqualsOption(option, selector):
            if option[1] is not None:
                self.checkFloat(i.ReductionSingleton().transmission_calculator.lambdaMin(selector), option[1])

            if option[2] is not None:
                self.checkFloat(i.ReductionSingleton().transmission_calculator.lambdaMax(selector), option[2])
            self.checkObj(i.ReductionSingleton().transmission_calculator.fitMethod(selector), str(option[0]).upper())

        def checkNotEqualOption(option, selector):
            self.assertTrue(not i.ReductionSingleton().transmission_calculator.fitMethod(selector) == str(option[0]).upper())
            if option[1] is not None:
                self.assertTrue(not i.ReductionSingleton().transmission_calculator.lambdaMin(selector) == option[1])
            if option[2] is not None:
                self.assertTrue(not i.ReductionSingleton().transmission_calculator.lambdaMax(selector) == option[2])

        def checkFitOption(option):
            if option[3] == "BOTH":
                checkEqualsOption(option, "SAMPLE")
                checkEqualsOption(option, "CAN")
            else:
                otheroption = "SAMPLE" if option[3] == "CAN" else "CAN"
                checkEqualsOption(option, option[3])
                checkNotEqualOption(option, otheroption)

        # transFitOnOff, transFit_ck, trans_min, trans_max, trans_opt
        # transFitOnOff_can, transFit_ck_can, trans_min_can, trans_max_can, trans_opt_can
        options = [
            ("Linear", 1.5, 12.5, "BOTH"),
            ("Logarithmic", 1.3, 12.3, "BOTH"),
            ("Polynomial3", 1.4, 12.4, "BOTH"),
            ("Logarithmic", 1.1, 12.1, "CAN"),
            ("Polynomial3", 1.6, 12.6, "SAMPLE"),
            ("Polynomial3", 1.2, 12.2, "SAMPLE"),
            ("Linear", 1.5, 12.5, "CAN"),
            ("Off", None, None, "CAN"),
            ("Polynomial4", None, None, "SAMPLE"),
            ("Linear", None, None, "CAN"),
            ("Off", None, None, "SAMPLE"),
            ("Linear", 2.5, 13.0, "CAN"),
        ]

        for option in options:
            print("Applying option ", str(option))
            i.TransFit(mode=option[0], lambdamin=option[1], lambdamax=option[2], selector=option[3])
            checkFitOption(option)

    def test_direct_files(self):  # direct_file, front_direct_file
        # this widget is read only, it does not allow changing.
        # It is changed only through the MaskFile

        ### THIS IS NOT DONE DIRECTLY THROUGH GHI ####
        i.ReductionSingleton().instrument.getDetector("REAR").correction_file = "rear_file"
        i.ReductionSingleton().instrument.getDetector("FRONT").correction_file = "front_file"

        ## This is done in the GUI ##
        self.checkStr(i.ReductionSingleton().instrument.detector_file("rear"), "rear_file")
        self.checkStr(i.ReductionSingleton().instrument.detector_file("front"), "front_file")

    def test_flood_files(self):  # floodRearFile, floolFrontFile
        options = [("REAR", "rear_file"), ("FRONT", "front_file"), ("REAR", ""), ("FRONT", "")]

        for option in options:
            i.SetDetectorFloodFile(option[1], option[0])
            self.checkStr(option[1], i.ReductionSingleton().prep_normalize.getPixelCorrFile(option[0]))

    def test_incident_monitors(self):  # monitor_spec, monitor_interp, trans_monitor, trans_interp
        options = [(2, True), (4, False), (3, True)]

        for option in options:
            i.SetMonitorSpectrum(option[0], option[1])
            self.checkFloat(i.ReductionSingleton().instrument.get_incident_mon(), option[0])
            self.checkObj(i.ReductionSingleton().instrument.is_interpolating_norm(), option[1])

            i.SetTransSpectrum(option[0], option[1])
            self.checkFloat(i.ReductionSingleton().instrument.incid_mon_4_trans_calc, option[0])
            self.checkObj(i.ReductionSingleton().transmission_calculator.interpolate, option[1])

    def test_detector_selector(self):
        options = [
            ("front-detector", "front-detector"),
            ("rear-detector", "rear-detector"),
            ("both", "rear-detector"),
            ("merged", "rear-detector"),
            ("FRONT", "front-detector"),
            ("REAR", "rear-detector"),
        ]

        for option in options:
            i.ReductionSingleton().instrument.setDetector(option[0])
            self.checkStr(i.ReductionSingleton().instrument.det_selection, option[0])
            self.checkStr(i.ReductionSingleton().instrument.cur_detector().name(), option[1])

        # TODO: for LOQ

    def test_Phi(self):  # phi_min, phi_max, mirror_phi
        def checkPhiValues(option):
            self.checkFloat(i.ReductionSingleton().mask.phi_min, option[0])
            self.checkFloat(i.ReductionSingleton().mask.phi_max, option[1])
            self.checkObj(i.ReductionSingleton().mask.phi_mirror, option[2])

        options = [(-88.0, 89.0, True), (-87.0, 88.5, False), (-90.0, 90.0, True)]

        for option in options:
            i.SetPhiLimit(option[0], option[1], option[2])
            checkPhiValues(option)

        for option in options:
            cmd = "L/PHI" if option[2] else "L/PHI/NOMIRROR"
            patter = "%s %f %f"
            i.Mask(patter % (cmd, option[0], option[1]))
            checkPhiValues(option)

    def test_scaling_options(self):
        # frontDetRescale, frontDetShift, frontDetRescaleCB, frontDetShiftCB
        # frontDetQmin, frontDetQmax, frontDetQrangeOnOff

        def testScalingValues(scale, shift, qMin=None, qMax=None, fitScale=False, fitShift=False):
            self.checkFloat(i.ReductionSingleton().instrument.getDetector("FRONT").rescaleAndShift.scale, scale)
            self.checkFloat(i.ReductionSingleton().instrument.getDetector("FRONT").rescaleAndShift.shift, shift)
            self.checkObj(i.ReductionSingleton().instrument.getDetector("FRONT").rescaleAndShift.fitScale, fitScale)
            self.checkObj(i.ReductionSingleton().instrument.getDetector("FRONT").rescaleAndShift.fitShift, fitShift)
            if qMin is not None:
                self.assertTrue(i.ReductionSingleton().instrument.getDetector("FRONT").rescaleAndShift.qRangeUserSelected)
                self.checkFloat(i.ReductionSingleton().instrument.getDetector("FRONT").rescaleAndShift.qMin, qMin)
                self.checkFloat(i.ReductionSingleton().instrument.getDetector("FRONT").rescaleAndShift.qMax, qMax)
            else:
                self.assertTrue(not i.ReductionSingleton().instrument.getDetector("FRONT").rescaleAndShift.qRangeUserSelected)

        options = [
            {"scale": 1.2, "shift": 0.3},
            {"scale": 1.3, "shift": 0.4, "fitScale": True},
            {"scale": 1.2, "shift": 0.3, "fitShift": True},
            {"scale": 1.4, "shift": 0.32, "qMin": 0.5, "qMax": 2.1},
            {"scale": 1.2, "shift": 0.41, "qMin": 0.53, "qMax": 2.12, "fitScale": True},
        ]

        for option in options:
            i.SetFrontDetRescaleShift(**option)
            testScalingValues(**option)

    def test_BACK(self):
        tofs = i.ReductionSingleton().instrument.get_TOFs(2)
        self.checkFloat(tofs[0], 85000)
        self.checkFloat(tofs[1], 98000)
        tofs = i.ReductionSingleton().instrument.get_TOFs(1)
        self.checkFloat(tofs[0], 35000)
        self.checkFloat(tofs[1], 65000)

    def test_time_shifts_for_added_event_files(self):
        # Check for correct input
        shifts = "12, 13, 14"
        num_files = 4
        output = i.check_time_shifts_for_added_event_files(number_of_files=num_files, time_shifts=shifts)
        self.assertTrue(not output)

        # Check warning is produced for mismatch in number of shifts and files, ideally #shifts = N-1 and #files=N
        shifts2 = "12, 13"
        num_files2 = 4
        output2 = i.check_time_shifts_for_added_event_files(number_of_files=num_files2, time_shifts=shifts2)
        self.assertTrue(output2.startswith("Error: Expected N-1 time shifts "))

        # Check error is produced for lexically incorrect shifts, ie shifts which cannot be converted to float
        shifts3 = "12, 13, a"
        num_files3 = 4
        output3 = i.check_time_shifts_for_added_event_files(number_of_files=num_files3, time_shifts=shifts3)
        self.assertTrue(output3.startswith("Error: Elements of the time"))


if __name__ == "__main__":
    unittest.main()
