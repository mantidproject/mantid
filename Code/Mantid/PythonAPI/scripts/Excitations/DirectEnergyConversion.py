"""Conversion class defined for 'direct' excitations conversions 
"""
import ConvertToEnergy
import CommonFunctions as common
from mantidsimple import *

#-------------- ElasticConversion class -------------
class DirectEnergyConversion(ConvertToEnergy.EnergyConversion):
    '''
    Performs a convert to energy assuming the provided instrument is an elastic instrument
    '''
    
    def __init__(self, prefix):
        super(self.__class__, self).__init__(prefix)
        
    def init_params(self):
        '''
        Attach analysis arguments that are particular to the ElasticConversion 
        '''
        self.save_formats = ['.spe','.nxs','.nxspe']
        self.fix_ei=False
        self.energy_bins = None
        self.background = False
        self.normalise_method = 'monitor-1'
#        self.normalise_method = 'current'
        self.map_file = None
        
        if (self.file_prefix == "CNCS" or self.file_prefix == "ARCS" or self.file_prefix == "SEQUOIA"):
            self.facility = "SNS"
            self.normalise_method  = 'current'
            #self.file_ext = '_event.nxs'
            #self.file_ext = '.dat'
            
        
        # The Ei requested
        self.ei_requested = None
        self.monitor_workspace = None
        
        self.time_bins = None
                
        # Detector diagnosis
        self.spectra_masks = None
        
        # Absolute normalisation
        self.abs_map_file = None
        self.abs_spectra_masks = None
        self.abs_mass = 1.0
        self.abs_rmm = 1.0
        self.applyDetectorEfficiency = True
     
    def init_idf_params(self):
        '''
        Initialise the parameters from the IDF file
        '''
        self.ei_mon_spectra = [int(self.get_default_parameter("ei-mon1-spec")), int(self.get_default_parameter("ei-mon2-spec"))]
        self.scale_factor = self.get_default_parameter("scale-factor")
        self.wb_scale_factor = self.get_default_parameter("wb-scale-factor")
        self.wb_integr_range = [self.get_default_parameter("wb-integr-min"), self.get_default_parameter("wb-integr-max")]
        self.mon1_norm_spec = int(self.get_default_parameter("norm-mon1-spec"))
        self.mon1_norm_range = [self.get_default_parameter("norm-mon1-min"), self.get_default_parameter("norm-mon1-max")]
        self.background_range = [self.get_default_parameter("bkgd-range-min"), self.get_default_parameter("bkgd-range-max")]
        self.monovan_integr_range = [self.get_default_parameter("monovan-integr-min"), self.get_default_parameter("monovan-integr-max")]
        self.van_mass = self.get_default_parameter("vanadium-mass")
        self.van_rmm = self.get_default_parameter("vanadium-rmm")

        self.abs_min_value = self.get_default_parameter('abs-average-min')
        self.abs_max_value = self.get_default_parameter('abs-average-max')
        self.abs_median_lbound = self.get_default_parameter('abs-median-lbound')
        self.abs_median_ubound = self.get_default_parameter('abs-median-ubound')
        self.abs_median_frac_low = self.get_default_parameter('abs-median-lo-frac')
        self.abs_median_frac_hi = self.get_default_parameter('abs-median-hi-frac')
        self.abs_median_sig = self.get_default_parameter('abs-median-signif')

    def get_default_parameter(self, name):
        if self.instrument is None:
            raise ValueError("Cannot init default parameter, instrument has not been loaded.")
        values = self.instrument.getNumberParameter(name)
        if len(values) != 1:
            raise ValueError('Instrument parameter file does not contain a definition for "%s". Cannot continue' % name)
        return values[0]
    
    def convert_to_energy(self, mono_run, ei, white_run=None, abs_mono_run=None, abs_ei=None, abs_white_run=None, save_path=None, Tzero=None):
        '''
        Convert mono-chromatic run to deltaE
        '''
        
        
        # Check if we need to perform the absolute normalisation first
        if not abs_mono_run is None:
            if abs_ei is None:
                abs_ei = ei
            mapping_file = self.abs_map_file
            spectrum_masks = self.spectra_masks 
            abs_norm_wkspace = self.do_conversion(abs_mono_run, abs_ei, abs_white_run, mapping_file, spectrum_masks)
            abs_average_factor = self.abs_average(abs_norm_wkspace)
            
            # TODO: Need a better check than this...
            if (abs_white_run is None):
                self.log("Performing Normalisation to Mono Vanadium.")
                absnorm_factor = abs_average_factor
            else:
                self.log("Performing Absolute Units Normalisation.")
                # Perform Abs Units...
                absnorm_factor = (self.van_rmm/self.van_mass) * abs_average_factor
                #  Scale by vanadium cross-section which is energy dependent up to a point
                ei_value = abs_norm_wkspace.getRun().getLogData('Ei').value
                if ei_value >= 200.0:
                    xsection = 421.0
                else:
                    xsection = 400.0 + (ei_value/10.0)
                absnorm_factor /= xsection
            mtd.deleteWorkspace(abs_norm_wkspace.getName())
        else:
            absnorm_factor = None

        # Figure out what to call the workspace 
        resultws_name = save_path
        if not resultws_name is None:
            if os.path.isdir(save_path):
                resultws_name = None
            else:
                resultws_name = os.path.split(save_path)[1]
            if resultws_name == '':
                resultws_name = None

        # Main run file conversion
        sample_wkspace = self.do_conversion(mono_run, ei, white_run, self.map_file, self.spectra_masks, resultws_name, Tzero)
        if not absnorm_factor is None:
            if (abs_white_run is not None):
                absnorm_factor *= (self.abs_mass/self.abs_rmm)
            sample_wkspace /= absnorm_factor
        
        if save_path is None:
            save_path = sample_wkspace.getName()
        elif os.path.isdir(save_path):
            save_path = os.path.join(save_path, sample_wkspace.getName())
            
        self.save_results(sample_wkspace, save_path)
        return sample_wkspace
        
    def do_conversion(self, mono_run, ei_guess, white_run=None, map_file=None, spectra_masks=None, resultws_name = None, Tzero=None):
        """
        Convert units of a mono-chromatic run to deltaE, including normalisation to a white-beam vanadium run.
        If multiple run files are passed to this function, they are summed into a run and then processed
        """
        if type(mono_run) == list:
            result_ws, det_info_file = self.load_data(mono_run[0], resultws_name)
            if len(mono_run) > 1:
                del mono_run[0]
                common.sum_files(result_ws, mono_run, self.file_prefix)
        elif type(str):
            result_ws, det_info_file = self.load_data(mono_run, resultws_name)
        else:
            raise TypeError("Run number must be a list or a string")
            
        # Special load monitor stuff.    
        if (self.file_prefix == "CNCS"):
            #self.log("--- CNCS ---")
            self.fix_ei = True
            ei_value = ei_guess
            if (Tzero is None):
            	tzero = (0.1982*(1+ei_value)**(-0.84098))*1000.0
            else:
            	tzero = Tzero
            # apply T0 shift
            ChangeBinOffset(result_ws, result_ws, -tzero)
            mon1_peak = 0.0
            self.applyDetectorEfficiency = True
        elif (self.file_prefix == "ARCS" or self.file_prefix == "SEQUOIA"):
            #self.log("***** ARCS/SEQUOIA *****")
            #self.log(mono_run)
            if mono_run.endswith("_event.nxs"):
                loader=LoadNexusMonitors(Filename=mono_run, OutputWorksapce="monitor_ws")    
            elif mono_run.endswith("_event.dat"):
                InfoFilename = mono_run.replace("_neutron_event.dat", "_runinfo.xml")
                loader=LoadPreNeXusMonitors(RunInfoFilename=InfoFilename,OutputWorkspace="monitor_ws")
            monitor_ws = loader.workspace()
            alg = GetEi(monitor_ws, int(self.ei_mon_spectra[0]), int(self.ei_mon_spectra[1]), ei_guess, False)
            ei_value = monitor_ws.getRun().getLogData("Ei").value
            if (self.fix_ei):
                ei_value = ei_guess
            if (Tzero is None):
                # TODO: Calculate T0
                #tzero = float(alg.getPropertyValue("Tzero"))
                tzero = 0.0
            else:	
            	tzero = Tzero
            mon1_peak = 0.0
            # apply T0 shift
            ChangeBinOffset(result_ws, result_ws, -tzero)
            self.applyDetectorEfficiency = True
        else:
            # Do ISIS stuff for Ei
            ei_value, mon1_peak = self.get_ei(result_ws, ei_guess)
            
        # For event mode, we are going to histogram in energy first, then go back to TOF
        if (self.facility == "SNS"):
            # Convert to Et
            ConvertUnits(result_ws, "_tmp_energy_ws", Target="DeltaE",EMode="Direct", EFixed=ei_value)
            RenameWorkspace("_tmp_energy_ws", result_ws)
            mtd.deleteWorkspace("_tmp_energy_ws")
            # Histogram
            Rebin(result_ws, "_tmp_rebin_ws", self.energy_bins)
            RenameWorkspace("_tmp_rebin_ws", result_ws)
            mtd.deleteWorkspace("_tmp_rebin_ws")
            # Convert back to TOF
            ConvertUnits(result_ws, result_ws, Target="TOF",EMode="Direct", EFixed=ei_value)
        
        bin_offset = -mon1_peak
        
        if self.background == True:
            # Remove the count rate seen in the regions of the histograms defined as the background regions, if the user defined a region
            ConvertToDistribution(result_ws)                                             
            FlatBackground(result_ws, result_ws, self.background_range[0] + bin_offset, self.background_range[1] + bin_offset, '', 'Mean')
            ConvertFromDistribution(result_ws)  
    
        self.normalise(result_ws, self.normalise_method, range_offset=bin_offset)
        
        ConvertUnits(result_ws, result_ws, Target="DeltaE",EMode='Direct')
        if not self.energy_bins is None:
            Rebin(result_ws, result_ws, self.energy_bins)
        
        if self.applyDetectorEfficiency:
            if (self.facility == "SNS"):
                # Need to be in lambda for detector efficiency correction
                ConvertUnits(result_ws, result_ws, Target="Wavelength", EMode="Direct")
                He3TubeEfficiency(result_ws, result_ws)
                ConvertUnits(result_ws, result_ws, Target="DeltaE",EMode='Direct')
            else:
                LoadDetectorInfo(result_ws, det_info_file)
                DetectorEfficiencyCor(result_ws, result_ws)

        # Ki/Kf Scaling...
        CorrectKiKf(result_ws, result_ws, EMode='Direct')

        # Make sure that our binning is consistent
        if not self.energy_bins is None:
            Rebin(result_ws, result_ws, self.energy_bins)
        
        self.apply_masking(result_ws, spectra_masks, map_file)

        ConvertToDistribution(result_ws)
        if white_run != None:
            white_ws = self.convert_white(white_run, spectra_masks, map_file)
            result_ws /= white_ws
            mtd.deleteWorkspace(white_ws.getName())
        
        # Overall scale factor
        result_ws *= self.scale_factor
        return result_ws
    
    def load_data(self, run_num, output_name):
        '''
        Load a run number or sum a set of run numbers
        '''
        data = common.load_run(self.file_prefix, run_num, output_name, self.file_ext)
        self.setup_mtd_instrument(data[0])
        return data
       
    def get_ei(self, input_ws, ei_guess):
        """
        Calculate incident energy of neutrons
        """
        fix_ei = str(self.fix_ei).lower()
        if fix_ei == 'true':
            fix_ei = True
        elif fix_ei == 'false':
            fix_ei = False
        elif fix_ei == 'fixei':
            fix_ei = True
        else:
            raise TypeError('Unknown option passed to get_ei "%s"' % fix_ei)

        # Calculate the incident energy
        alg = GetEi(input_ws, int(self.ei_mon_spectra[0]), int(self.ei_mon_spectra[1]), ei_guess, fix_ei)
        mon1_peak = float(alg.getPropertyValue("FirstMonitorPeak"))
        mon1_index = int(alg.getPropertyValue("FirstMonitorIndex"))
        ei = input_ws.getSampleDetails().getLogData("Ei").value
        # Adjust the TOF such that the first monitor peak is at t=0
        ChangeBinOffset(input_ws, input_ws, -mon1_peak)
        mon1_det = input_ws.getDetector(mon1_index)
        mon1_pos = mon1_det.getPos()
        src_name = input_ws.getInstrument().getSource().getName()
        MoveInstrumentComponent(input_ws, src_name, X=mon1_pos.getX(), Y=mon1_pos.getY(), Z=mon1_pos.getZ(), RelativePosition=False)
        return ei, mon1_peak

    def apply_masking(self, result_ws, spec_masks, map_file):
        '''
        Mask and group detectors based on input parameters
        '''
        if not spec_masks is None:
            MaskDetectors(result_ws, SpectraList=spec_masks)
        if not map_file is None:
            GroupDetectors(result_ws, result_ws, map_file, KeepUngroupedSpectra=0)

    def convert_white(self, white_run, spectra_masks, map_file): 
        '''
        Normalise to a specified white-beam run
        '''
        white_ws = common.load_run(self.file_prefix, white_run, '_tmp_white_', self.file_ext)[0]
        self.normalise(white_ws, self.normalise_method)
       
        ConvertUnits(white_ws, white_ws, "Energy", AlignBins=0)
        # This both integrates the workspace into one bin spectra and sets up common bin boundaries for all spectra
        low = self.wb_integr_range[0]
        upp = self.wb_integr_range[1]
        if low > upp:
            raise ValueError("White beam integration range is inconsistent. low=%d, upp=%d" % (low,upp))
        delta = 2.0*(upp - low)
        Rebin(white_ws, white_ws, [low, delta, upp])
        self.apply_masking(white_ws, spectra_masks, map_file)

        # White beam scale factor
        white_ws *= self.wb_scale_factor
        return white_ws

    def normalise(self, data_ws, method, range_offset=0.0):
        '''
        Apply normalisation using specified source
        '''
        method = method.lower()
        if method == 'monitor-1':
            range_min = self.mon1_norm_range[0] + range_offset
            range_max = self.mon1_norm_range[1] + range_offset
            NormaliseToMonitor(InputWorkspace=data_ws, OutputWorkspace=data_ws, MonitorSpectrum=int(self.mon1_norm_spec), IntegrationRangeMin=range_min, IntegrationRangeMax=range_max,IncludePartialBins=True)
        elif method == 'current':
            NormaliseByCurrent(InputWorkspace=data_ws, OutputWorkspace=data_ws)
        elif method == 'none':
            return
        else:
            raise RuntimeError('Normalisation scheme ' + reference + ' not found. It must be one of monitor-1, current, peak or none')
            
    def abs_average(self, data_ws):
        '''
        Compute the average Y value of a workspace.
        
        The average is computed by collapsing the workspace to a single bin per spectra then masking
        masking out detectors given by the FindDetectorsOutsideLimits and MedianDetectorTest algorithms.
        The average is then the computed as the using the remainder and factoring in their errors as weights, i.e.
        
            average = sum(Yvalue[i]*weight[i]) / sum(weights)
            
        where only those detectors that are unmasked are used and the weight[i] = 1/errorValue[i].
        '''

        e_low = self.monovan_integr_range[0]
        e_upp = self.monovan_integr_range[1]
        if e_low > e_upp:
            raise ValueError("Inconsistent mono-vanadium integration range defined!")
        Rebin(data_ws, data_ws, [e_low, 2.*(e_upp-e_low), e_upp])
        
        min_value = self.abs_min_value
        max_value = self.abs_max_value
        median_lbound = self.abs_median_lbound
        median_ubound = self.abs_median_ubound
        median_frac_low = self.abs_median_frac_low
        median_frac_hi = self.abs_median_frac_hi
        median_sig = self.abs_median_sig

        self.mask_detectors_outside_range(data_ws, min_value, max_value,median_lbound, median_ubound, median_frac_low, median_frac_hi, median_sig)
        ConvertFromDistribution(data_ws)

        nhist = data_ws.getNumberHistograms()
        average_value = 0.0
        weight_sum = 0.0
        for i in range(nhist):
            try:
                det = data_ws.getDetector(i)
            except Exception:
                continue
            if det.isMasked():
                continue
            y_value = data_ws.dataY(i)[0]
            if y_value != y_value:
                continue
            weight = 1.0/data_ws.dataE(i)[0]
            average_value += y_value * weight
            weight_sum += weight

        average_value /= weight_sum
        return average_value
    
    def mask_detectors_outside_range(self, data_ws, min_value, max_value,median_lbound, median_ubound, median_frac_lo, median_frac_hi, median_sig):
        '''
        Masks detecrors on the given workspace according the ranges given where:
            min_value - lower bound of meaningful value;
            max_value - upper bound of meaningful value;
            median_lbound - lower bound defining outliers as fraction of median value;
            median_ubound - upper bound defining outliers as fraction of median value;
            median_frac_lo - lower acceptable bound as fraction of median value;
            median_frac_hi - upper acceptable bound as fraction of median value;
            media_sig - error criterion as a multiple of error bar i.e. to fail the test, the magnitude of the
                        difference with respect to the median value must also exceed this number of error bars.
        '''
        # Limit test
        median_tests_ws = '_tmp_abs_median_tests'
        fdol_alg = FindDetectorsOutsideLimits(data_ws, median_tests_ws, HighThreshold=max_value, LowThreshold=min_value)
        MaskDetectors(data_ws, SpectraList=fdol_alg.getPropertyValue('BadSpectraNums'))
        # Median tests
        median_test_alg = MedianDetectorTest(data_ws, median_tests_ws, LowThreshold=median_lbound, HighThreshold=median_ubound)
        MaskDetectors(data_ws, SpectraList=median_test_alg.getPropertyValue('BadSpectraNums'))
        median_test_alg = MedianDetectorTest(data_ws, median_tests_ws, SignificanceTest=median_sig, LowThreshold=median_frac_lo, HighThreshold=median_frac_hi)
        MaskDetectors(data_ws, SpectraList=median_test_alg.getPropertyValue('BadSpectraNums'))
        mtd.deleteWorkspace(median_tests_ws)

    def save_results(self, workspace, save_path, formats = None):
        '''
        Save the result workspace to the specfied filename using the list of formats specified in 
        self.save_formats
        '''
        if save_path == '':
            raise ValueError('Empty filename is not allowed for saving')
        if formats is None:
            formats = self.save_formats
        if type(formats) == str:
            formats = [formats]
        #Make sure we just have a file stem
        save_path = os.path.splitext(save_path)[0]
        for ext in formats:
            filename = save_path + ext
            if ext == '.spe':
                SaveSPE(workspace, filename)
            elif ext == '.nxs':
                SaveNexus(workspace, filename)
            elif ext == '.nxspe':
                SaveNXSPE(workspace, filename)
            else:
                self.log('Unknown file format "%s" encountered while saving results.')
#-----------------------------------------------------------------
