# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import os
import sys
from tempfile import TemporaryDirectory
import unittest
import importlib as imp

from mantid.simpleapi import *
from mantid import api, config

from Direct.ReductionWrapper import *
import MariReduction as mr


class test_helper(ReductionWrapper):
    def __init__(self, web_var=None):
        """sets properties defaults for the instrument with Name"""
        ReductionWrapper.__init__(self, "MAR", web_var)

    def set_custom_output_filename(self):
        """define custom name of output files if standard one is not satisfactory
        In addition to that, example of accessing reduction properties
        Changing them if necessary
        """

        def custom_name(prop_man):
            """sample function which builds filename from
            incident energy and run number and adds some auxiliary information
            to it.
            """

            # Note -- properties have the same names  as the list of advanced and
            # main properties
            ei = PropertyManager.incident_energy.get_current()
            # sample run is more then just list of runs, so we use
            # the formalization below to access its methods
            run_num = prop_man.sample_run
            name = "SOMETHING{0}_{1:<3.2f}meV_rings".format(run_num, ei)
            return name

        # Uncomment this to use custom filename function
        # Note: the properties are stored in prop_man class accessed as
        # below.
        return lambda: custom_name(self.reducer.prop_man)
        # use this method to use standard file name generating function
        # return None

    @iliad
    def reduce(self, input_file=None, output_directory=None):
        self.reducer._clear_old_results()
        if input_file:
            self.reducer.prop_man.sample_run = input_file
        run = self.reducer.prop_man.sample_run

        result = []
        if PropertyManager.incident_energy.multirep_mode():
            en_range = self.reducer.prop_man.incident_energy
            for ind, en in enumerate(en_range):
                ws = CreateSampleWorkspace()
                AddSampleLog(ws, LogName="run_number", LogText=str(run))
                PropertyManager.sample_run.set_action_suffix("#{0}_reduced".format(ind + 1))
                PropertyManager.sample_run.synchronize_ws(ws)
                result.append(ws)
                self.reducer._old_runs_list.append(ws.name())
        else:
            ws = CreateSampleWorkspace()
            AddSampleLog(ws, LogName="run_number", LogText=str(run))
            PropertyManager.sample_run.set_action_suffix("_reduced")
            PropertyManager.sample_run.synchronize_ws(ws)
            result.append(ws)

        if len(result) == 1:
            result = result[0]
        return result


# -----------------------------------------------------------------------------------------------------------------------------------------
# -----------------------------------------------------------------------------------------------------------------------------------------
# -----------------------------------------------------------------------------------------------------------------------------------------
# -----------------------------------------------------------------------------------------------------------------------------------------


