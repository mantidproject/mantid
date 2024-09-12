# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import datetime
import os
import platform
import shutil
import time
import unittest

from Direct.ISISDirecInelasticConfig import UserProperties, MantidConfigDirectInelastic

from mantid import config


class ISISDirectInelasticConfigTest(unittest.TestCase):
    def __init__(self, methodName):
        # This has been copied from ISIS admin script processing user's office data
        # and all this has the format below
        # if format changes, the class have to change too as the class
        # verifies the data are exactly as defined here
        self.instrument = "MERLIN"
        self.cycle = "CYCLE20151"
        nrbnumber = "1310168"
        self.rbnumber = "RB" + nrbnumber
        self.start_date = "20150503"
        self.userID = "tuf666699"
        return super(ISISDirectInelasticConfigTest, self).__init__(methodName)

    def get_save_dir(self):
        targetDir = config["defaultsave.directory"]
        if targetDir is None or len(targetDir) == 0:
            if platform.system() == "Windows":
                targetDir = str(os.environ["TEMP"])
            else:
                targetDir = "/tmp"
        return targetDir

    def write_test_file(self, filename):
        """Method writes test file with name provided and
        specified contents
        """
        dir_name = os.path.dirname(filename)
        if not os.path.exists(dir_name):
            os.makedirs(dir_name)

        fh = open(filename, "w")
        fh.write("****************************************\n")
        fh.write("first test variable = AAAA # something\n")
        fh.write("second test var  Test_reduction_file # other\n ")
        fh.close()

    def setUp(self):
        # Create user's folder structure in default save directory.
        # the administrative script (not here) builds all this for real in /home
        targetDir = self.get_save_dir()

        self.rbdir = os.path.join(targetDir, self.userID, self.rbnumber)
        self.UserScriptRepoDir = os.path.join(targetDir, "UserScripts")
        self.MapMaskDir = os.path.join(targetDir, "MapMaskDir")
        self.userRootDir = os.path.join(targetDir, self.userID)
        if not os.path.exists(self.rbdir):
            os.makedirs(self.rbdir)
        if not os.path.exists(self.UserScriptRepoDir):
            os.makedirs(self.UserScriptRepoDir)
        if not os.path.exists(self.MapMaskDir):
            os.makedirs(self.MapMaskDir)
        if not os.path.exists(self.userRootDir):
            os.makedirs(self.userRootDir)

    def makeFakeSourceReductionFile(self, mcf, contents=None):
        all_instr_names = mcf._user.get_all_instruments()
        all_files = []
        for instr in all_instr_names:
            instr_name = instr

            file_path = os.path.join(self.UserScriptRepoDir, "direct_inelastic", instr_name.upper())
            if not os.path.exists(file_path):
                os.makedirs(file_path)

            file_name = mcf._sample_reduction_file(instr_name)
            all_files.append(file_name)
            full_file = os.path.join(file_path, file_name)
            if os.path.isfile(full_file):
                os.remove(full_file)
            fh = open(full_file, "w")
            fh.write("#Test reduction file\n")
            if contents is None:
                fh.write("Contents=Fake_reduction_file_for_{0}".format(instr_name))
            else:
                fh.write("Contents={0}".format(contents))
            fh.close()
        if len(all_files) > 1:
            return all_files
        else:
            return full_file

    def tearDown(self):
        # Clean-up user's folder structure
        if os.path.exists(self.rbdir):
            shutil.rmtree(self.rbdir, ignore_errors=True)
        if os.path.exists(self.UserScriptRepoDir):
            shutil.rmtree(self.UserScriptRepoDir, ignore_errors=True)
        if os.path.exists(self.MapMaskDir):
            shutil.rmtree(self.MapMaskDir, ignore_errors=True)
        if os.path.exists(self.userRootDir):
            shutil.rmtree(self.userRootDir, ignore_errors=True)

    def test_UserProperties(self):
        user = UserProperties(self.userID)

        user.set_user_properties(self.instrument, self.rbdir, "CYCLE20144B", self.start_date)
        self.assertEqual(user.cycleID, "2014_4B")

        # set_user_properties(self,instrument,rb_folder_or_id,cycle,start_date):
        user.set_user_properties(self.instrument, self.rbdir, self.cycle, self.start_date)

        id = user._recent_dateID
        self.assertEqual(user._instrument[id], "MERLIN")
        self.assertEqual(user._cycle_IDs[id], ("2015", "1"))
        self.assertEqual(user._start_dates[id], datetime.date(2015, 5, 3))
        self.assertEqual(user._rb_dirs[id], self.rbdir)
        self.assertEqual(user.userID, self.userID)

        self.assertEqual(user.instrument, "MERLIN")
        self.assertEqual(user.cycleID, "2015_1")
        self.assertEqual(user.start_date, datetime.date(2015, 5, 3))
        self.assertEqual(user.rb_dir, self.rbdir)

        self.assertRaises(RuntimeError, user.set_user_properties, "SANS2D", self.rbdir, self.cycle, self.start_date)
        self.assertRaises(RuntimeError, user.set_user_properties, "HET", self.rbdir, self.cycle, "201400000")
        # self.assertRaises(RuntimeError,user.set_user_properties,'HET',self.rbdir,'20144',self.start_date)
        self.assertRaises(RuntimeError, user.set_user_properties, "HET", self.rbdir, "CYC20144", self.start_date)
        self.assertRaises(RuntimeError, user.set_user_properties, "HET", self.rbdir, "CYCLE201444", self.start_date)

        self.assertRaises(RuntimeError, user.set_user_properties, "HET", "missing/folder", self.cycle, "19990101")
        # user.set_user_properties('HET','missing/folder',self.cycle,"19990101")
        # this refers to recent experiment, not the old one
        self.assertTrue(user.rb_dir_exist)

        rbdir = os.path.join(self.userRootDir, "RB1000666")
        if not os.path.exists(rbdir):
            os.makedirs(rbdir)
        user.set_user_properties("MARI", rbdir, "CYCLE20001", "20000112")
        if os.path.exists(rbdir):
            shutil.rmtree(rbdir)

        self.assertEqual(len(user.instrument), 6)
        self.assertEqual(len(user._instrument), 2)

        self.assertEqual(user._recent_dateID, id)
        self.assertEqual(user._start_dates["2000-01-12"], datetime.date(2000, 1, 12))

        rbdir = os.path.join(self.userRootDir, "RB1999666")
        if not os.path.exists(rbdir):
            os.makedirs(rbdir)
        user.set_user_properties("MERLIN", rbdir, "CYCLE20163", "20161201")
        if os.path.exists(rbdir):
            shutil.rmtree(rbdir)

        self.assertEqual(len(user._instrument), 3)
        id = user._recent_dateID
        self.assertEqual(id, "2016-12-01")
        self.assertEqual(user._instrument[id], "MERLIN")
        self.assertEqual(user.instrument, "MERLIN")

    def test_build_config(self):
        # script verifies the presence of a folder, not its contents.
        # for the script to work, let's run it on default save directory
        MantidDir = os.path.split(os.path.realpath(__file__))[0]
        HomeRootDir = self.get_save_dir()

        mcf = MantidConfigDirectInelastic(MantidDir, HomeRootDir, self.UserScriptRepoDir, self.MapMaskDir)

        self.assertRaises(
            RuntimeError, MantidConfigDirectInelastic, "MissingMantidFolder", HomeRootDir, self.UserScriptRepoDir, self.MapMaskDir
        )
        self.assertRaises(
            RuntimeError, MantidConfigDirectInelastic, MantidDir, "MissingUserRootFolder", self.UserScriptRepoDir, self.MapMaskDir
        )
        self.assertRaises(RuntimeError, MantidConfigDirectInelastic, MantidDir, HomeRootDir, "MissingUserScriptRepoDir", self.MapMaskDir)
        self.assertRaises(RuntimeError, MantidConfigDirectInelastic, MantidDir, HomeRootDir, self.UserScriptRepoDir, "MissingMapMaskDir")

        user = UserProperties(self.userID)
        user.set_user_properties(self.instrument, self.rbdir, self.cycle, self.start_date)

        # clear up the previous
        if os.path.exists(os.path.join(self.userRootDir, ".mantid")):
            shutil.rmtree(os.path.join(self.userRootDir, ".mantid"))

        mcf.init_user(user)
        self.makeFakeSourceReductionFile(mcf)

        self.assertEqual(len(mcf._dynamic_configuration), 6)

        mcf.generate_config()

        config_file = os.path.join(self.userRootDir, ".mantid", "Mantid.user.properties")
        self.assertTrue(os.path.exists(os.path.join(self.userRootDir, ".mantid")))
        self.assertTrue(os.path.exists(config_file))

        cur_cycleID = mcf._user.cycleID
        instr = mcf._user.instrument
        targ_file = mcf._target_reduction_file(instr, cur_cycleID)

        reduction_file = os.path.join(mcf._user.rb_dir, targ_file)
        self.assertTrue(os.path.isfile(reduction_file))

        self.assertTrue(mcf.config_need_replacing(config_file))
        start_date = user.start_date
        date_in_apast = datetime.date(start_date.year, start_date.month, start_date.day - 1)
        time_user_touched = time.mktime(date_in_apast.timetuple())
        os.utime(config_file, (time_user_touched, time_user_touched))
        self.assertFalse(mcf.config_need_replacing(config_file))
        # clear up
        if os.path.exists(os.path.join(self.userRootDir, ".mantid")):
            shutil.rmtree(os.path.join(self.userRootDir, ".mantid"))

    def test_build_3Experiments_config(self):
        # script verifies the presence of a folder, not its contents.
        # for the script to work, let's run it on default save directory
        MantidDir = os.path.split(os.path.realpath(__file__))[0]
        HomeRootDir = self.get_save_dir()
        mcf = MantidConfigDirectInelastic(MantidDir, HomeRootDir, self.UserScriptRepoDir, self.MapMaskDir)

        user = UserProperties(self.userID)
        user.set_user_properties(self.instrument, self.rbdir, self.cycle, self.start_date)

        rbnum2 = "RB1999000"

        targetDir = self.get_save_dir()
        rbdir2 = os.path.join(targetDir, self.userID, rbnum2)
        if not os.path.exists(rbdir2):
            os.makedirs(rbdir2)
        user.set_user_properties("MARI", rbdir2, "CYCLE20001", "20000124")

        rbnum3 = "RB1204000"
        rbdir3 = os.path.join(targetDir, self.userID, rbnum3)
        if not os.path.exists(rbdir3):
            os.makedirs(rbdir3)
        user.set_user_properties("MAPS", rbdir3, "CYCLE20044", "20041207")

        # clear up the previous
        if os.path.exists(os.path.join(self.userRootDir, ".mantid")):
            shutil.rmtree(os.path.join(self.userRootDir, ".mantid"))

        mcf.init_user(user)

        self.makeFakeSourceReductionFile(mcf)
        self.assertEqual(len(mcf._dynamic_configuration), 6)
        self.assertEqual(mcf._dynamic_configuration[1], "default.instrument=MERLIN")

        mcf.generate_config()

        config_file = os.path.join(self.userRootDir, ".mantid", "Mantid.user.properties")
        self.assertTrue(os.path.exists(os.path.join(self.userRootDir, ".mantid")))
        self.assertTrue(os.path.exists(config_file))

        self.assertTrue(mcf.config_need_replacing(config_file))
        start_date = user.start_date
        date_in_apast = datetime.date(start_date.year, start_date.month, start_date.day - 1)
        time_in_a_past = time.mktime(date_in_apast.timetuple())
        os.utime(config_file, (time_in_a_past, time_in_a_past))
        self.assertFalse(mcf.config_need_replacing(config_file))
        # --------------------------------------------------------------------
        user1ID = "tuf299966"
        user1RootDir = os.path.join(self.get_save_dir(), user1ID)
        if not os.path.exists(user1RootDir):
            os.makedirs(user1RootDir)
        #
        user1 = UserProperties(user1ID)
        user1.set_user_properties("MARI", rbdir2, "CYCLE20991", "20990124")

        mcf.init_user(user1)
        self.makeFakeSourceReductionFile(mcf)

        mcf.generate_config()
        self.assertEqual(len(mcf._dynamic_configuration), 6)
        self.assertEqual(mcf._dynamic_configuration[1], "default.instrument=MARI")
        config_file = os.path.join(self.userRootDir, ".mantid", "Mantid.user.properties")
        self.assertTrue(os.path.exists(os.path.join(user1RootDir, ".mantid")))
        self.assertTrue(os.path.exists(config_file))
        #
        # Check sample reduction file
        #
        full_rb_path = rbdir2
        cycle_id = user1.cycleID
        instr = user1.instrument
        target_file = mcf._target_reduction_file(instr, cycle_id)
        full_target_file = os.path.join(full_rb_path, target_file)
        self.assertTrue(os.path.exists(full_target_file))
        # Fresh target file should always be replaced
        self.assertTrue(mcf.script_need_replacing(full_target_file))
        # modify target file access time:
        access_time = os.path.getmtime(full_target_file)
        now = time.time()
        os.utime(full_target_file, (access_time, now))
        # should not replace modified target file
        self.assertFalse(mcf.script_need_replacing(full_target_file))

        # --------------------------------------------------------------------
        # clean up
        if os.path.exists(os.path.join(self.userRootDir, ".mantid")):
            shutil.rmtree(os.path.join(self.userRootDir, ".mantid"), ignore_errors=True)
        if os.path.exists(rbdir2):
            shutil.rmtree(rbdir2, ignore_errors=True)
        if os.path.exists(rbdir3):
            shutil.rmtree(rbdir3, ignore_errors=True)
        if os.path.exists(user1RootDir):
            shutil.rmtree(user1RootDir, ignore_errors=True)
        #

    def test_replace_user_variables(self):
        user = UserProperties("wkc26243")
        user.set_user_properties(self.instrument, self.rbdir, self.cycle, self.start_date)

        targ_string = user.replace_variables("$instrument$ReductionScript$cycleID$.py")
        self.assertEqual(self.instrument + "ReductionScript2015_1.py", targ_string)
        self.assertEqual(user.GID, "1310168")

    def test_parse_file_description(self):
        this_file = os.path.realpath(__file__)
        file_dir = os.path.dirname(this_file)
        test_xml = os.path.join(file_dir, "User_files_description_test.xml")
        self.assertTrue(os.path.exists(test_xml))

        MantidDir = os.path.split(os.path.realpath(__file__))[0]
        HomeRootDir = self.get_save_dir()
        mcf = MantidConfigDirectInelastic(MantidDir, HomeRootDir, self.UserScriptRepoDir, self.MapMaskDir)

        user = UserProperties(self.userID)
        user.set_user_properties(self.instrument, self.rbdir, self.cycle, self.start_date)
        mcf.init_user(user)

        # test old defaults, deployed if no User_files_description are defined
        files_to_copy = mcf._parse_user_files_description(None)
        self.assertEqual(len(files_to_copy), 1)
        source = files_to_copy[0][0]
        dest = files_to_copy[0][1]
        repl = files_to_copy[0][2]
        self.assertEqual(os.path.basename(source), "MERLINReduction_Sample.py")
        self.assertEqual(os.path.basename(dest), "MERLINReduction_2015_1.py")
        self.assertEqual(repl, None)

        # test files defined by test xml file
        files_to_copy = mcf._parse_user_files_description(test_xml)
        self.assertEqual(len(files_to_copy), 2)

        # define and generate test files to copy.
        # We know what files are and what their contents is from User_files_description_test.xml
        for file_descr in files_to_copy:
            source = file_descr[0]
            dest = file_descr[1]
            if os.path.exists(dest):
                os.remove(dest)
            self.write_test_file(source)

        # Check copy_reduction_sample
        mcf.copy_reduction_sample(test_xml)
        for file_pair in files_to_copy:
            dest = file_pair[1]
            self.assertTrue(os.path.exists(dest))

        # Clean up
        for file_pair in files_to_copy:
            source = file_pair[0]
            dest = file_pair[1]
            if os.path.exists(dest):
                os.remove(dest)
            if os.path.exists(source):
                os.remove(source)

    def test_init_user(self):
        MantidDir = os.path.split(os.path.realpath(__file__))[0]
        HomeRootDir = self.get_save_dir()
        mcf = MantidConfigDirectInelastic(MantidDir, HomeRootDir, self.UserScriptRepoDir, self.MapMaskDir)

        user = UserProperties(self.userID)
        user.set_user_properties(self.instrument, self.rbdir, self.cycle, self.start_date)

        mcf.init_user(self.userID, user)
        user1 = mcf._user
        self.assertEqual(user, user1)

        mcf.init_user(user)
        user2 = mcf._user
        self.assertEqual(user, user2)

        self.assertRaises(RuntimeError, mcf.init_user, "bla_bla_bla")

    def test_various_user_constructors(self):
        user = UserProperties("tusxxx MARI RB1040506 153 150821")
        sus = str(user)
        self.assertEqual(sus, "tusxxx MARI RB1040506 2015_3 2015-08-21")

        user1 = UserProperties(sus)
        sus1 = str(user1)
        self.assertEqual(sus, sus1)

        user2 = UserProperties("tusxxx", "MARI", "RB1040506", "2015_3", "20150821")
        sus2 = str(user2)
        self.assertEqual(sus, sus2)

        user2.rb_dir = "bla_bla_bla"
        self.assertFalse(user2.rb_dir_exist)

        user3 = UserProperties(self.userID, self.instrument, self.rbdir, self.cycle, self.start_date)
        self.assertTrue(user3.rb_dir_exist)

        rb_folder = user3.rb_dir
        base, rb = os.path.split(rb_folder)
        user3.rb_dir = base
        self.assertTrue(user3.rb_dir_exist)
        rb_folder1 = user3.rb_dir
        self.assertEqual(rb_folder, rb_folder1)

    def test_copy_multiplpe_ucf(self):
        # script verifies the presence of a folder, not its contents.
        # for the script to work, let's run it on default save directory
        MantidDir = os.path.split(os.path.realpath(__file__))[0]
        HomeRootDir = self.get_save_dir()
        mcf = MantidConfigDirectInelastic(MantidDir, HomeRootDir, self.UserScriptRepoDir, self.MapMaskDir)

        user = UserProperties(self.userID)
        user.set_user_properties(self.instrument, self.rbdir, self.cycle, self.start_date)

        rbnum2 = "RB1999000"

        targetDir = self.get_save_dir()
        rbdir2 = os.path.join(targetDir, self.userID, rbnum2)
        if not os.path.exists(rbdir2):
            os.makedirs(rbdir2)
        user.set_user_properties("MARI", rbdir2, "CYCLE20001", "20000124")

        rbnum3 = "RB1204000"
        rbdir3 = os.path.join(targetDir, self.userID, rbnum3)
        if not os.path.exists(rbdir3):
            os.makedirs(rbdir3)
        user.set_user_properties("MAPS", rbdir3, "CYCLE20044", "20041207")

        # clear up the previous
        if os.path.exists(os.path.join(self.userRootDir, ".mantid")):
            shutil.rmtree(os.path.join(self.userRootDir, ".mantid"))

        mcf.init_user(user)
        # Generate fake test files to copy to test users
        self.makeFakeSourceReductionFile(mcf)

        mcf.generate_config()

        #
        # Check sample reduction files
        #
        # Sample file for MERLIN:
        rbdir1 = self.rbdir
        mer_file = os.path.join(rbdir1, "MERLINReduction_2015_1.py")
        self.assertTrue(os.path.isfile(mer_file))
        mar_file = os.path.join(rbdir2, "MARIReduction_2015_1.py")
        self.assertTrue(os.path.isfile(mar_file))
        maps_file = os.path.join(rbdir3, "MAPSReduction_2015_1.py")
        self.assertTrue(os.path.isfile(maps_file))

        # --------------------------------------------------------------------
        # clean up
        if os.path.exists(os.path.join(self.userRootDir, ".mantid")):
            shutil.rmtree(os.path.join(self.userRootDir, ".mantid"), ignore_errors=True)
        if os.path.exists(rbdir2):
            shutil.rmtree(rbdir2, ignore_errors=True)
        if os.path.exists(rbdir3):
            shutil.rmtree(rbdir3, ignore_errors=True)
        #

    def test_copy_multiplpe_instr(self):
        # script verifies the presence of a folder, not its contents.
        # for the script to work, let's run it on default save directory
        MantidDir = os.path.split(os.path.realpath(__file__))[0]
        HomeRootDir = self.get_save_dir()
        mcf = MantidConfigDirectInelastic(MantidDir, HomeRootDir, self.UserScriptRepoDir, self.MapMaskDir)

        user = UserProperties(self.userID)
        user.set_user_properties(self.instrument, self.rbdir, self.cycle, self.start_date)

        rbnum2 = "RB1999000"

        targetDir = self.get_save_dir()
        rbdir2 = os.path.join(targetDir, self.userID, rbnum2)
        if not os.path.exists(rbdir2):
            os.makedirs(rbdir2)
        user.set_user_properties("MERLIN", rbdir2, "CYCLE20151", "20150508")

        # clear up the previous
        if os.path.exists(os.path.join(self.userRootDir, ".mantid")):
            shutil.rmtree(os.path.join(self.userRootDir, ".mantid"))

        mcf.init_user(user)
        # Generate fake test files to copy to test users
        self.makeFakeSourceReductionFile(mcf)

        mcf.generate_config()
        #
        # Check sample reduction files
        #
        # Sample file for MERLIN:
        rbdir1 = self.rbdir
        mer_file = os.path.join(rbdir1, "MERLINReduction_2015_1.py")
        self.assertTrue(os.path.isfile(mer_file))
        mar_file = os.path.join(rbdir2, "MERLINReduction_2015_1.py")
        self.assertTrue(os.path.isfile(mar_file))

        # --------------------------------------------------------------------
        # clean up
        if os.path.exists(os.path.join(self.userRootDir, ".mantid")):
            shutil.rmtree(os.path.join(self.userRootDir, ".mantid"), ignore_errors=True)
        if os.path.exists(rbdir2):
            shutil.rmtree(rbdir2, ignore_errors=True)
        #


if __name__ == "__main__":
    # test = ISISDirectInelasticConfigTest('test_copy_multiplpe_ucf')
    # test._set_up()
    # test.run()
    # test._tear_down()

    unittest.main()
