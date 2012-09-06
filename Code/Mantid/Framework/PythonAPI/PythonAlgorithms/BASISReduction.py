"""*WIKI* 

This algorithm is meant to temporarily deal with letting BASIS reduce lots 
of files via Mantid. The syntax for the run number designation will allow 
groups of runs to be joined. Examples:

1. 2144-2147,2149,2156
2. 2144-2147;2149;2156

Example 1 will be summed into a single run
Example 2 will have three run groups

*WIKI*"""

import mantid.simpleapi as api
from mantid.api import *
from mantid.kernel import *
from mantid import config
import os

MICROEV_TO_MILLIEV = 1000.0
DEFAULT_BINS = [0., 0., 0.]

class BASISReduction(PythonAlgorithm):
    def category(self):
        return "Inelastic;PythonAlgorithms"

    def name(self):
        return "BASISReduction"

    def PyInit(self):
        self._short_inst = "BSS"
        self._long_inst = "BASIS"
        self._extension = "_event.nxs"

        self.declareProperty("RunNumbers", "", "Sample run numbers")
        self.declareProperty("NoMonitorNorm", False, 
                             "Stop monitor normalization")
        self.declareProperty(FloatArrayProperty("EnergyBins", DEFAULT_BINS, 
                                                direction=Direction.Input), 
                             "Energy transfer binning scheme (in ueV)")
        self.declareProperty(FloatArrayProperty("MomentumTransferBins", 
                                                DEFAULT_BINS, 
                                                direction=Direction.Input), 
                             "Momentum transfer binning scheme")
        self.declareProperty("MaskGroupFileDir", "/SNS/BSS/shared/autoreduce",
                             "Directory location for standard masking and grouping files.")
        grouping_type = ["None", "Low-Resolution", "By-Tube"]
        self.declareProperty("GroupDetectors", "None", 
                             StringListValidator(grouping_type), 
                             "Switch for grouping detectors")
        
    def PyExec(self):
        config['default.facility'] = "SNS"
        config['default.instrument'] = self._long_inst
        self._etBins = self.getProperty("EnergyBins").value / MICROEV_TO_MILLIEV
        self._qBins = self.getProperty("MomentumTransferBins").value
        self._noMonNorm = self.getProperty("NoMonitorNorm").value
        self._ancFileDir = self.getProperty("MaskGroupFileDir").value
        self._groupDetOpt = self.getProperty("GroupDetectors").value

        datasearch = config["datasearch.searcharchive"]
        if (datasearch != "On"):
            config["datasearch.searcharchive"] = "On"

        config.appendDataSearchDir(self._ancFileDir)

        api.LoadMask(Instrument='BASIS', OutputWorkspace='BASIS_MASK', 
                     InputFile='BASIS_Mask.xml')

        self._run_list = self._getRuns(self.getProperty("RunNumbers").value)
        for run_set in self._run_list:
            self._samWs = self._makeRunName(run_set[0])
            self._samWsRun = str(run_set[0])
            for run in run_set:
                ws_name = self._makeRunName(run)
                mon_ws_name = ws_name  + "_monitors"
                run_file = self._makeRunFile(run)
                
                api.Load(Filename=run_file, OutputWorkspace=ws_name)
                api.MaskDetectors(Workspace=ws_name, 
                                  MaskedWorkspace='BASIS_MASK')
                api.ModeratorTzero(InputWorkspace=ws_name, OutputWorkspace=ws_name)
                api.LoadParameterFile(Workspace=ws_name, 
                                      Filename='BASIS_silicon_111_Parameters.xml')
                api.ConvertUnits(InputWorkspace=ws_name, 
                                 OutputWorkspace=ws_name,
                                 Target='Wavelength', EMode='Indirect')
                
                if not self._noMonNorm:
                    api.LoadNexusMonitors(Filename=run_file, 
                                          OutputWorkspace=mon_ws_name)
                    api.ModeratorTzero(InputWorkspace=mon_ws_name, 
                                       OutputWorkspace=mon_ws_name)
                    api.Rebin(InputWorkspace=mon_ws_name, 
                              OutputWorkspace=mon_ws_name, Params='10')
                    api.ConvertUnits(InputWorkspace=mon_ws_name, 
                                     OutputWorkspace=mon_ws_name, 
                                     Target='Wavelength')
                    api.OneMinusExponentialCor(InputWorkspace=mon_ws_name,
                                               OutputWorkspace=mon_ws_name,
                                               C='0.20749999999999999', 
                                               C1='0.001276')
                    api.Scale(InputWorkspace=mon_ws_name, 
                          OutputWorkspace=mon_ws_name,
                          Factor='9.9999999999999995e-07')
                    api.RebinToWorkspace(WorkspaceToRebin=ws_name, 
                                         WorkspaceToMatch=mon_ws_name,
                                         OutputWorkspace=ws_name)
                    api.Divide(LHSWorkspace=ws_name, RHSWorkspace=mon_ws_name, 
                               OutputWorkspace=ws_name)
                    # Remove monitor workspace from list
                    api.DeleteWorkspace(mon_ws_name)
                    
                api.ConvertUnits(InputWorkspace=ws_name, OutputWorkspace=ws_name, 
                                 Target='DeltaE', EMode='Indirect')
                api.CorrectKiKf(InputWorkspace=ws_name, OutputWorkspace=ws_name, 
                                EMode='Indirect')
                
                if self._samWs != ws_name:
                    api.Plus(LHSWorkspace=self._samWs, RHSWorkspace=ws_name,
                             OutputWorkspace=self._samWs)
                    api.DeleteWorkspace(ws_name)  

            # Run things that need to be done once    
            api.Rebin(InputWorkspace=self._samWs, OutputWorkspace=self._samWs, 
                      Params=self._etBins)
            if self._groupDetOpt != "None":
                if self._groupDetOpt == "Low-Resolution":
                    grp_file = "BASIS_Grouping_LR.xml"
                else:
                    grp_file = "BASIS_Grouping.xml"
                api.GroupDetectors(InputWorkspace=self._samWs, 
                                   OutputWorkspace=self._samWs,
                                   MapFile=grp_file, Behaviour="Sum")
                
            self._samSqwWs = self._samWs+'_sqw'
            api.SofQW3(InputWorkspace=self._samWs, 
                       OutputWorkspace=self._samSqwWs,
                       QAxisBinning=self._qBins, EMode='Indirect', 
                       EFixed='2.0826')
            
            dave_grp_filename = self._makeRunName(self._samWsRun, 
                                                  False) + ".grp"
            api.SaveDaveGrp(Filename=dave_grp_filename, 
                            InputWorkspace=self._samSqwWs,
                            ToMicroEV=True)
            processed_filename = self._makeRunName(self._samWsRun, 
                                                   False) + "_sqw.nxs"
            api.SaveNexus(Filename=processed_filename, 
                          InputWorkspace=self._samSqwWs) 

    def _getRuns(self, rlist):
        """
        Create sets of run numbers for analysis. A semicolon indicates a 
        separate group of runs to be processed together.
        """
        run_list = []
        rlvals = rlist.split(';')
        for rlval in rlvals:
            iap = IntArrayProperty("", rlval)  
            run_list.append(iap.value)
        return run_list

    def _makeRunName(self, run, useShort=True):
        """
        Make name like BSS_24234
        """
        if useShort:
            return self._short_inst + "_" + str(run)
        else:
            return self._long_inst + "_" + str(run)
    
    def _makeRunFile(self, run):
        """
        Make name like BSS24234
        """
        return self._short_inst + str(run) 
    
# Register algorithm with Mantid.
registerAlgorithm(BASISReduction)