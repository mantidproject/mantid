# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name
import os
import sys
import systemtesting
from mantid.simpleapi import *
from mantid.api import Workspace, IEventWorkspace

from Direct.PropertyManager import PropertyManager
import ISIS_MariReduction as mr

try:
    from importlib import reload
except ImportError:
    pass

# ----------------------------------------------------------------------


class ISIS_ReductionWebLike(systemtesting.MantidSystemTest):
    def __init__(self):
        systemtesting.MantidSystemTest.__init__(self)

        # prepare reduction variable
        self.rd = mr.ReduceMARIFromFile()
        self.rd.def_main_properties()
        self.rd.def_advanced_properties()

        save_folder = config["defaultsave.directory"]

        self.rd.save_web_variables(os.path.join(save_folder, "reduce_vars.py"))

    def runTest(self):
        # run reduction using saved variables like web variables
        web_var_folder = config["defaultsave.directory"]
        sys.path.insert(0, web_var_folder)
        reload(mr)

        # change these variables to save result as nxs workspace
        mr.web_var.advanced_vars["save_format"] = "nxs"
        # web services currently needs input file to be defined
        input_file = "MAR11001.RAW"
        mr.main(input_file, web_var_folder)

        #  verify if result was indeed written
        self.rd.reducer.sample_run = input_file
        saveFileName = self.rd.reducer.save_file_name
        oputputFile = os.path.join(web_var_folder, saveFileName + ".nxs")

        self.assertTrue(os.path.exists(oputputFile))

        web_var_file = os.path.join(web_var_folder, "reduce_vars")
        if os.path.exists(web_var_file + ".py"):
            os.remove(web_var_file + ".py")
        if os.path.exists(web_var_file + ".pyc"):
            os.remove(web_var_file + ".pyc")

    def get_result_workspace(self):
        """Returns the result workspace to be checked"""
        if "outWS" in mtd:
            return "outWS"
        saveFileName = self.rd.reducer.save_file_name
        Load(Filename=saveFileName + ".nxs", OutputWorkspace="outWS")
        # outWS *= 0.997979227566217
        fullRezPath = FileFinder.getFullPath(saveFileName + ".nxs")
        os.remove(fullRezPath)
        return "outWS"

    def get_reference_file(self):
        return "MARIReduction.nxs"

    def validate(self):
        """Returns the name of the workspace & file to compare"""
        # tolerance defined outside of init
        # pylint: disable=W0201
        self.tolerance = 1e-6
        # tolerance_is_rel_err defined outside of init
        # pylint: disable=W0201
        self.tolerance_is_rel_err = True
        self.disableChecking.append("SpectraMap")
        self.disableChecking.append("Instrument")
        self.disableChecking.append("Sample")
        result = self.get_result_workspace()
        reference = self.get_reference_file()
        return result, reference


class ISIS_ReductionWrapperValidate(systemtesting.MantidSystemTest):
    def __init__(self):
        systemtesting.MantidSystemTest.__init__(self)
        self.result = False

    def runTest(self):
        # prepare reduction variable
        # At the moment MARI reduction differs from it original by
        # less then 1% due to changes in the procedure. At the moment
        # we have to account for this but when we make it the same,
        # the code below should be commented. Meanwhile it tests workspace
        # workflow
        # ------------------------------------------------------
        # ref_file = 'MARIReduction.nxs'
        # file = FileFinder.getFullPath(ref_file)
        # etalon_ws = Load(file)
        # etalon_ws/=0.997979227566217
        # ------------------------------------------------------
        rd = mr.ReduceMARIFromFile()
        rd.def_main_properties()
        rd.def_advanced_properties()
        # this is correct workflow for the ref file
        # rd.reducer.prop_man.save_file_name = ref_file
        # temporary workflow, until we fix workspace adjustment
        # disable pylint -- access to protected member
        # pylint: disable=W0212
        rd._tolerr = 1.0e-6
        rd.reducer.prop_man.save_file_name = "MARIReduction.nxs"
        rd.validate_run_number = 11001
        try:
            rez, mess = rd.run_reduction()
            self.result = rez
            if not rez:
                print("*** Validation failed: {0}".format(mess))
            if mess.find("Created") > -1:  # validation still failed due to missing validation file
                print("*** Validation failed: {0}".format(mess))
                self.result = False
        except RuntimeError as err:
            print("*** Validation failed with error: {0}".format(err.message))
            self.result = False
        rd.reducer.prop_man.save_file_name = None

    def validate(self):
        """Returns the name of the workspace & file to compare"""
        return self.result


