"""
    Implementation of reduction steps for SANS
"""
import os
import math
from reduction import ReductionStep
from reduction import extract_workspace_name

# Mantid imports
from mantidsimple import *
    
    
class BaseBeamFinder(ReductionStep):
    """
        Base beam finder. Holds the position of the beam center
        and the algorithm for calculates it using the beam's
        displacement under gravity
    """
    def __init__(self, beam_center_x=0.0, beam_center_y=0.0):
        super(BaseBeamFinder, self).__init__()
        self._beam_center_x = beam_center_x
        self._beam_center_y = beam_center_y
        self._beam_radius = None
        
    def get_beam_center(self):
        return [self._beam_center_x, self._beam_center_y]
    
    def execute(self, reducer, workspace=None):
        return "Beam Center set at: %g %g" % (self._beam_center_x, self._beam_center_y)
        
    def _find_beam(self, direct_beam, reducer, workspace=None):
        """
            Find the beam center.
            @param reducer: Reducer object for which this step is executed
        """
        # Load the file to extract the beam center from, and process it.
        filepath = reducer._full_file_path(self._datafile)
        
        # Check whether that file was already meant to be processed
        workspace = "beam_center"
        if filepath in reducer._data_files.values():
            for k in reducer._data_files.iterkeys():
                if reducer._data_files[k]==filepath:
                    workspace = k
                    
        reducer._data_loader.__class__(datafile=filepath).execute(reducer, workspace)
        
        # Integrate over all wavelength bins so that we process a single detector image
        Integration(workspace, workspace+'_int')
        beam_center = FindCenterOfMassPosition(workspace+'_int',
                                               Output = None,
                                               NPixelX=reducer.instrument.nx_pixels,
                                               NPixelY=reducer.instrument.ny_pixels,
                                               DirectBeam = direct_beam,
                                               BeamRadius = self._beam_radius)
        ctr_str = beam_center.getPropertyValue("CenterOfMass")
        ctr = ctr_str.split(',')
        
        self._beam_center_x = float(ctr[0])
        self._beam_center_y = float(ctr[1])
        
        # Move detector array to correct position. Do it here so that we don't need to
        # move it if we need to load that data set for analysis later.
        # Note: the position of the detector in Z is now part of the load
        MoveInstrumentComponent(workspace, reducer.instrument.detector_ID, 
                                X = -(self._beam_center_x-reducer.instrument.nx_pixels/2.0+0.5) * reducer.instrument.pixel_size_x/1000.0, 
                                Y = -(self._beam_center_y-reducer.instrument.ny_pixels/2.0+0.5) * reducer.instrument.pixel_size_y/1000.0, 
                                RelativePosition="1")        
        
        return "Beam Center found at: %g %g" % (self._beam_center_x, self._beam_center_y)


class ScatteringBeamCenter(BaseBeamFinder):
    """
        Find the beam center using the scattering data
    """  
    def __init__(self, datafile, beam_radius=3):
        super(ScatteringBeamCenter, self).__init__()
        ## Location of the data file used to find the beam center
        self._datafile = datafile
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
    def __init__(self, trans=0.0, error=0.0):
        super(BaseTransmission, self).__init__()
        self._trans = float(trans)
        self._error = float(error)
        
    def get_transmission(self):
        return [self._trans, self._error]
    
    def execute(self, reducer, workspace):
        ApplyTransmissionCorrection(InputWorkspace=workspace, 
                                    TransmissionValue=self._trans,
                                    TransmissionError=self._error, 
                                    OutputWorkspace=workspace) 
        
        return "Transmission correction applied for T = %g +- %g" % (self._trans, self._error)
  
class BeamSpreaderTransmission(BaseTransmission):
    """
        Calculate transmission using the beam-spreader method
    """
    def __init__(self, sample_spreader, direct_spreader,
                       sample_scattering, direct_scattering,
                       spreader_transmission=1.0, spreader_transmission_err=0.0): 
        super(BeamSpreaderTransmission, self).__init__()
        self._sample_spreader = sample_spreader
        self._direct_spreader = direct_spreader
        self._sample_scattering = sample_scattering
        self._direct_scattering = direct_scattering
        self._spreader_transmission = spreader_transmission
        self._spreader_transmission_err = spreader_transmission_err
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
            self._transmission_ws = "transmission_fit"
            
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
            if reducer._dark_current_subtracter is not None:
                reducer._dark_current_subtracter.execute(reducer, sample_spreader_ws)
                reducer._dark_current_subtracter.execute(reducer, direct_spreader_ws)
                reducer._dark_current_subtracter.execute(reducer, sample_scatt_ws)
                reducer._dark_current_subtracter.execute(reducer, direct_scatt_ws)
            
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
        ApplyTransmissionCorrection(InputWorkspace=workspace, 
                                    TransmissionWorkspace=self._transmission_ws, 
                                    OutputWorkspace=workspace)                
        
        trans_ws = mtd[self._transmission_ws]
        self._trans = trans_ws.dataY(0)[0]
        self._error = trans_ws.dataE(0)[0]

        return "Transmission correction applied for T = %g +- %g" % (self._trans, self._error)
          
            
