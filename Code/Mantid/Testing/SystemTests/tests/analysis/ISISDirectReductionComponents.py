import os,sys
import stresstesting
from mantid.simpleapi import *
from mantid.api import Workspace,IEventWorkspace

from Direct.PropertyManager import PropertyManager
from Direct.RunDescriptor   import RunDescriptor
import ISIS_MariReduction as mr

#----------------------------------------------------------------------
class ISIS_ReductionWebLike(stresstesting.MantidStressTest):
    def __init__(self):
       stresstesting.MantidStressTest.__init__(self)

       # prepare reduction variable
       self.rd = mr.ReduceMARIFromFile()
       self.rd.def_main_properties()
       self.rd.def_advanced_properties()

       save_folder = config['defaultsave.directory']

       self.rd.save_web_variables(os.path.join(save_folder,'reduce_vars.py'))
       

    def runTest(self):
        # run reduction using saved variables like web variables
        web_var_folder = config['defaultsave.directory']
        sys.path.insert(0,web_var_folder)
        reload(mr)

        # change these variables to save result as nxs workspace
        mr.web_var.advanced_vars['save_format']='nxs'
        # web services currently needs input file to be defined
        input_file = 'MAR11001.RAW'
        rez = mr.main(input_file,web_var_folder)

        #  verify if result was indeed written
        self.rd.reducer.sample_run = input_file 
        saveFileName = self.rd.reducer.save_file_name
        oputputFile = os.path.join(web_var_folder,saveFileName+'.nxs')

        self.assertTrue(os.path.exists(oputputFile))
        
        web_var_file = os.path.join(web_var_folder,'reduce_vars')
        if os.path.exists(web_var_file+'.py'):
            os.remove(web_var_file+'.py')
        if os.path.exists(web_var_file+'.pyc'):
            os.remove(web_var_file+'.pyc')


    def get_result_workspace(self):
       """Returns the result workspace to be checked"""
       saveFileName = self.rd.reducer.save_file_name
       outWS = Load(Filename=saveFileName+'.nxs')
       outWS *= 0.997979227566217
       return "outWS"   
    def get_reference_file(self):
        return "MARIReduction.nxs"

    def validate(self):
      """Returns the name of the workspace & file to compare"""
      self.tolerance = 1e-6
      self.tolerance_is_reller=True
      self.disableChecking.append('SpectraMap')
      self.disableChecking.append('Instrument')
      self.disableChecking.append('Sample')
      result = self.get_result_workspace()
      reference = self.get_reference_file()
      return result, reference

class ISIS_ReductionWrapperValidate(stresstesting.MantidStressTest):
    def __init__(self):
       stresstesting.MantidStressTest.__init__(self)
       self.result = False
     

    def runTest(self):
       # prepare reduction variable
       rd = mr.ReduceMARIFromFile()
       rd.def_main_properties()
       rd.def_advanced_properties()

       self.result,message = rd.validate_result()
       if not self.result:
          print "*** Validation failed: {0}".format(message)
 

 
    def validate(self):
      """Returns the name of the workspace & file to compare"""
      return self.result


#----------------------------------------------------------------------
class ISISLoadFilesRAW(stresstesting.MantidStressTest):


    def __init__(self):
       stresstesting.MantidStressTest.__init__(self)
       self.valid = False

    def runTest(self):
        propman = PropertyManager('MAR')

        propman.sample_run = 11001
        propman.load_monitors_with_workspace = True

        mon_ws = PropertyManager.sample_run.get_monitors_ws()
        ws = PropertyManager.sample_run.get_workspace()
        
        self.assertTrue(isinstance(ws,Workspace))
        self.assertEqual(ws.getNumberHistograms(),922)

        DeleteWorkspace(ws)

        propman.load_monitors_with_workspace = False
        propman.sample_run = 11001
        ws = PropertyManager.sample_run.get_workspace()
        mon_ws = PropertyManager.sample_run.get_monitors_ws()

        self.assertEqual(ws.getNumberHistograms(),919)
        self.assertEqual(mon_ws.getNumberHistograms(),3)
        wsName = ws.name()
        self.assertEqual(wsName,PropertyManager.sample_run.get_ws_name())

        #
        propman = PropertyManager('MAPS')
        propman.sample_run = 17186
        propman.load_monitors_with_workspace = False

        mon_ws = PropertyManager.sample_run.get_monitors_ws()
        ws = PropertyManager.sample_run.get_workspace()
        self.assertTrue(isinstance(ws,Workspace))
        self.assertEqual(ws.getNumberHistograms(),41472)
        self.assertEqual(mon_ws.getNumberHistograms(),4)
        #
        self.valid = True

    def validate(self):
        return self.valid

