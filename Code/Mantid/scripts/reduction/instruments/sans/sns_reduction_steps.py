"""
    Implementation of reduction steps for SNS EQSANS    
"""
import os
import sys
import pickle
import math
from reduction import ReductionStep
from sans_reduction_steps import BaseTransmission, BaseBeamFinder, WeightedAzimuthalAverage
from sans_reduction_steps import DirectBeamTransmission as SingleFrameDirectBeamTransmission
from sans_reduction_steps import SaveIqAscii as BaseSaveIqAscii
from sans_reduction_steps import SensitivityCorrection as BaseSensitivityCorrection
from reduction import extract_workspace_name, find_file, find_data
from eqsans_config import EQSANSConfig
from eqsans_load import LoadRun

# Mantid imports
from MantidFramework import *
from mantidsimple import *
    
class Normalize(ReductionStep):
    """
        Normalize the data to the accelerator current
    """
    def __init__(self, normalize_to_beam=True):
        super(Normalize, self).__init__()
        self._normalize_to_beam = normalize_to_beam
        
    def get_normalization_spectrum(self):
        return -1
    
    def execute(self, reducer, workspace):
        # Flag the workspace as dirty
        reducer.dirty(workspace)
        
        flux_data_path = None
        if self._normalize_to_beam:
            # Find available beam flux file
            # First, check whether we have access to the SNS mount, if
            # not we will look in the data directory
            
            flux_files = find_file(filename="bl6_flux_at_sample", data_dir=reducer._data_path)
            if len(flux_files)>0:
                flux_data_path = flux_files[0]
                mantid.sendLogMessage("Using beam flux file: %s" % flux_data_path)
            else:
                mantid.sendLogMessage("Could not find beam flux file!")
                
            if flux_data_path is not None:
                beam_flux_ws = "__beam_flux"
                LoadAscii(flux_data_path, beam_flux_ws, Separator="Tab", Unit="Wavelength")
                ConvertToHistogram(beam_flux_ws, beam_flux_ws)
                RebinToWorkspace(beam_flux_ws, workspace, beam_flux_ws)
                NormaliseToUnity(beam_flux_ws, beam_flux_ws)
                Divide(workspace, beam_flux_ws, workspace)
                mtd[workspace].getRun().addProperty_str("beam_flux_ws", beam_flux_ws, True)
            else:
                flux_data_path = "Could not find beam flux file!"
            
        #NormaliseByCurrent(workspace, workspace)
        proton_charge = mantid.getMatrixWorkspace(workspace).getRun()["proton_charge"].getStatistics().mean
        duration = mantid.getMatrixWorkspace(workspace).getRun()["proton_charge"].getStatistics().duration
        frequency = mantid.getMatrixWorkspace(workspace).getRun()["frequency"].getStatistics().mean
        acc_current = 1.0e-12 * proton_charge * duration * frequency
        
        if mtd[workspace].getRun().hasProperty("is_separate_corrections"):
            pixel_adj = mtd[workspace].getRun().getProperty("pixel_adj").value
            Scale(InputWorkspace=pixel_adj, OutputWorkspace=pixel_adj, Factor=1.0/acc_current, Operation="Multiply")    
        else:
            Scale(InputWorkspace=workspace, OutputWorkspace=workspace, Factor=1.0/acc_current, Operation="Multiply")
        
        return "Data [%s] normalized to accelerator current\n  Beam flux file: %s" % (workspace, flux_data_path) 
    
    
