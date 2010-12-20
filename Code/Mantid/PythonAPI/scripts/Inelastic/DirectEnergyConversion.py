"""
Conversion class defined for conversion to deltaE for
'direct' inelastic geometry instruments
   
The class defines various methods to allow users to convert their 
files to DeltaE.

Example:

Assuming we have the follwing data files for MARI. 
NOTE: This assumes that the data path for these runs is in the 
Mantid preferences.

mono-sample: 11015
white: 11060
mono-van: 11001
with sample mass 10g and RMM 435.96

reducer = DirectEnergyConversion('MARI')

# Alter defaults if necessary
reducer.normalise_method = 'monitor-2'
reducer.background = False
reducer.fix_ei = True
reducer.save_formats = ['.spe']
# 
Set parameters for these runs
reducer.map_file = 'mari_res.map'
reducer.energy_bins = '-10,0.1,80'

Run the conversion
deltaE_wkspace = reducer.convert_to_energy(11015, 85, 11060, 11001)
"""
import CommonFunctions as common
import diagnostics
from mantidsimple import *
import glob
import os.path

def setup_reducer(inst_name):
    """
    Given an instrument name or prefix this sets up a converter
    object for the reduction
    """
    try:
        return DirectEnergyConversion(inst_name)
    except RuntimeError:
        raise RuntimeError('Unknown instrument "%s", cannot continue' % inst_name)
    

class DirectEnergyConversion(object):
    """
    Performs a convert to energy assuming the provided instrument is an elastic instrument
    """

    def diagnose(self, white_run, sample_run=None, other_white=None, remove_zero=None, 
                 tiny=None, large=None, median_lo=None, median_hi=None, signif=None, 
                 bkgd_threshold=None, bkgd_range=None, variation=None, hard_mask=None,
                 print_results=False):
        """
        A pass through method to the 'real' one in diagnostics.py

        Run diagnostics on the provided run and white beam files.

        There are 3 possible tests, depending on the input given:
          White beam diagnosis
          Background tests
          Second white beam
            
        Required inputs:
        
          white_run  - The run number or filepath of the white beam run
        
        Optional inputs:
          sample_run - The run number or filepath of the sample run for the background test (default = None)
          other_white   - If provided an addional set of tests is performed on this file. (default = None)
          remove_zero - If true then zeroes in the data will count as failed (default = False)
          tiny          - Minimum threshold for acceptance (default = 1e-10)
          large         - Maximum threshold for acceptance (default = 1e10)
          median_lo     - Fraction of median to consider counting low (default = 0.1)
          median_hi     - Fraction of median to consider counting high (default = 3.0)
          signif        - Counts within this number of multiples of the 
                          standard dev will be kept (default = 3.3)
          bkgd_threshold - High threshold for background removal in multiples of median (default = 5.0)
          bkgd_range - The background range as a list of 2 numbers: [min,max]. 
                       If not present then they are taken from the parameter file. (default = None)
          variation  - The number of medians the ratio of the first/second white beam can deviate from
                       the average by (default=1.1)
          hard_mask  - A file specifying those spectra that should be masked without testing
          print_results - If True then the results are printed to std out
          inst_name  - The name of the instrument to perform the diagnosis.
                       If it is not provided then the default instrument is used (default = None)
          """
        self.spectra_masks = \
                           diagnostics.diagnose(white_run, sample_run, other_white, remove_zero,
                                                tiny, large, median_lo, median_hi, signif,
                                                bkgd_threshold, bkgd_range, variation, hard_mask,
                                                print_results, self.instr_name)
        return self.spectra_masks
    
    def do_white(self, white_run, spectra_masks, map_file): 
        """
        Normalise to a specified white-beam run
        """
        whitews_name = common.create_resultname(white_run, prefix = self.instr_name, suffix='-white')
        if mtd.workspaceExists(whitews_name):
            return mtd[whitews_name]

        # Load
        white_data = self.load_data(white_run, 'white-beam')
        # Normalise
        white_ws = self.normalise(white_data, whitews_name, self.normalise_method)
        # Units conversion
        ConvertUnits(white_ws, white_ws, "Energy", AlignBins=0)
        # This both integrates the workspace into one bin spectra and sets up common bin boundaries for all spectra
        low = self.wb_integr_range[0]
        upp = self.wb_integr_range[1]
        if low > upp:
            raise ValueError("White beam integration range is inconsistent. low=%d, upp=%d" % (low,upp))
        delta = 2.0*(upp - low)
        Rebin(white_ws, white_ws, [low, delta, upp])

        # Masking and grouping
        white_ws = self.remap(white_ws, spectra_masks, map_file)

        # White beam scale factor
        white_ws *= self.wb_scale_factor

        return white_ws

    def mono_van(self, mono_van, ei_guess, white_run=None, map_file=None,
                 spectra_masks=None, result_name = None, Tzero=None):
        """Convert a mono vanadium run to DeltaE.
        If multiple run files are passed to this function, they are summed into a run and then processed         
        """
                # Load data
        sample_data = self.load_data(mono_van, 'mono-van')
        # Create the result name if necessary
        if result_name is None:
            result_name = common.create_resultname(mono_van, prefix=self.instr_name)

        return self._do_mono(sample_data, sample_data, result_name, ei_guess, 
                                   white_run, map_file, spectra_masks, Tzero)

    def mono_sample(self, mono_run, ei_guess, white_run=None, map_file=None,
                    spectra_masks=None, result_name = None, Tzero=None):
        """Convert a mono-chromatic sample run to DeltaE.
        If multiple run files are passed to this function, they are summed into a run and then processed
        """
        # Load data
        sample_data = self.load_data(mono_run, 'mono-sample')
        # Create the result name if necessary
        if result_name is None:
            result_name = common.create_resultname(mono_run, prefix=self.instr_name)

        return self._do_mono(sample_data, sample_data, result_name, ei_guess, 
                                  white_run, map_file, spectra_masks, Tzero)


