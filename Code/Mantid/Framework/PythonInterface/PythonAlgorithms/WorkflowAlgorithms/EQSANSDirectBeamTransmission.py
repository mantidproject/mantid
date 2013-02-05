'''*WIKI* 
    Compute I(q) for reduced EQSANS data
*WIKI*'''
from mantid.api import *
from mantid.kernel import *
import mantid.simpleapi as api
import os

class EQSANSDirectBeamTransmission(PythonAlgorithm):
    
    def category(self):
        return 'Workflow\\SANS;PythonAlgorithms'
    
    def name(self):
        return 'EQSANSDirectBeamTransmission'
    
    def PyInit(self):
        self.declareProperty(MatrixWorkspaceProperty("InputWorkspace", "", 
                                                     direction=Direction.Input))
        self.declareProperty(FileProperty("SampleDataFilename", "",
                                          action=FileAction.Load,                                          
                                          extensions=['xml', 'nxs', 'nxs.h5']))
        self.declareProperty(FileProperty("EmptyDataFilename", "",
                                          action=FileAction.Load,                                          
                                          extensions=['xml', 'nxs', 'nxs.h5']))
        self.declareProperty("BeamRadius", 3.0, "Beam radius [pixels]")
        self.declareProperty("ThetaDependent", True, 
                             "If true, a theta-dependent correction will be applied")
        self.declareProperty(FileProperty("DarkCurrentFilename", "",
                                          action=FileAction.OptionalLoad,                                          
                                          extensions=['xml', 'nxs', 'nxs.h5']))
        self.declareProperty("UseSampleDarkCurrent", True, 
                             "If true, the sample dark current will be used")
        self.declareProperty("BeamCenterX", 0.0, "Beam center position in X")
        self.declareProperty("BeamCenterY", 0.0, "Beam center position in Y")
        self.declareProperty("FitFramesTogether", False,
                             "If true, the two frames will be fit together")
        self.declareProperty("ReductionProperties", "__sans_reduction_properties", 
                             validator=StringMandatoryValidator(),
                             doc="Property manager name for the reduction")
        self.declareProperty(MatrixWorkspaceProperty("OutputWorkspace", "", 
                                                     direction = Direction.Output))
        self.declareProperty("OutputMessage", "", 
                             direction=Direction.Output, doc = "Output message")
    
    def PyExec(self):
        input_ws_name = self.getPropertyValue('InputWorkspace')
        workspace = self.getProperty('InputWorkspace').value
        
        # Perform transmission correction according to whether or not
        # we are in frame-skipping mode
        if self.getProperty('FitFramesTogether').value or \
            not workspace.getRun().hasProperty('is_frame_skipping') or \
            (workspace.getRun().hasProperty('is_frame_skipping') \
            and workspace.getRun().getProperty('is_frame_skipping').value == 0):
            output_ws_name = self.getPropertyValue('OutputWorkspace')
            msg, ws = self._call_sans_transmission(workspace, output_ws_name)
            self.setPropertyValue("OutputMessage", msg)
            self.setProperty("OutputWorkspace", ws)
        else:
            self._with_frame_skipping(input_ws_name)            
    
    def _call_sans_transmission(self, workspace, output_workspace_name):
        """
            Call the generic transmission correction for SANS
            @param workspace: workspace to correct
            @param output_workspace_name: name of the output workspace
        """
        alg = AlgorithmManager.create('SANSDirectBeamTransmission')
        alg.initialize()
        alg.setChild(True)
        alg.setProperty('InputWorkspace', workspace)
        sample_data_file = self.getProperty("SampleDataFilename").value
        alg.setProperty("SampleDataFilename", sample_data_file)
        empty_data_file = self.getProperty("EmptyDataFilename").value
        alg.setProperty("EmptyDataFilename", empty_data_file)
        beam_radius = self.getProperty("BeamRadius").value
        alg.setProperty("BeamRadius", beam_radius)
        theta_dependent = self.getProperty("ThetaDependent").value
        alg.setProperty("ThetaDependent", theta_dependent)
        dark_current_file = self.getProperty("DarkCurrentFilename").value
        alg.setProperty("DarkCurrentFilename", dark_current_file)
        use_sample_dc = self.getProperty("UseSampleDarkCurrent").value
        alg.setProperty("UseSampleDarkCurrent", use_sample_dc)
        center_x = self.getProperty("BeamCenterX").value
        alg.setProperty("BeamCenterX", center_x)
        center_y = self.getProperty("BeamCenterY").value
        alg.setProperty("BeamCenterY", center_y)
        red_props = self.getProperty("ReductionProperties").value
        alg.setProperty("ReductionProperties", red_props)
        alg.setPropertyValue("OutputWorkspace", output_workspace_name)
        alg.execute()
        if alg.existsProperty('OutputMessage'):
            output_msg = alg.getProperty('OutputMessage').value
        else:
            output_msg = None
        output_ws = alg.getProperty('OutputWorkspace').value
        return (output_msg, output_ws)
    
    def _with_frame_skipping(self, workspace):
        """
            Perform transmission correction assuming frame-skipping
        """
        import TransmissionUtils
        
        sample_file = self.getPropertyValue("SampleDataFilename")
        empty_file = self.getPropertyValue("EmptyDataFilename")
        
        property_manager_name = self.getProperty("ReductionProperties").value
        property_manager = PropertyManagerDataService.retrieve(property_manager_name)
        
        # Build the name we are going to give the transmission workspace
        sample_basename = os.path.basename(sample_file)
        empty_basename = os.path.basename(empty_file)
        entry_name = "Transmission%s%s" % (sample_basename, empty_basename)
        trans_ws_name = "__transmission_fit_%s" % sample_basename
        trans_ws = None
        
        if property_manager.existsProperty(entry_name):
            trans_ws_name = property_manager.getProperty(entry_name)
            if AnalysisDataService.doesExist(trans_ws_name):
                trans_ws = AnalysisDataService.retrieve(trans_ws_name)

        output_str = ""
        if trans_ws is None:
            trans_ws_name = "transmission_fit_"+workspace
            # Load data files
            sample_mon_ws, empty_mon_ws, first_det, output_str, monitor_det_ID = TransmissionUtils.load_monitors(self, property_manager)
            
            def _crop_and_compute(wl_min_prop, wl_max_prop, suffix):
                # Get the wavelength band from the run properties
                if mtd[workspace].getRun().hasProperty(wl_min_prop):
                    wl_min = mtd[workspace].getRun().getProperty(wl_min_prop).value
                else:
                    raise RuntimeError, "DirectBeamTransmission could not retrieve the %s property" % wl_min_prop
                
                if mtd[workspace].getRun().hasProperty(wl_max_prop):
                    wl_max = mtd[workspace].getRun().getProperty(wl_max_prop).value
                else:
                    raise RuntimeError, "DirectBeamTransmission could not retrieve the %s property" % wl_max_prop
                
                api.Rebin(InputWorkspace=workspace, OutputWorkspace=workspace+suffix,
                      Params="%4.1f,%4.1f,%4.1f" % (wl_min, 0.1, wl_max),
                      PreserveEvents=False)
                api.Rebin(InputWorkspace=sample_mon_ws, OutputWorkspace=sample_mon_ws+suffix,
                      Params="%4.1f,%4.1f,%4.1f" % (wl_min, 0.1, wl_max),
                      PreserveEvents=False)
                api.Rebin(InputWorkspace=empty_mon_ws, OutputWorkspace=empty_mon_ws+suffix,
                      Params="%4.1f,%4.1f,%4.1f" % (wl_min, 0.1, wl_max),
                      PreserveEvents=False)
                TransmissionUtils.calculate_transmission(self, sample_mon_ws+suffix, empty_mon_ws+suffix, first_det, self._transmission_ws+suffix)
                api.RebinToWorkspace(self._transmission_ws+suffix, workspace, OutputWorkspace=self._transmission_ws+suffix)
                api.RebinToWorkspace(self._transmission_ws+suffix+'_unfitted', workspace, OutputWorkspace=self._transmission_ws+suffix+'_unfitted')
                return self._transmission_ws+suffix
                
            # First frame
            trans_frame_1 = _crop_and_compute("wavelength_min", "wavelength_max", "_frame1")
            
            # Second frame
            trans_frame_2 = _crop_and_compute("wavelength_min_frame2", "wavelength_max_frame2", "_frame2")
            
            api.Plus(trans_frame_1, trans_frame_2, self._transmission_ws)
            api.Plus(trans_frame_1+'_unfitted', trans_frame_2+'_unfitted', self._transmission_ws+'_unfitted')

            # Clean up            
            for ws in [trans_frame_1, trans_frame_2, 
                       trans_frame_1+'_unfitted', trans_frame_2+'_unfitted',
                       sample_mon_ws, empty_mon_ws]:
                if AnalysisDataService.doesExist(ws):
                    AnalysisDataService.remove(ws) 
            
        # 2- Apply correction (Note: Apply2DTransCorr)
        #Apply angle-dependent transmission correction using the zero-angle transmission
        TransmissionUtils.apply_transmission(self, workspace)
  
        
registerAlgorithm(EQSANSDirectBeamTransmission)
