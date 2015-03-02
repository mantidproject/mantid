import stresstesting
from mantid.simpleapi import *
from mantid.api import Workspace
import os,shutil

from abc import ABCMeta, abstractmethod
from Direct.PropertyManager  import PropertyManager


#----------------------------------------------------------------------
class ISISDirectInelasticReduction(stresstesting.MantidStressTest):
    """A base class for the ISIS direct inelastic tests
    
    The workflow is defined in the runTest() method, simply
    define an __init__ method and set the following properties
    on the object
        - instr_name: A string giving the instrument name for the test
        - sample_run: An integer run number of the sample or a a workspace
        - incident_energy: A float value for the Ei guess
        - bins: A list of rebin parameters
        - white_beam: An integer giving a white_beam_file or a workspace
        - mono_van: An integer giving a mono-vanadium run or a workspace or None
        - map_file: An optional string pointing to a map file 
        - sample_mass: A float value for the sample mass or None
        - sample_rmm: A float value for the sample rmm or None
        - hard_mask: An hard mask file or None
    """
    __metaclass__ = ABCMeta # Mark as an abstract class

    @abstractmethod
    def get_reference_file(self):
      """Returns the name of the reference file to compare against"""
      raise NotImplementedError("Implement get_reference_file to return "
                                "the name of the file to compare against.")

    @abstractmethod
    def get_result_workspace(self):
        """Returns the result workspace to be checked"""

    @abstractmethod
    def runTest(self):
      """Defines the workflow for the test"""
     # rename workspace to the name expected by unit test framework



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

    def _is_numeric(self, obj):
      """Returns true if the object is an int or float, false otherwise"""
      if type(obj) != float or type(obj) != int:
        return True
      else:
        return False
      
    def _is_workspace(self, obj):
      """ Returns True if the object is a workspace"""
      return isinstance(obj, Workspace)
    def __init__(self):
        stresstesting.MantidStressTest.__init__(self)
        # this is temporary parameter 
        self.scale_to_fix_abf=1 

#------------------------- MARI tests -------------------------------------------------

class MARIReductionFromFile(ISISDirectInelasticReduction):

  def __init__(self):
    ISISDirectInelasticReduction.__init__(self)

    from ISIS_MariReduction import ReduceMARIFromFile

    self.red = ReduceMARIFromFile()
    self.red.def_advanced_properties()
    self.red.def_main_properties()
    # temporary fix to account for different monovan integral
    self.scale_to_fix_abf = 0.997979227566217 

  def runTest(self):
       outWS = self.red.reduce()
       outWS*=self.scale_to_fix_abf



  def get_result_workspace(self):
      """Returns the result workspace to be checked"""
      return "outWS"   
  def get_reference_file(self):
    return "MARIReduction.nxs"
    
class MARIReductionFromFileCache(ISISDirectInelasticReduction):

  def __init__(self):
    ISISDirectInelasticReduction.__init__(self)
    self.tolerance = 1e-9
    from ISIS_MariReduction import ReduceMARIFromFile

    self.red = ReduceMARIFromFile()
    self.red.def_advanced_properties()
    self.red.def_main_properties()

  def prepare_test_file(self):
      """ This method will run instead of pause and 
          would copy run file 11001 into 11002 emulating
          appearance of this file from instrument
      """ 
      self._counter+=1
      if self._counter>= 3:
        source =  FileFinder.findRuns('11001')[0]
        targ_path = config['defaultsave.directory']
        targ_file = os.path.join(targ_path,'MAR11002.raw')
        shutil.copy2(source ,targ_file )

        self._file_to_clear = targ_file
        self._counter = 0
      


  def runTest(self):
       self.red.wait_for_file = 10
       self.red._debug_wait_for_files_operation = self.prepare_test_file
       self._counter=0

       self.red.reducer.prop_man.sample_run = [11001,11002]
       MARreducedRuns = self.red.run_reduction()

       RenameWorkspace(InputWorkspace=MARreducedRuns[0],OutputWorkspace='MARreducedFromFile')
       RenameWorkspace(InputWorkspace=MARreducedRuns[1],OutputWorkspace='MARreducedWithCach')

       self.red.wait_for_file =0
       self.red._debug_wait_for_files_operation = None
       os.remove(self._file_to_clear)

  def validate(self):
      """Returns the name of the workspace & file to compare"""
      super(MARIReductionFromFileCache,self).validate()
      self.tolerance = 1e-9
      return 'MARreducedFromFile', 'MARreducedWithCach'

  def validateMethod(self):
      return "validateWorkspaceToWorkspace"


  def get_result_workspace(self):
      """Returns the result workspace to be checked"""
      return "outWS"   
  def get_reference_file(self):
    return "MARIReduction.nxs"

    