class BeamStopTransmission(BaseTransmission):
    """
        Perform the transmission correction for EQ-SANS using the beam stop hole
    """
    def __init__(self, normalize_to_unity=False, theta_dependent=False):
        super(BeamStopTransmission, self).__init__()
        self._normalize = normalize_to_unity
        self._theta_dependent = theta_dependent
        self._transmission_ws = None
    
    def execute(self, reducer, workspace):
        # Keep track of workspaces to delete when we clean up
        ws_for_deletion = []
        if self._transmission_ws is not None and mtd.workspaceExists(self._transmission_ws):
            # We have everything we need to apply the transmission correction
            pass
        elif mtd[workspace].getRun().hasProperty("transmission_ws"):
            trans_prop = mtd[workspace].getRun().getProperty("transmission_ws")
            if mtd.workspaceExists(trans_prop.value):
                self._transmission_ws = trans_prop.value
            else:
                raise RuntimeError, "The transmission workspace %s is no longer available" % trans_prop.value
        else:
            raw_ws = workspace
            if mtd[workspace].getRun().hasProperty("event_ws"):
                raw_ws_prop = mtd[workspace].getRun().getProperty("event_ws")
                if mtd.workspaceExists(raw_ws_prop.value):
                    if mtd[workspace].getRun().hasProperty("wavelength_min"):
                        wl_min = mtd[workspace].getRun().getProperty("wavelength_min").value
                    else:
                        raise RuntimeError, "Beam-hole transmission correction could not get minimum wavelength"
                    if mtd[workspace].getRun().hasProperty("wavelength_max_frame2"):
                        wl_max = mtd[workspace].getRun().getProperty("wavelength_max_frame2").value
                    elif mtd[workspace].getRun().hasProperty("wavelength_max"):
                        wl_max = mtd[workspace].getRun().getProperty("wavelength_max").value
                    else:
                        raise RuntimeError, "Beam-hole transmission correction could not get maximum wavelength"
                    raw_ws = '__'+raw_ws_prop.value+'_histo'
                    # Need to convert to workspace until somebody fixes Integration. Ticket #3277
                    Rebin(raw_ws_prop.value, raw_ws, "%4.2f,%4.2f,%4.2f" % (wl_min, 0.1, wl_max), False)
                    ws_for_deletion.append(raw_ws)
                else:
                    raise RuntimeError, "The event workspace %s is no longer available" % raw_ws_prop.value

            # The transmission calculation only works on the original un-normalized counts
            if not reducer.is_clean(raw_ws):
                raise RuntimeError, "The transmission can only be calculated using un-modified data"

            if mtd[workspace].getRun().hasProperty("beam_center_x"):
                beam_center_x = mtd[workspace].getRun().getProperty("beam_center_x").value
            else:
                raise RuntimeError, "Transmission correction algorithm could not get beam center x position"
            if mtd[workspace].getRun().hasProperty("beam_center_y"):
                beam_center_y = mtd[workspace].getRun().getProperty("beam_center_y").value
            else:
                raise RuntimeError, "Transmission correction algorithm could not get beam center y position"
    
            if self._transmission_ws is None:
                self._transmission_ws = "beam_hole_transmission_"+workspace
    
            # Calculate the transmission as a function of wavelength
            EQSANSTransmission(InputWorkspace=raw_ws,
                               OutputWorkspace=self._transmission_ws,
                               XCenter=beam_center_x,
                               YCenter=beam_center_y,
                               NormalizeToUnity = self._normalize)
            
            mantid[workspace].getRun().addProperty_str("transmission_ws", self._transmission_ws, True)

        # Apply the transmission. For EQSANS, we just divide by the 
        # transmission instead of using the angular dependence of the
        # correction.
        reducer.dirty(workspace)

        output_str = "Beam hole transmission correction applied"
        # Get the beam spectrum, if available
        transmission_ws = self._transmission_ws
        if mtd[workspace].getRun().hasProperty("beam_flux_ws"):
            beam_flux_ws_name = mtd[workspace].getRun().getProperty("beam_flux_ws").value
            if mtd.workspaceExists(beam_flux_ws_name):
                beam_flux_ws = mtd[beam_flux_ws_name]
                transmission_ws = "__transmission_tmp"
                ws_for_deletion.append(transmission_ws)
                Divide(self._transmission_ws, beam_flux_ws, transmission_ws)
                output_str += "\n  Transmission corrected for beam spectrum"
            else:
                output_str += "\n  Transmission was NOT corrected for beam spectrum: inconsistent meta-data!"
        else:
            output_str += "\n  Transmission was NOT corrected for beam spectrum: check your normalization option!"
        
        if self._theta_dependent:
            # To apply the transmission correction using the theta-dependent algorithm
            # we should get the beam spectrum out of the measured transmission
            # We should then re-apply it when performing normalization
            ApplyTransmissionCorrection(workspace, workspace, transmission_ws)
        else:
            Divide(workspace, transmission_ws, workspace)
        #ReplaceSpecialValues(workspace, workspace, NaNValue=0.0,NaNError=0.0)
        
        # Clean up 
        for ws in ws_for_deletion:
            if mtd.workspaceExists(ws):
                mtd.deleteWorkspace(ws)
                
        return output_str
    
    
