"""
    Implementation of reduction steps for SANS
"""
import os
import sys
import math
from reduction import ReductionStep
from reduction import extract_workspace_name
from reduction import validate_step

# Mantid imports
from mantidsimple import *
    
    
class BaseBeamFinder(ReductionStep):
    """
        Base beam finder. Holds the position of the beam center
        and the algorithm for calculates it using the beam's
        displacement under gravity
    """
    def __init__(self, beam_center_x=None, beam_center_y=None):
        """
            Initial beam center is given in pixel coordinates
            @param beam_center_x: pixel position of the beam in x
            @param beam_center_y: pixel position of the beam in y
        """
        super(BaseBeamFinder, self).__init__()
        self._beam_center_x = beam_center_x
        self._beam_center_y = beam_center_y
        self._beam_radius = None
        
        # Define detector edges to be be masked
        self._x_mask_low = 0
        self._x_mask_high = 0
        self._y_mask_low = 0
        self._y_mask_high = 0 
        
    def set_masked_edges(self, x_low=0, x_high=0, y_low=0, y_high=0):
        """
            Sets the number of pixels to mask on the edges before
            doing the center of mass computation
        """
        self._x_mask_low = x_low
        self._x_mask_high = x_high
        self._y_mask_low = y_low
        self._y_mask_high = y_high
        return self
        
    def get_beam_center(self):
        """
            Returns the beam center
        """
        return [self._beam_center_x, self._beam_center_y]
    
    def execute(self, reducer, workspace=None):
        return "Beam Center set at: %s %s" % (str(self._beam_center_x), str(self._beam_center_y))
        
    def _find_beam(self, direct_beam, reducer, workspace=None):
        """
            Find the beam center.
            @param reducer: Reducer object for which this step is executed
        """
        # Load the file to extract the beam center from, and process it.
        filepath = reducer._full_file_path(self._datafile)
        
        # Check whether that file was already meant to be processed
        workspace_default = "beam_center_"+extract_workspace_name(filepath)
        workspace = workspace_default
        if filepath in reducer._data_files.values():
            for k in reducer._data_files.iterkeys():
                if reducer._data_files[k]==filepath:
                    workspace = k                    
                   
        reducer._data_loader.__class__(datafile=filepath).execute(reducer, workspace)

        # Integrate over all wavelength bins so that we process a single detector image
        Integration(workspace, workspace+'_int')

        # Mask edges of the detector
        mantid.sendLogMessage("Masking beam data: %g %g %g %g" % (self._x_mask_low, self._x_mask_high, self._y_mask_low, self._y_mask_high))  
        mask = Mask()
        mask.mask_edges(self._x_mask_low, self._x_mask_high, self._y_mask_low, self._y_mask_high)
        mask.execute(reducer, workspace+'_int')
                        
        # NOTE: Version 1 of this algorithm computer the center in pixel coordinates (as in the HFIR IGOR code)  
        #
        # beam_center = FindCenterOfMassPosition(workspace+'_int',
        #                                       Output = None,
        #                                       NPixelX=reducer.instrument.nx_pixels,
        #                                       NPixelY=reducer.instrument.ny_pixels,
        #                                       DirectBeam = direct_beam,
        #                                       BeamRadius = self._beam_radius)
        
        # We must convert the beam radius from pixels to meters
        if self._beam_radius is not None:
            self._beam_radius *= reducer.instrument.pixel_size_x/1000.0
        beam_center = FindCenterOfMassPosition(workspace+'_int',
                                               Output = None,
                                               DirectBeam = direct_beam,
                                               BeamRadius = self._beam_radius)
        ctr_str = beam_center.getPropertyValue("CenterOfMass")
        ctr = ctr_str.split(',')
        mantid.sendLogMessage("Beam coordinate in real-space: %s" % str(ctr))  
        
        # Compute the relative distance to the current beam center in pixels
        if self._beam_center_x is None or self._beam_center_y is None:
            [self._beam_center_x, self._beam_center_y] = reducer.instrument.get_default_beam_center()
            
        # Move detector array to correct position. Do it here so that we don't need to
        # move it if we need to load that data set for analysis later.
        # Note: the position of the detector in Z is now part of the load
        # Note: if the relative translation is correct, we shouldn't have to keep 
        #       the condition below. Check with EQSANS data (where the beam center and the data are the same workspace)
        if not workspace == workspace_default:
            old_ctr = reducer.instrument.get_coordinate_from_pixel(self._beam_center_x, self._beam_center_y)
            MoveInstrumentComponent(workspace, reducer.instrument.detector_ID, 
                                    X = old_ctr[0]-float(ctr[0]),
                                    Y = old_ctr[1]-float(ctr[1]),
                                    RelativePosition="1")        

        [self._beam_center_x, self._beam_center_y] = reducer.instrument.get_pixel_from_coordinate(float(ctr[0]), float(ctr[1]))
        
        return "Beam Center found at: %g %g" % (self._beam_center_x, self._beam_center_y)


class ScatteringBeamCenter(BaseBeamFinder):
    """
        Find the beam center using the scattering data
    """  
    def __init__(self, datafile, beam_radius=3):
        """
            @param datafile: beam center data file
            @param beam_radius: beam radius in pixels
        """
        super(ScatteringBeamCenter, self).__init__()
        ## Location of the data file used to find the beam center
        self._datafile = datafile
        ## Beam radius in pixels
        self._beam_radius = beam_radius
        
    def execute(self, reducer, workspace=None):
        """
            Find the beam center.
            @param reducer: Reducer object for which this step is executed
        """
        return super(ScatteringBeamCenter, self)._find_beam(False, reducer, workspace)

class DirectBeamCenter(BaseBeamFinder):
    """
        Find the beam center using the direct beam
    """  
    def __init__(self, datafile):
        super(DirectBeamCenter, self).__init__()
        ## Location of the data file used to find the beam center
        self._datafile = datafile
        
    def execute(self, reducer, workspace=None):
        """
            Find the beam center.
            @param reducer: Reducer object for which this step is executed
        """
        return super(DirectBeamCenter, self)._find_beam(True, reducer, workspace)