class MARIReductionFromWorkspace(ISISDirectInelasticReduction):

  def __init__(self):
    ISISDirectInelasticReduction.__init__(self)

    from ISIS_MariReduction import ReduceMARIFromWorkspace

    self.red = ReduceMARIFromWorkspace()
    self.red.def_advanced_properties()
    self.red.def_main_properties()

    self.scale_to_fix_abf = 0.997979227566217


  def runTest(self):
      """Defines the workflow for the test"""

      outWS=self.red.reduce()
      # temporary fix to account for different monovan integral
      outWS*=self.scale_to_fix_abf

    
  def get_result_workspace(self):
      """Returns the result workspace to be checked"""
      return "outWS"

  def get_reference_file(self):
    return "MARIReduction.nxs"

class MARIReductionMon2Norm(ISISDirectInelasticReduction):

  def __init__(self):
    ISISDirectInelasticReduction.__init__(self)

    from ISIS_MariReduction import ReduceMARIMon2Norm

    self.red = ReduceMARIMon2Norm()
    self.red.def_advanced_properties()
    self.red.def_main_properties()

  def runTest(self):
      """Defines the workflow for the test"""

      outWS=self.red.reduce()
      # temporary fix to account for different monovan integral
      outWS*=0.989834962505304   

  def get_result_workspace(self):
      """Returns the result workspace to be checked"""
      return "outWS"

  def get_reference_file(self):
    return "MARIReduction.nxs"

  def validate(self):
      result,reference = super(MARIReductionMon2Norm,self).validate()
      self.tolerance = 1e-3
      return result,reference


class MARIReductionMonSeparate(ISISDirectInelasticReduction):

  def __init__(self):
    ISISDirectInelasticReduction.__init__(self)
    # This test has not been run properly so reference file is kind-of 
    # arbitrary. It just checks that this reduction works. 
    # Mari reduction masks are not correct for monitors loaded separately, 
    # This explains all the difference encountered. 
    from ISIS_MariReduction import ReduceMARIMonitorsSeparate

    self.red = ReduceMARIMonitorsSeparate()
    self.red.def_advanced_properties()
    self.red.def_main_properties()

  def runTest(self):
      """Defines the workflow for the test"""
      # temporary fix cross-influence of tests for MARI. changes to nex ticket make this unnecessary
      PropertyManager.mono_correction_factor.set_cash_mono_run_number(None)
      outWS=self.red.reduce()
      # temporary fix to account for different monovan integral
      outWS*=0.997966051169129


  def get_result_workspace(self):
      """Returns the result workspace to be checked"""
      return "outWS"

  def get_reference_file(self):
      # monitor separate for MARI needs new maps and masks so, it is easier to redefine
      # reference file for the time being
    return "MARIReductionMonSeparate.nxs"

class MARIReductionSum(ISISDirectInelasticReduction):

  def __init__(self):

    ISISDirectInelasticReduction.__init__(self)
    from ISIS_MariReduction import MARIReductionSum

    self.red = MARIReductionSum()
    self.red.def_advanced_properties()
    self.red.def_main_properties()

  def runTest(self):
      """Defines the workflow for the test
      It verifies operation on summing two files on demand. No absolute units
      """
      outWS=self.red.reduce()
      #outWS*=1.00001556766686
    
  def get_result_workspace(self):
      """Returns the result workspace to be checked"""
      return "outWS"

  def get_reference_file(self):
    return "MARIReductionSum.nxs"

class MARIReductionWaitAndSum(ISISDirectInelasticReduction):

  def __init__(self):

    ISISDirectInelasticReduction.__init__(self)
    from ISIS_MariReduction import MARIReductionSum

    self.red = MARIReductionSum()
    self.red.def_advanced_properties()
    self.red.def_main_properties()

  def prepare_test_file(self):
      """ This method will run instead of pause and 
          would copy run file 11015 into 11002 emulating
          appearance of this file from instrument
      """ 
      self._counter+=1
      if self._counter>= 3:
        source =  FileFinder.findRuns('11015')[0]
        targ_path = config['defaultsave.directory']
        targ_file = os.path.join(targ_path,'MAR11002.raw')
        shutil.copy2(source ,targ_file )

        self._file_to_clear = targ_file
        self._counter = 0

  def runTest(self):
      """Defines the workflow for the test
      It verifies operation on summing two files on demand. with wait for 
      files appearing on data search path
      """
      self.red.wait_for_file = 100
      self.red._debug_wait_for_files_operation = self.prepare_test_file
      self._counter=0

      self.red.reducer.prop_man.sample_run=[11001,11002]
      outWS = self.red.run_reduction()

      self.red.wait_for_file =0
      self.red._debug_wait_for_files_operation = None
      os.remove(self._file_to_clear)

    
  def get_result_workspace(self):
      """Returns the result workspace to be checked"""
      return "outWS"

  def get_reference_file(self):
    return "MARIReductionSum.nxs"

#------------------------- MAPS tests -------------------------------------------------

