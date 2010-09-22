"""
    Implementation of reduction steps for SANS
"""
import os
import math
from Reducer import ReductionStep
from Reducer import extract_workspace_name

# Mantid imports
from mantidsimple import *
    
    
class BaseBeamFinder(ReductionStep):
    """
        Base beam finder. Holds the position of the beam center
        and the algorithm for calculates it using the beam's
        displacement under gravity
        TODO: Maintain HFIR-ISIS compatibility
    """
    def __init__(self, beam_center_x=0.0, beam_center_y=0.0):
        super(BaseBeamFinder, self).__init__()
        self._beam_center_x = beam_center_x
        self._beam_center_y = beam_center_y
        self._beam_radius = None
        
    def get_beam_center(self):
        return [self._beam_center_x, self._beam_center_y]
    
    def execute(self, reducer, workspace=None):
        pass
        
    def _find_beam(self, direct_beam, reducer, workspace=None):
        """
            Find the beam center.
            @param reducer: Reducer object for which this step is executed
        """
        # Load the file to extract the beam center from, and process it.
        filepath = reducer._full_file_path(self._datafile)
        Load(filepath, "beam_center")
        
        beam_center = FindCenterOfMassPosition("beam_center",
                                               Output = None,
                                               NPixelX=reducer.instrument.nx_pixels,
                                               NPixelY=reducer.instrument.ny_pixels,
                                               DirectBeam = direct_beam,
                                               BeamRadius = self._beam_radius)
        ctr_str = beam_center.getPropertyValue("CenterOfMass")
        ctr = ctr_str.split(',')
        
        self._beam_center_x = float(ctr[0])
        self._beam_center_y = float(ctr[1])


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
        super(ScatteringBeamCenter, self)._find_beam(False, reducer, workspace)

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
        super(DirectBeamCenter, self)._find_beam(True, reducer, workspace)

class BaseTransmission(ReductionStep):
    """
        Base transmission. Holds the transmission value
        as well as the algorithm for calculating it.
        TODO: ISIS doesn't use ApplyTransmissionCorrection, perhaps it's in Q1D, can we remove it from here?
    """
    def __init__(self, trans=0.0, error=0.0):
        super(BaseTransmission, self).__init__()
        self._trans = trans
        self._error = error
        
    def get_transmission(self):
        return [self._trans, self._error]
    
    def execute(self, reducer, workspace):
        ApplyTransmissionCorrection(InputWorkspace=workspace, 
                                    TransmissionValue=self._trans,
                                    TransmissionError=self._error, 
                                    OutputWorkspace=workspace) 
        
  
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
                self._data_file = reducer._data_files[workspace]
            else:
                raise RuntimeError, "SANSReductionSteps.LoadRun doesn't recognize workspace handle %s" % workspace
        
        # Load data
        filepath = reducer._full_file_path(self._data_file)
        if reducer.instrument.wavelength is None:
            LoadSpice2D(filepath, workspace)
        else:
            LoadSpice2D(filepath, workspace, 
                        Wavelength=reducer.instrument.wavelength,
                        WavelengthSpread=reducer.instrument.wavelength_spread)
        
        # Store the sample-detector distance
        if self._sample_det_dist is None:
            reducer.instrument.sample_detector_distance = mtd[workspace].getInstrument().getNumberParameter("sample-detector-distance")[0]        
            mantid.sendLogMessage("Loaded %s: sample-detector distance = %g" %(workspace, reducer.instrument.sample_detector_distance))
        else:
            reducer.instrument.sample_detector_distance = self._sample_det_dist
        
        reducer.instrument.sample_detector_distance += self._sample_det_offset
        
        # Move detector array to correct position
        # Note: the position of the detector in Z is now part of the load
        MoveInstrumentComponent(workspace, reducer.instrument.detector_ID, 
                                X = -(reducer.get_beam_center()[0]-reducer.instrument.nx_pixels/2.0+0.5) * reducer.instrument.pixel_size_x/1000.0, 
                                Y = -(reducer.get_beam_center()[1]-reducer.instrument.ny_pixels/2.0+0.5) * reducer.instrument.pixel_size_y/1000.0, 
                                RelativePosition="1")
        
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
                      StartWorkspaceIndex = str(self._normalization_spectrum), 
                      EndWorkspaceIndex   = str(self._normalization_spectrum))      

        Divide(workspace, norm_ws, workspace)
        
        mtd.deleteWorkspace(norm_ws)
        
        # HFIR-specific: If we count for monitor we need to multiply by 1e8
        if self._normalization_spectrum == reducer.NORMALIZATION_MONITOR:         
            Scale(workspace, workspace, 1.0e8, 'Multiply')
            
