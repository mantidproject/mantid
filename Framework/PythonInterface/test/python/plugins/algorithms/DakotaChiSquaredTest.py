import unittest,os
import mantid


class DakotaChiSquaredTest(unittest.TestCase):

    def makeFiles(self):
    	mantid.simpleapi.CreateWorkspace(OutputWorkspace='data',DataX='1,2,3,4,5',DataY='1,0,1,4,4',DataE='1,0,1,2,2')
    	mantid.simpleapi.CreateWorkspace(OutputWorkspace='sim',DataX='1,2,3,4,5',DataY='1,1,1,1,1',DataE='0,0,0,0,0')
    	mantid.simpleapi.CreateWorkspace(OutputWorkspace='simwrong',DataX='1,2,3,4',DataY='1,1,1,1',DataE='0,0,0,0')

    	self.datafile=os.path.join(mantid.config.getString('defaultsave.directory'),'DakotaChiSquared_data.nxs')
    	self.simfile=os.path.join(mantid.config.getString('defaultsave.directory'),'DakotaChiSquared_sim.nxs')
    	self.simwrongfile=os.path.join(mantid.config.getString('defaultsave.directory'),'DakotaChiSquared_simwrong.nxs')
    	self.chifile=os.path.join(mantid.config.getString('defaultsave.directory'),'DakotaChiSquared_chi.txt')

    	mantid.simpleapi.SaveNexus('data',self.datafile)
    	mantid.simpleapi.SaveNexus('sim',self.simfile)
    	mantid.simpleapi.SaveNexus('simwrong',self.simwrongfile)

    	mantid.api.AnalysisDataService.remove("data")
    	mantid.api.AnalysisDataService.remove("sim")
    	mantid.api.AnalysisDataService.remove("simwrong")


    def cleanup(self):
    	if os.path.exists(self.datafile):
                	os.remove(self.datafile)
    	if os.path.exists(self.simfile):
                	os.remove(self.simfile)
    	if os.path.exists(self.simwrongfile):
                	os.remove(self.simwrongfile)
    	if os.path.exists(self.chifile):
                	os.remove(self.chifile)

    def test_wrongType(self):
    	self.makeFiles()
    	try:
    		mantid.simpleapi.DakotaChiSquared(self.datafile,'CNCS_7860_event.nxs',self.chifile)
    	except RuntimeError as e:
    		self.assertNotEquals(str(e).find('Wrong workspace type for calculated file'),-1)
    	except:
    		assert False, "Raised the wrong exception type"
    	else:
    		assert False, "Didn't raise any exception"
    	try:
    		mantid.simpleapi.DakotaChiSquared('CNCS_7860_event.nxs',self.simfile,self.chifile)
    	except RuntimeError as e:
    		self.assertNotEquals(str(e).find('Wrong workspace type for data file'),-1)
    	except:
    		assert False, "Raised the wrong exception type"
    	else:
    		assert False, "Didn't raise any exception"
    	self.cleanup()


    def test_wrongSize(self):
    	self.makeFiles()
    	try:
    		mantid.simpleapi.DakotaChiSquared(self.datafile,self.simwrongfile,self.chifile)
    	except RuntimeError as e:
    		self.assertNotEquals(str(e).find('The file sizes are different'),-1)
    	except:
    		assert False, "Raised the wrong exception type"
    	else:
    		assert False, "Didn't raise any exception"
    	self.cleanup()


    def test_value(self):
    	self.makeFiles()
    	try:
    		mantid.simpleapi.DakotaChiSquared(self.datafile,self.simfile,self.chifile)
    		f = open(self.chifile,'r')
    		chistr=f.read()
    		self.assertEquals(chistr,'4.5 obj_fn\n')
    		f.close()
    	except:
    		assert False, "Raised an exception"
    	self.cleanup()

    def test_output(self):
    	self.makeFiles()
    	try:
    		alg=mantid.simpleapi.DakotaChiSquared(self.datafile,self.simfile,self.chifile)
    		self.assertEquals(len(alg),2)
    		self.assertEquals(alg[0],4.5)
    		self.assertEquals(alg[1].getName(),"alg")
    		self.assertEquals(alg[1].blocksize(),5)
    		self.assertEquals(alg[1].getNumberHistograms(),1)
    		self.assertEquals(alg[1].dataY(0)[3],1.5)
    		mantid.api.AnalysisDataService.remove("alg")
    		alg1=mantid.simpleapi.DakotaChiSquared(self.datafile,self.simfile,self.chifile,ResidualsWorkspace="res")
    		self.assertEquals(alg1[0],4.5)
    		self.assertEquals(alg1[1].getName(),"res")
    		mantid.api.AnalysisDataService.remove("res")
    	except:
    		assert False, "Raised an exception"
    	self.cleanup()

if __name__=="__main__":
    unittest.main()