class MAPSDgreduceReduction(ISISDirectInelasticReduction):

  def requiredMemoryMB(self):
      """Far too slow for managed workspaces. They're tested in other places. Requires 10Gb"""
      return 10000

  def __init__(self):
    ISISDirectInelasticReduction.__init__(self)

    from ISIS_MAPS_DGSReduction import ReduceMAPS

    self.red = ReduceMAPS()
    self.red.def_advanced_properties()
    self.red.def_main_properties()

  def runTest(self):

      outWS=self.red.reduce()
      #New WBI value 0.02720959162181584
      #Old WBI Value 0.027209867107187088
      # fix old system test. 
      #outWS*=0.02720959162181584/0.027209867107187088

      # rename workspace to the name expected by unit test framework
      #RenameWorkspace(InputWorkspace=outWS,OutputWorkspace=wsName)
      self.ws_name = 'outWS'


  def get_reference_file(self):
    return "MAPSDgreduceReduction.nxs"
  def get_result_workspace(self):
        """Returns the result workspace to be checked"""
        return self.ws_name 


#------------------------- MERLIN tests -------------------------------------------------

class MERLINReduction(ISISDirectInelasticReduction):

  def requiredMemoryMB(self):
      """Far too slow for managed workspaces. They're tested in other places. Requires 16Gb"""
      return 16000

  def __init__(self):
    ''' Test relies on MERLIN_Parameters.xml file introduced in July 2014
    ''' 
    ISISDirectInelasticReduction.__init__(self)

    from ISIS_MERLINReduction import ReduceMERLIN

    self.red = ReduceMERLIN()
    self.red.def_advanced_properties()
    self.red.def_main_properties()

  def runTest(self):
       outWS = self.red.reduce()
    
  def get_reference_file(self):
    return "MERLINReduction.nxs"
  def get_result_workspace(self):
      """Returns the result workspace to be checked"""
      return "outWS"

  def validate(self):
      self.tolerance = 1e-6
      self.tolerance_is_reller=True
      self.disableChecking.append('SpectraMap')
      self.disableChecking.append('Instrument')
      result = self.get_result_workspace()
      reference = self.get_reference_file()
      return result, reference

#------------------------- LET tests -------------------------------------------------
#

class LETReduction(stresstesting.MantidStressTest):

  def requiredMemoryMB(self):
      """Far too slow for managed workspaces. They're tested in other places. Requires 2Gb"""
      return 2000

  def runTest(self):
      """
      Run the LET reduction with event NeXus files
      
      Relies on LET_Parameters.xml file from June 2013
      """
      from ISIS_LETReduction import ReduceLET_OneRep
      red = ReduceLET_OneRep()
      red.def_main_properties()
      red.def_advanced_properties()

      outWS=red.reduce()


  def validate(self):
      self.tolerance = 1e-6
      self.tolerance_is_reller=True
      self.disableChecking.append('SpectraMap')
      self.disableChecking.append('Instrument')

      return "outWS", "LETReduction.nxs"

class LETReductionEvent2014Multirep(stresstesting.MantidStressTest):
  """
  written in a hope that most of the stuff find here will eventually find its way into main reduction routines
  """

  def requiredMemoryMB(self):
      """Far too slow for managed workspaces. They're tested in other places. Requires 20Gb"""
      return 20000

  def runTest(self):
      """
      Run the LET reduction with event NeXus files
      
      Relies on LET_Parameters.xml file from June 2013
      """
      from ISIS_LETReduction import ReduceLET_MultiRep2014
      red = ReduceLET_MultiRep2014()

      red.def_advanced_properties()
      red.def_main_properties()


      out_ws_list=red.reduce()

      #mults =[41.178539329370217/41.178300987983413,72.235863046309746/72.231475173892022]
      #New normalization for 3.4 meV: 41.178539329370217
      #Old normalization for 3.4 meV: 41.178300987983413
      #New normalization for 8 meV: 72.235863046309746
      #Old normalization for 8 meV: 72.231475173892022
      #for ind,ws in enumerate(out_ws_list):
      #  ws *=mults[ind]





  def validate(self):
      self.tolerance = 1e-6
      self.tolerance_is_reller=False
      self.disableChecking.append('SpectraMap')
      self.disableChecking.append('Instrument')

      return "LETreducedEi3.4","LET14305_3_4mev.nxs","LETreducedEi8.0", "LET14305_8_0mev.nxs"

class LETReductionEvent2015Multirep(stresstesting.MantidStressTest):
  """
  written in a hope that most of the stuff find here will eventually find its way into main reduction routines
  """

  def requiredMemoryMB(self):
      """Far too slow for managed workspaces. They're tested in other places. Requires 20Gb"""
      return 20000

  def runTest(self):
      """
      Run the LET reduction with event NeXus files
      
      Relies on LET_Parameters.xml file from June 2013
      """
      from ISIS_LETReduction import ReduceLET_MultiRep2015
      red = ReduceLET_MultiRep2015()

      red.def_advanced_properties()
      red.def_main_properties()


      out_ws_list=red.reduce()

      #for ind,ws in enumerate(out_ws_list):
      #  ws *=mults[ind]





  def validate(self):
      self.tolerance = 1e-6
      self.tolerance_is_reller=False
      self.disableChecking.append('SpectraMap')
      self.disableChecking.append('Instrument')

      return "LETreducedEi3.4","LET14305_3_4meV2015.nxs","LETreducedEi8.0", "LET14305_8_0meV2015.nxs"

