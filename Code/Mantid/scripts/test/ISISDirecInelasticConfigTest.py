import os
os.environ["PATH"] = r"c:/Mantid/Code/builds/br_master/bin/Release;"+os.environ["PATH"]
import unittest
import shutil
import datetime
from mantid import config
from Direct.ISISDirecInelasticConfig import UserProperties,MantidConfigDirectInelastic



class ISISDirectInelasticConfigTest(unittest.TestCase):
    def __init__(self, methodName):
        # This has been copied from ISIS admin script and has the format below
        # if format there changes, the class have to change too as the class
        # verifies the data are exactly as defined here
        self.instrument= "MERLIN"
        self.cycle     = "CYCLE20151"
        nrbnumber = "1310168"
        self.rbnumber  = "RB" + nrbnumber
        self.start_date= '20150503'
        return super(ISISDirectInelasticConfigTest, self).__init__(methodName)

    def setUp(self):
        targetDir = config['defaultsave.directory']
        self.rbdir = os.path.join(targetDir,self.instrument,self.cycle,self.rbnumber)
        if not os.path.exists(self.rbdir):
            os.makedirs(self.rbdir)
    def teadDown(self):
        if os.path.exists(self.rbdir):
            shutil.rmtree(self.rbdir)

    def test_UserProperties(self):
        user = UserProperties()

        user.set_user_properties(self.instrument,self.start_date,self.cycle,self.rbdir)

        id = user._recent_dateID
        self.assertEqual(user.instrument[id],'MERLIN')
        self.assertEqual(user.cycle_IDlist[id],('2015','1'))
        self.assertEqual(user.start_dates[id],datetime.date(2015,05,03))
        self.assertEqual(user.rb_dir[id],self.rbdir)


if __name__=="__main__":
    unittest.main()