class BaseTransmission(ReductionStep):
    """
        Base transmission. Holds the transmission value
        as well as the algorithm for calculating it.
        TODO: ISIS doesn't use ApplyTransmissionCorrection, perhaps it's in Q1D, can we remove it from here?
    """
    def __init__(self, trans=0.0, error=0.0, theta_dependent=True):
        super(BaseTransmission, self).__init__()
        self._trans = float(trans)
        self._error = float(error)
        self._theta_dependent = theta_dependent
        self._dark_current_data = None
        
    def set_theta_dependence(self, theta_dependence=True):
        """
            Set the flag for whether or not we want the full theta-dependence
            included in the correction. Setting this flag to false will result
            in simply dividing by the zero-angle transmission.
            @param theta_dependence: theta-dependence included if True
        """
        self._theta_dependent = theta_dependence
        
    def set_dark_current(self, dark_current=None):
        """
            Set the dark current data file to be subtracted from each tranmission data file
            @param dark_current: path to dark current data file
        """
        self._dark_current_data = dark_current
                
    def get_transmission(self):
        return [self._trans, self._error]
    
    def execute(self, reducer, workspace):
        if self._theta_dependent:
            ApplyTransmissionCorrection(InputWorkspace=workspace, 
                                        TransmissionValue=self._trans,
                                        TransmissionError=self._error, 
                                        OutputWorkspace=workspace) 
        else:
            CreateSingleValuedWorkspace("transmission", self._trans, self._error)
            Divide(workspace, "transmission", workspace)
        
        return "Transmission correction applied for T = %g +- %g" % (self._trans, self._error)
  
class BeamSpreaderTransmission(BaseTransmission):
    """
        Calculate transmission using the beam-spreader method
    """
    def __init__(self, sample_spreader, direct_spreader,
                       sample_scattering, direct_scattering,
                       spreader_transmission=1.0, spreader_transmission_err=0.0,
                       theta_dependent=True, dark_current=None): 
        super(BeamSpreaderTransmission, self).__init__(theta_dependent=theta_dependent)
        self._sample_spreader = sample_spreader
        self._direct_spreader = direct_spreader
        self._sample_scattering = sample_scattering
        self._direct_scattering = direct_scattering
        self._spreader_transmission = spreader_transmission
        self._spreader_transmission_err = spreader_transmission_err
        self._dark_current_data = dark_current
        ## Transmission workspace (output of transmission calculation)
        self._transmission_ws = None
        
    def execute(self, reducer, workspace=None):
        """
            Calculate transmission and apply correction
            @param reducer: Reducer object for which this step is executed
            @param workspace: workspace to apply correction to
        """
        if self._transmission_ws is None:
            # 1- Compute zero-angle transmission correction (Note: CalcTransCoef)
            self._transmission_ws = "transmission_fit_"+workspace
            
            sample_spreader_ws = "_trans_sample_spreader"
            filepath = reducer._full_file_path(self._sample_spreader)
            Load(filepath, sample_spreader_ws)
            
            direct_spreader_ws = "_trans_direct_spreader"
            filepath = reducer._full_file_path(self._direct_spreader)
            Load(filepath, direct_spreader_ws)
            
            sample_scatt_ws = "_trans_sample_scatt"
            filepath = reducer._full_file_path(self._sample_scattering)
            Load(filepath, sample_scatt_ws)
            
            direct_scatt_ws = "_trans_direct_scatt"
            filepath = reducer._full_file_path(self._direct_scattering)
            Load(filepath, direct_scatt_ws)
            
            # Subtract dark current
            if self._dark_current_data is not None and len(str(self._dark_current_data).strip())>0:
                dc = SubtractDarkCurrent(self._dark_current_data)
                dc.execute(reducer, sample_spreader_ws)
                dc.execute(reducer, direct_spreader_ws)
                dc.execute(reducer, sample_scatt_ws)
                dc.execute(reducer, direct_scatt_ws)
                        
            # Get normalization for transmission calculation
            norm_spectrum = reducer.NORMALIZATION_TIME
            if reducer._normalizer is not None:
                norm_spectrum = reducer._normalizer.get_normalization_spectrum()
            
            # Calculate transmission. Use the reduction method's normalization channel (time or beam monitor)
            # as the monitor channel.
            CalculateTransmissionBeamSpreader(SampleSpreaderRunWorkspace=sample_spreader_ws, 
                                              DirectSpreaderRunWorkspace=direct_spreader_ws,
                                              SampleScatterRunWorkspace=sample_scatt_ws, 
                                              DirectScatterRunWorkspace=direct_scatt_ws, 
                                              OutputWorkspace=self._transmission_ws,
                                              SpreaderTransmissionValue=str(self._spreader_transmission), 
                                              SpreaderTransmissionError=str(self._spreader_transmission_err))

        # 2- Apply correction (Note: Apply2DTransCorr)
        #Apply angle-dependent transmission correction using the zero-angle transmission
        if self._theta_dependent:
            ApplyTransmissionCorrection(InputWorkspace=workspace, 
                                        TransmissionWorkspace=self._transmission_ws, 
                                        OutputWorkspace=workspace)          
        else:
            Divide(workspace, self._transmission_ws, workspace)  
        
        trans_ws = mtd[self._transmission_ws]
        self._trans = trans_ws.dataY(0)[0]
        self._error = trans_ws.dataE(0)[0]

        return "Transmission correction applied for T = %g +- %g" % (self._trans, self._error)
          
            
class DirectBeamTransmission(BaseTransmission):
    """
        Calculate transmission using the direct beam method
    """
    def __init__(self, sample_file, empty_file, beam_radius=3.0, theta_dependent=True, dark_current=None):
        super(DirectBeamTransmission, self).__init__(theta_dependent=theta_dependent)
        ## Location of the data files used to calculate transmission
        self._sample_file = sample_file
        self._empty_file = empty_file
        ## Dark current data file
        self._dark_current_data = dark_current
        ## Radius of the beam
        self._beam_radius = beam_radius
        ## Transmission workspace (output of transmission calculation)
        self._transmission_ws = None
        
    def execute(self, reducer, workspace=None):
        """
            Calculate transmission and apply correction
            @param reducer: Reducer object for which this step is executed
            @param workspace: workspace to apply correction to
        """
        if self._transmission_ws is None:
            # 1- Compute zero-angle transmission correction (Note: CalcTransCoef)
            self._transmission_ws = "transmission_fit_"+workspace

            sample_ws = "_transmission_sample"
            filepath = reducer._full_file_path(self._sample_file)
            Load(filepath, sample_ws)
            
            empty_ws = "_transmission_empty"
            filepath = reducer._full_file_path(self._empty_file)
            Load(filepath, empty_ws)
            
            # Subtract dark current
            if self._dark_current_data is not None and len(str(self._dark_current_data).strip())>0:
                dc = SubtractDarkCurrent(self._dark_current_data)
                dc.execute(reducer, sample_ws)
                dc.execute(reducer, empty_ws)        
            
            # Find which pixels to sum up as our "monitor". At this point we have moved the detector
            # so that the beam is at (0,0), so all we need is to sum the area around that point.
            #TODO: in IGOR, the error-weighted average is computed instead of simply summing up the pixels
            cylXML = '<infinite-cylinder id="transmission_monitor">' + \
                       '<centre x="0.0" y="0.0" z="0.0" />' + \
                       '<axis x="0.0" y="0.0" z="1.0" />' + \
                       '<radius val="%12.10f" />' % (self._beam_radius*reducer.instrument.pixel_size_x/1000.0) + \
                     '</infinite-cylinder>\n'
                     
            det_finder = FindDetectorsInShape(Workspace=workspace, ShapeXML=cylXML)
            det_list = det_finder.getPropertyValue("DetectorList")
            
            first_det = int(det_list.split(',')[0])
            
            GroupDetectors(InputWorkspace=empty_ws,  OutputWorkspace="empty_mon",  DetectorList=det_list, KeepUngroupedSpectra="1")
            GroupDetectors(InputWorkspace=sample_ws, OutputWorkspace="sample_mon", DetectorList=det_list, KeepUngroupedSpectra="1")
            
            #TODO: check that both workspaces have the same masked spectra
            
            # Get normalization for transmission calculation
            norm_spectrum = reducer.NORMALIZATION_TIME
            if reducer._normalizer is not None:
                norm_spectrum = reducer._normalizer.get_normalization_spectrum()
            
            # Calculate transmission. Use the reduction method's normalization channel (time or beam monitor)
            # as the monitor channel.
            CalculateTransmission(DirectRunWorkspace="empty_mon", SampleRunWorkspace="sample_mon", 
                                  OutputWorkspace=self._transmission_ws,
                                  IncidentBeamMonitor=str(norm_spectrum), 
                                  TransmissionMonitor=str(first_det))
            
            if mtd.workspaceExists(empty_ws):
                mtd.deleteWorkspace(empty_ws)
            if mtd.workspaceExists(sample_ws):
                mtd.deleteWorkspace(sample_ws)          
            
        # 2- Apply correction (Note: Apply2DTransCorr)
        #Apply angle-dependent transmission correction using the zero-angle transmission
        if self._theta_dependent:
            ApplyTransmissionCorrection(InputWorkspace=workspace, 
                                        TransmissionWorkspace=self._transmission_ws, 
                                        OutputWorkspace=workspace)          
        else:
            Divide(workspace, self._transmission_ws, workspace)  

        trans_ws = mtd[self._transmission_ws]
        self._trans = trans_ws.dataY(0)[0]
        self._error = trans_ws.dataE(0)[0]

        return "Transmission correction applied for T = %g +- %g" % (self._trans, self._error)
            