# -------------------------------------------------------------------------------------------
#         This actually does the conversion for the mono-sample and mono-vanadium runs
#  
# -------------------------------------------------------------------------------------------        
    def _do_mono(self, data_ws, monitor_ws, result_name, ei_guess, 
                 white_run=None, map_file=None, spectra_masks=None, Tzero=None):
        """
        Convert units of a given workspace to deltaE, including possible 
        normalisation to a white-beam vanadium run.
        """
        # Special load monitor stuff.    
        if (self.instr_name == "CNCS"):
            self.fix_ei = True
            ei_value = ei_guess
            if (Tzero is None):
            	tzero = (0.1982*(1+ei_value)**(-0.84098))*1000.0
            else:
            	tzero = Tzero
            # apply T0 shift
            ChangeBinOffset(data_ws, result_name, -tzero)
            mon1_peak = 0.0
            self.apply_detector_eff = True
        elif (self.instr_name == "ARCS" or self.instr_name == "SEQUOIA"):
            mono_run = common.loaded_file('mono-sample')
                           
            if mono_run.endswith("_event.nxs"):
                loader=LoadNexusMonitors(Filename=mono_run, OutputWorkspace="monitor_ws")    
            elif mono_run.endswith("_event.dat"):
                InfoFilename = mono_run.replace("_neutron_event.dat", "_runinfo.xml")
                loader=LoadPreNeXusMonitors(RunInfoFilename=InfoFilename,OutputWorkspace="monitor_ws")
            monitor_ws = loader.workspace()
            alg = GetEi(monitor_ws, int(self.ei_mon_spectra[0]), int(self.ei_mon_spectra[1]), ei_guess, False)
            
            Tzero = float(alg.getPropertyValue("Tzero"))
                    
            if (self.fix_ei):
                ei_value = ei_guess    
            else:
                ei_value = monitor_ws.getRun().getLogData("Ei").value
            
            if (Tzero is None):
                tzero = 0.0
            else:	
            	tzero = Tzero
            
            mon1_peak = 0.0
            # apply T0 shift
            ChangeBinOffset(data_ws, result_name, -tzero)
            self.apply_detector_eff = True
        else:
            # Do ISIS stuff for Ei
            # Both are these should be run properties really
            ei_value, mon1_peak = self.get_ei(monitor_ws, result_name, ei_guess)

        # As we've shifted the TOF so that mon1 is at t=0.0 we need to account for this in FlatBackground and normalisation
        bin_offset = -mon1_peak

        # Get the workspace the converted data will end up in
        result_ws = mtd[result_name]
        
        # For event mode, we are going to histogram in energy first, then go back to TOF
        if (self.facility == "SNS"):
            if self.background == True:
                # Extract the time range for the background determination before we throw it away
                background_bins = "%s,%s,%s" % (self.background_range[0] + bin_offset, (self.background_range[1]-self.background_range[0]), self.background_range[1] + bin_offset)
                Rebin(result_ws, "background_origin_ws", background_bins)
            # Convert to Et
            ConvertUnits(result_ws, "_tmp_energy_ws", Target="DeltaE",EMode="Direct", EFixed=ei_value)
            RenameWorkspace("_tmp_energy_ws", result_ws)
            # Histogram
            Rebin(result_ws, "_tmp_rebin_ws", self.energy_bins)
            RenameWorkspace("_tmp_rebin_ws", result_ws)
            # Convert back to TOF
            ConvertUnits(result_ws, result_ws, Target="TOF",EMode="Direct", EFixed=ei_value)
        else:
            # TODO: This algorithm needs to be separated so that it doesn't actually
            # do the correction as well so that it can be moved next to LoadRaw where
            # it belongs
            LoadDetectorInfo(result_ws, common.loaded_file('mono'))


        if self.background == True:
            # Remove the count rate seen in the regions of the histograms defined as the background regions, if the user defined a region
            ConvertToDistribution(result_ws)    
            if (self.facility == "SNS"):
                FlatBackground("background_origin_ws", "background_ws", self.background_range[0] + bin_offset, self.background_range[1] + bin_offset, '', 'Mean', 'Return Background')
                # Delete the raw data background region workspace
                mtd.deleteWorkspace("background_origin_ws")
                # Convert to distribution to make it compatible with the data workspace (result_ws).
                ConvertToDistribution("background_ws") 
                # Subtract the background
                Minus(result_ws, "background_ws", result_ws)
                # Delete the determined background 
                mtd.deleteWorkspace("background_ws")
            else:
                FlatBackground(result_ws, result_ws, self.background_range[0] + bin_offset, self.background_range[1] + bin_offset, '', 'Mean')
            ConvertFromDistribution(result_ws)  

        # Normalise using the chosen method
        # TODO: This really should be done as soon as possible after loading
        self.normalise(result_ws, result_ws, self.normalise_method, range_offset=bin_offset)

        ConvertUnits(result_ws, result_ws, Target="DeltaE",EMode='Direct')
        if not self.energy_bins is None:
            Rebin(result_ws, result_ws, self.energy_bins)
        
        if self.apply_detector_eff:
            if (self.facility == "SNS"):
                # Need to be in lambda for detector efficiency correction
                ConvertUnits(result_ws, result_ws, Target="Wavelength", EMode="Direct")
                He3TubeEfficiency(result_ws, result_ws)
                ConvertUnits(result_ws, result_ws, Target="DeltaE",EMode='Direct')
            else:
                DetectorEfficiencyCor(result_ws, result_ws)

        # Ki/Kf Scaling...
        CorrectKiKf(result_ws, result_ws, EMode='Direct')

        # Make sure that our binning is consistent
        if not self.energy_bins is None:
            Rebin(result_ws, result_ws, self.energy_bins)
        
        # Masking and grouping
        result_ws = self.remap(result_ws, spectra_masks, map_file)

        ConvertToDistribution(result_ws)
        # White beam correction
        if white_run is not None:
            white_ws = self.do_white(white_run, spectra_masks, map_file)
            result_ws /= white_ws
        
        # Overall scale factor
        result_ws *= self.scale_factor
        return result_ws