class DirectBeamTransmission(BaseTransmission):
    """
        Calculate transmission using the direct beam method
    """
    def __init__(self, sample_file, empty_file, beam_radius=3.0):
        super(DirectBeamTransmission, self).__init__()
        ## Location of the data files used to calculate transmission
        self._sample_file = sample_file
        self._empty_file = empty_file
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
            self._transmission_ws = "transmission_fit"

            sample_ws = "_transmission_sample"
            filepath = reducer._full_file_path(self._sample_file)
            Load(filepath, sample_ws)
            
            empty_ws = "_transmission_empty"
            filepath = reducer._full_file_path(self._empty_file)
            Load(filepath, empty_ws)
            
            # Subtract dark current
            if reducer._dark_current_subtracter is not None:
                reducer._dark_current_subtracter.execute(reducer, sample_ws)
                reducer._dark_current_subtracter.execute(reducer, empty_ws)
            
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
            
        # 2- Apply correction (Note: Apply2DTransCorr)
        #Apply angle-dependent transmission correction using the zero-angle transmission
        ApplyTransmissionCorrection(InputWorkspace=workspace, 
                                    TransmissionWorkspace=self._transmission_ws, 
                                    OutputWorkspace=workspace)            

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
            self._dark_current_ws = extract_workspace_name(filepath)
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
            timer_ws = "tmp_timer"     
            CropWorkspace(workspace, timer_ws,
                          StartWorkspaceIndex = str(reducer.NORMALIZATION_TIME), 
                          EndWorkspaceIndex   = str(reducer.NORMALIZATION_TIME))  
            
        # Scale the stored dark current by the counting time
        scaled_dark_ws = "scaled_dark_current"
        Multiply(self._dark_current_ws, timer_ws, scaled_dark_ws)
        
        # Set time and monitor channels to zero, so that we subtract only detectors
        mtd[scaled_dark_ws].dataY(reducer.NORMALIZATION_TIME)[0] = 0
        mtd[scaled_dark_ws].dataE(reducer.NORMALIZATION_TIME)[0] = 0
        mtd[scaled_dark_ws].dataY(reducer.NORMALIZATION_MONITOR)[0] = 0
        mtd[scaled_dark_ws].dataE(reducer.NORMALIZATION_MONITOR)[0] = 0
        
        # Perform subtraction
        Minus(workspace, scaled_dark_ws, workspace)  
        
        return "Dark current subtracted [%s]" % (scaled_dark_ws)
          
class LoadRun(ReductionStep):
    """
        Load a data file, move its detector to the right position according
        to the beam center and normalize the data.
    """
    #TODO: Move this to HFIR-specific module 
    def __init__(self, datafile=None, sample_det_dist=None, sample_det_offset=0):
        super(LoadRun, self).__init__()
        self._data_file = datafile
        self.set_sample_detector_distance(sample_det_dist)
        self.set_sample_detector_offset(sample_det_offset)
        
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
        
    def execute(self, reducer, workspace):      
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
        
        # Load data
        filepath = reducer._full_file_path(data_file)
        if reducer.instrument.wavelength is None:
            LoadSpice2D(filepath, workspace)
        else:
            LoadSpice2D(filepath, workspace, 
                        Wavelength=reducer.instrument.wavelength,
                        WavelengthSpread=reducer.instrument.wavelength_spread)
        
        # Store the sample-detector distance
        reducer.instrument.sample_detector_distance = mtd[workspace].getInstrument().getNumberParameter("sample-detector-distance")[0]
        
        if self._sample_det_dist is not None:
            original_distance = reducer.instrument.sample_detector_distance
            reducer.instrument.sample_detector_distance = self._sample_det_dist
            MoveInstrumentComponent(workspace, reducer.instrument.detector_ID,
                                    Z = (self._sample_det_dist-original_distance)/1000.0, 
                                    RelativePosition="1")
        elif not self._sample_det_offset == 0:
            reducer.instrument.sample_detector_distance += self._sample_det_offset
            MoveInstrumentComponent(workspace, reducer.instrument.detector_ID,
                                    Z = self._sample_det_offset/1000.0, 
                                    RelativePosition="1")
        
        mantid.sendLogMessage("Loaded %s: sample-detector distance = %g" %(workspace, reducer.instrument.sample_detector_distance))
        
        # Move detector array to correct position
        # Note: the position of the detector in Z is now part of the load
        MoveInstrumentComponent(workspace, reducer.instrument.detector_ID, 
                                X = -(reducer.get_beam_center()[0]-reducer.instrument.nx_pixels/2.0+0.5) * reducer.instrument.pixel_size_x/1000.0, 
                                Y = -(reducer.get_beam_center()[1]-reducer.instrument.ny_pixels/2.0+0.5) * reducer.instrument.pixel_size_y/1000.0, 
                                RelativePosition="1")
        
        return "Data file loaded: %s" % (workspace)
    
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
            
    def clean(self):
        mtd.deleteWorkspace(norm_ws)
            