class SubtractDarkCurrent(ReductionStep):
    """
        Subtract the dark current from the input workspace.
        Works only if the proton charge time series is available from DASlogs.
    """
    def __init__(self, dark_current_file):
        super(SubtractDarkCurrent, self).__init__()
        self._dark_current_file = dark_current_file
        self._dark_current_ws = None
        
    def execute(self, reducer, workspace):
        """
            Subtract the dark current from the input workspace.
            If no timer workspace is provided, the counting time will be extracted
            from the input workspace.
            
            @param reducer: Reducer object for which this step is executed
            @param workspace: input workspace
        """
        # Sanity check
        if self._dark_current_file is None:
            raise RuntimeError, "SubtractDarkCurrent called with no defined dark current file"

        # Check whether the dark current was already loaded, otherwise load it
        # Load dark current, which will be used repeatedly
        if self._dark_current_ws is None:
            filepath = find_data(self._dark_current_file, instrument=reducer.instrument.name())
            self._dark_current_ws = ReductionStep._create_unique_name(filepath, "dc")
            #self._dark_current_ws = "__dc_"+extract_workspace_name(filepath)
            reducer._data_loader.clone(data_file=filepath).execute(reducer, self._dark_current_ws)
        # Normalize the dark current data to counting time
        dark_duration = mtd[self._dark_current_ws].getRun()["proton_charge"].getStatistics().duration
        duration = mtd[workspace].getRun()["proton_charge"].getStatistics().duration
        scaling_factor = duration/dark_duration
    
        # Scale the stored dark current by the counting time
        scaled_dark_ws = "__scaled_dark_current"
        RebinToWorkspace(WorkspaceToRebin=self._dark_current_ws, WorkspaceToMatch=workspace, OutputWorkspace=scaled_dark_ws)
        Scale(InputWorkspace=scaled_dark_ws, OutputWorkspace=scaled_dark_ws, Factor=scaling_factor, Operation="Multiply")
        
        # Perform subtraction
        Minus(workspace, scaled_dark_ws, workspace)  

        # Clean up 
        if mtd.workspaceExists(scaled_dark_ws):
            mtd.deleteWorkspace(scaled_dark_ws)
        
        return "Dark current subtracted [%s]" % extract_workspace_name(self._dark_current_file)
    
