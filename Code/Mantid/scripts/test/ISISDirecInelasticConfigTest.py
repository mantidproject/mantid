import os
os.environ["PATH"] = r"c:/Mantid/Code/builds/br_master/bin/Release;"+os.environ["PATH"]
import unittest
import shutil
import datetime
import time
import platform
from mantid import config
from Direct.ISISDirecInelasticConfig import UserProperties,MantidConfigDirectInelastic



class ISISDirectInelasticConfigTest(unittest.TestCase):
    def __init__(self, methodName):
        # This has been copied from ISIS admin script processing user's office data
        # and all this has the format below
        # if format changes, the class have to change too as the class
        # verifies the data are exactly as defined here
        self.instrument= "MERLIN"
        self.cycle     = "CYCLE20151"
        nrbnumber = "1310168"
        self.rbnumber  = "RB" + nrbnumber
        self.start_date= '20150503'
        self.userID = 'tuf666699'
        return super(ISISDirectInelasticConfigTest, self).__init__(methodName)

    def get_save_dir(self):
        targetDir = config['defaultsave.directory']
        if targetDir is None or len(targetDir) == 0:
            if platform.system() == 'Windows':
                targetDir = str(os.environ['TEMP'])
            else:
                targetDir = '/tmp'
        return targetDir 

    def setUp(self):
        # Create user's folder structure in default save directory.
        # the administrative script (not here) builds all this for real in /home
        targetDir = self.get_save_dir()

        self.rbdir = os.path.join(targetDir,self.userID,self.rbnumber)
        self.UserScriptRepoDir = os.path.join(targetDir,'UserScripts')
        self.MapMaskDir        = os.path.join(targetDir,'MapMaskDir')
        self.userRootDir = os.path.join(targetDir,self.userID)
        if not os.path.exists(self.rbdir):
            os.makedirs(self.rbdir)
        if not os.path.exists(self.UserScriptRepoDir):
            os.makedirs(self.UserScriptRepoDir)
        if not os.path.exists(self.MapMaskDir):
            os.makedirs(self.MapMaskDir)
        if not os.path.exists(self.userRootDir):
            os.makedirs(self.userRootDir)

    def makeFakeSourceReductionFile(self,mcf,contents=None):

        instr_name = mcf._user.get_last_instrument()

        file_path = os.path.join(self.UserScriptRepoDir,'direct_inelastic',instr_name.upper())
        if not os.path.exists(file_path):
            os.makedirs(file_path)
        
        file_name = mcf._sample_reduction_file(instr_name)
        full_file = os.path.join(file_path,file_name)
        if os.path.isfile(full_file):
            os.remove(full_file)
        fh=open(full_file,'w')
        fh.write('#Test reduction file\n')
        fh.write('Contents={0}'.format(contents))
        fh.close()
        return full_file


    def tearDown(self):
        # Clean-up user's folder structure
        if os.path.exists(self.rbdir):
            shutil.rmtree(self.rbdir)
        if os.path.exists(self.UserScriptRepoDir):
            shutil.rmtree(self.UserScriptRepoDir)
        if os.path.exists(self.MapMaskDir):
            shutil.rmtree(self.MapMaskDir)
        if os.path.exists(self.userRootDir):
            shutil.rmtree(self.userRootDir)

    def test_UserProperties(self):
        user = UserProperties()

        user.set_user_properties(self.instrument,self.start_date,self.cycle,self.rbdir)

        id = user._recent_dateID
        self.assertEqual(user.instrument[id],'MERLIN')
        self.assertEqual(user.cycle_IDlist[id],('2015','1'))
        self.assertEqual(user.start_dates[id],datetime.date(2015,05,03))
        self.assertEqual(user.rb_dir[id],self.rbdir)

        self.assertRaises(RuntimeError,user.set_user_properties,'SANS2D',self.start_date,self.cycle,self.rbdir)
        self.assertRaises(RuntimeError,user.set_user_properties,'HET','201400000',self.cycle,self.rbdir)
        self.assertRaises(RuntimeError,user.set_user_properties,'HET',self.start_date,'20144',self.rbdir)
        self.assertRaises(RuntimeError,user.set_user_properties,'HET',self.start_date,'CYC20144',self.rbdir)
        self.assertRaises(RuntimeError,user.set_user_properties,'HET',self.start_date,'CYCLE201444',self.rbdir)
        self.assertRaises(RuntimeError,user.set_user_properties,'HET',self.start_date,self.cycle,'missing/folder')

        rbdir = os.path.join(self.userRootDir,'RB1000666')
        if not os.path.exists(rbdir):
            os.makedirs(rbdir)
        user.set_user_properties('MARI','20000112','CYCLE20001',rbdir)
        if os.path.exists(rbdir):
            shutil.rmtree(rbdir)

        self.assertEqual(len(user.instrument),2)
        self.assertEqual(user._recent_dateID,id)
        self.assertEqual(user.start_dates['2000-01-12'],datetime.date(2000,01,12))

        targetDir = self.get_save_dir()
        rbdir = os.path.join(self.userRootDir,'RB1999666')
        if not os.path.exists(rbdir):
            os.makedirs(rbdir)
        user.set_user_properties('MERLIN','20161201','CYCLE20163',rbdir)
        if os.path.exists(rbdir):
            shutil.rmtree(rbdir)

        self.assertEqual(len(user.instrument),3)
        id = user._recent_dateID
        self.assertEqual(id,'2016-12-01')
        self.assertEqual(user.instrument[id],'MERLIN')

    def test_build_config(self):
        # script verifies the presence of a folder, not its contents.
        # for the script to work, let's run it on default save directory
        MantidDir = os.path.split(os.path.realpath(__file__))[0]
        HomeRootDir = self.get_save_dir()

        mcf = MantidConfigDirectInelastic(MantidDir,HomeRootDir,self.UserScriptRepoDir,self.MapMaskDir)

        self.assertRaises(RuntimeError,MantidConfigDirectInelastic,'MissingMantidFolder',HomeRootDir,self.UserScriptRepoDir,self.MapMaskDir)
        self.assertRaises(RuntimeError,MantidConfigDirectInelastic,MantidDir,'MissingUserRootFolder',self.UserScriptRepoDir,self.MapMaskDir)
        self.assertRaises(RuntimeError,MantidConfigDirectInelastic,MantidDir,HomeRootDir,'MissingUserScriptRepoDir',self.MapMaskDir)
        self.assertRaises(RuntimeError,MantidConfigDirectInelastic,MantidDir,HomeRootDir,self.UserScriptRepoDir,'MissingMapMaskDir')

        user = UserProperties()
        user.set_user_properties(self.instrument,self.start_date,self.cycle,self.rbdir)

        # clear up the previous
        if os.path.exists(os.path.join(self.userRootDir,'.mantid')):
            shutil.rmtree(os.path.join(self.userRootDir,'.mantid'))


        mcf.init_user(self.userID,user)
        self.makeFakeSourceReductionFile(mcf)

        self.assertEqual(len(mcf._dynamic_configuration),6)

        mcf.generate_config()

        config_file = os.path.join(self.userRootDir,'.mantid','Mantid.user.properties')
        self.assertTrue(os.path.exists(os.path.join(self.userRootDir,'.mantid')))
        self.assertTrue(os.path.exists(config_file))

        cur_cycleID = mcf._user.get_last_cycleID()
        instr = mcf._user.get_last_instrument()
        targ_file = mcf._target_reduction_file(instr,cur_cycleID)

        reduction_file = os.path.join(mcf._user.get_last_rbdir(),targ_file)
        self.assertTrue(os.path.isfile(reduction_file))

        self.assertFalse(mcf.config_need_replacing(config_file))
        start_date = user.get_start_date()
        date_in_apast=datetime.date(start_date.year,start_date.month,start_date.day-1)
        time_in_a_past = time.mktime(date_in_apast.timetuple())
        os.utime(config_file,(time_in_a_past,time_in_a_past))
        self.assertTrue(mcf.config_need_replacing(config_file))
        # clear up
        if os.path.exists(os.path.join(self.userRootDir,'.mantid')):
            shutil.rmtree(os.path.join(self.userRootDir,'.mantid'))

    def test_build_3Experiments_config(self):
        # script verifies the presence of a folder, not its contents.
        # for the script to work, let's run it on default save directory
        MantidDir = os.path.split(os.path.realpath(__file__))[0]
        HomeRootDir = self.get_save_dir()
        mcf = MantidConfigDirectInelastic(MantidDir,HomeRootDir,self.UserScriptRepoDir,self.MapMaskDir)

        user = UserProperties()
        user.set_user_properties(self.instrument,self.start_date,self.cycle,self.rbdir)


        rbnum2='RB1999000'

        targetDir = self.get_save_dir()
        rbdir2 = os.path.join(targetDir,self.userID,rbnum2)
        if not os.path.exists(rbdir2):
            os.makedirs(rbdir2)
        user.set_user_properties('MARI','20000124','CYCLE20001',rbdir2)

        rbnum3='RB1204000'
        rbdir3 = os.path.join(targetDir,self.userID,rbnum3)
        if not os.path.exists(rbdir3):
            os.makedirs(rbdir3)
        user.set_user_properties('MAPS','20041207','CYCLE20044',rbdir3)

        # clear up the previous
        if os.path.exists(os.path.join(self.userRootDir,'.mantid')):
            shutil.rmtree(os.path.join(self.userRootDir,'.mantid'))


        mcf.init_user(self.userID,user)

        fake_source=self.makeFakeSourceReductionFile(mcf)
        self.assertEqual(len(mcf._dynamic_configuration),6)
        self.assertEqual(mcf._dynamic_configuration[1],'default.instrument=MERLIN')

        mcf.generate_config()

        config_file = os.path.join(self.userRootDir,'.mantid','Mantid.user.properties')
        self.assertTrue(os.path.exists(os.path.join(self.userRootDir,'.mantid')))
        self.assertTrue(os.path.exists(config_file))

        self.assertFalse(mcf.config_need_replacing(config_file))
        start_date = user.get_start_date()
        date_in_apast=datetime.date(start_date.year,start_date.month,start_date.day-1)
        time_in_a_past = time.mktime(date_in_apast.timetuple())
        os.utime(config_file,(time_in_a_past,time_in_a_past))
        self.assertTrue(mcf.config_need_replacing(config_file))
        #--------------------------------------------------------------------
        user1ID = 'tuf299966'
        user1RootDir = os.path.join(self.get_save_dir(),user1ID)
        if not os.path.exists(user1RootDir):
            os.makedirs(user1RootDir)
        #
        user1 = UserProperties()
        user1.set_user_properties('MARI','20990124','CYCLE20991',rbdir2)

        mcf.init_user(user1ID,user1)
        source_file = self.makeFakeSourceReductionFile(mcf)

        mcf.generate_config()
        self.assertEqual(len(mcf._dynamic_configuration),6)
        self.assertEqual(mcf._dynamic_configuration[1],'default.instrument=MARI')
        config_file = os.path.join(self.userRootDir,'.mantid','Mantid.user.properties')
        self.assertTrue(os.path.exists(os.path.join(user1RootDir,'.mantid')))
        self.assertTrue(os.path.exists(config_file))
        #
        # Check sample reduction file
        #
        full_rb_path = rbdir2
        cycle_id = user1.get_last_cycleID()
        instr = user1.get_last_instrument()
        target_file = mcf._target_reduction_file(instr,cycle_id)
        full_target_file = os.path.join(full_rb_path,target_file)
        self.assertTrue(os.path.exists(full_target_file))
        # Fresh target file should always be replaced
        self.assertTrue(mcf.script_need_replacing(source_file,full_target_file))
        # modify target file access time:
        access_time = os.path.getmtime(full_target_file)
        now = time.time()
        os.utime(full_target_file,(access_time ,now))
        # should not replace modified target file
        self.assertFalse(mcf.script_need_replacing(source_file,full_target_file))

        #--------------------------------------------------------------------
        # clean up
        if os.path.exists(os.path.join(self.userRootDir,'.mantid')):
            shutil.rmtree(os.path.join(self.userRootDir,'.mantid'))
        if os.path.exists(rbdir2):
            shutil.rmtree(rbdir2)
        if os.path.exists(rbdir3):
            shutil.rmtree(rbdir3)
        if os.path.exists(user1RootDir):
            shutil.rmtree(user1RootDir)





if __name__=="__main__":
    unittest.main()