class SubtractDarkCurrent(ReductionStep):
    """
        ReductionStep class that subtracts the dark current from the data.
        The loaded dark current is stored by the ReductionStep object so that the
        subtraction can be applied to multiple data sets without reloading it.
    """
    def __init__(self, dark_current_file, timer_ws=None):
        """
            @param timer_ws: if provided, will be used to scale the dark current
        """
        super(SubtractDarkCurrent, self).__init__()
        self._timer_ws = timer_ws
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
            filepath = reducer._full_file_path(self._dark_current_file)
            self._dark_current_ws = "_dark_"+extract_workspace_name(filepath)
            Load(filepath, self._dark_current_ws)
            
            # Normalize the dark current data to counting time
            darktimer_ws = self._dark_current_ws+"_timer"
            CropWorkspace(self._dark_current_ws, darktimer_ws,
                          StartWorkspaceIndex = str(reducer.NORMALIZATION_TIME), 
                          EndWorkspaceIndex   = str(reducer.NORMALIZATION_TIME))        
            
            Divide(self._dark_current_ws, darktimer_ws, self._dark_current_ws)      
    
        # If no timer workspace was provided, get the counting time from the data
        timer_ws = self._timer_ws
        if timer_ws is None:
            timer_ws = "_tmp_timer"     
            CropWorkspace(workspace, timer_ws,
                          StartWorkspaceIndex = str(reducer.NORMALIZATION_TIME), 
                          EndWorkspaceIndex   = str(reducer.NORMALIZATION_TIME))  
            
        # Scale the stored dark current by the counting time
        scaled_dark_ws = "_scaled_dark_current"
        Multiply(self._dark_current_ws, timer_ws, scaled_dark_ws)
        
        # Set time and monitor channels to zero, so that we subtract only detectors
        mtd[scaled_dark_ws].dataY(reducer.NORMALIZATION_TIME)[0] = 0
        mtd[scaled_dark_ws].dataE(reducer.NORMALIZATION_TIME)[0] = 0
        mtd[scaled_dark_ws].dataY(reducer.NORMALIZATION_MONITOR)[0] = 0
        mtd[scaled_dark_ws].dataE(reducer.NORMALIZATION_MONITOR)[0] = 0
        
        # Perform subtraction
        Minus(workspace, scaled_dark_ws, workspace)  
        if mtd.workspaceExists(scaled_dark_ws):
            mtd.deleteWorkspace(scaled_dark_ws)
        
        return "Dark current subtracted [%s]" % (scaled_dark_ws)
          