# ----------------------------------------------------------------------
class ISISLoadFilesRAW(systemtesting.MantidSystemTest):
    def __init__(self):
        systemtesting.MantidSystemTest.__init__(self)
        self.valid = False

    def runTest(self):
        propman = PropertyManager("MAR")

        propman.sample_run = 11001
        propman.load_monitors_with_workspace = True

        mon_ws = PropertyManager.sample_run.get_monitors_ws()
        ws = PropertyManager.sample_run.get_workspace()

        self.assertTrue(isinstance(ws, Workspace))
        self.assertEqual(ws.getNumberHistograms(), 922)

        DeleteWorkspace(ws)

        propman.load_monitors_with_workspace = False
        propman.sample_run = 11001
        ws = PropertyManager.sample_run.get_workspace()
        mon_ws = PropertyManager.sample_run.get_monitors_ws()

        self.assertEqual(ws.getNumberHistograms(), 919)
        self.assertEqual(mon_ws.getNumberHistograms(), 3)

        #
        propman = PropertyManager("MAPS")
        propman.sample_run = 17186
        propman.load_monitors_with_workspace = False

        mon_ws = PropertyManager.sample_run.get_monitors_ws()
        ws = PropertyManager.sample_run.get_workspace()
        self.assertTrue(isinstance(ws, Workspace))
        self.assertEqual(ws.getNumberHistograms(), 41472)
        self.assertEqual(mon_ws.getNumberHistograms(), 4)

        #
        self.valid = True

    def validate(self):
        return self.valid


class ISISLoadFilesMER(systemtesting.MantidSystemTest):
    def __init__(self):
        systemtesting.MantidSystemTest.__init__(self)
        self.valid = False

    def runTest(self):
        #
        propman = PropertyManager("MER")
        propman.sample_run = 6398  # (raw file)
        propman.det_cal_file = 6399
        propman.load_monitors_with_workspace = False
        propman.mon1_norm_spec = 69633
        propman.ei_mon1_spec = [69634, 69635, 69636, 69637]
        propman.ei_mon2_spec = [69638, 69639, 69640, 69641]

        mon_ws = PropertyManager.sample_run.get_monitors_ws()
        self.assertNotEqual(mon_ws, None)

        ws = PropertyManager.sample_run.get_workspace()
        self.assertTrue(isinstance(ws, Workspace))
        self.assertEqual(ws.getNumberHistograms(), 69632)
        self.assertEqual(mon_ws.getNumberHistograms(), 9)

        propman.sample_run = None  # (clean things up)
        propman.load_monitors_with_workspace = True
        propman.sample_run = 6398

        mon_ws = PropertyManager.sample_run.get_monitors_ws()
        self.assertNotEqual(mon_ws, None)
        ws = PropertyManager.sample_run.get_workspace()
        self.assertTrue(isinstance(ws, Workspace))
        self.assertEqual(ws.getNumberHistograms(), 69641)
        self.assertEqual(mon_ws.getNumberHistograms(), 69641)

        propman.sample_run = 18492  # (histogram nxs file )
        propman.det_cal_file = None
        mon_ws = PropertyManager.sample_run.get_monitors_ws()
        self.assertTrue("SR_MER018492" in mtd)
        self.assertNotEqual(mon_ws, None)
        ws = PropertyManager.sample_run.get_workspace()
        self.assertTrue(isinstance(ws, Workspace))
        self.assertEqual(ws.getNumberHistograms(), 69641)
        self.assertEqual(mon_ws.getNumberHistograms(), 69641)
        self.assertEqual(mon_ws.getIndexFromSpectrumNumber(69638), 69637)
        det = mon_ws.getDetector(69632)
        self.assertTrue(det.isMonitor())
        det = mon_ws.getDetector(69631)
        self.assertFalse(det.isMonitor())

        #  enable when bug #10980 is fixed
        propman.sample_run = None  # delete all
        self.assertFalse("SR_MER018492" in mtd)
        propman.sample_run = 18492  # (histogram nxs file )
        propman.load_monitors_with_workspace = False
        propman.det_cal_file = None
        mon_ws = PropertyManager.sample_run.get_monitors_ws()
        self.assertNotEqual(mon_ws, None)
        self.assertTrue("SR_MER018492_monitors" in mtd)

        ws = PropertyManager.sample_run.get_workspace()
        self.assertTrue(isinstance(ws, Workspace))
        self.assertEqual(ws.getNumberHistograms(), 69632)
        self.assertEqual(mon_ws.getNumberHistograms(), 9)
        self.assertEqual(mon_ws.getIndexFromSpectrumNumber(69633), 0)
        det = mon_ws.getDetector(0)
        self.assertTrue(det.isMonitor())

        ei_ws = GetAllEi(mon_ws, 69634, 69638, IgnoreSecondMonitor=False)
        self.assertTrue(isinstance(ei_ws, Workspace))

        en_peaks = ei_ws.readX(0)
        self.assertAlmostEqual(len(en_peaks), 1)
        self.assertAlmostEqual(en_peaks[0], 108.94, 2)

        self.valid = True

    def validate(self):
        return self.valid


