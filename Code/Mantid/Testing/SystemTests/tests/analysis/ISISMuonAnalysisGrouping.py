import math
import stresstesting
from mantid.simpleapi import *

from abc import ABCMeta, abstractmethod

#----------------------------------------------------------------------
class ISISMuonAnalysisGrouping(stresstesting.MantidStressTest):
    """A base class for the ISIS Muon Analysis tests
    
    The workflow is defined in the runTest() method, simply
    define an __init__ method and set the following properties
    on the object
        - file_name: String pointing to nexus file to be used.
        - map_name: String pointing to xml grouping file.
        - instr_name: A string giving the instrument name.
        - sample_run: An integer run number of the sample
        - period_data: A boolean denoting whether the file has period data.
        - asym: A boolean to tell whether the plot type is assymetry or not.
        - logs: A boolean to tell whether the plot type is logorithmic or not.
        - x_min: Float value of the minimum x.
        - x_max: Float value of the maximum x.
    """
    __metaclass__ = ABCMeta # Mark as an abstract class

    @abstractmethod
    def get_reference_file(self):
      """Returns the name of the reference file to compare against"""
      raise NotImplementedError("Implmenent get_reference_file to return "
                                "the name of the file to compare against.")

    def get_result_workspace(self):
        """Returns the result workspace to be checked"""
        return (self.instr_name + str(self.sample_run) )
  
    def runTest(self):
      """Defines the workflow for the test"""

      self._validate_properties()
      
      outputWS = (self.instr_name + str(self.sample_run) )
      
      # Load
      LoadMuonNexus(Filename=self.file_name, OutputWorkspace='MuonAnalysis' )
      
      # Group, Crop, Clone
      if(self.period_data):
        GroupDetectors(InputWorkspace='MuonAnalysis_1', OutputWorkspace=outputWS, MapFile=self.map_name)
      else:
        GroupDetectors(InputWorkspace='MuonAnalysis', OutputWorkspace=outputWS, MapFile=self.map_name)
      CropWorkspace(InputWorkspace=outputWS, OutputWorkspace=outputWS, XMin=self.x_min, XMax=self.x_max)
      CloneWorkspace(InputWorkspace=outputWS, OutputWorkspace=(outputWS + '_Raw') )
      GroupWorkspaces(InputWorkspaces=outputWS + ',' + outputWS + '_Raw', OutputWorkspace='MuonGroup')

      if(self.logs):
        Logarithm(InputWorkspace=outputWS, OutputWorkspace=outputWS)
      if(self.asym):
        RemoveExpDecay(InputWorkspace=outputWS, OutputWorkspace=outputWS)
      
      
    def validate(self):
      """Returns the name of the workspace & file to compare"""
      self.tolerance = 1e-7
      self.disableChecking.append('SpectraMap')
      self.disableChecking.append('Instrument')
      result = self.get_result_workspace()
      reference = self.get_reference_file()
      return result, reference

    def _validate_properties(self):
      """Check the object properties are
      in an expected state to continue
      """      
      if type(self.file_name) != str:
        raise RuntimeError("file_name property should be a string")
      if type(self.map_name) != str:
        raise RuntimeError("map_name property should be a string")
      if type(self.instr_name) != str:
        raise RuntimeError("instr_name property should be a string")
      if type(self.period_data) != bool:
        raise RuntimeError("period_data property should be a bool")
      if type(self.asym) != bool:
        raise RuntimeError("asym property should be a bool")
      if type(self.logs) != bool:
        raise RuntimeError("log property should be a bool")



#------------------------- ARGUS group fwd test -------------------------------------------------

class ARGUSAnalysisFromFile(ISISMuonAnalysisGrouping):

  def __init__(self):
    ISISMuonAnalysisGrouping.__init__(self)
    self.file_name = 'argus0044309.nxs'
    self.map_name = 'ARGUSFwdGrouping.xml'
    self.instr_name = 'ARGUS'
    self.sample_run = 44309
    self.period_data = False
    self.asym = False
    self.logs = True
    self.x_min = 3
    self.x_max = 10
    
  def get_reference_file(self):
    return "ARGUSAnalysisLogFwd.nxs"


#------------------------- EMU group fwd test -------------------------------------------------

class EMUAnalysisFromFile(ISISMuonAnalysisGrouping):

  def __init__(self):
    ISISMuonAnalysisGrouping.__init__(self)
    self.file_name = 'emu00031895.nxs'
    self.map_name = 'EMUFwdGrouping.xml'
    self.instr_name = 'EMU'
    self.sample_run = 31895
    self.period_data = True
    self.asym = True
    self.logs = False
    self.x_min = 0.11
    self.x_max = 10

 
  def get_reference_file(self):
    return "EMUAnalysisAsymFwd.nxs"


#------------------------- HiFi group 0 test -------------------------------------------------

class HiFiAnalysisFromFile(ISISMuonAnalysisGrouping):

  def __init__(self):
    ISISMuonAnalysisGrouping.__init__(self)
    self.file_name = 'hifi00038401.nxs'
    self.map_name = 'HiFi0Grouping.xml'
    self.instr_name = 'Hifi'
    self.sample_run = 38401
    self.period_data = False
    self.asym = True
    self.logs = False
    self.x_min = 0.1199
    self.x_max = 7.4999
 
  def get_reference_file(self):
    return "HiFiAnalysisAsym0.nxs"
    

#------------------------- MuSR Group 1 test -------------------------------------------------

class MuSRAnalysisFromFile(ISISMuonAnalysisGrouping):

  def __init__(self):
    ISISMuonAnalysisGrouping.__init__(self)
    self.file_name = 'MUSR00015192.nxs'
    self.map_name = 'MuSR1Grouping.xml'
    self.instr_name = 'MuSR'
    self.sample_run = 15192
    self.period_data = True
    self.asym = False
    self.logs = True
    self.x_min = 1.4
    self.x_max = 3.9

 
  def get_reference_file(self):
    return "MuSRAnalysisLog1.nxs"
 
 