class LoadRun(ReductionStep):
    """
        Load a data file, move its detector to the right position according
        to the beam center and normalize the data.
    """
    #TODO: Move this to HFIR-specific module 
    def __init__(self, datafile=None, sample_det_dist=None, sample_det_offset=0, beam_center=None):
        """
            @param datafile: file path of data to load
            @param sample_det_dist: sample-detector distance [mm] (will overwrite header info)
            @param sample_det_offset: sample-detector distance offset [mm]
            @param beam_center: [center_x, center_y] [pixels]
        """
        super(LoadRun, self).__init__()
        self._data_file = datafile
        self.set_sample_detector_distance(sample_det_dist)
        self.set_sample_detector_offset(sample_det_offset)
        self.set_beam_center(beam_center)
        
    def clone(self, data_file=None):
        if data_file is None:
            data_file = self._data_file
        return LoadRun(datafile=data_file, sample_det_dist=self._sample_det_dist,
                       sample_det_offset=self._sample_det_offset, beam_center=self._beam_center)
        
    def set_sample_detector_distance(self, distance):
        # Check that the distance given is either None of a float
        if distance is not None:
            try:
                float(distance)
            except:
                raise RuntimeError, "LoadRun.set_sample_detector_distance expects a float: %s" % str(distance)
        self._sample_det_dist = distance
        
    def set_sample_detector_offset(self, offset):
        # Check that the offset given is either None of a float
        if offset is not None:
            try:
                float(offset)
            except:
                raise RuntimeError, "LoadRun.set_sample_detector_offset expects a float: %s" % str(offset)
        self._sample_det_offset = offset
        
    def set_beam_center(self, beam_center):
        """
            Sets the beam center to be used when loading the file
            @param beam_center: [pixel_x, pixel_y]
        """
        if beam_center is None:
            self._beam_center = None
        
        # Check that we have pixel numbers (int)            
        elif type(beam_center) == list:
            if len(beam_center) == 2:
                try:
                    int(beam_center[0])
                    int(beam_center[1])
                    self._beam_center = [beam_center[0], beam_center[1]]
                except:
                    raise RuntimeError, "LoadRun.set_beam_center expects a list of two integers\n  %s" % sys.exc_value
            else:
                raise RuntimeError, "LoadRun.set_beam_center expects a list of two integers. Found length %d" % len(beam_center)
            
        else:
            raise RuntimeError, "LoadRun.set_beam_center expects a list of two integers. Found %s" % type(beam_center)
        
    def execute(self, reducer, inputworkspace, outputworkspace=None):
        """
            Loads a data file.
            Note: Files are ALWAYS reloaded when this method is called.
            We do this because speed is not an issue and we ensure that the data
            is always pristine. We could only load files that are not already loaded
            by using the 'dirty' flag and checking for the existence of the workspace.
        """
        if outputworkspace is not None:
            workspace = outputworkspace 
        else:
            workspace = inputworkspace
        # If we don't have a data file, look up the workspace handle
        if self._data_file is None:
            if workspace in reducer._data_files:
                data_file = reducer._data_files[workspace]
            elif workspace in reducer._extra_files:
                data_file = reducer._extra_files[workspace]
            else:
                raise RuntimeError, "SANSReductionSteps.LoadRun doesn't recognize workspace handle %s" % workspace
        else:
            data_file = self._data_file
        
        def _load_data_file(file_name, wks_name):
            filepath = reducer._full_file_path(file_name)
            if reducer.instrument.wavelength is None:
                LoadSpice2D(filepath, wks_name)
            else:
                LoadSpice2D(filepath, wks_name, 
                            Wavelength=reducer.instrument.wavelength,
                            WavelengthSpread=reducer.instrument.wavelength_spread)

        # Check whether we have a list of files that need merging
        if type(data_file)==list:
            for i in range(len(data_file)):
                if i==0:
                    _load_data_file(data_file[i], workspace)
                else:
                    _load_data_file(data_file[i], 'tmp_wksp')
                    Plus(LHSWorkspace=workspace,
                         RHSWorkspace='tmp_wksp',
                         OutputWorkspace=workspace)
            if mtd.workspaceExists('tmp_wksp'):
                mtd.deleteWorkspace('tmp_wksp')
        else:
            _load_data_file(data_file, workspace)
        
        # Get the original sample-detector distance from the data file
        sdd = mtd[workspace].getRun().getProperty("sample-detector-distance").value
        
        if self._sample_det_dist is not None:
            MoveInstrumentComponent(workspace, reducer.instrument.detector_ID,
                                    Z = (self._sample_det_dist-sdd)/1000.0, 
                                    RelativePosition="1")
            sdd = self._sample_det_dist            
        elif not self._sample_det_offset == 0:
            MoveInstrumentComponent(workspace, reducer.instrument.detector_ID,
                                    Z = self._sample_det_offset/1000.0, 
                                    RelativePosition="1")
            sdd += self._sample_det_offset

        # Store the sample-detector distance.
        mantid[workspace].getRun().addProperty_dbl("sample_detector_distance", sdd, True)
            
        # Move detector array to correct position
        # Note: the position of the detector in Z is now part of the load
        if self._beam_center is not None:            
            [pixel_ctr_x, pixel_ctr_y] = self._beam_center
        else:
            [pixel_ctr_x, pixel_ctr_y] = reducer.get_beam_center()
        if pixel_ctr_x is not None and pixel_ctr_y is not None:
            [beam_ctr_x, beam_ctr_y] = reducer.instrument.get_coordinate_from_pixel(pixel_ctr_x, pixel_ctr_y)
            [default_pixel_x, default_pixel_y] = reducer.instrument.get_default_beam_center()
            [default_x, default_y] = reducer.instrument.get_coordinate_from_pixel(default_pixel_x, default_pixel_y)
            MoveInstrumentComponent(workspace, reducer.instrument.detector_ID, 
                                    X = default_x-beam_ctr_x,
                                    Y = default_y-beam_ctr_y,
                                    RelativePosition="1")
        else:
            mantid.sendLogMessage("Beam center isn't defined: skipping beam center alignment for %s" % workspace)
        
        n_files = 1
        if type(data_file)==list:
            n_files = len(data_file)
        return "Data set loaded: %s [%g file(s)]" % (workspace, n_files)
    
class Normalize(ReductionStep):
    """
        Normalize the data to timer or a spectrum, typically a monitor, 
        with in the workspace. By default the normalization is done with 
        respect to the Instrument's incident monitor
        TODO: Move the HFIR-specific code to its own module
    """
    def __init__(self, normalization_spectrum=None):
        super(Normalize, self).__init__()
        self._normalization_spectrum = normalization_spectrum
        
    def get_normalization_spectrum(self):
        return self._normalization_spectrum
        
    def execute(self, reducer, workspace):
        if self._normalization_spectrum is None:
            self._normalization_spectrum = reducer.instrument.get_incident_mon()
        
        # Get counting time or monitor
        norm_ws = workspace+"_normalization"

        CropWorkspace(workspace, norm_ws,
                      StartWorkspaceIndex = self._normalization_spectrum, 
                      EndWorkspaceIndex   = self._normalization_spectrum)      

        Divide(workspace, norm_ws, workspace)
        
        # HFIR-specific: If we count for monitor we need to multiply by 1e8
        if self._normalization_spectrum == reducer.NORMALIZATION_MONITOR:         
            Scale(workspace, workspace, 1.0e8, 'Multiply')
            
        if reducer._absolute_scale is not None:
            scale,_ = reducer._absolute_scale.get_scaling_factor()
            Scale(workspace, workspace, scale, 'Multiply')
                        
    def clean(self):
        mtd.deleteWorkspace(norm_ws)
            