class WeightedAzimuthalAverage(ReductionStep):
    """
        ReductionStep class that performs azimuthal averaging
        and transforms the 2D reduced data set into I(Q).
    """
    def __init__(self, binning="0.01,0.001,0.11", suffix="_Iq", error_weighting=False):
        super(WeightedAzimuthalAverage, self).__init__()
        self._binning = binning
        self._suffix = suffix
        self._error_weighting = error_weighting
        
    def execute(self, reducer, workspace):
        output_ws = workspace+str(self._suffix)    
        Q1DWeighted(workspace, output_ws, self._binning, 
                    PixelSizeX=reducer.instrument.pixel_size_x,
                    PixelSizeY=reducer.instrument.pixel_size_y, ErrorWeighting=self._error_weighting)  
        
    def get_output_workspace(self, workspace):
        return workspace+str(self._suffix)
            
class SolidAngle(ReductionStep):
    """
        ReductionStep class that performs the solid angle correction.
    """
    def execute(self, reducer, workspace):
        SolidAngleCorrection(workspace, workspace)
            
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
        if self._efficiency_ws is None:
            # Load the flood data
            filepath = reducer._full_file_path(self._flood_data)
            flood_ws = extract_workspace_name(filepath)
            
            Load(filepath, flood_ws)

            # Subtract dark current
            if reducer._dark_current_subtracter is not None:
                reducer._dark_current_subtracter.execute(reducer, flood_ws)
            
            # Correct flood data for solid angle effects (Note: SA_Corr_2DSAS)
            if reducer._solid_angle_correcter is not None:
                # Move detector array to correct position, necessary to apply solid angle correction
                # Note: the position of the detector in Z is now part of the load
                MoveInstrumentComponent(flood_ws, reducer.instrument.detector_ID, 
                                        X = -(reducer.get_beam_center()[0]-reducer.instrument.nx_pixels/2.0+0.5) * reducer.instrument.pixel_size_x/1000.0, 
                                        Y = -(reducer.get_beam_center()[1]-reducer.instrument.ny_pixels/2.0+0.5) * reducer.instrument.pixel_size_y/1000.0, 
                                        RelativePosition="1")
                        
                reducer._solid_angle_correcter.execute(reducer, flood_ws)
        
            # Create efficiency profile: 
            # Divide each pixel by average signal, and mask high and low pixels.
            CalculateEfficiency(flood_ws, flood_ws, self._min_sensitivity, self._max_sensitivity)
            self._efficiency_ws = flood_ws
            
        # Divide by detector efficiency
        Divide(workspace, self._efficiency_ws, workspace)
        
        # Copy over the efficiency's masked pixels to the reduced workspace
        masked_detectors = GetMaskedDetectors(self._efficiency_ws)
        MaskDetectors(workspace, None, masked_detectors.getPropertyValue("DetectorList"))        
    