#-------------------------------------------------------------------------------

    def convert_to_energy(self, mono_run, ei, white_run=None, mono_van=None,\
                          abs_ei=None, abs_white_run=None, save_path=None, Tzero=None):
        """
        One-shot function to convert the given runs to energy
        """
        # Check if we need to perform the absolute normalisation first
        if not mono_van is None:
            if abs_ei is None:
                abs_ei = ei
            mapping_file = self.abs_map_file
            spectrum_masks = self.spectra_masks 
            monovan_wkspace = self.mono_van(mono_van, abs_ei, abs_white_run, mapping_file, spectrum_masks)
            
            # TODO: Need a better check than this...
            if (abs_white_run is None):
                self.log("Performing Normalisation to Mono Vanadium.")
                norm_factor = self.calc_average(monovan_wkspace)
            else:
                self.log("Performing Absolute Units Normalisation.")
                # Perform Abs Units...
                norm_factor = self.monovan_abs(monovan_wkspace)
            mtd.deleteWorkspace(monovan_wkspace.getName())
        else:
            norm_factor = None

        # Figure out what to call the workspace 
        result_name = mono_run
        if not result_name is None:
            result_name = common.create_resultname(mono_run)
        
        # Main run file conversion
        sample_wkspace = self.mono_sample(mono_run, ei, white_run, self.map_file,
                                          self.spectra_masks, result_name, Tzero)
        if not norm_factor is None:
            sample_wkspace /= norm_factor

        # Save then finish
        self.save_results(sample_wkspace, save_path)
        return sample_wkspace

