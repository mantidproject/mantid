#pylint: disable=no-init,invalid-name
import math
import stresstesting
from mantid.simpleapi import *

from abc import ABCMeta, abstractmethod

#----------------------------------------------------------------------
class ISISMuonAnalysis(stresstesting.MantidStressTest):
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
        - x_min: Float value of the minimum x.
        - x_max: Float value of the maximum x.
        - rebin: Boolean to tell whether rebinning is to be done.
        - rebin_fixed: Optional boolean to tell if the rebinning is in fixed steps.
        - rebin_params: A string containing the rebin parameters. See wiki rebin for more info.
    """
    __metaclass__ = ABCMeta # Mark as an abstract class

    @abstractmethod
    def get_reference_file(self):
        """Returns the name of the reference file to compare against"""
        raise NotImplementedError("Implmenent get_reference_file to return "
                                "the name of the file to compare against.")

    def get_result_workspace(self):
        """Returns the result workspace to be checked"""
        return self.instr_name + str(self.sample_run) 

    def runTest(self):
        """Defines the workflow for the test"""

        self._validate_properties()

        outputWS = (self.instr_name + str(self.sample_run) )

      # Load
        LoadMuonNexus(Filename=self.file_name, OutputWorkspace='MuonAnalysis' )

      # Group, Crop, Clone
        if self.period_data:
            GroupDetectors(InputWorkspace='MuonAnalysis_1', OutputWorkspace=outputWS, MapFile=self.map_name)
        else:
            GroupDetectors(InputWorkspace='MuonAnalysis', OutputWorkspace=outputWS, MapFile=self.map_name)
        CropWorkspace(InputWorkspace=outputWS, OutputWorkspace=outputWS, XMin=self.x_min, XMax=self.x_max)
        CloneWorkspace(InputWorkspace=outputWS, OutputWorkspace=(outputWS + '_Raw') )

      # Rebin then...
        if self.rebin :

            ws = mtd[outputWS]
            binSize = ws.dataX(0)[1]-ws.dataX(0)[0]
            firstX = ws.dataX(0)[0]
            lastX = ws.dataX(0)[ws.blocksize()]

            if self.rebin_fixed:
                Rebin(InputWorkspace=outputWS, OutputWorkspace=outputWS, Params=str(binSize*float(self.rebin_params) ) )
            else:
                Rebin(InputWorkspace=outputWS, OutputWorkspace=outputWS, Params=self.rebin_params)

            numberOfFullBunchedBins = math.floor((lastX - firstX) / binSize )

        # ...Crop
            if numberOfFullBunchedBins > 0:
                lastX = firstX + numberOfFullBunchedBins*binSize
                lastX_str = '%.15f' % lastX
                CropWorkspace(InputWorkspace=outputWS, OutputWorkspace=outputWS, XMax=lastX_str )

        GroupWorkspaces(InputWorkspaces=outputWS + ',' + outputWS + '_Raw', OutputWorkspace='MuonGroup')

        if self.asym:
            AsymmetryCalc(InputWorkspace=outputWS, OutputWorkspace=outputWS, ForwardSpectra='0', BackwardSpectra='1')

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
        if type(self.instr_name) != str:
            raise RuntimeError("instr_name property should be a string")
        if type(self.file_name) != str:
            raise RuntimeError("file_name property should be a string")
        if type(self.period_data) != bool:
            raise RuntimeError("period_data property should be a bool")



#------------------------- ARGUS tests -------------------------------------------------

class ARGUSAnalysisFromFile(ISISMuonAnalysis):

    def __init__(self):
        ISISMuonAnalysis.__init__(self)
        self.file_name = 'argus0044309.nxs'
        self.map_name = 'ARGUSGrouping.xml'
        self.instr_name = 'ARGUS'
        self.sample_run = 44309
        self.asym = True
        self.period_data = False
        self.x_min = 2
        self.x_max = 12
        self.rebin = True
        self.rebin_fixed = True
        self.rebin_params = '1'

    def get_reference_file(self):
        return "ARGUSAnalysis.nxs"


#------------------------- EMU tests -------------------------------------------------

class EMUAnalysisFromFile(ISISMuonAnalysis):

    def __init__(self):
        ISISMuonAnalysis.__init__(self)
        self.file_name = 'emu00031895.nxs'
        self.map_name = 'EMUGrouping.xml'
        self.instr_name = 'EMU'
        self.sample_run = 31895
        self.asym = True
        self.period_data = True
        self.x_min = 0.11
        self.x_max = 10
        self.rebin = False

    def get_reference_file(self):
        return "EMUAnalysis.nxs"


#------------------------- HiFi tests -------------------------------------------------

class HiFiAnalysisFromFile(ISISMuonAnalysis):

    def __init__(self):
        ISISMuonAnalysis.__init__(self)
        self.file_name = 'hifi00038401.nxs'
        self.map_name = 'HiFiGrouping.xml'
        self.instr_name = 'Hifi'
        self.sample_run = 38401
        self.asym = True
        self.period_data = False
        self.x_min = 1
        self.x_max = 5
        self.rebin = True
        self.rebin_fixed = True
        self.rebin_params = '1'

    def get_reference_file(self):
        return "HiFiAnalysis.nxs"


#------------------------- MuSR tests -------------------------------------------------

class MuSRAnalysisFromFile(ISISMuonAnalysis):

    def __init__(self):
        ISISMuonAnalysis.__init__(self)
        self.file_name = 'MUSR00015192.nxs'
        self.map_name = 'MuSRGrouping.xml'
        self.instr_name = 'MuSR'
        self.sample_run = 15192
        self.asym = True
        self.period_data = True
        self.x_min = 0.11
        self.x_max = 10
        self.rebin = True
        self.rebin_fixed = False
        self.rebin_params = '0.11,0.0159999,10'

    def get_reference_file(self):
        return "MuSRAnalysis.nxs"

 