class ISISLoadFilesLET(systemtesting.MantidSystemTest):
    def __init__(self):
        systemtesting.MantidSystemTest.__init__(self)
        self.valid = False

    def runTest(self):
        #
        propman = PropertyManager("LET")

        propman.sample_run = 6278  # event nexus file
        propman.load_monitors_with_workspace = False

        # Here we have known problem of propman loading new IDF, and
        # workspace is written using old IDF. New IDF has mon1_norm_spec =73729
        # and ei_mon1_spec=73734   (on January 2015) and old
        # IDF -- mon1_norm_spec =40961 and 40966 (forever)
        # Normalized by monitor-1. -- need monitor1 and ei needs ei_mon1_spec
        # This problem is hopefully fixed in reduction now, but here
        # we have to specify these values manually to guard against
        # changes in a future. This issue should be now fixed through varions means
        propman.normalise_method = "monitor-1"
        # Adjust old IDF to new changes. This may change when loader is modified.
        propman.ei_mon2_spec = [5506, 5505, 5507]
        propman.spectra_to_monitors_list = [5506, 5505, 5507]
        #
        ws = PropertyManager.sample_run.get_workspace()
        # apply IDF property, correspondent to this particular time interval
        propman.update_defaults_from_instrument(ws.getInstrument())
        self.assertEqual(int(propman.mon1_norm_spec), 40961)
        self.assertEqual(propman.ei_mon1_spec, 40966)

        mon_ws = PropertyManager.sample_run.get_monitors_ws()
        self.assertNotEqual(mon_ws, None)

        self.assertTrue(isinstance(ws, IEventWorkspace))
        self.assertEqual(ws.getNumberHistograms(), 40960)
        self.assertTrue(isinstance(mon_ws, Workspace))
        #
        self.assertEqual(mon_ws.getNumberHistograms(), 11)

        ei_ws = GetAllEi(mon_ws, 40966, 40967, IgnoreSecondMonitor=True)
        self.assertTrue(isinstance(ei_ws, Workspace))

        self.valid = True

    def validate(self):
        return self.valid


if __name__ == "__main__":
    tester = ISISLoadFilesMER()
    tester.runTest()