class Mask(ReductionStep):
    """
        Marks some spectra so that they are not included in the analysis
        TODO: Maintain HFIR-ISIS compatibility
    """
    def __init__(self, nx_low=0, nx_high=0, ny_low=0, ny_high=0):
        """
            Initalize masking and optionally define a "picture frame" outside of
            which the spectra from all detectors are to be masked.
            @param nx_low: number of pixels to mask on the lower-x side of the detector
            @param nx_high: number of pixels to mask on the higher-x side of the detector
            @param ny_low: number of pixels to mask on the lower-y side of the detector
            @param ny_high: number of pixels to mask on the higher-y side of the detector
        """
        super(Mask, self).__init__()
        self._nx_low = nx_low
        self._nx_high = nx_high
        self._ny_low = ny_low
        self._ny_high = ny_high
        
        #the region to be masked expressed as xml that can be passed to MaskDetectorsInShape 
        self._xml = {'shape_defs' : '', 'sum_shapes': '<algebra val="'}
        #these spectra will be masked by the algorithm MaskDetectors
        self._spec_list = ''
        
    def add_xml_shape(self, id, complete_xml_element):
        if not complete_xml_element.startswith('<') :
            raise ValueError('Excepted xml string but found: ' + str(complete_xml_element))
        self._xml[shape_defs] += complete_xml_element
        self._xml[shape_defs] += str(id)+':'
        
    def add_infinite_plane(self, id, plane_pt, normal_pt):
        self.add_xml_shape(id, '<infinite-plane id="' + str(id) + '">' + \
            '<point-in-plane x="' + str(plane_pt[0]) + '" y="' + str(plane_pt[1]) + '" z="' + str(plane_pt[2]) + '" />' + \
            '<normal-to-plane x="' + str(normal_pt[0]) + '" y="' + str(normal_pt[1]) + '" z="' + str(normal_pt[2]) + '" />'+ \
            '</infinite-plane>\n')

    def _infinite_cylinder(self, id, centre, radius, axis):
        return '<infinite-cylinder id="' + str(id) + '">' + \
            '<centre x="' + str(centre[0]) + '" y="' + str(centre[1]) + '" z="' + str(centre[2]) + '" />' + \
            '<axis x="' + str(axis[0]) + '" y="' + str(axis[1]) + '" z="' + str(axis[2]) + '" />' + \
            '<radius val="' + str(radius) + '" />' + \
            '</infinite-cylinder>\n'

    def add_cylinder(self, id, radius, xcentre, ycentre, algebra=''):
        '''Mask a cylinder on the input workspace.'''
        self.add_xml_shape(id,
            self._infinite_cylinder(id, [xcentre, ycentre, 0.0], radius, [0,0,1]))

    def add_outside_cylinder(self, id, radius, xcentre = '0.0', ycentre = '0.0'):
        '''Mask out the outside of a cylinder or specified radius'''
        id = str(id)
        new_id = 'complement_reserved_'+id
        xml_string = self._infinite_cylinder(new_id, [xcentre, ycentre, 0.0], radius, [0,0,1])
        xml_string += '<'+id + ' val="#' + new_id+'" />'
        self.add_xml_shape(id, xml_string)

    def ConvertToSpecList(self, maskstring):
        '''
            Convert a mask string to a spectra list
            6/8/9 RKH attempt to add a box mask e.g.  h12+v34 (= one pixel at intersection), h10>h12+v101>v123 (=block 3 wide, 23 tall)
        '''
        #Compile spectra ID list
        if maskstring == '':
            return ''
        masklist = maskstring.split(',')
        
        orientation = ReductionSingleton().instrument.get_orientation
        detector = ReductionSingleton().instrument.cur_detector()
        firstspec = detector.first_spec_num
        dimension = detector.n_columns
        speclist = ''
        for x in masklist:
            x = x.lower()
            if '+' in x:
                bigPieces = x.split('+')
                if '>' in bigPieces[0]:
                    pieces = bigPieces[0].split('>')
                    low = int(pieces[0].lstrip('hv'))
                    upp = int(pieces[1].lstrip('hv'))
                else:
                    low = int(bigPieces[0].lstrip('hv'))
                    upp = low
                if '>' in bigPieces[1]:
                    pieces = bigPieces[1].split('>')
                    low2 = int(pieces[0].lstrip('hv'))
                    upp2 = int(pieces[1].lstrip('hv'))
                else:
                    low2 = int(bigPieces[1].lstrip('hv'))
                    upp2 = low2            
                if 'h' in bigPieces[0] and 'v' in bigPieces[1]:
                    ydim=abs(upp-low)+1
                    xdim=abs(upp2-low2)+1
                    speclist += spectrumBlock(firstspec,low, low2,ydim, xdim, dimension,orientation) + ','
                elif 'v' in bigPieces[0] and 'h' in bigPieces[1]:
                    xdim=abs(upp-low)+1
                    ydim=abs(upp2-low2)+1
                    speclist += spectrumBlock(firstspec,low2, low,nstrips, dimension, dimension,orientation)+ ','
                else:
                    print "error in mask, ignored:  " + x
            elif '>' in x:
                pieces = x.split('>')
                low = int(pieces[0].lstrip('hvs'))
                upp = int(pieces[1].lstrip('hvs'))
                if 'h' in pieces[0]:
                    nstrips = abs(upp - low) + 1
                    speclist += spectrumBlock(firstspec,low, 0,nstrips, dimension, dimension,orientation)  + ','
                elif 'v' in pieces[0]:
                    nstrips = abs(upp - low) + 1
                    speclist += spectrumBlock(firstspec,0,low, dimension, nstrips, dimension,orientation)  + ','
                else:
                    for i in range(low, upp + 1):
                        speclist += str(i) + ','
            elif 'h' in x:
                speclist += spectrumBlock(firstspec,int(x.lstrip('h')), 0,1, dimension, dimension,orientation) + ','
            elif 'v' in x:
                speclist += spectrumBlock(firstspec,0,int(x.lstrip('v')), dimension, 1, dimension,orientation) + ','
            else:
                speclist += x.lstrip('s') + ','
        
        self._spec_list += speclist
    
    def set_spec_list(self, speclist):
        self._spec_list = speclist.rstrip(',')

    def execute(self, reducer, workspace):
        if self._xml['shape_defs'] != '':
            if self._xml['sum_shapes'].endswith(':'):
                self._xml['sum_shapes'] = self._xml['sum_shapes'].rpartition(':')[0]
            xml_string = self._xml['shape_defs'] + self._xml['sum_shapes']
            MaskDetectorsInShape(workspace, xml_string)
        # Get a list of detector pixels to mask
        masked_pixels = reducer.instrument.get_masked_pixels(self._nx_low,
                                                             self._nx_high,
                                                             self._ny_low,
                                                             self._ny_high)
        
        # Transform the list of pixels into a list of Mantid detector IDs
        masked_detectors = reducer.instrument.get_masked_detectors(masked_pixels)
        
        # Mask the pixels by passing the list of IDs
        MaskDetectors(workspace, None, masked_detectors)
        
        if self._spec_list != '':
            MaskDetectors(workspace, SpectraList = self._spec_list)