class AzimuthalAverageByFrame(WeightedAzimuthalAverage):
    """
        ReductionStep class that performs azimuthal averaging
        and transforms the 2D reduced data set into I(Q).
        Done for each frame independently.
    """
    def __init__(self, binning=None, suffix="_Iq", error_weighting=False, n_bins=100, n_subpix=1, log_binning=False, scale=False):
        super(AzimuthalAverageByFrame, self).__init__(binning, suffix, error_weighting, n_bins, n_subpix, log_binning)
        self._is_frame_skipping = False
        self._scale = scale
        self._tolerance = 1.3
        self._compute_resolution = False
        self._sample_aperture_radius = 5.0
        
    def compute_resolution(self, sample_aperture_diameter=10.0):
        """
            Sets the flag to compute the Q resolution
            @param sample_aperture_diameter: diameter of the sample aperture, in mm
        """
        self._compute_resolution = True
        self._sample_aperture_radius = sample_aperture_diameter/2.0
        
    def get_output_workspace(self, workspace):
        if not self._is_frame_skipping:
            return workspace+self._suffix
        return [workspace+'_frame1'+self._suffix, workspace+'_frame2'+self._suffix]
        
    def execute(self, reducer, workspace):
        # We will need the pixel dimensions to compute the Q resolution        
        pixel_size_x = mtd[workspace].getInstrument().getNumberParameter("x-pixel-size")[0]
        pixel_size_y = mtd[workspace].getInstrument().getNumberParameter("y-pixel-size")[0]
        
        # Get the source aperture radius
        source_aperture_radius = 10.0
        if mtd[workspace].getRun().hasProperty("source-aperture-diameter"):
            source_aperture_radius = mtd[workspace].getRun().getProperty("source-aperture-diameter").value/2.0

        if mtd[workspace].getRun().hasProperty("is_frame_skipping") \
            and mtd[workspace].getRun().getProperty("is_frame_skipping").value==0:
            self._is_frame_skipping = False
            output_str = super(AzimuthalAverageByFrame, self).execute(reducer, workspace)
            if self._compute_resolution:
                EQSANSResolution(InputWorkspace=workspace+self._suffix, 
                                  ReducedWorkspace=workspace, OutputBinning=self._binning,
                                  PixelSizeX=pixel_size_x, PixelSizeY=pixel_size_y,
                                  SourceApertureRadius=source_aperture_radius,
                                  SampleApertureRadius=self._sample_aperture_radius)
            return output_str
        
        self._is_frame_skipping = True
        
        # Get wavelength bands
        # First frame
        wl_min_f1 = None
        wl_max_f1 = None
        if mtd[workspace].getRun().hasProperty("wavelength_min"):
            wl_min_f1 = mtd[workspace].getRun().getProperty("wavelength_min").value
        if mtd[workspace].getRun().hasProperty("wavelength_max"):
            wl_max_f1 = mtd[workspace].getRun().getProperty("wavelength_max").value
        if wl_min_f1 is None and wl_max_f1 is None:
            raise RuntimeError, "Could not get the wavelength band for frame 1"
        
        # Second frame
        wl_min_f2 = None
        wl_max_f2 = None
        if mtd[workspace].getRun().hasProperty("wavelength_min_frame2"):
            wl_min_f2 = mtd[workspace].getRun().getProperty("wavelength_min_frame2").value
        if mtd[workspace].getRun().hasProperty("wavelength_max_frame2"):
            wl_max_f2 = mtd[workspace].getRun().getProperty("wavelength_max_frame2").value
        if wl_min_f2 is None and wl_max_f2 is None:
            raise RuntimeError, "Could not get the wavelength band for frame 2"
        
        # Compute binning
        qmin, qstep, qmax = self._get_binning(reducer, workspace, min(wl_min_f1, wl_min_f2), max(wl_max_f1, wl_max_f2))
        self._binning = "%g, %g, %g" % (qmin, qstep, qmax)
        # Average second frame
        Rebin(workspace, workspace+'_frame2', "%4.2f,%4.2f,%4.2f" % (wl_min_f2, 0.1, wl_max_f2), False)
        ReplaceSpecialValues(workspace+'_frame2', workspace+'_frame2', NaNValue=0.0,NaNError=0.0)
        
        if mtd[workspace].getRun().hasProperty("is_separate_corrections"):
            wl_adj = mtd[workspace].getRun().getProperty("wl_adj").value
            Rebin(wl_adj, wl_adj+'_frame2', "%4.2f,%4.2f,%4.2f" % (wl_min_f2, 0.1, wl_max_f2), False)
            mtd[workspace+'_frame2'].getRun().addProperty_str("wl_adj", wl_adj+'_frame2', True)
            
        super(AzimuthalAverageByFrame, self).execute(reducer, workspace+'_frame2')
        if self._compute_resolution:
            EQSANSResolution(InputWorkspace=workspace+'_frame2'+self._suffix, 
                              ReducedWorkspace=workspace, OutputBinning=self._binning,
                              MinWavelength=wl_min_f2, MaxWavelength=wl_max_f2,
                              PixelSizeX=pixel_size_x, PixelSizeY=pixel_size_y,
                              SourceApertureRadius=source_aperture_radius,
                              SampleApertureRadius=self._sample_aperture_radius)                                                                        
            
        # Average first frame
        Rebin(workspace, workspace+'_frame1', "%4.2f,%4.2f,%4.2f" % (wl_min_f1, 0.1, wl_max_f1), False)
        ReplaceSpecialValues(workspace+'_frame1', workspace+'_frame1', NaNValue=0.0,NaNError=0.0)
        
        if mtd[workspace].getRun().hasProperty("is_separate_corrections"):
            wl_adj = mtd[workspace].getRun().getProperty("wl_adj").value
            Rebin(wl_adj, wl_adj+'_frame1', "%4.2f,%4.2f,%4.2f" % (wl_min_f1, 0.1, wl_max_f1), False)
            mtd[workspace+'_frame1'].getRun().addProperty_str("wl_adj", wl_adj+'_frame1', True)
        super(AzimuthalAverageByFrame, self).execute(reducer, workspace+'_frame1')
        
        # Workspace operations do not keep Dx, so scale frame 1 before putting
        # in the Q resolution
        # Scale frame 1 to frame 2
        if self._scale:
            iq_f1 = mtd[workspace+'_frame1'+self._suffix].dataY(0)
            iq_f2 = mtd[workspace+'_frame2'+self._suffix].dataY(0)
            
            scale_f1 = 0.0
            scale_f2 = 0.0
            scale_factor = 1.0
            for i in range(len(iq_f1)):
                if iq_f1[i]>0 and iq_f2[i]>0:
                    scale_f1 += iq_f1[i]
                    scale_f2 += iq_f2[i]
            if scale_f1>0 and scale_f2>0:
                scale_factor = scale_f2/scale_f1
                scale_f1 = 0.0
                scale_f2 = 0.0
                for i in range(len(iq_f1)):
                    if iq_f1[i]>0 and iq_f2[i]>0 \
                    and scale_factor*iq_f1[i]/iq_f2[i]<self._tolerance \
                    and iq_f2[i]/(scale_factor*iq_f1[i])<self._tolerance:
                        scale_f1 += iq_f1[i]
                        scale_f2 += iq_f2[i]
                if scale_f1>0 and scale_f2>0:
                    scale_factor = scale_f2/scale_f1
            
            Scale(InputWorkspace=workspace+'_frame1'+self._suffix, OutputWorkspace=workspace+'_frame1'+self._suffix, Factor=scale_factor, Operation="Multiply")
            
        if self._compute_resolution:
            EQSANSResolution(InputWorkspace=workspace+'_frame1'+self._suffix, 
                             ReducedWorkspace=workspace, OutputBinning=self._binning,
                             MinWavelength=wl_min_f1, MaxWavelength=wl_max_f1,
                             PixelSizeX=pixel_size_x, PixelSizeY=pixel_size_y,
                             SourceApertureRadius=source_aperture_radius,
                             SampleApertureRadius=self._sample_aperture_radius)       
                    
        # Add output workspaces to the list of important output workspaces
        for item in self.get_output_workspace(workspace):
            if item in reducer.output_workspaces:
                reducer.output_workspaces.remove(item)
        reducer.output_workspaces.append(self.get_output_workspace(workspace))                

        # Clean up 
        for ws in [workspace+'_frame1', workspace+'_frame2']:
            if mtd.workspaceExists(ws):
                mtd.deleteWorkspace(ws)
        
        return "Performed radial averaging for two frames"
        
    def get_data(self, workspace):
        if not self._is_frame_skipping:
            return super(AzimuthalAverageByFrame, self).get_data(workspace)
        
        class DataSet(object):
            x=[]
            y=[]
            dy=[]
        
        d = DataSet()
        d.x = mtd[self.get_output_workspace(workspace)[0]].dataX(0)[1:]
        d.y = mtd[self.get_output_workspace(workspace)[0]].dataY(0)
        d.dx = mtd[self.get_output_workspace(workspace)[0]].dataE(0)
        return d
        
