import stresstesting
from mantid.simpleapi import *
from mantid.api import Workspace


#----------------------------------------------------------------------
class ConvertToMDworkflow(stresstesting.MantidStressTest):
    """
    """


  
    def runTest(self):

    # let's load test event workspace, which has been already preprocessed and available in Mantid Test folder
        WS_Name='CNCS_7860_event'
        Load(Filename=WS_Name,OutputWorkspace=WS_Name)
        # this workspace has been  obtained from an inelastic experiment with input energy Ei = 3. 
        # Usually this energy is stored in workspace
        # but if it is not, we have to provide it for inelastic conversion to work.
        AddSampleLog(Workspace=WS_Name,LogName='Ei',LogText='3.0',LogType='Number')
        # disable multithreaded splitting as BoxID-s are assigned in random manner
        # AddSampleLog(Workspace=WS_Name,LogName='NUM_THREADS',LogText='0',LogType='Number')
        #
        # set up target ws name and remove target workspace with the same name which can occasionally exist.
        RezWS = 'WS_4D'
        try:
            DeleteWorkspace(RezWS)
        except ValueError:
            print "Target ws ",RezWS," not found in analysis data service\n"
    #
        #---> Start loop over contributing files
        for i in xrange(0,20,5):
            # the following operations simulate different workspaces, obtained from experiment using rotating crystal;
            # For real experiment we  usually just load these workspaces from nxspe files with proper Psi values defined there
            # and have to set up ub matrix
            SourceWS = 'SourcePart'+str(i)
            # ws emulation begin ----> 
            CloneWorkspace(InputWorkspace=WS_Name,OutputWorkspace=SourceWS)
            # using scattering on a crystal with cubic lattice and 1,0,0 direction along the beam.
            SetUB(Workspace=SourceWS,a='1.4165',b='1.4165',c='1.4165',u='1,0,0',v='0,1,0')	
            # rotated by proper number of degrees around axis Y
            AddSampleLog(Workspace=SourceWS,LogName='Psi',LogText=str(i)+'.0',LogType='Number Series')
            SetGoniometer(Workspace=SourceWS,Axis0='Psi,0,1,0,1')
            # ws emulation, end ---------------------------------------------------------------------------------------
 
            ConvertToMD(InputWorkspace=SourceWS,OutputWorkspace=RezWS,QDimensions='Q3D',QConversionScales='HKL',\
            OverwriteExisting=0,dEAnalysisMode='Direct',MinValues='-3,-3,-3,-1',MaxValues='3,3,3,3',\
            SplitInto="20,20,1,1")
            # delete source workspace from memory;
            DeleteWorkspace(SourceWS)


    def validate(self):
      """Returns the name of the workspace & file to compare"""
      self.tolerance = 1e-5
      #elf.disableChecking.append('SpectraMap')
      #elf.disableChecking.append('Instrument')
      result = 'WS_4D'
      reference = "ConvertToMDSample.nxs"
      
      valNames = [result,reference]
      from mantid.simpleapi import Load,CompareMDWorkspaces,FrameworkManager,SaveNexus
      
      Load(Filename=reference,OutputWorkspace=valNames[1])

      checker = AlgorithmManager.create("CompareMDWorkspaces")
      checker.setLogging(True)
      checker.setPropertyValue("Workspace1",result)
      checker.setPropertyValue("Workspace2",valNames[1])
      checker.setPropertyValue("Tolerance", str(self.tolerance))
      checker.setPropertyValue("IgnoreBoxID", "1")
      checker.setPropertyValue("CheckEvents", "1")

      checker.execute()
      if checker.getPropertyValue("Equals") != "1":
           print " Workspaces do not match, result: ",checker.getPropertyValue("Result")
           print self.__class__.__name__
           SaveMD(InputWorkspace=valNames[0],Filename=self.__class__.__name__+'-mismatch.nxs')
           return False

      
      return True