class WeightedAzimuthalAverage(ReductionStep):
    """
        ReductionStep class that performs azimuthal averaging
        and transforms the 2D reduced data set into I(Q).
    """
    def __init__(self, binning=None, suffix="_Iq", error_weighting=False, n_bins=100, n_subpix=1):
        super(WeightedAzimuthalAverage, self).__init__()
        self._binning = binning
        self._suffix = suffix
        self._error_weighting = error_weighting
        self._nbins = n_bins
        self._nsubpix = n_subpix
        
    def execute(self, reducer, workspace):
        # Q range                        
        beam_ctr = reducer._beam_finder.get_beam_center()
        if self._binning is None:
            # Wavelength. Read in the wavelength bins. Skip the first one which is not set up properly for EQ-SANS
            x = mtd[workspace].dataX(1)
            x_length = len(x)
            if x_length < 2:
                raise RuntimeError, "Azimuthal averaging expects at least one wavelength bin"
            wavelength_max = (x[x_length-2]+x[x_length-1])/2.0
            wavelength_min = (x[0]+x[1])/2.0
                    
            # Q min is one pixel from the center
            mindist = min(reducer.instrument.pixel_size_x, reducer.instrument.pixel_size_y)
            qmin = 4*math.pi/wavelength_max*math.sin(0.5*math.atan(mindist/reducer.instrument.sample_detector_distance))
            dxmax = reducer.instrument.pixel_size_x*max(beam_ctr[0],reducer.instrument.nx_pixels-beam_ctr[0])
            dymax = reducer.instrument.pixel_size_y*max(beam_ctr[1],reducer.instrument.ny_pixels-beam_ctr[1])
            maxdist = math.sqrt(dxmax*dxmax+dymax*dymax)
            qmax = 4*math.pi/wavelength_min*math.sin(0.5*math.atan(maxdist/reducer.instrument.sample_detector_distance))
            qstep = (qmax-qmin)/self._nbins
            
            f_step = (qmax-qmin)/qstep
            n_step = math.floor(f_step)
            if f_step-n_step>10e-10:
                qmax = qmin+qstep*n_step
                
            self._binning = "%g, %g, %g" % (qmin, qstep, qmax)
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
        SolidAngleCorrection(workspace, workspace)
        
        return "Solid angle correction applied" 
            