class DirectBeamTransmission(SingleFrameDirectBeamTransmission):
    """
        Calculate transmission using the direct beam method
    """
    def __init__(self, sample_file, empty_file, beam_radius=3.0, theta_dependent=True, dark_current=None, 
                 use_sample_dc=False, combine_frames=True):
        super(DirectBeamTransmission, self).__init__(sample_file, empty_file, beam_radius, theta_dependent, 
                                                     dark_current, use_sample_dc)
        ## Whether of not we need to combine the two frames in frame-skipping mode when performing the fit
        self._combine_frames = combine_frames
        
    def set_combine_frames(self, combine_frames=True):
        self._combine_frames = combine_frames
        
    def get_transmission(self):
        #mtd.sendLogMessage("TOF SANS doesn't have a single zero-angle transmission. DirectBeamTransmission.get_transmission() should not be called.")
        return [0, 0]
        
    def execute(self, reducer, workspace):
        if self._combine_frames or \
            (mtd[workspace].getRun().hasProperty("is_frame_skipping") \
             and mtd[workspace].getRun().getProperty("is_frame_skipping").value==0):
            return super(DirectBeamTransmission, self).execute(reducer, workspace)
    
        output_str = ""
        if self._transmission_ws is None:
            self._transmission_ws = "transmission_fit_"+workspace
            # Load data files
            sample_mon_ws, empty_mon_ws, first_det, output_str = self._load_monitors(reducer, workspace)
            
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
                
                Rebin(workspace, workspace+suffix, "%4.1f,%4.1f,%4.1f" % (wl_min, 0.1, wl_max), False)
                Rebin(sample_mon_ws, sample_mon_ws+suffix, "%4.1f,%4.1f,%4.1f" % (wl_min, 0.1, wl_max), False)
                Rebin(empty_mon_ws, empty_mon_ws+suffix, "%4.1f,%4.1f,%4.1f" % (wl_min, 0.1, wl_max), False)
                self._calculate_transmission(sample_mon_ws+suffix, empty_mon_ws+suffix, first_det, self._transmission_ws+suffix)
                RebinToWorkspace(self._transmission_ws+suffix, workspace, OutputWorkspace=self._transmission_ws+suffix)
                RebinToWorkspace(self._transmission_ws+suffix+'_unfitted', workspace, OutputWorkspace=self._transmission_ws+suffix+'_unfitted')
                return self._transmission_ws+suffix
                
            # First frame
            trans_frame_1 = _crop_and_compute("wavelength_min", "wavelength_max", "_frame1")
            
            # Second frame
            trans_frame_2 = _crop_and_compute("wavelength_min_frame2", "wavelength_max_frame2", "_frame2")
            
            Plus(trans_frame_1, trans_frame_2, self._transmission_ws)
            Plus(trans_frame_1+'_unfitted', trans_frame_2+'_unfitted', self._transmission_ws+'_unfitted')

            # Clean up            
            for ws in [trans_frame_1, trans_frame_2, 
                       trans_frame_1+'_unfitted', trans_frame_2+'_unfitted',
                       sample_mon_ws, empty_mon_ws]:
                if mtd.workspaceExists(ws):
                    mtd.deleteWorkspace(ws)
            
        # Add output workspace to the list of important output workspaces
        reducer.output_workspaces.append([self._transmission_ws, self._transmission_ws+'_unfitted'])
            
        # 2- Apply correction (Note: Apply2DTransCorr)
        #Apply angle-dependent transmission correction using the zero-angle transmission
        if mtd[workspace].getRun().hasProperty("is_separate_corrections"):
            wl_adj = mtd[workspace].getRun().getProperty("wl_adj").value
            Divide(wl_adj, self._transmission_ws, wl_adj)  
        else:
            if self._theta_dependent:
                ApplyTransmissionCorrection(InputWorkspace=workspace, 
                                            TransmissionWorkspace=self._transmission_ws, 
                                            OutputWorkspace=workspace)          
            else:
                Divide(workspace, self._transmission_ws, workspace)  
        
        return "Transmission correction applied for both frame independently [%s]\n%s\n" % (self._transmission_ws, output_str)
    