class WeightedAzimuthalAverage(ReductionStep):
    """
        ReductionStep class that performs azimuthal averaging
        and transforms the 2D reduced data set into I(Q).
    """
    def __init__(self, binning=None, suffix="_Iq", error_weighting=False, n_bins=100, n_subpix=1, log_binning=False):
        super(WeightedAzimuthalAverage, self).__init__()
        self._binning = binning
        self._suffix = suffix
        self._error_weighting = error_weighting
        self._nbins = n_bins
        self._nsubpix = n_subpix
        self._log_binning = log_binning
        
    def execute(self, reducer, workspace):
        # Q range                        
        beam_ctr = reducer._beam_finder.get_beam_center()
        if beam_ctr[0] is None or beam_ctr[1] is None:
            raise RuntimeError, "Azimuthal averaging could not proceed: beam center not set"
        if self._binning is None:
            # Wavelength. Read in the wavelength bins. Skip the first one which is not set up properly for EQ-SANS
            x = mtd[workspace].dataX(1)
            x_length = len(x)
            if x_length < 2:
                raise RuntimeError, "Azimuthal averaging expects at least one wavelength bin"
            wavelength_max = (x[x_length-2]+x[x_length-1])/2.0
            wavelength_min = (x[0]+x[1])/2.0
            if wavelength_min==0 or wavelength_max==0:
                raise RuntimeError, "Azimuthal averaging needs positive wavelengths"
                    
            sample_detector_distance = mtd[workspace].getRun().getProperty("sample_detector_distance").value
            # Q min is one pixel from the center, unless we have the beam trap size
            try:
                mindist = mtd[workspace].getRun().getProperty("beam-trap-radius").value
            except:
                mindist = min(reducer.instrument.pixel_size_x, reducer.instrument.pixel_size_y)
            qmin = 4*math.pi/wavelength_max*math.sin(0.5*math.atan(mindist/sample_detector_distance))
            dxmax = reducer.instrument.pixel_size_x*max(beam_ctr[0],reducer.instrument.nx_pixels-beam_ctr[0])
            dymax = reducer.instrument.pixel_size_y*max(beam_ctr[1],reducer.instrument.ny_pixels-beam_ctr[1])
            maxdist = math.sqrt(dxmax*dxmax+dymax*dymax)
            qmax = 4*math.pi/wavelength_min*math.sin(0.5*math.atan(maxdist/sample_detector_distance))
            
            if not self._log_binning:
                qstep = (qmax-qmin)/self._nbins
                f_step = (qmax-qmin)/qstep
                n_step = math.floor(f_step)
                if f_step-n_step>10e-10:
                    qmax = qmin+qstep*n_step
                self._binning = "%g, %g, %g" % (qmin, qstep, qmax)
            else:
                # Note: the log binning in Mantid is x_i+1 = x_i * ( 1 + dx )
                qstep = (math.log10(qmax)-math.log10(qmin))/self._nbins
                f_step = (math.log10(qmax)-math.log10(qmin))/qstep
                n_step = math.floor(f_step)
                if f_step-n_step>10e-10:
                    qmax = math.pow(10.0, math.log10(qmin)+qstep*n_step)
                self._binning = "%g, %g, %g" % (qmin, -(math.pow(10.0,qstep)-1.0), qmax)
        else:
            toks = self._binning.split(',')
            if len(toks)<3:
                raise RuntimeError, "Invalid binning provided: %s" % str(self._binning)
            qmin = float(toks[0])
            qmax = float(toks[2])
            
        output_ws = workspace+str(self._suffix)    
        Q1DWeighted(workspace, output_ws, self._binning,
                    NPixelDivision=self._nsubpix,
                    PixelSizeX=reducer.instrument.pixel_size_x,
                    PixelSizeY=reducer.instrument.pixel_size_y, ErrorWeighting=self._error_weighting)  
        ReplaceSpecialValues(output_ws, output_ws, NaNValue=0.0, NaNError=0.0, InfinityValue=0.0, InfinityError=0.0)
        return "Performed radial averaging between Q=%g and Q=%g" % (qmin, qmax)
        
    def get_output_workspace(self, workspace):
        return workspace+str(self._suffix)
    
    def get_data(self, workspace):
        class DataSet(object):
            x=[]
            y=[]
            dy=[]
        
        d = DataSet()
        d.x = mtd[self.get_output_workspace(workspace)].dataX(0)[1:]
        d.y = mtd[self.get_output_workspace(workspace)].dataY(0)
        d.dx = mtd[self.get_output_workspace(workspace)].dataE(0)
        return d
            
class SolidAngle(ReductionStep):
    """
        ReductionStep class that performs the solid angle correction.
    """
    def execute(self, reducer, workspace):
        SANSSolidAngleCorrection(workspace, workspace)
        
        return "Solid angle correction applied" 
            
class IQxQy(ReductionStep):
    """
        ReductionStep class that performs I(Qx,Qy) calculation
    """
    def __init__(self, nbins=100, suffix="_Iq"):
        super(IQxQy, self).__init__()
        self._nbins = nbins
        self._suffix = suffix
    
    def get_output_workspace(self, workspace):
        return workspace+str(self._suffix)+'xy'
    
    def execute(self, reducer, workspace):
        beam_ctr = reducer._beam_finder.get_beam_center()
        if beam_ctr[0] is None or beam_ctr[1] is None:
            raise RuntimeError, "I(Qx,Qy) computation could not proceed: beam center not set"
        
        # Wavelength. Read in the wavelength bins. Skip the first one which is not set up properly for EQ-SANS
        x = mtd[workspace].dataX(1)
        x_length = len(x)
        if x_length < 2:
            raise RuntimeError, "I(Qx,Qy) computation expects at least one wavelength bin"
        wavelength_min = (x[0]+x[1])/2.0
        if wavelength_min==0:
            raise RuntimeError, "I(Qx,Qy) computation needs positive wavelengths"
        sample_detector_distance = mtd[workspace].getRun().getProperty("sample_detector_distance").value
        dxmax = reducer.instrument.pixel_size_x*max(beam_ctr[0],reducer.instrument.nx_pixels-beam_ctr[0])
        dymax = reducer.instrument.pixel_size_y*max(beam_ctr[1],reducer.instrument.ny_pixels-beam_ctr[1])
        maxdist = max(dxmax, dymax)
        qmax = 4*math.pi/wavelength_min*math.sin(0.5*math.atan(maxdist/sample_detector_distance))
        
        output_ws = self.get_output_workspace(workspace)
        
        Qxy(InputWorkspace=workspace, OutputWorkspace=output_ws,
            MaxQxy=qmax, DeltaQ=qmax/self._nbins, SolidAngleWeighting=False)
        
        return "Computed I(Qx,Qy)" 
            