#----------------------------------------------------------------------------------
#                        Reduction steps
#----------------------------------------------------------------------------------

     
    def get_ei(self, input_ws, resultws_name, ei_guess):
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
        ChangeBinOffset(input_ws, resultws_name, -mon1_peak)
        mon1_det = input_ws.getDetector(mon1_index)
        mon1_pos = mon1_det.getPos()
        src_name = input_ws.getInstrument().getSource().getName()
        MoveInstrumentComponent(resultws_name, src_name, X=mon1_pos.getX(), Y=mon1_pos.getY(), Z=mon1_pos.getZ(), RelativePosition=False)
        return ei, mon1_peak

    def remap(self, result_ws, spec_masks, map_file):
        """
        Mask and group detectors based on input parameters
        """
        if not spec_masks is None:
            MaskDetectors(result_ws, MaskedWorkspace=spec_masks)
        if not map_file is None:
            GroupDetectors(result_ws, result_ws, map_file, KeepUngroupedSpectra=0)

        return mtd[str(result_ws)]

    def normalise(self, data_ws, result_ws, method, range_offset=0.0):
        """
        Apply normalisation using specified source
        """
        method = method.lower()
        if method == 'monitor-1':
            range_min = self.mon1_norm_range[0] + range_offset
            range_max = self.mon1_norm_range[1] + range_offset
            NormaliseToMonitor(InputWorkspace=data_ws, OutputWorkspace=result_ws, MonitorSpectrum=int(self.mon1_norm_spec), 
                               IntegrationRangeMin=range_min, IntegrationRangeMax=range_max,IncludePartialBins=True)
            output = mtd[str(result_ws)]
        elif method == 'current':
            NormaliseByCurrent(InputWorkspace=data_ws, OutputWorkspace=result_ws)
            output = mtd[str(result_ws)]
        elif method == 'none':
            if str(data_ws) != str(result_ws):
                CloneWorkspace(InputWorkspace=data_ws, OutputWorkspace=result_ws)
            output = mtd[str(result_ws)]
        else:
            raise RuntimeError('Normalisation scheme ' + reference + ' not found. It must be one of monitor-1, current, peak or none')

        return output
            
    def calc_average(self, data_ws):
        """
        Compute the average Y value of a workspace.
        
        The average is computed by collapsing the workspace to a single bin per spectra then masking
        masking out detectors given by the FindDetectorsOutsideLimits and MedianDetectorTest algorithms.
        The average is then the computed as the using the remainder and factoring in their errors as weights, i.e.
        
            average = sum(Yvalue[i]*weight[i]) / sum(weights)
            
        where only those detectors that are unmasked are used and the weight[i] = 1/errorValue[i].
        """

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

        self.mask_detectors_outside_range(data_ws, min_value, max_value, median_lbound,
                                          median_ubound, median_frac_low, median_frac_hi, median_sig)
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
            y_value = data_ws.readY(i)[0]
            if y_value != y_value:
                continue
            weight = 1.0/data_ws.readE(i)[0]
            average_value += y_value * weight
            weight_sum += weight

        return average_value / weight_sum

    def monovan_abs(self, ei_workspace):
        """Calculates the scaling factor required for normalisation to absolute units.
        The given workspace must contain an Ei value. This will have happened if GetEi
        has been run
        """
        averageY = self.calc_average(ei_workspace)
        absnorm_factor = averageY * (self.van_rmm/self.van_mass) 
        #  Scale by vanadium cross-section which is energy dependent up to a point
        run = ei_workspace.getRun()
        try:
            ei_prop = run['Ei']
        except KeyError:
            raise RuntimeError('The given workspace "%s" does not contain an Ei value. Run GetEi first.' % str(ei_workspace))
        
        ei_value = ei_prop.value
        if ei_value >= 200.0:
            xsection = 421.0
        else:
            xsection = 400.0 + (ei_value/10.0)
        absnorm_factor /= xsection
        return absnorm_factor * (self.sample_mass/self.sample_rmm)

    
    def mask_detectors_outside_range(self, data_ws, min_value, max_value, median_lbound, 
                                     median_ubound, median_frac_lo, median_frac_hi, median_sig):
        """
        Masks detecrors on the given workspace according the ranges given where:
            min_value - lower bound of meaningful value;
            max_value - upper bound of meaningful value;
            median_lbound - lower bound defining outliers as fraction of median value;
            median_ubound - upper bound defining outliers as fraction of median value;
            median_frac_lo - lower acceptable bound as fraction of median value;
            median_frac_hi - upper acceptable bound as fraction of median value;
            media_sig - error criterion as a multiple of error bar i.e. to fail the test, the magnitude of the
                        difference with respect to the median value must also exceed this number of error bars.
        """
        # Limit test
        median_tests_ws = '_tmp_abs_median_tests'
        fdol_alg = FindDetectorsOutsideLimits(data_ws, median_tests_ws, HighThreshold=max_value, LowThreshold=min_value)
        MaskDetectors(data_ws, MaskedWorkspace=fdol_alg.workspace())
        # Median tests
        median_test_alg = MedianDetectorTest(data_ws, median_tests_ws, LowThreshold=median_lbound, HighThreshold=median_ubound)
        MaskDetectors(data_ws, MaskedWorkspace=median_test_alg.workspace())
        median_test_alg = MedianDetectorTest(data_ws, median_tests_ws, SignificanceTest=median_sig,
                                             LowThreshold=median_frac_lo, HighThreshold=median_frac_hi)
        MaskDetectors(data_ws, MaskedWorkspace=median_test_alg.workspace())
        mtd.deleteWorkspace(median_tests_ws)

    def save_results(self, workspace, save_path, formats = None):
        """
        Save the result workspace to the specfied filename using the list of formats specified in 
        formats. If formats is None then the default list is used
        """
        if save_path is None:
            save_path = workspace.getName()
        elif os.path.isdir(save_path):
            save_path = os.path.join(save_path, workspace.getName())
        elif save_path == '':
            raise ValueError('Empty filename is not allowed for saving')
        else:
            pass

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
    

    #-------------------------------------------------------------------------------
    def load_data(self, runs, file_type):
        """
        Load a run or list of runs. If a list of runs is given then
        they are summed into one.
        """
        if type(runs) == list:
            result_ws = common.load_run(runs[0], file_type)
            if len(runs) > 1:
                del runs[0]
                common.sum_files(result_ws.getName(), runs)
                alg = RenameWorkspace(result_ws, 'summed-' + file_type)
                result_ws = alg.workspace()
        elif type(str):
            result_ws = common.load_run(runs, file_type)
        else:
            raise TypeError("Run number must be a list or a string")

        self.setup_mtd_instrument(result_ws)
        return result_ws

    #---------------------------------------------------------------------------
    # Behind the scenes stuff
    #---------------------------------------------------------------------------

    def __init__(self, instr_name):
        """
        Constructor
        """
        self._to_stdout = True
        self._log_to_mantid = False
        self._idf_values_read = False
        # Instrument and default parameter setup
        self.initialise(instr_name)

    def initialise(self, instr_name):
        """
        Initialise the attributes of the class
        """
        # Instrument name might be a prefix, query Mantid for the full name
        self.instr_name = mtd.settings.facility().instrument(instr_name).name()
        mtd.settings['default.instrument'] = self.instr_name
        self.setup_mtd_instrument()
        # Initialize the default parameters from the instrument as attributes of the
        # class
        self.init_params()

    def setup_mtd_instrument(self, workspace = None):
        if workspace != None:
            self.instrument = workspace.getInstrument()
        else:
            # Load an empty instrument
            idf_dir = mtd.getConfigProperty('instrumentDefinition.directory')
            instr_pattern = os.path.join(idf_dir,self.instr_name + '*_Definition.xml')
            idf_files = glob.glob(instr_pattern)
            if len(idf_files) > 0:
                tmp_ws_name = '_tmp_empty_instr'
                LoadEmptyInstrument(idf_files[0],tmp_ws_name)
                self.instrument = mtd[tmp_ws_name].getInstrument()
                # Instrument is cached so this is fine
                mtd.deleteWorkspace(tmp_ws_name)
            else:
                self.instrument = None
                raise RuntimeError('Cannot load instrument for prefix "%s"' % self.instr_name)
        # Initialise IDF parameters
        self.init_idf_params()

        
    def init_params(self):
        """
        Attach analysis arguments that are particular to the ElasticConversion 
        """
        self.save_formats = ['.spe','.nxs','.nxspe']
        self.fix_ei=False
        self.energy_bins = None
        self.background = False
        self.normalise_method = 'monitor-1'
        self.map_file = None
        
        if (self.instr_name == "CNCS" or self.instr_name == "ARCS" or self.instr_name == "SEQUOIA"):
            self.facility = "SNS"
            self.normalise_method  = 'current'
        else:
            self.facility = str(mtd.settings.facility())
        
        # The Ei requested
        self.ei_requested = None
        self.monitor_workspace = None
        
        self.time_bins = None
                
        # Detector diagnosis
        self.spectra_masks = None
        
        # Absolute normalisation
        self.abs_map_file = None
        self.abs_spectra_masks = None
        self.sample_mass = 1.0
        self.sample_rmm = 1.0
        self.apply_detector_eff = True
     
    def init_idf_params(self):
        """
        Initialise the parameters from the IDF file if necessary
        """
        if self._idf_values_read == True:
            return

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

        # Mark IDF files as read
        self._idf_values_read = True

    def get_default_parameter(self, name):
        if self.instrument is None:
            raise ValueError("Cannot init default parameter, instrument has not been loaded.")
        values = self.instrument.getNumberParameter(name)
        if len(values) != 1:
            raise ValueError('Instrument parameter file does not contain a definition for "%s". Cannot continue' % name)
        return values[0]
    
    def log(self, msg):
        """Send a log message to the location defined
        """
        if self._to_stdout:
            print msg
        if self._log_to_mantid:
            mtd.sendLogMessage(msg)

#-----------------------------------------------------------------