class SaveIqAscii(BaseSaveIqAscii):
    """
        Save the reduced data to ASCII files
    """
    def __init__(self, process=None):
        super(SaveIqAscii, self).__init__(process)
        
    def execute(self, reducer, workspace):
        log_text = super(SaveIqAscii, self).execute(reducer, workspace)
        
        # Determine which directory to use
        output_dir = reducer._data_path
        if reducer._output_path is not None:
            if os.path.isdir(reducer._output_path):
                output_dir = reducer._output_path
            else:
                raise RuntimeError, "SaveIqAscii could not save in the following directory: %s" % reducer._output_path
            
        if reducer._two_dim_calculator is not None:
            if not hasattr(reducer._two_dim_calculator, "get_output_workspace"):
                
                for output_ws in [workspace+'_Iqxy', workspace+'_frame1_Iqxy', workspace+'_frame2_Iqxy']:
                    if mtd.workspaceExists(output_ws):
                        filename = os.path.join(output_dir, output_ws+'.dat')
                        SaveNISTDAT(InputWorkspace=output_ws, Filename=filename)
                        
                        if len(log_text)>0:
                            log_text += '\n'
                        log_text += "I(Qx,Qy) for %s saved in %s" % (output_ws, filename)
            
        return log_text
            
class SensitivityCorrection(BaseSensitivityCorrection):
    """
        Perform sensitivity correction as a function of wavelength
    """
    def __init__(self, flood_data, min_sensitivity=0.5, max_sensitivity=1.5, dark_current=None, 
                 beam_center=None, use_sample_dc=False):
        super(SensitivityCorrection, self).__init__(flood_data=flood_data, 
                                                    min_sensitivity=min_sensitivity, max_sensitivity=max_sensitivity, 
                                                    dark_current=dark_current, beam_center=beam_center, use_sample_dc=use_sample_dc)
    
    def execute(self, reducer, workspace):
        # Perform standard sensitivity correction
        # If the sensitivity correction workspace exists, just apply it. Otherwise create it.      
        output_str = "   Using data set: %s" % extract_workspace_name(self._flood_data)
        if self._efficiency_ws is None:
            self._compute_efficiency(reducer, workspace)
            
        if mtd[workspace].getRun().hasProperty("is_separate_corrections"):
            wl_adj = mtd[workspace].getRun().getProperty("wl_adj").value
            pixel_adj = mtd[workspace].getRun().getProperty("pixel_adj").value
            Divide(pixel_adj, self._efficiency_ws, pixel_adj)
            xvec = mtd[wl_adj].dataX(0)
            yvec = mtd[wl_adj].dataY(0)
            evec = mtd[wl_adj].dataE(0)
            for i in range(mtd[wl_adj].getNumberBins()):
                wl = (xvec[i+1]+xvec[i])/2.0
                yvec[i] /= 1.0-math.exp(-0.105*wl)
                evec[i] /= 1.0-math.exp(-0.105*wl)            
        else:
            # Modify for wavelength dependency of the efficiency of the detector tubes
            EQSANSSensitivityCorrection(InputWorkspace=workspace, EfficiencyWorkspace=self._efficiency_ws,     
                                        Factor=0.95661, Error=0.005, OutputWorkspace=workspace,
                                        OutputEfficiencyWorkspace="__wl_efficiency")
        
        # Copy over the efficiency's masked pixels to the reduced workspace
        masked_detectors = GetMaskedDetectors(self._efficiency_ws)
        MaskDetectors(workspace, None, masked_detectors.getPropertyValue("DetectorList"))        
        
        return "Wavelength-dependent sensitivity correction applied\n%s\n" % output_str
        