class SaveIqAscii(ReductionStep):
    def __init__(self):
        super(SaveIqAscii, self).__init__()
        
    def execute(self, reducer, workspace):
        if reducer._azimuthal_averager is not None:
            output_ws = reducer._azimuthal_averager.get_output_workspace(workspace)
            filename = os.path.join(reducer._data_path, output_ws+'.txt')
            SaveAscii(Filename=filename, Workspace=output_ws)
            
            
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
        if self._background_ws is None:
            # Apply the reduction steps that we normally apply to the data
            self._background_ws = extract_workspace_name(self._background_file)
            for item in reducer._2D_steps():
                item.execute(reducer, self._background_ws)
        
            # The transmission correction is set separately
            if self._transmission is not None:
                self._transmission.execute(reducer, self._background_ws) 
        
        Minus(workspace, self._background_ws, workspace)
        
 
class SampleGeomCor(ReductionStep):
    """
        Correct the neutron count rates for the size of the sample
    """
    def __init__(self):
        super(SampleGeomCor, self).__init__()

        # string specifies the sample's shape
        self._default_shape = 'cylinder-axis-along'
        self._shape = self._default_shape
        self._width = self._thickness = self._height = 1.0
        # dictionary contains the list of all shapes that can be calculated and the id number for them
        self._shape_ids = {1 : 'cylinder-axis-up',
                           2 : 'cuboid',
                           3 : 'cylinder-axis-along'}

    def execute(self, reducer, workspace):
        """
            Divide the counts by the volume of the sample
        """
        
        try:
            if self._shape == 'cylinder-axis-up':
    		    # Volume = circle area * height
    		    # Factor of four comes from radius = width/2
    		    scale_factor = self._height*math.pi*math.pow(self._width,2)/4.0
            elif self._shape == 'cuboid':
    		    scale_factor = (self._width*self._height*self._thickness)
            elif self._shape == 'cylinder-axis-along':
    		    # Factor of four comes from radius = width/2
    		    scale_factor = self._thickness*math.pi*math.pow(self._width, 2)/4.0
            else:
                raise NotImplemented('Shape "'+self._shape+'" is not in the list of supported shapes')
        except TypeError:
            raise TypeError('Error calculating sample volume with width='+str(self._width) + ' height='+str(self._height) + 'and thickness='+str(self._thickness)) 
        
        # Multiply by the calculated correction factor
	    #ws = mtd[workspace]
	    #ws *= scalefactor
        CreateSingleValuedWorkspace('temp', scale_factor)
        Divide(workspace, 'temp', workspace)
    
    def read_from_workspace(self, workspace):
        sample_details = mtd[workspace].getSampleInfo()
        self.set_geometry(sample_details.getGeometryFlag())
        self.set_thickness(sample_details.getThickness())
        self.set_height(sample_details.getHeight())
        self.set_width(sample_details.getWidth())
        
    def set_geometry(self, shape):
        """
            Sets the sample's shape from a string or an ID. If the ID is not
            in the list of allowed values the shape is set to the default but
            shape strings not in the list are not checked
        """
        try:
            # deal with ID numbers as arguments
            shape = self._shape_ids[int(shape)]
        except ValueError:
            # means that we weren't passed an ID number, the code below treats it as a shape name
            pass
        except KeyError:
            mantid.sendLogMessage("::SANS::Warning: Invalid geometry type for sample: " + str(shape) + ". Setting default to " + self._default_shape)
            shape = self._default_shape
        self._shape = shape
        
    def set_width(self, width):
        self._width = float(width)
        # For a disk the height=width
        if self._shape.startswith('cylinder'):
            self._height = self._width
            
    def set_height(self, height):
        self._height = float(height)
        
        # For a cylinder and sphere the height=width=radius
        if self._shape.startswith('cylinder'):
            self._width = self._height

    def set_thickness(self, thickness):
        """
            Simply sets the variable _thickness to the value passed
        """
        if not self._shape == 'cuboid':
            mantid.sendLogMessage('::SANS::Warning: Can''t set thickness for shape "'+self._shape+'"')
        self._thickness = float(thickness)
    
    def set_dimensions_to_unity(self):
        self._width = self._thickness = self._height = 1.0

    def __str__(self):
        return '-- Sample Geometry --\n' + \
               '    Shape: ' + self._shape+'\n'+\
               '    Width: ' + str(self._width)+'\n'+\
               '    Height: ' + str(self._height)+'\n'+\
               '    Thickness: ' + str(self._thickness)+'\n'

class StripEndZeros(ReductionStep):
    def __init__(self, flag_value = 0.0):
        super(StripEndZeros, self).__init__()
        self._flag_value = flag_value
        
    def execute(self, reducer, workspace):
        result_ws = mantid.getMatrixWorkspace(workspace)
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