class SensitivityCorrection(ReductionStep):
    """
        Compute the sensitivity correction and apply it to the input workspace.
        
        The ReductionStep object stores the sensitivity, so that the object
        be re-used on multiple data sets and the sensitivity will not be
        recalculated.
    """
    def __init__(self, flood_data, min_sensitivity=0.5, max_sensitivity=1.5, dark_current=None, beam_center=None):
        super(SensitivityCorrection, self).__init__()
        self._flood_data = flood_data
        self._dark_current_data = dark_current
        self._efficiency_ws = None
        self._min_sensitivity = min_sensitivity
        self._max_sensitivity = max_sensitivity
        
        # Beam center for flood data
        self._beam_center = beam_center
        
    @validate_step
    def set_beam_center(self, beam_center):
        """
             Set the reduction step that will find the beam center position
             @param beam_center: ReductionStep object
        """
        self._beam_center = beam_center
        
    def get_beam_center(self):
        """
            Returns the beam center found by the beam center finder
        """
        if self._beam_center is not None:
            return self._beam_center.get_beam_center()
        else:
            return None
    
    def execute(self, reducer, workspace):
        # If the sensitivity correction workspace exists, just apply it.
        # Otherwise create it.      
        #TODO: check that the workspaces have the same binning!
        if self._efficiency_ws is None:
            # Load the flood data
            filepath = reducer._full_file_path(self._flood_data)
            flood_ws = "flood_"+extract_workspace_name(filepath)
            
            beam_center = None
            # Find the beam center if we need to
            if self._beam_center is not None:
                self._beam_center.execute(reducer)
                beam_center = self._beam_center.get_beam_center()
                
            loader = reducer._data_loader.clone(data_file=filepath)
            loader.set_beam_center(beam_center)
            loader.execute(reducer, flood_ws)

            # Subtract dark current
            if self._dark_current_data is not None and len(str(self._dark_current_data).strip())>0 \
                and reducer._dark_current_subtracter_class is not None:
                reducer._dark_current_subtracter_class(self._dark_current_data).execute(reducer, flood_ws)
            
            # Correct flood data for solid angle effects (Note: SA_Corr_2DSAS)
            # Note: Moving according to the beam center is done in LoadRun above
            if reducer._solid_angle_correcter is not None:
                reducer._solid_angle_correcter.execute(reducer, flood_ws)
        
            # Create efficiency profile: 
            # Divide each pixel by average signal, and mask high and low pixels.
            CalculateEfficiency(flood_ws, "efficiency", self._min_sensitivity, self._max_sensitivity)
            self._efficiency_ws = "efficiency"
            
        # Divide by detector efficiency
        Divide(workspace, self._efficiency_ws, workspace)
        ReplaceSpecialValues(workspace, workspace, InfinityValue=0.0, InfinityError=0.0)
        
        # Copy over the efficiency's masked pixels to the reduced workspace
        masked_detectors = GetMaskedDetectors(self._efficiency_ws)
        MaskDetectors(workspace, None, masked_detectors.getPropertyValue("DetectorList"))        
    
        return "Sensitivity correction applied [%s]" % (self._efficiency_ws)

class Mask(ReductionStep):
    """
        Marks some spectra so that they are not included in the analysis
    """
    def __init__(self):
        """
            Initalize masking 
        """
        super(Mask, self).__init__()
        self._nx_low = 0
        self._nx_high = 0
        self._ny_low = 0
        self._ny_high = 0
        
        self._xml = []

        #these spectra will be masked by the algorithm MaskDetectors
        self.spec_list = []
        
        # List of pixels to mask
        self.masked_pixels = []
        
    def mask_edges(self, nx_low=0, nx_high=0, ny_low=0, ny_high=0):
        """
            Define a "picture frame" outside of which the spectra from all detectors are to be masked.
            @param nx_low: number of pixels to mask on the lower-x side of the detector
            @param nx_high: number of pixels to mask on the higher-x side of the detector
            @param ny_low: number of pixels to mask on the lower-y side of the detector
            @param ny_high: number of pixels to mask on the higher-y side of the detector        
        """
        self._nx_low = nx_low
        self._nx_high = nx_high
        self._ny_low = ny_low
        self._ny_high = ny_high        
        
    def add_xml_shape(self, complete_xml_element):
        """
            Add an arbitrary shape to region to be masked
            @param complete_xml_element: description of the shape to add
        """
        if not complete_xml_element.startswith('<') :
            raise ValueError('Excepted xml string but found: ' + str(complete_xml_element))
        self._xml.append(complete_xml_element)

    def _infinite_plane(self, id, plane_pt, normal_pt, complement = False):
        """
            Generates xml code for an infinte plane
            @param id: a string to refer to the shape by
            @param plane_pt: a point in the plane
            @param normal_pt: the direction of a normal to the plane
            @param complement: mask in the direction of the normal or away
            @return the xml string
        """
        if complement:
            addition = '#'
        else:
            addition = ''
        return '<infinite-plane id="' + str(id) + '">' + \
            '<point-in-plane x="' + str(plane_pt[0]) + '" y="' + str(plane_pt[1]) + '" z="' + str(plane_pt[2]) + '" />' + \
            '<normal-to-plane x="' + str(normal_pt[0]) + '" y="' + str(normal_pt[1]) + '" z="' + str(normal_pt[2]) + '" />'+ \
            '</infinite-plane>\n'

    def _infinite_cylinder(self, centre, radius, axis, id='shape'):
        """
            Generates xml code for an infintely long cylinder
            @param centre: a tupple for a point on the axis
            @param radius: cylinder radius
            @param axis: cylinder orientation
            @param id: a string to refer to the shape by
            @return the xml string
        """
        return '<infinite-cylinder id="' + str(id) + '">' + \
            '<centre x="' + str(centre[0]) + '" y="' + str(centre[1]) + '" z="' + str(centre[2]) + '" />' + \
            '<axis x="' + str(axis[0]) + '" y="' + str(axis[1]) + '" z="' + str(axis[2]) + '" />' + \
            '<radius val="' + str(radius) + '" /></infinite-cylinder>\n'

    def add_cylinder(self, radius, xcentre, ycentre, ID='shape'):
        '''Mask the inside of a cylinder on the input workspace.'''
        self.add_xml_shape(
            self._infinite_cylinder([xcentre, ycentre, 0.0], radius, [0,0,1], id=ID)+'<algebra val="' + str(ID) + '"/>')
            

    def add_outside_cylinder(self, radius, xcentre = 0.0, ycentre = 0.0, ID='shape'):
        '''Mask out the outside of a cylinder or specified radius'''
        self.add_xml_shape(
            self._infinite_cylinder([xcentre, ycentre, 0.0], radius, [0,0,1], id=ID)+'<algebra val="#' + str(ID) + '"/>')

    def add_pixel_rectangle(self, x_min, x_max, y_min, y_max):
        """
            Mask out a rectangle area defined in pixel coordinates.
            @param x_min: Minimum x to mask
            @param x_max: Maximum x to mask
            @param y_min: Minimum y to mask
            @param y_max: Maximum y to mask
        """
        for ix in range(x_min, x_max+1):
            for iy in range(y_min, y_max+1):
                self.masked_pixels.append([ix, iy])

    def add_detector_list(self, det_list):
        """
            Mask the given detectors
            @param det_list: list of detector IDs
        """
        self.spec_list.extend(det_list) 

    def execute(self, reducer, workspace, instrument=None):
        for shape in self._xml:
            MaskDetectorsInShape(workspace, shape)
        
        if instrument is None:
            instrument = reducer.instrument
        # Get a list of detector pixels to mask
        if self._nx_low != 0 or self._nx_high != 0 or self._ny_low != 0 or self._ny_high != 0:            
            self.masked_pixels.extend(instrument.get_masked_pixels(self._nx_low,
                                                                   self._nx_high,
                                                                   self._ny_low,
                                                                   self._ny_high))

        if len(self.spec_list)>0:
            MaskDetectors(workspace, SpectraList = self.spec_list)
            
        # Mask out internal list of pixels
        if len(self.masked_pixels)>0:
            # Transform the list of pixels into a list of Mantid detector IDs
            masked_detectors = instrument.get_detector_from_pixel(self.masked_pixels)
            # Mask the pixels by passing the list of IDs
            MaskDetectors(workspace, DetectorList = masked_detectors)
            
        masked_detectors = GetMaskedDetectors(workspace)
        mantid.sendLogMessage("Mask check %s: %g masked pixels" % (workspace, len(masked_detectors.getPropertyValue("DetectorList"))))  
            
        return "Mask applied %s: %g masked pixels" % (workspace, len(masked_detectors.getPropertyValue("DetectorList")))