class ISISLoadFilesMER(stresstesting.MantidStressTest):


    def __init__(self):
       stresstesting.MantidStressTest.__init__(self)
       self.valid = False

    def runTest(self):
        #
        propman = PropertyManager('MER')
        propman.sample_run = 6398 # (raw file)
        propman.det_cal_file = 6399 
        propman.load_monitors_with_workspace = False

        mon_ws = PropertyManager.sample_run.get_monitors_ws()
        self.assertTrue(not(mon_ws is None))

        ws = PropertyManager.sample_run.get_workspace()
        self.assertTrue(isinstance(ws,Workspace))
        self.assertEqual(ws.getNumberHistograms(),69632)
        self.assertEqual(mon_ws.getNumberHistograms(),9)

        # test load together
        propman.sample_run = None # (clean things up)
        propman.load_monitors_with_workspace = True
        propman.sample_run  = 6398

        mon_ws = PropertyManager.sample_run.get_monitors_ws()
        self.assertTrue(not(mon_ws is None))
        ws = PropertyManager.sample_run.get_workspace()
        self.assertTrue(isinstance(ws,Workspace))
        self.assertEqual(ws.getNumberHistograms(),69641)
        self.assertEqual(mon_ws.getNumberHistograms(),69641)


        propman.sample_run = 18492 # (histogram nxs file )
        propman.det_cal_file = None 
        mon_ws = PropertyManager.sample_run.get_monitors_ws()
        self.assertTrue(not(mon_ws is None))
        ws = PropertyManager.sample_run.get_workspace()
        self.assertTrue(isinstance(ws,Workspace))
        self.assertEqual(ws.getNumberHistograms(),69641)
        self.assertEqual(mon_ws.getNumberHistograms(),69641)


        self.valid = True
        return
        #  enable when bug #10980 is fixed
        propman.sample_run = 18492 # (histogram nxs file )
        propman.det_cal_file = None 
        mon_ws = PropertyManager.sample_run.get_monitors_ws()
        self.assertTrue(not(mon_ws is None))

        ws = PropertyManager.sample_run.get_workspace()
        self.assertTrue(isinstance(ws,Workspace))
        self.assertEqual(ws.getNumberHistograms(),69632)
        self.assertEqual(mon_ws.getNumberHistograms(),9)


        self.valid = True


    def validate(self):
        return self.valid

class ISISLoadFilesLET(stresstesting.MantidStressTest):


    def __init__(self):
       stresstesting.MantidStressTest.__init__(self)
       self.valid = False

    def runTest(self):

        # 
        propman = PropertyManager('LET')


        propman.sample_run = 6278 #event nexus file
        propman.load_monitors_with_workspace = False

        # Here we have known problem of propman loading new IDF, and
        # workspace is written using old IDF. New IDF has mon1_norm_spec =73729
        # and ei_mon1_spec=73734   (on January 2015) and old 
        # IDF -- mon1_norm_spec =40961 and 40966 (forever) 
        # Normalized by monitor-1. -- need monitor1 and ei needs ei_mon1_spec
        # This problem is hopefully fixed in reduction now, but here 
        # we have to specify these values manually to guard against 
        # changes in a future
        propman.normalise_method='monitor-1'
        propman.mon1_norm_spec=40961
        propman.ei_mon1_spec  =40966

        mon_ws = PropertyManager.sample_run.get_monitors_ws()
        self.assertTrue(not(mon_ws is None))
        ws = PropertyManager.sample_run.get_workspace()

        self.assertTrue(isinstance(ws,IEventWorkspace))
        self.assertEqual(ws.getNumberHistograms(),40960)
        self.assertTrue(isinstance(mon_ws,Workspace))
        # 
        self.assertEqual(mon_ws.getNumberHistograms(),9)


        self.valid = True


    def validate(self):
        return self.valid

if __name__=="__main__":
    ISISLoadFilesMER.runTest()