class ReductionWrapperTest(unittest.TestCase):
    def __init__(self, methodName):
        return super(ReductionWrapperTest, self).__init__(methodName)

    def setUp(self):
        pass

    def tearDown(self):
        pass

    def test_default_fails(self):
        red = ReductionWrapper("MAR")

        self.assertRaises(NotImplementedError, red.def_main_properties)
        self.assertRaises(NotImplementedError, red.def_advanced_properties)
        self.assertTrue("reduce" in dir(red))

    def test_export_advanced_values(self):
        red = mr.ReduceMARI()

        main_prop = red.def_main_properties()
        adv_prop = red.def_advanced_properties()

        # see what have changed and what have changed as advanced properties.
        all_changed_prop = red.reducer.prop_man.getChangedProperties()

        self.assertEqual(set(list(main_prop.keys()) + list(adv_prop.keys())), all_changed_prop)

        with TemporaryDirectory() as reduce_vars_dir:
            reduce_vars_file = os.path.join(reduce_vars_dir, "reduce_vars.py")
            # save web variables
            red.save_web_variables(reduce_vars_file)
            self.assertTrue(os.path.isfile(reduce_vars_file))

            # restore saved parameters.
            sys.path.insert(0, reduce_vars_dir)

            import reduce_vars as rv

            self.assertDictEqual(rv.standard_vars, main_prop)
            self.assertDictEqual(rv.advanced_vars, adv_prop)
            self.assertTrue(hasattr(rv, "variable_help"))

            imp.reload(mr)

            # tis will run MARI reduction, which probably not work from unit tests
            # will move this to system tests
            # rez = mr.main()

            self.assertTrue(mr.web_var)
            self.assertEqual(mr.web_var.standard_vars, main_prop)
            self.assertEqual(mr.web_var.advanced_vars, adv_prop)

    def test_validate_settings(self):
        dsp = config.getDataSearchDirs()
        # clear all not to find any files
        config.setDataSearchDirs("")

        red = mr.ReduceMARI()
        ok, level, errors = red.validate_settings()

        self.assertFalse(ok)
        self.assertEqual(level, 2)
        self.assertEqual(len(errors), 7)

        # this run should be in data search directory for basic Mantid
        red.reducer.wb_run = 11001
        red.reducer.det_cal_file = "11001"
        red.reducer.monovan_run = None
        red.reducer.hard_mask_file = None
        red.reducer.map_file = None
        red.reducer.save_format = "nxspe"

        path = []
        for item in dsp:
            path.append(item)
        config.setDataSearchDirs(path)

        # hack -- let's pretend we are running from webservices
        # but web var are empty (not to overwrite values above)
        red._run_from_web = True
        red._wvs.standard_vars = {}
        red._wvs.advanced_vars = {}
        ok, level, errors = red.validate_settings()
        if not ok:
            print("Errors found at level", level)
            print(errors)

        self.assertTrue(ok)
        self.assertEqual(level, 0)
        self.assertEqual(len(errors), 0)

        # this is how we set it up from web
        red._wvs.advanced_vars = {"save_format": ""}
        ok, level, errors = red.validate_settings()

        self.assertFalse(ok)
        self.assertEqual(level, 1)
        self.assertEqual(len(errors), 1)

    #
    def test_set_from_constructor(self):
        red = mr.ReduceMARI()

        main_prop = red.def_main_properties()
        adv_prop = red.def_advanced_properties()
        adv_prop["map_file"] = "some_map"
        adv_prop["data_file_ext"] = ".nxs"
        main_prop["sample_run"] = 10000

        class ww(object):
            def __init__(self):
                self.standard_vars = None
                self.advanced_vars = None

        web_var = ww
        web_var.standard_vars = main_prop
        web_var.advanced_vars = adv_prop

        red1 = mr.ReduceMARI(web_var)

        self.assertTrue(red1._run_from_web)
        self.assertEqual(red1.reducer.prop_man.map_file, "some_map.map")
        self.assertEqual(red1.reducer.prop_man.data_file_ext, ".nxs")
        self.assertEqual(red1.reducer.prop_man.sample_run, 10000)

        web_var.advanced_vars = None
        web_var.standard_vars["sample_run"] = 2000

        red2 = mr.ReduceMARI(web_var)
        self.assertTrue(red2._run_from_web)
        self.assertEqual(red2.reducer.prop_man.sample_run, 2000)

    #
    def test_custom_print_name(self):
        th = test_helper()
        th.reducer.prop_man.sample_run = 100
        th.reducer.prop_man.incident_energy = [10.01, 20]

        th.reduce()

        save_file = th.reducer.prop_man.save_file_name
        # such strange name because custom print function above access workspace,
        # generated by reduction
        self.assertEqual(save_file, "SOMETHINGSR_MAR000100#2_reduced_10.01meV_rings")

        PropertyManager.incident_energy.next()
        save_file = th.reducer.prop_man.save_file_name
        # now reduction have not been run, and the name is generated from run number
        self.assertEqual(save_file, "SOMETHINGSR_MAR000100#2_reduced_20.00meV_rings")

    def test_return_run_list(self):
        th = test_helper()

        th.reducer.prop_man.sample_run = 200
        th.run_reduction()
        # standard reduction would save and delete workspace but our simplified one
        # will just keep it
        name = "SR_MAR000200_reduced"
        self.assertTrue(name in mtd)

        th.reducer.prop_man.sample_run = 300
        # new run deletes the old one
        self.assertFalse(name in mtd)

        rez = th.run_reduction()
        self.assertTrue(isinstance(rez, api.Workspace))
        self.assertTrue("rez" in mtd)
        self.assertEqual(rez.name(), "rez")

        th.reducer.prop_man.sample_run = [300, 400]
        th.run_reduction()
        self.assertFalse("SR_MAR000300_reduced" in mtd)
        self.assertTrue("SR_MAR000400_reduced" in mtd)

        th.reducer.prop_man.sample_run = [500, 600]
        self.assertFalse("SR_MAR000400_reduced" in mtd)
        th.run_reduction()
        self.assertFalse("SR_MAR000500_reduced" in mtd)
        self.assertTrue("SR_MAR000600_reduced" in mtd)

        th.reducer.prop_man.sample_run = [300, 400]
        runs = th.run_reduction()
        self.assertTrue("runs#1of2" in mtd)
        self.assertTrue("runs#2of2" in mtd)
        self.assertEqual(runs[0].name(), "runs#1of2")
        self.assertEqual(runs[1].name(), "runs#2of2")

        th.reducer.prop_man.incident_energy = [10, 20]
        th.reducer.prop_man.sample_run = 300
        th.run_reduction()
        self.assertTrue("SR_MAR000300#1_reduced" in mtd)
        self.assertTrue("SR_MAR000300#2_reduced" in mtd)
        th.reducer.prop_man.sample_run = 400
        th.run_reduction()
        self.assertFalse("SR_MAR000300#1_reduced" in mtd)
        self.assertFalse("SR_MAR000300#2_reduced" in mtd)
        self.assertTrue("SR_MAR000400#1_reduced" in mtd)
        self.assertTrue("SR_MAR000400#2_reduced" in mtd)

    def test_check_archive_logs(self):
        th = test_helper()

        propman = th.reducer.prop_man
        # define unique log file to use instead of real log file
        # for testing all branches of log validation routine
        test_dir = config.getString("defaultsave.directory")
        test_log = os.path.normpath(test_dir + "lastrun.txt")
        # clear up rubbish from previous runs if any
        if os.path.isfile(test_log):
            os.remove(test_log)
        propman.archive_upload_log_file = test_log

        # no log file at all
        ok, run_num, info = th._check_progress_log_run_completed(10)
        self.assertTrue(ok)
        self.assertEqual(run_num, 0)
        self.assertEqual(info, "log test disabled as no log file available")

        # Upload log have appeared:
        with open(test_log, "w") as fh:
            fh.write("MAR 1000 0 \n")
        # need to set up the test log value again, as log test had been disabled automatically if no log file was found
        propman.archive_upload_log_file = test_log

        # log file states data available
        ok, run_num, info = th._check_progress_log_run_completed(10)
        self.assertTrue(ok)
        self.assertEqual(run_num, 1000)
        self.assertEqual(len(info), 0)

        # no changes for the second attempt to look at file
        ok, run_num, info = th._check_progress_log_run_completed(1000)
        self.assertTrue(ok)
        self.assertEqual(run_num, 1000)
        self.assertEqual(info, "no new data have been added to archive")

        ok, run_num, info = th._check_progress_log_run_completed(1001)
        self.assertFalse(ok)
        self.assertEqual(run_num, 1000)
        self.assertEqual(info, "no new data have been added to archive")

        with open(test_log, "w") as fh:
            fh.write("MAR 1001 0 \n")
        m_time = os.path.getmtime(test_log)
        # Update modification time manually as some OS and some tests do not update it properly
        m_time = m_time + 1
        os.utime(test_log, (m_time, m_time))
        # next attempt is successfull
        ok, run_num, info = th._check_progress_log_run_completed(1001)
        self.assertEqual(info, "")
        self.assertEqual(run_num, 1001)
        self.assertTrue(ok)

        os.remove(test_log)


if __name__ == "__main__":
    # tester=ReductionWrapperTest('test_check_archive_logs')
    # tester.run()
    unittest.main()