class SensitivityCorrection(ReductionStep):
    """
        Compute the sensitivity correction and apply it to the input workspace.
        
        The ReductionStep object stores the sensitivity, so that the object
        be re-used on multiple data sets and the sensitivity will not be
        recalculated.
    """
    def __init__(self, flood_data, min_sensitivity=0.5, max_sensitivity=1.5):
        super(SensitivityCorrection, self).__init__()
        self._flood_data = flood_data
        self._efficiency_ws = None
        self._min_sensitivity = min_sensitivity
        self._max_sensitivity = max_sensitivity
        
    def execute(self, reducer, workspace):
        # If the sensitivity correction workspace exists, just apply it.
        # Otherwise create it.      
        #TODO: check that the workspaces have the same binning!
        if self._efficiency_ws is None:
            # Load the flood data
            filepath = reducer._full_file_path(self._flood_data)
            flood_ws = extract_workspace_name(filepath)
            
            reducer._data_loader.__class__(datafile=filepath).execute(reducer, flood_ws)

            # Subtract dark current
            if reducer._dark_current_subtracter is not None:
                reducer._dark_current_subtracter.execute(reducer, flood_ws)
            
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
        if not complete_xml_element.startswith('<') :
            raise ValueError('Excepted xml string but found: ' + str(complete_xml_element))
        self._xml.append(complete_xml_element)

    def _infinite_plane(self, id, plane_pt, normal_pt, complement = False):
        if complement:
            addition = '#'
        else:
            addition = ''
        return '<infinite-plane id="' + str(id) + '">' + \
            '<point-in-plane x="' + str(plane_pt[0]) + '" y="' + str(plane_pt[1]) + '" z="' + str(plane_pt[2]) + '" />' + \
            '<normal-to-plane x="' + str(normal_pt[0]) + '" y="' + str(normal_pt[1]) + '" z="' + str(normal_pt[2]) + '" />'+ \
            '</infinite-plane><algebra val="'+addition+str(id)+'"/>\n'

    def _infinite_cylinder(self, centre, radius, axis, complement=False, id='shape'):
        if complement:
            addition = '#'
        else:
            addition = ''
        return '<infinite-cylinder id="' + str(id) + '">' + \
            '<centre x="' + str(centre[0]) + '" y="' + str(centre[1]) + '" z="' + str(centre[2]) + '" />' + \
            '<axis x="' + str(axis[0]) + '" y="' + str(axis[1]) + '" z="' + str(axis[2]) + '" />' + \
            '<radius val="' + str(radius) + '" />' + \
            '</infinite-cylinder><algebra val="'+addition+str(id)+'"/>\n'

    def add_cylinder(self, radius, xcentre, ycentre, ID='shape'):
        '''Mask the inside of a cylinder on the input workspace.'''
        self.add_xml_shape(
            self._infinite_cylinder([xcentre, ycentre, 0.0], radius, [0,0,1],
            complement=False, id=ID))
            

    def add_outside_cylinder(self, radius, xcentre = 0.0, ycentre = 0.0, ID='shape'):
        '''Mask out the outside of a cylinder or specified radius'''
        self.add_xml_shape(
            self._infinite_cylinder([xcentre, ycentre, 0.0], radius, [0,0,1],
            complement=True, id=ID))

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

    def execute(self, reducer, workspace, instrument=None):
        for shape in self._xml:
            MaskDetectorsInShape(workspace, shape)
        
        if instrument is None:
            instrument = reducer.instrument
        # Get a list of detector pixels to mask
        if self._nx_low != 0 or self._nx_high != 0 or self._ny_low != 0 or self._ny_high != 0:
            masked_pixels = instrument.get_masked_pixels(self._nx_low,
                                                             self._nx_high,
                                                             self._ny_low,
                                                             self._ny_high)
            # Transform the list of pixels into a list of Mantid detector IDs
            masked_detectors = instrument.get_detector_from_pixel(masked_pixels)
            
            # Mask the pixels by passing the list of IDs
            MaskDetectors(workspace, DetectorList = masked_detectors)

        if self.spec_list != '':
            MaskDetectors(workspace, SpectraList = self.spec_list)
            
        # Mask out internal list of pixels
        if len(self.masked_pixels)>0:
            masked_detectors = instrument.get_detector_from_pixel(self.masked_pixels)
            MaskDetectors(workspace, DetectorList = masked_detectors)
            
        return "Mask applied"

    def view_mask(self):
        """
            Display the masked detectors in the bank in a different color
            in instrument view
        """
        ws_name = 'CurrentMask'
        ReductionSingleton().instrument.view(ws_name)
        # Mark up "dead" detectors with error value 
        FindDeadDetectors(ws_name, ws_name, DeadValue=500)

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
    def __init__(self):
        super(SaveIqAscii, self).__init__()
        
    def execute(self, reducer, workspace):
        if reducer._azimuthal_averager is not None:
            output_ws = reducer._azimuthal_averager.get_output_workspace(workspace)
            filename = os.path.join(reducer._data_path, output_ws+'.txt')
            SaveAscii(Filename=filename, Workspace=output_ws)
            
            return "I(q) saved in %s" % (filename)
            
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
        
    def execute(self, reducer, workspace):
        log_text = ''
        if self._background_ws is None:
            # Apply the reduction steps that we normally apply to the data
            self._background_ws = extract_workspace_name(self._background_file)
            reducer._extra_files[self._background_ws] = self._background_file
            for item in reducer._2D_steps():
                output = item.execute(reducer, self._background_ws)
                if output is not None:
                    log_text = log_text + '  ' + str(output) +'\n'
        
            # The transmission correction is set separately
            if self._transmission is not None:
                self._transmission.execute(reducer, self._background_ws) 
        
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
        if (not self._shape is None) and (not self._shape == 'cuboid'):
            mantid.sendLogMessage('::SANS::Warning: Can\'t set thickness for shape "'+self._shape+'"')
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