class UnitsConvert(ReductionStep):
    """
        Executes ConvertUnits and then Rebin on the same workspace. If no re-bin limits are
        set for the x-values of the final workspace the range of the first spectrum is used.
    """
    def __init__(self, units, rebin = 'Rebin'):
        super(UnitsConvert, self).__init__()
        self._units = units
        self.wav_low = None
        self.wav_high = None
        self.wav_step = None
        # currently there are two possible re-bin algorithms, the other is InterpolatingRebin
        self.rebin_alg = rebin

    def execute(self, reducer, workspace):
        ConvertUnits(workspace, workspace, self._units)
        
        low_wav = self.wav_low
        high_wav = self.wav_high
        
        if low_wav is None and high_wav is None:
            low_wav = min(mtd[workspace].readX(0))
            high_wav = max(mtd[workspace].readX(0))

         
        rebin_com = self.rebin_alg+'(workspace, workspace, "'+\
            self._get_rebin(low_wav, self.wav_step, high_wav)+'")'
        eval(rebin_com)

    def _get_rebin(self, low, step, high):
        """
            Convert the range limits and step into a form passable to re-bin
            @param low: first number in the Rebin string, the first bin boundary
            @param step: bin width
            @param high: high bin boundary
        """        
        return str(low)+', ' + str(step) + ', ' + str(high)

    def get_rebin(self):
        """
            Get the string that is passed as the "param" property to Rebin
            @return the string that is passed to Rebin
        """
        return self._get_rebin(self.wav_low, self.wav_step, self.wav_high)
    
    def set_rebin(self, w_low = None, w_step = None, w_high = None, override=True):
        """
            Set the parameters that are passed to Rebin
            @param w_low: first number in the Rebin string, the first bin boundary
            @param w_step: bin width
            @param w_high: high bin boundary
        """
        if not w_low is None:
            if self.wav_low is None or override:
                self.wav_low = float(w_low)
        if not w_step is None:
            if self.wav_step is None or override:
                self.wav_step = float(w_step)
        if not w_high is None:
            if self.wav_high is None or override:
                self.wav_high = float(w_high)

    def get_range(self):
        """
            Get the values of the highest and lowest boundaries
            @return low'_'high
        """
        return str(self.wav_low)+'_'+str(self.wav_high)

    def set_range(self, w_low = None, w_high = None):
        """
            Set the highest and lowest bin boundary values
            @param w_low: first number in the Rebin string, the first bin boundary
            @param w_high: high bin boundary
        """
        self.set_rebin(w_low, None, w_high)

    def __str__(self):
        return '    Wavelength range: ' + self.get_rebin()

class CorrectToFileStep(ReductionStep):
    """
        Runs the algorithm CorrectToFile()
    """
    def __init__(self, file='', corr_type='', operation=''):
        """
            Parameters passed to this function are passed to
            the CorrectToFile() algorithm
            @param file: full path of the correction file
            @param corr_type: "deltaE", "TOF", "SpectrumNumber" or any valid setting for CorrectToFile()'s as FirstColumnValue property
            @param operation: set to "Divide" or "Multiply"
        """
        super(CorrectToFileStep, self).__init__()
        self._filename = file
        self._corr_type = corr_type
        self._operation = operation

    def get_filename(self):
        return self._filename

    def set_filename(self, filename):
        self._filename = filename

    def execute(self, reducer, workspace):
        if self._filename:
            CorrectToFile(workspace, self._filename, workspace,
                          self._corr_type, self._operation)
            
            
class SaveIqAscii(ReductionStep):
    """
        Save the reduced data to ASCII files
    """
    def __init__(self):
        super(SaveIqAscii, self).__init__()
        
    def execute(self, reducer, workspace):
        log_text = ""
        if reducer._azimuthal_averager is not None:
            output_ws = reducer._azimuthal_averager.get_output_workspace(workspace)
            filename = os.path.join(reducer._data_path, output_ws+'.txt')
            SaveAscii(Filename=filename, Workspace=output_ws)
            
            log_text = "I(Q) saved in %s" % (filename)
        
        if reducer._two_dim_calculator is not None:
            output_ws = reducer._two_dim_calculator.get_output_workspace(workspace)
            filename = os.path.join(reducer._data_path, output_ws+'.dat')
            SaveNISTDAT(InputWorkspace=output_ws, Filename=filename)
            
            if len(log_text)>0:
                log_text += '\n'
            log_text += "I(Qx,Qy) saved in %s" % (filename)
            
        return log_text
            
class SubtractBackground(ReductionStep):
    """
        Subtracts background from a sample data workspace.
        The processed background workspace is stored for later use.
    """
    def __init__(self, background_file, transmission=None):
        """
            @param background_file: file name of background data set
            @param transmission: ReductionStep object to compute the background transmission
        """
        super(SubtractBackground, self).__init__()
        self._background_file = background_file
        self._background_ws = None
        self._transmission = transmission
        
    def set_transmission(self, trans):
        """
             Set the reduction step that will apply the transmission correction
             @param trans: ReductionStep object
        """
        if issubclass(trans.__class__, BaseTransmission) or trans is None:
            self._transmission = trans
        else:
            raise RuntimeError, "SubtractBackground.set_transmission expects an object of class ReductionStep"
        
    def get_transmission(self):
        if self._transmission is not None:
            return self._transmission.get_transmission()
        else:
            return None
        
    def set_trans_theta_dependence(self, theta_dependence):
        if self._transmission is not None:
            self._transmission.set_theta_dependence(theta_dependence)
        else:
            raise RuntimeError, "A transmission algorithm must be selected before setting the theta-dependence of the correction."
            
    def set_trans_dark_current(self, dark_current):
        if self._transmission is not None:
            self._transmission.set_dark_current(dark_current)
        else:
            raise RuntimeError, "A transmission algorithm must be selected before setting its dark current correction."
                    
    def execute(self, reducer, workspace):
        log_text = ''
        if self._background_ws is None:
            # Apply the reduction steps that we normally apply to the data
            self._background_ws = "bck_"+extract_workspace_name(self._background_file)
            reducer._extra_files[self._background_ws] = self._background_file
            for item in reducer._2D_steps():
                output = item.execute(reducer, self._background_ws)
                if output is not None:
                    log_text = log_text + '   ' + str(output) +'\n'
        
            # The transmission correction is set separately
            if self._transmission is not None:
                output = self._transmission.execute(reducer, self._background_ws)
                if output is not None:
                    log_text = log_text + '   ' + str(output) +'\n'
        
        Minus(workspace, self._background_ws, workspace)
        
        return "Background subtracted [%s]\n%s" % (self._background_ws, log_text)
        
 
class GetSampleGeom(ReductionStep):
    """
        Loads, stores, retrieves, etc. data about the geometry of the sample
        On initialisation this class will return default geometry values (compatible with the Colette software)
        There are functions to override these settings
        On execute if there is geometry information in the workspace this will override any unset attributes
    """
    # IDs for each shape as used by the Colette software
    _shape_ids = {1 : 'cylinder-axis-up',
                  2 : 'cuboid',
                  3 : 'cylinder-axis-along'}
    _default_shape = 'cylinder-axis-along'
    
    def __init__(self):
        super(GetSampleGeom, self).__init__()

        # string specifies the sample's shape
        self._shape = None
        self._width = self._thickness = self._height = None

    def _get_default(self, attrib):
        if attrib == 'shape':
            return self._default_shape
        elif attrib == 'width' or attrib == 'thickness' or attrib == 'height':
            return 1.0

    def set_shape(self, new_shape):
        """
            Sets the sample's shape from a string or an ID. If the ID is not
            in the list of allowed values the shape is set to the default but
            shape strings not in the list are not checked
        """
        try:
            # deal with ID numbers as arguments
            new_shape = self._shape_ids[int(new_shape)]
        except ValueError:
            # means that we weren't passed an ID number, the code below treats it as a shape name
            pass
        except KeyError:
            mantid.sendLogMessage("::SANS::Warning: Invalid geometry type for sample: " + str(new_shape) + ". Setting default to " + self._default_shape)
            new_shape = self._default_shape
        self._shape = new_shape
        #check that the dimensions that we have make sense for our new shape
        if not self._width is None:
            self.width = self._width
        if not self._thickness is None:
            self.thickness = self._thickness
    
    def get_shape(self):
        if self._shape is None:
            return self._get_default('shape')
        else:
            return self._shape
        
    def set_width(self, width):
        self._width = float(width)
        # For a disk the height=width
        if (not self._shape is None) and (self._shape.startswith('cylinder')):
            self._height = self._width

    def get_width(self):
        if self._width is None:
            return self._get_default('width')
        else:
            return self._width
            
    def set_height(self, height):
        self._height = float(height)
        
        # For a cylinder and sphere the height=width=radius
        if (not self._shape is None) and (self._shape.startswith('cylinder')):
            self._width = self._height

    def get_height(self):
        if self._height is None:
            return self._get_default('height')
        else:
            return self._height

    def set_thickness(self, thickness):
        """
            Simply sets the variable _thickness to the value passed
        """
        #as only cuboids use the thickness the warning below may be informative
        #if (not self._shape is None) and (not self._shape == 'cuboid'):
        #    mantid.sendLogMessage('::SANS::Warning: Can\'t set thickness for shape "'+self._shape+'"')
        self._thickness = float(thickness)

    def get_thickness(self):
        if self._thickness is None:
            return self._get_default('thickness')
        else:
            return self._thickness

    shape = property(get_shape, set_shape, None, None)
    width = property(get_width, set_width, None, None)
    height = property(get_height, set_height, None, None)
    thickness = property(get_thickness, set_thickness, None, None)

    def set_dimensions_to_unity(self):
        self._width = self._thickness = self._height = 1.0

    def execute(self, reducer, workspace):
        """
            Reads the geometry information stored in the workspace
            but doesn't replace values that have been previously set
        """
        wksp = mtd[workspace] 
        if wksp.isGroup():
            wksp = wksp[0]
        sample_details = wksp.getSampleInfo()

        if self._shape is None:
            self.shape = sample_details.getGeometryFlag()
        if self._thickness is None:
            self.thickness = sample_details.getThickness()
        if self._width is None:
            self.width = sample_details.getWidth()
        if self._height is None:
            self.height = sample_details.getHeight()

    def __str__(self):
        return '-- Sample Geometry --\n' + \
               '    Shape: ' + self.shape+'\n'+\
               '    Width: ' + str(self.width)+'\n'+\
               '    Height: ' + str(self.height)+'\n'+\
               '    Thickness: ' + str(self.thickness)+'\n'

class SampleGeomCor(ReductionStep):
    """
        Correct the neutron count rates for the size of the sample
    """
    def __init__(self, geometry):
        """
            Takes a reference to the sample geometry
            @param geometry: A GetSampleGeom object to load the sample dimensions from
            @raise TypeError: if an object of the wrong type is passed to it
        """
        super(SampleGeomCor, self).__init__()

        if issubclass(geometry.__class__, GetSampleGeom):
            self.geo = geometry
        else:
            raise TypeError, 'Sample geometry correction requires a GetSampleGeom object'

    def execute(self, reducer, workspace):
        """
            Divide the counts by the volume of the sample
        """

        try:
            if self.geo.shape == 'cylinder-axis-up':
                # Volume = circle area * height
                # Factor of four comes from radius = width/2
                volume = self.geo.height*math.pi
                volume *= math.pow(self.geo.width,2)/4.0
            elif self.geo.shape == 'cuboid':
                volume = self.geo.width
                volume *= self.geo.height*self.geo.thickness
            elif self.geo.shape == 'cylinder-axis-along':
                # Factor of four comes from radius = width/2
                volume = self.geo.thickness*math.pi
                volume *= math.pow(self.geo.width, 2)/4.0
            else:
                raise NotImplemented('Shape "'+self.geo.shape+'" is not in the list of supported shapes')
        except TypeError:
            raise TypeError('Error calculating sample volume with width='+str(self._width) + ' height='+str(self._height) + 'and thickness='+str(self._thickness)) 
        
        ws = mtd[workspace]
        ws /= volume

class StripEndZeros(ReductionStep):
    def __init__(self, flag_value = 0.0):
        super(StripEndZeros, self).__init__()
        self._flag_value = flag_value
        
    def execute(self, reducer, workspace):
        result_ws = mantid.getMatrixWorkspace(workspace)
        if result_ws.getNumberHistograms() != 1:
            #Strip zeros is only possible on 1D workspaces
            return

        y_vals = result_ws.readY(0)
        length = len(y_vals)
        # Find the first non-zero value
        start = 0
        for i in range(0, length):
            if ( y_vals[i] != self._flag_value ):
                start = i
                break
        # Now find the last non-zero value
        stop = 0
        length -= 1
        for j in range(length, 0,-1):
            if ( y_vals[j] != self._flag_value ):
                stop = j
                break
        # Find the appropriate X values and call CropWorkspace
        x_vals = result_ws.readX(0)
        startX = x_vals[start]
        # Make sure we're inside the bin that we want to crop
        endX = 1.001*x_vals[stop + 1]
        CropWorkspace(workspace,workspace,startX,endX)
