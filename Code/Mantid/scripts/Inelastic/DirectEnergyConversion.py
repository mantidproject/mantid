"""
Conversion class defined for conversion to deltaE for
'direct' inelastic geometry instruments

The class defines various methods to allow users to convert their
files to DeltaE.

Example:

Assuming we have the following data files for MARI.
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
reducer.save_format = ['.spe']
#
Set parameters for these runs
reducer.map_file = 'mari_res.map'
reducer.energy_bins = '-10,0.1,80'

Run the conversion
deltaE_wkspace = reducer.convert_to_energy(11015, 85, 11060, 11001)
"""
import CommonFunctions as common
import diagnostics
from mantid.simpleapi import *
from mantid.kernel import funcreturns
from mantid import api
import glob
import sys
import os.path
import math

def setup_reducer(inst_name):
    """
    Given an instrument name or prefix this sets up a converter
    object for the reduction
    """
    try:
        return DirectEnergyConversion(inst_name)
    except RuntimeError, exc:
        raise RuntimeError('Unknown instrument "%s" or wrong IDF file for this instrument, cannot continue' % inst_name)


class DirectEnergyConversion(object):
    """
    Performs a convert to energy assuming the provided instrument is an elastic instrument
    """

    def diagnose(self, white, **kwargs):
        """
            Run diagnostics on the provided workspaces.

            This method does some additional processing before moving on to the diagnostics:
              1) Computes the white beam integrals, converting to energy
              2) Computes the background integral using the instrument defined range
              3) Computes a total count from the sample

            These inputs are passed to the diagnostics functions

            Required inputs:

              white  - A workspace, run number or filepath of a white beam run. A workspace is assumed to
                       have simple been loaded and nothing else.

            Optional inputs:
              sample - A workspace, run number or filepath of a sample run. A workspace is assumed to
                       have simple been loaded and nothing else. (default = None)
              second_white - If provided an additional set of tests is performed on this. (default = None)
              hard_mask  - A file specifying those spectra that should be masked without testing (default=None)
              tiny        - Minimum threshold for acceptance (default = 1e-10)
              huge        - Maximum threshold for acceptance (default = 1e10)
              background_test_range - A list of two numbers indicating the background range (default=instrument defaults)
              van_out_lo  - Lower bound defining outliers as fraction of median value (default = 0.01)
              van_out_hi  - Upper bound defining outliers as fraction of median value (default = 100.)
              van_lo      - Fraction of median to consider counting low for the white beam diag (default = 0.1)
              van_hi      - Fraction of median to consider counting high for the white beam diag (default = 1.5)
              van_sig  - Error criterion as a multiple of error bar i.e. to fail the test, the magnitude of the\n"
                          "difference with respect to the median value must also exceed this number of error bars (default=0.0)
              samp_zero    - If true then zeroes in the vanadium data will count as failed (default = True)
              samp_lo      - Fraction of median to consider counting low for the white beam diag (default = 0)
              samp_hi      - Fraction of median to consider counting high for the white beam diag (default = 2.0)
              samp_sig  - Error criterion as a multiple of error bar i.e. to fail the test, the magnitude of the\n"
                          "difference with respect to the median value must also exceed this number of error bars (default=3.3)
              variation  - The number of medians the ratio of the first/second white beam can deviate from
                           the average by (default=1.1)
              bleed_test - If true then the CreatePSDBleedMask algorithm is run
              bleed_maxrate - If the bleed test is on then this is the maximum framerate allowed in a tube
              bleed_pixels - If the bleed test is on then this is the number of pixels ignored within the
                             bleed test diagnostic
              print_results - If True then the results are printed to the screen
        """
        lhs_names = funcreturns.lhs_info('names')
        if len(lhs_names) > 0:
            var_name = lhs_names[0]
        else:
            var_name = "diag_mask"

        # Check for any keywords that have not been supplied and put in the values set on reducer
        for par in self.__diag_params:
            arg = par.lstrip('diag_')
            if arg not in kwargs:
                kwargs[arg] = getattr(self, arg)

        # if input parameter is workspace rather then run, we want to keep it
        self._keep_wb_workspace = False
        if isinstance(white,str) and white in mtd:
         self._keep_wb_workspace = True
        if isinstance(white,api.Workspace) : # it is workspace itself
            self._keep_wb_workspace = True

        # If we have a hard_mask, check the instrument name is defined
        if 'hard_mask_file' in kwargs:
            if 'instrument_name' not in kwargs:
                kwargs['instrument_name'] = self.instr_name

        if kwargs['use_hard_mask_only'] :
            if mtd.doesExist('hard_mask_ws'):
                diag_mask = mtd['hard_mask_ws']
            else: # build hard mask
                # in this peculiar way we can obtain working mask which accounts for initial data grouping in the data file.
                # SNS or 1 to 1 maps may probably avoid this stuff and can load masks directly
                whitews_name = common.create_resultname(white, suffix='-white')
                if whitews_name in mtd:
                    DeleteWorkspace(Workspace=whitews_name)
                # Load
                white_data = self.load_data(white,whitews_name,self._keep_wb_workspace)

                diag_mask= LoadMask(Instrument=self.instr_name,InputFile=kwargs['hard_mask_file'],
                               OutputWorkspace='hard_mask_ws')
                MaskDetectors(Workspace=white_data, MaskedWorkspace=diag_mask)
                diag_mask,masked_list = ExtractMask(InputWorkspace=white_data)
                DeleteWorkspace(Workspace=whitews_name)

            return diag_mask

        # Get the white beam vanadium integrals
        whiteintegrals = self.do_white(white, None, None,None) # No grouping yet
        if 'second_white' in kwargs:
            second_white = kwargs['second_white']
            if second_white is None:
                del kwargs['second_white']
            else:
                other_whiteintegrals = self.do_white(second_white, None, None,None) # No grouping yet
                kwargs['second_white'] = other_whiteintegrals


        # Get the background/total counts from the sample if present
        if 'sample' in kwargs:
            sample = kwargs['sample']
            del kwargs['sample']
            # If the bleed test is requested then we need to pass in the sample_run as well
            if kwargs.get('bleed_test', False):
                kwargs['sample_run'] = sample

            # Set up the background integrals
            result_ws = self.load_data(sample)
            result_ws = self.normalise(result_ws, result_ws.name(), self.normalise_method)
            if 'background_test_range' in kwargs:
                bkgd_range = kwargs['background_test_range']
                del kwargs['background_test_range']
            else:
                bkgd_range = self.background_test_range
            background_int = Integration(result_ws,
                                         RangeLower=bkgd_range[0],RangeUpper=bkgd_range[1],
                                         IncludePartialBins=True)
            total_counts = Integration(result_ws, IncludePartialBins=True)
            background_int = ConvertUnits(background_int, "Energy", AlignBins=0)
            self.log("Diagnose: finished convertUnits ")
            background_int *= self.scale_factor
            diagnostics.normalise_background(background_int, whiteintegrals, kwargs.get('second_white',None))
            kwargs['background_int'] = background_int
            kwargs['sample_counts'] = total_counts

        # Check how we should run diag
        if self.diag_spectra is None:
            # Do the whole lot at once
            diagnostics.diagnose(whiteintegrals, **kwargs)
        else:
            banks = self.diag_spectra.split(";")
            bank_spectra = []
            for b in banks:
                token = b.split(",")  # b = "(,)"
                if len(token) != 2:
                    raise ValueError("Invalid bank spectra specification in diag %s" % self.diag_spectra)
                start = int(token[0].lstrip('('))
                end = int(token[1].rstrip(')'))
                bank_spectra.append((start,end))

            for index, bank in enumerate(bank_spectra):
                kwargs['start_index'] = bank[0] - 1
                kwargs['end_index'] = bank[1] - 1
                diagnostics.diagnose(whiteintegrals, **kwargs)

        if 'sample_counts' in kwargs:
            DeleteWorkspace(Workspace='background_int')
            DeleteWorkspace(Workspace='total_counts')
        if 'second_white' in kwargs:
            DeleteWorkspace(Workspace=kwargs['second_white'])
        # Return a mask workspace
        diag_mask, det_ids = ExtractMask(InputWorkspace=whiteintegrals,OutputWorkspace=var_name)

        DeleteWorkspace(Workspace=whiteintegrals)
        self.spectra_masks = diag_mask
        return diag_mask


    def do_white(self, white_run, spectra_masks, map_file,mon_number=None):
        """
        Create the workspace, which each spectra containing the correspondent white beam integral (single value)

        These integrals are used as estimate for detector efficiency in wide range of energies
        (rather the detector electronic's efficiency as the geuger counters are very different in efficiency)
        and is used to remove the influence of this efficiency to the different detectors.
        """


        whitews_name = common.create_resultname(white_run, suffix='-white')
        if whitews_name in mtd:
            DeleteWorkspace(Workspace=whitews_name)
        # Load
        white_data = self.load_data(white_run,whitews_name,self._keep_wb_workspace)

        # Normalise
        self.__in_white_normalization = True;
        white_ws = self.normalise(white_data, whitews_name, self.normalise_method,0.0,mon_number)
        self.__in_white_normalization = False;

        # Units conversion
        white_ws = ConvertUnits(InputWorkspace=white_ws,OutputWorkspace=whitews_name, Target= "Energy", AlignBins=0)
        self.log("do_white: finished convertUnits ")
        # This both integrates the workspace into one bin spectra and sets up common bin boundaries for all spectra
        low = self.wb_integr_range[0]
        upp = self.wb_integr_range[1]
        if low > upp:
            raise ValueError("White beam integration range is inconsistent. low=%d, upp=%d" % (low,upp))
        delta = 2.0*(upp - low)
        white_ws = Rebin(InputWorkspace=white_ws,OutputWorkspace=whitews_name, Params=[low, delta, upp])
        # Why aren't we doing this...
        #Integration(white_ws, white_ws, RangeLower=low, RangeUpper=upp)

        # Masking and grouping
        white_ws = self.remap(white_ws, spectra_masks, map_file)

        # White beam scale factor
        white_ws *= self.wb_scale_factor
        return white_ws

    def mono_van(self, mono_van, ei_guess, white_run=None, map_file=None,
                 spectra_masks=None, result_name=None, Tzero=None):
        """Convert a mono vanadium run to DeltaE.
        If multiple run files are passed to this function, they are summed into a run and then processed
        """
        # Load data
        sample_data = self.load_data(mono_van)
        # Create the result name if necessary
        if result_name is None:
            result_name = common.create_resultname(mono_van)

        monovan = self._do_mono(sample_data, sample_data, result_name, ei_guess,
                                white_run, map_file, spectra_masks, Tzero)
        # Normalize by vanadium sample weight
        monovan /= float(self.van_mass)/float(self.van_rmm)
        return monovan

    def mono_sample(self, mono_run, ei_guess, white_run=None, map_file=None,
                    spectra_masks=None, result_name=None, Tzero=None):
        """Convert a mono-chromatic sample run to DeltaE.
        If multiple run files are passed to this function, they are summed into a run and then processed
        """
        # Load data
        sample_data = self.load_data(mono_run)
        # Create the result name if necessary
        if result_name is None:
            result_name = common.create_resultname(mono_run, prefix=self.instr_name)

        mono_s=self._do_mono(sample_data, sample_data, result_name, ei_guess,
                                  white_run, map_file, spectra_masks, Tzero)
        return mono_s


# -------------------------------------------------------------------------------------------
#         This actually does the conversion for the mono-sample and mono-vanadium runs
#
# -------------------------------------------------------------------------------------------
    def _do_mono_SNS(self, data_ws, monitor_ws, result_name, ei_guess,
                 white_run=None, map_file=None, spectra_masks=None, Tzero=None):

        # Special load monitor stuff.
        if (self.instr_name == "CNCS" or self.instr_name == "HYSPEC"):
            self.fix_ei = True
            ei_value = ei_guess
            if (self.instr_name == "HYSPEC"):
                Tzero=4.0 + 107.0 / (1+math.pow((ei_value/31.0),3.0))
                self.log("Determined T0 of %s for HYSPEC" % str(Tzero))
            if (Tzero is None):
                tzero = (0.1982*(1+ei_value)**(-0.84098))*1000.0
            else:
                tzero = Tzero
            # apply T0 shift
            ScaleX(InputWorkspace=data_ws,OutputWorkspace=result_name,Operation="Add",Factor=-tzero)
            mon1_peak = 0.0
        elif (self.instr_name == "ARCS" or self.instr_name == "SEQUOIA"):
            if 'Filename' in data_ws.getRun(): mono_run = data_ws.getRun()['Filename'].value
            else: raise RuntimeError('Cannot load monitors for event reduction. Unable to determine Filename from mono workspace, it should have been added as a run log.')

            self.log("mono_run = %s (%s)" % (mono_run,type(mono_run)),'debug')

            if mono_run.endswith("_event.nxs"):
                monitor_ws=LoadNexusMonitors(Filename=mono_run)
            elif mono_run.endswith("_event.dat"):
                InfoFilename = mono_run.replace("_neutron_event.dat", "_runinfo.xml")
                monitor_ws=LoadPreNexusMonitors(RunInfoFilename=InfoFilename)

            argi = {};
            argi['Monitor1Spec']=int(self.ei_mon_spectra[0]);
            argi['Monitor2Spec']=int(self.ei_mon_spectra[1]);
            argi['EnergyEstimate']=ei_guess;
            argi['FixEi'] =self.fix_ei
            if hasattr(self, 'ei_mon_peak_search_range'):
                argi['PeakSearchRange']=self.ei_mon_peak_search_range;

            try:
                ei_calc,firstmon_peak,firstmon_index,TzeroCalculated = \
                    GetEi(InputWorkspace=monitor_ws,**argi)
            except:
                self.log("Error in GetEi. Using entered values.")
                #monitor_ws.getRun()['Ei'] = ei_value
                ei_value = ei_guess
                AddSampleLog(Workspace=monitor_ws,LogName= 'Ei',LogText= ei_value,LogType= "Number")
                ei_calc = None
                TzeroCalculated = Tzero

            # Set the tzero to be the calculated value
            if (TzeroCalculated is None):
                tzero = 0.0
            else:
                tzero = TzeroCalculated

            # If we are fixing, then use the guess if given
            if (self.fix_ei):
                ei_value = ei_guess
                # If a Tzero has been entered, use it, if we are fixing.
                if (Tzero is not None):
                    tzero = Tzero
            else:
                if (ei_calc is not None):
                    ei_value = ei_calc
                else:
                    ei_value = ei_guess

            mon1_peak = 0.0
            # apply T0 shift
            ScaleX(InputWorkspace=data_ws,OutputWorkspace= result_name,Operation="Add",Factor=-tzero)
            self.incident_energy = ei_value
        else:
            # Do ISIS stuff for Ei
            # Both are these should be run properties really
            ei_value, mon1_peak = self.get_ei(monitor_ws, result_name, ei_guess)
            self.incident_energy = ei_value

        # As we've shifted the TOF so that mon1 is at t=0.0 we need to account for this in CalculateFlatBackground and normalisation
        bin_offset = -mon1_peak

        # For event mode, we are going to histogram in energy first, then go back to TOF
        if self.check_background== True:
           # Extract the time range for the background determination before we throw it away
           background_bins = "%s,%s,%s" % (self.bkgd_range[0] + bin_offset, (self.bkgd_range[1]-self.bkgd_range[0]), self.bkgd_range[1] + bin_offset)
           Rebin(InputWorkspace=result_name,OutputWorkspace= "background_origin_ws",Params=background_bins)

        # Convert to Et
        ConvertUnits(InputWorkspace=result_name,OutputWorkspace= "_tmp_energy_ws", Target="DeltaE",EMode="Direct", EFixed=ei_value)
        RenameWorkspace(InputWorkspace="_tmp_energy_ws",OutputWorkspace= result_name)
        # Histogram
        Rebin(InputWorkspace=result_name,OutputWorkspace= "_tmp_rebin_ws",Params= self.energy_bins, PreserveEvents=False)
        RenameWorkspace(InputWorkspace="_tmp_rebin_ws",OutputWorkspace= result_name)
        # Convert back to TOF
        ConvertUnits(InputWorkspace=result_name,OutputWorkspace=result_name, Target="TOF",EMode="Direct", EFixed=ei_value)

        if self.check_background == True:
            # Remove the count rate seen in the regions of the histograms defined as the background regions, if the user defined such region
            ConvertToDistribution(Workspace=result_name)

            CalculateFlatBackground(InputWorkspace="background_origin_ws",OutputWorkspace= "background_ws",
                               StartX= self.bkgd_range[0] + bin_offset,EndX= self.bkgd_range[1] + bin_offset,
                               WorkspaceIndexList= '',Mode= 'Mean',OutputMode= 'Return Background')
            # Delete the raw data background region workspace
            DeleteWorkspace("background_origin_ws")
            # Convert to distribution to make it compatible with the data workspace (result_name).
            ConvertToDistribution(Workspace="background_ws")
            # Subtract the background
            Minus(LHSWorkspace=result_name,RHSWorkspace= "background_ws",OutputWorkspace=result_name)
             # Delete the determined background
            DeleteWorkspace("background_ws")

            ConvertFromDistribution(Workspace=result_name)

        # Normalize using the chosen method
        # This should be done as soon as possible after loading and usually happens at diag. Here just in case if diag was bypassed
        self.normalise(mtd[result_name], result_name, self.normalise_method, range_offset=bin_offset)



        # This next line will fail the SystemTests
        #ConvertUnits(result_ws, result_ws, Target="DeltaE",EMode='Direct', EFixed=ei_value)
        # But this one passes...
        ConvertUnits(InputWorkspace=result_name,OutputWorkspace=result_name, Target="DeltaE",EMode='Direct')
        self.log("_do_mono: finished ConvertUnits for : "+result_name)



        if not self.energy_bins is None:
            Rebin(InputWorkspace=result_name,OutputWorkspace=result_name,Params= self.energy_bins,PreserveEvents=False)

        if self.apply_detector_eff:
           # Need to be in lambda for detector efficiency correction
            ConvertUnits(InputWorkspace=result_name,OutputWorkspace= result_name, Target="Wavelength", EMode="Direct", EFixed=ei_value)
            He3TubeEfficiency(InputWorkspace=result_name,OutputWorkspace=result_name)
            ConvertUnits(InputWorkspace=result_name,OutputWorkspace= result_name, Target="DeltaE",EMode='Direct', EFixed=ei_value)
        ############
        return

    def _do_mono_ISIS(self, data_ws, monitor_ws, result_name, ei_guess,
                 white_run=None, map_file=None, spectra_masks=None, Tzero=None):

        # Do ISIS stuff for Ei
        # Both are these should be run properties really
        ei_value, mon1_peak = self.get_ei(monitor_ws, result_name, ei_guess)
        self.incident_energy = ei_value

        # As we've shifted the TOF so that mon1 is at t=0.0 we need to account for this in CalculateFlatBackground and normalization
        bin_offset = -mon1_peak

        if self.check_background == True:
            # Remove the count rate seen in the regions of the histograms defined as the background regions, if the user defined such region
            CalculateFlatBackground(InputWorkspace=result_name,OutputWorkspace=result_name,
                                    StartX= self.bkgd_range[0] + bin_offset,EndX= self.bkgd_range[1] + bin_offset,
                                     WorkspaceIndexList= '',Mode= 'Mean',SkipMonitors='1')


        # Normalize using the chosen method+group
        # : This really should be done as soon as possible after loading
        self.normalise(mtd[result_name], result_name, self.normalise_method, range_offset=bin_offset)



        # This next line will fail the SystemTests
        #ConvertUnits(result_ws, result_ws, Target="DeltaE",EMode='Direct', EFixed=ei_value)
        # But this one passes...
        ConvertUnits(InputWorkspace=result_name,OutputWorkspace=result_name, Target="DeltaE",EMode='Direct')
        self.log("_do_mono: finished ConvertUnits for : "+result_name)



        if not self.energy_bins is None:
            Rebin(InputWorkspace=result_name,OutputWorkspace=result_name,Params= self.energy_bins,PreserveEvents=False)

        if self.apply_detector_eff:
           DetectorEfficiencyCor(InputWorkspace=result_name,OutputWorkspace=result_name)
           self.log("_do_mono: finished DetectorEfficiencyCor for : "+result_name)
        #############
        return

    def _do_mono(self, data_ws, monitor_ws, result_name, ei_guess,
                 white_run=None, map_file=None, spectra_masks=None, Tzero=None):
        """
        Convert units of a given workspace to deltaE, including possible
        normalization to a white-beam vanadium run.
        """
        if (self.__facility == "SNS"):
           self._do_mono_SNS(data_ws,monitor_ws,result_name,ei_guess,
                         white_run, map_file, spectra_masks, Tzero)
        else:
            self._do_mono_ISIS(data_ws,monitor_ws,result_name,ei_guess,
                               white_run, map_file, spectra_masks, Tzero)

        #######################
        # Ki/Kf Scaling...
        if self.apply_kikf_correction:
            self.log('Start Applying ki/kf corrections to the workspace : '+result_name)
            CorrectKiKf(InputWorkspace=result_name,OutputWorkspace= result_name, EMode='Direct')
            self.log('finished applying ki/kf corrections for : '+result_name)

        # Make sure that our binning is consistent
        if not self.energy_bins is None:
            Rebin(InputWorkspace=result_name,OutputWorkspace= result_name,Params= self.energy_bins)

        # Masking and grouping
        result_ws = mtd[result_name]
        result_ws = self.remap(result_ws, spectra_masks, map_file)


        ConvertToDistribution(Workspace=result_ws)
        # White beam correction
        if white_run is not None:
            white_ws = self.do_white(white_run, spectra_masks, map_file,None)
            result_ws /= white_ws

        # Overall scale factor
        result_ws *= self.scale_factor
        return result_ws


#-------------------------------------------------------------------------------

    def convert_to_energy(self, mono_run, ei, white_run=None, mono_van=None,\
                          abs_ei=None, abs_white_run=None, save_path=None, Tzero=None, \
                          motor=None, offset=None):
        """
        One-shot function to convert the given runs to energy
        """
        self.incident_energy = ei;
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
                norm_factor = self.calc_average(monovan_wkspace,ei)
            else:
                self.log("Performing Absolute Units Normalisation.")
                # Perform Abs Units...
                norm_factor = self.monovan_abs(monovan_wkspace)
            DeleteWorkspace(monovan_wkspace.name())
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

        #calculate psi from sample environment motor and offset

        if self.__facility == 'ISIS' :
            default_offset=float('NaN')
            if not (motor is None) and sample_wkspace.getRun().hasProperty(motor):
                default_offset = 0
        else:
            default_offset=0
        if (offset is None):
              self.motor_offset = default_offset
        else:
             self.motor_offset = float(offset)

        self.motor=0
        if not (motor is None):
        # Check if motor name exists
            if sample_wkspace.getRun().hasProperty(motor):
                self.motor=sample_wkspace.getRun()[motor].value[0]
                self.log("Motor value is %s" % self.motor)
            else:
                self.log("Could not find such sample environment log. Will use psi=offset")
        self.psi = self.motor+self.motor_offset

        # Save then finish
        self.save_results(sample_wkspace, save_path)
        # Clear loaded raw data to free up memory
        common.clear_loaded_data()

        return sample_wkspace

#----------------------------------------------------------------------------------
#                        Reduction steps
#----------------------------------------------------------------------------------

    def get_ei(self, input_ws, resultws_name, ei_guess):
        """
        Calculate incident energy of neutrons and the time of the of the
        peak in the monitor spectrum
        The X data is corrected to set the first monitor peak at t=0 by subtracting
            t_mon + t_det_delay
        where the detector delay time is retrieved from the the instrument
        The position of the "source" component is also moved to match the position of
        the first monitor
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

        self.incident_energy= ei_guess
        #-------------------------------------------------------------
        # check if monitors are in the main workspace or provided separately
        monitor_ws = input_ws;
        monitors_from_separate_ws=False;
        if type(monitor_ws) is str:
            monitor_ws = mtd[monitor_ws]
        try:
            # check if the spectra with correspondent number is present in the workspace
            nsp = monitor_ws.getIndexFromSpectrumNumber(int(self.ei_mon_spectra[0]));
        except RuntimeError as err:
            monitors_from_separate_ws = True
            mon_ws = monitor_ws.getName()+'_monitors'
            try:
                monitor_ws = mtd[mon_ws];
            except:
                print "**** ERROR while attempting to get spectra {0} from workspace: {1}, error: {2} ".format(self.ei_mon_spectra[0],monitor_ws.getName(), err)
                raise
        #------------------------------------------------

        # Calculate the incident energy
        ei,mon1_peak,mon1_index,tzero = \
            GetEi(InputWorkspace=monitor_ws, Monitor1Spec=int(self.ei_mon_spectra[0]), Monitor2Spec=int(self.ei_mon_spectra[1]),
                  EnergyEstimate=ei_guess,FixEi=self.fix_ei)

        self.incident_energy = ei
        # copy incident energy obtained on monitor workspace to detectors workspace
        if monitors_from_separate_ws:
            AddSampleLog(Workspace=input_ws,LogName='Ei',LogText=str(ei),LogType='Number')
            # if monitors are separated from the input workspace, we need to move them too as this is what happening when monitors are integrated into workspace
            result_mon_name=resultws_name+'_monitors';
            ScaleX(InputWorkspace=mon_ws,OutputWorkspace=result_mon_name,Operation="Add",Factor=-mon1_peak,
                   InstrumentParameter="DelayTime",Combine=True)


        # Adjust the TOF such that the first monitor peak is at t=0
        ScaleX(InputWorkspace=input_ws,OutputWorkspace=resultws_name,Operation="Add",Factor=-mon1_peak,
               InstrumentParameter="DelayTime",Combine=True)

        mon1_det = monitor_ws.getDetector(mon1_index)
        mon1_pos = mon1_det.getPos()
        src_name = input_ws.getInstrument().getSource().getName()
        MoveInstrumentComponent(Workspace=resultws_name,ComponentName= src_name, X=mon1_pos.getX(), Y=mon1_pos.getY(), Z=mon1_pos.getZ(), RelativePosition=False)
        return ei, mon1_peak

    def remap(self, result_ws, spec_masks, map_file):
        """
        Mask and group detectors based on input parameters
        """
        if not spec_masks is None:
            MaskDetectors(Workspace=result_ws, MaskedWorkspace=spec_masks)
        if not map_file is None:
            result_ws = GroupDetectors(InputWorkspace=result_ws,OutputWorkspace=result_ws,
                                       MapFile= map_file, KeepUngroupedSpectra=0, Behaviour='Average')

        return result_ws

    def getMonitorWS(self,data_ws,method,mon_number=None):
        """get pointer to monitor workspace.

           Explores different ways of finding monitor workspace in Mantid and returns the python pointer to the
           workspace which contains monitors.


        """
        if mon_number is None:
            if method == 'monitor-2':
               mon_spectr_num=int(self.mon2_norm_spec)
            else:
               mon_spectr_num=int(self.mon1_norm_spec)
        else:
               mon_spectr_num=mon_number

        monWS_name = data_ws.getName()+'_monitors'
        if monWS_name in mtd:
               mon_ws = mtd[monWS_name];
        else:
            # get pointer to the workspace
            mon_ws=data_ws;

            # get the index of the monitor spectra
            ws_index= mon_ws.getIndexFromSpectrumNumber(mon_spectr_num);
            # create monitor workspace consisting of single index
            mon_ws = ExtractSingleSpectrum(InputWorkspace=data_ws,OutputWorkspace=monWS_name,WorkspaceIndex=ws_index);

        mon_index = mon_ws.getIndexFromSpectrumNumber(mon_spectr_num);
        return (mon_ws,mon_index);



    def normalise(self, data_ws, result_name, method, range_offset=0.0,mon_number=None):
        """
        Apply normalization using specified source
        """
        if method is None :
            method = "undefined"
        if not method in self.__normalization_methods:
            raise KeyError("Normalization method: "+method+" is not among known normalization methods")

        # Make sure we don't call this twice
        method = method.lower()
        done_log = "DirectInelasticReductionNormalisedBy"

        if done_log in data_ws.getRun() or method == 'none':
            if data_ws.name() != result_name:
                CloneWorkspace(InputWorkspace=data_ws, OutputWorkspace=result_name)
            output = mtd[result_name]
            return output;

        if method == 'monitor-1' or method == 'monitor-2' :
            # get monitor's workspace
            try:
                mon_ws,mon_index = self.getMonitorWS(data_ws,method,mon_number)
            except :
                if self.__in_white_normalization: # we can normalize wb integrals by current separately as they often do not have monitors
                    method = 'current'
                else:
                    raise


        if method == 'monitor-1':
            range_min = self.norm_mon_integration_range[0] + range_offset
            range_max = self.norm_mon_integration_range[1] + range_offset


            output=NormaliseToMonitor(InputWorkspace=data_ws, OutputWorkspace=result_name, MonitorWorkspace=mon_ws, MonitorWorkspaceIndex=mon_index,
                                   IntegrationRangeMin=float(str(range_min)), IntegrationRangeMax=float(str(range_max)),IncludePartialBins=True)#,
# debug line:                                   NormalizationFactorWSName='NormMonWS'+data_ws.getName())
        elif method == 'monitor-2':

            # Found TOF range, correspondent to incident energy monitor peak;
            ei = self.incident_energy;
            x=[0.8*ei,ei,1.2*ei]
            y=[1]*2;
            range_ws=CreateWorkspace(DataX=x,DataY=y,UnitX='Energy',ParentWorkspace=mon_ws);
            range_ws=ConvertUnits(InputWorkspace=range_ws,Target='TOF',EMode='Elastic');
            x=range_ws.dataX(0);
            # Normalize to monitor 2
            output=NormaliseToMonitor(InputWorkspace=data_ws, OutputWorkspace=result_name, MonitorWorkspace=mon_ws, MonitorWorkspaceIndex=mon_index,
                                   IntegrationRangeMin=x[0], IntegrationRangeMax=x[2],IncludePartialBins=True)
# debug line:                      IntegrationRangeMin=x[0], IntegrationRangeMax=x[2],IncludePartialBins=True,NormalizationFactorWSName='NormMonWS'+data_ws.getName())

        elif method == 'current':
            NormaliseByCurrent(InputWorkspace=data_ws, OutputWorkspace=result_name)
            output = mtd[result_name]
        else:
            raise RuntimeError('Normalization scheme ' + reference + ' not found. It must be one of monitor-1, monitor-2, current, or none')

        # Add a log to the workspace to say that the normalization has been done
        AddSampleLog(Workspace=output, LogName=done_log,LogText=method)
        return output

    def calc_average(self, data_ws,energy_incident):
        """
        Compute the average Y value of a workspace.

        The average is computed by collapsing the workspace to a single bin per spectra then masking
        masking out detectors given by the FindDetectorsOutsideLimits and MedianDetectorTest algorithms.
        The average is then the computed as the using the remainder and factoring in their errors as weights, i.e.

            average = sum(Yvalue[i]*weight[i]) / sum(weights)

        where only those detectors that are unmasked are used and the weight[i] = 1/errorValue[i].
        """

        if self.monovan_integr_range is None :
            self.monovan_integr_range = [self.monovan_lo_frac*energy_incident,self.monovan_hi_frac*energy_incident];


        e_low = self.monovan_integr_range[0]
        e_upp = self.monovan_integr_range[1]
        if e_low > e_upp:
            raise ValueError("Inconsistent mono-vanadium integration range defined!")
        data_ws=Rebin(InputWorkspace=data_ws,OutputWorkspace=data_ws,Params= [e_low, 2.*(e_upp-e_low), e_upp])
        data_ws=ConvertToMatrixWorkspace(InputWorkspace=data_ws,OutputWorkspace= data_ws)

        args = {}
        args['tiny'] = self.tiny
        args['huge'] = self.huge
        args['van_out_lo'] = self.monovan_lo_bound
        args['van_out_hi'] = self.monovan_hi_bound
        args['van_lo'] = self.monovan_lo_frac
        args['van_hi'] = self.monovan_hi_frac
        args['van_sig'] = self.samp_sig
        args['use_hard_mask_only']=self.use_hard_mask_only;
        args['print_results'] = False

        diagnostics.diagnose(data_ws, **args)
        monovan_masks,det_ids = ExtractMask(InputWorkspace=data_ws,OutputWorkspace='monovan_masks')
        MaskDetectors(Workspace=data_ws, MaskedWorkspace=monovan_masks)
        DeleteWorkspace(Workspace=monovan_masks)

        ConvertFromDistribution(Workspace=data_ws)
        nhist = data_ws.getNumberHistograms()
        average_value = 0.0
        weight_sum = 0.0
        ic =0
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
            ic = ic+1;
            weight = 1.0/data_ws.readE(i)[0]
            average_value += y_value * weight
            weight_sum += weight

        if weight_sum == 0:
            raise RuntimeError(str.format(" Vanadium weighth calculated from {0} spectra out of {1} histohrams is equal to 0 and sum: {2} Check vanadium intergation ranges or diagnostic settings ",str(ic),str(nhist),str(average_value)))
        return average_value / weight_sum

    def monovan_abs(self, ei_workspace):
        """Calculates the scaling factor required for normalisation to absolute units.
        The given workspace must contain an Ei value. This will have happened if GetEi
        has been run
        """
        #  Scale by vanadium cross-section which is energy dependent up to a point
        run = ei_workspace.getRun()
        try:
            ei_prop = run['Ei']
        except KeyError:
            raise RuntimeError('The given workspace "%s" does not contain an Ei value. Run GetEi first.' % str(ei_workspace))

        ei_value = ei_prop.value
        absnorm_factor = self.calc_average(ei_workspace,ei_value)


        if ei_value >= 200.0:
            xsection = 421.0
        else:
            xsection = 400.0 + (ei_value/10.0)
        absnorm_factor /= xsection
        return absnorm_factor * (float(self.sample_mass)/float(self.sample_rmm))

    def save_results(self, workspace, save_file=None, formats = None):
        """
        Save the result workspace to the specfied filename using the list of formats specified in
        formats. If formats is None then the default list is used
        """
        if save_file is None:
            save_file = workspace.getName()
        elif os.path.isdir(save_file):
            save_file = os.path.join(save_file, workspace.getName())
        elif save_file == '':
            raise ValueError('Empty filename is not allowed for saving')
        else:
            pass

        if formats is None:
            formats = self.save_format
        if type(formats) == str:
            formats = [formats]



        # if ext is none, no need to write anything
        if formats is None:
            return

        save_file = os.path.splitext(save_file)[0]
        # this is mainly for debugging purposes as real save do not return anything

        for ext in formats:
            if ext in self.__save_formats :
                filename = save_file + ext
                self.__save_formats[ext](workspace,filename)
            else:
                self.log("Unknown file format {0} requested while saving results.".format(ext))

    #-------------------------------------------------------------------------------
    def load_data(self, runs,new_ws_name=None,keep_previous_ws=False):
        """
        Load a run or list of runs. If a list of runs is given then
        they are summed into one.
        """

        # this can be a workspace too
        calibration = self.det_cal_file;
        monitors_inWS=self.load_monitors_with_workspace
        result_ws = common.load_runs(self.instr_name, runs, calibration=calibration, sum=True,load_with_workspace=monitors_inWS)

        mon_WsName = result_ws.getName()+'_monitors'
        if mon_WsName in mtd:
            monitor_ws = mtd[mon_WsName];
        else:
            monitor_ws = None;


        if new_ws_name != None :
            mon_WsName = new_ws_name+'_monitors'
            if keep_previous_ws:
                result_ws = CloneWorkspace(InputWorkspace = result_ws,OutputWorkspace = new_ws_name)
                if monitor_ws:
                    monitor_ws = CloneWorkspace(InputWorkspace = monitor_ws,OutputWorkspace = mon_WsName)
            else:
                result_ws = RenameWorkspace(InputWorkspace=result_ws,OutputWorkspace=new_ws_name)
                if monitor_ws:
                    monitor_ws=RenameWorkspace(InputWorkspace=monitor_ws,OutputWorkspace=mon_WsName)

        self.setup_mtd_instrument(result_ws)
        if monitor_ws and self.spectra_to_monitors_list:
            for specID in self.spectra_to_monitors_list:
                self.copy_spectrum2monitors(result_ws,monitor_ws,specID);

        return result_ws

    @staticmethod
    def copy_spectrum2monitors(wsName,monWSName,spectraID):
       """
        this routine copies a spectrum form workspace to monitor workspace and rebins it according to monitor workspace binning

        @param wsName    -- the name of event workspace which detector is considered as monitor or Mantid pointer to this workspace
        @param monWSName -- the name of histogram workspace with monitors where one needs to place the detector's spectra or Mantid pointer to this workspace
        @param spectraID -- the ID of the spectra to copy.

         TODO: As extract sinele spectrum works only with WorkspaceIndex, we have to assume that WorkspaceIndex=spectraID-1;
         this does not always correct, so it is better to change ExtractSingleSpectrum to accept workspaceID
       """
       if isinstance(wsName,str):
            ws = mtd[wsName];
       else:
           ws = wsName;
       if isinstance(monWSName,str):
            monWS = mtd[monWSName];
       else:
           monWS = monWSName;
       # ----------------------------
       ws_index = spectraID-1;
       done_log_name = 'Copied_mon:'+str(ws_index);
       if done_log_name in monWS.getRun():
           return;
       #
       x_param = monWS.readX(0);
       bins = [x_param[0],x_param[1]-x_param[0],x_param[-1]];
       ExtractSingleSpectrum(InputWorkspace=ws,OutputWorkspace='tmp_mon',WorkspaceIndex=ws_index)
       Rebin(InputWorkspace='tmp_mon',OutputWorkspace='tmp_mon',Params=bins,PreserveEvents='0')
       # should be vice versa but Conjoin invalidate ws pointers and hopefully nothing could happen with workspace during conjoining
       AddSampleLog(Workspace=monWS,LogName=done_log_name,LogText=str(ws_index),LogType='Number');
       ConjoinWorkspaces(InputWorkspace1=monWS,InputWorkspace2='tmp_mon')

       if 'tmp_mon' in mtd:
           DeleteWorkspace(WorkspaceName='tmp_mon');

    #---------------------------------------------------------------------------
    # Behind the scenes stuff
    #---------------------------------------------------------------------------

    def __init__(self, instr_name=None):
        """
        Constructor
        """
        self._to_stdout = True
        self.__in_white_normalization=False; # variable use in normalize method to check if normalization can be changed from the requested;
        self._log_to_mantid = False
        self._idf_values_read = False
        self._keep_wb_workspace=False #  when input data for reducer is wb workspace rather then run number, we want to keep this workspace. But usually not
        self.spectra_masks = None;

        if not (instr_name is None or len(instr_name)==0 or instr_name == '__empty_') : # first time run or empty run
            self.initialise(instr_name)
        else: # just reinitialize idf parameters to defaults
            self.init_idf_params(True);


#----------------------------------------------------------------------------------
#              Complex setters/getters
#----------------------------------------------------------------------------------
    @property
    def load_monitors_with_workspace(self):
        if hasattr(self,'_load_monitors_with_workspace'):
            return self._load_monitors_with_workspace;
        else:
            return False;
    @load_monitors_with_workspace.setter
    def load_monitors_with_workspace(self,val):
        try:
            if val>0:
                do_load=True;
            else:
                do_load=False;
        except ValueError:
                do_load=False;

        setattr(self,'_load_monitors_with_workspace',do_load);

    @property
    def ei_mon_spectra(self):

        if hasattr(self,'_ei_mon1_spectra'):
            s1 = self._ei_mon1_spectra;
        else:
            s1 = self.get_default_parameter('ei-mon1-spec');
        if hasattr(self,'_ei_mon2_spectra'):
            s2 = self._ei_mon2_spectra;
        else:
            s2 = self.get_default_parameter('ei-mon2-spec');
        spectra = [s1,s2]
        return spectra;
    @ei_mon_spectra.setter
    def ei_mon_spectra(self,val):
        if val is None:
            if hasattr(self,'_ei_mon1_spectra'):
                delattr(self,'_ei_mon1_spectra')
            if hasattr(self,'_ei_mon2_spectra'):
                delattr(self,'_ei_mon2_spectra')
            return;
        if not isinstance(val,list) or len(val) != 2:
            raise SyntaxError(' shoud assign list of two integers numbers here')
        if not math.isnan(val[0]):
            setattr(self,'_ei_mon1_spectra',val[0])
        if not math.isnan(val[1]):
            setattr(self,'_ei_mon2_spectra',val[1])


    @property
    def det_cal_file(self):
        if hasattr(self,'_det_cal_file'):
            return self._det_cal_file
        return None;

    @det_cal_file.setter
    def det_cal_file(self,val):

       if val is None:
           self._det_cal_file = None;
           return;

       if isinstance(val,api.Workspace):
          # workspace provided
          self._det_cal_file = val;
          return;

        # workspace name
       if str(val) in mtd:
          self._det_cal_file = mtd[str(val)];
          return;

       # file name probably provided
       if isinstance(val,str):
          if val is 'None':
              val = None;
          self._det_cal_file = val;
          return;

       if isinstance(val,list) and len(val)==0:
           self._det_cal_file = None;
           return;

       if isinstance(val,int):
          self._det_cal_file = common.find_file(val);
          return;


       raise NameError('Detector calibration file name can be a workspace name present in Mantid or string describing file name');

    @property
    def spectra_to_monitors_list(self):
        if not hasattr(self,'_spectra_to_monitors_list'):
           return None;
        return self._spectra_to_monitors_list;
    @spectra_to_monitors_list.setter
    def spectra_to_monitors_list(self,spectra_list):
        """ Sets copy spectra to monitors variable as a list of monitors using different forms of input

        """
        if spectra_list is None:
            self._spectra_to_monitors_list=None;
            return;

        if isinstance(spectra_list,str):
            if spectra_list is 'None':
                self._spectra_to_monitors_list=None;
            else:
                spectra = spectra_list.split(',');
                self._spectra_to_monitors_list = [];
                for spectum in spectra :
                    self._spectra_to_monitors_list.append(int(spectum));
        else:
            if isinstance(spectra_list,list):
                if len(spectra_list) == 0:
                    self._spectra_to_monitors_list=None;
                else:
                    self._spectra_to_monitors_list=[];
                    for i in range(0,len(spectra_list)):
                        self._spectra_to_monitors_list.append(int(spectra_list[i]));
            else:
                self._spectra_to_monitors_list =[int(spectra_list)];
        return;
    @property
    def instr_name(self):
        return self._instr_name
    @instr_name.setter
    def instr_name(self,new_name):

       if not hasattr(self,'instr_name') :
           self._instr_name=None

       if new_name is None:
           return

       # Instrument name might be a prefix, query Mantid for the full name
       short_name=''
       full_name=''
       try :
        instrument = config.getFacility().instrument(new_name)
        short_name = instrument.shortName()
        full_name = instrument.name()
       except:
           # it is possible to have wrong facility:
           facilities = config.getFacilities()
           old_facility = str(config.getFacility())
           for facility in facilities:
               config.setString('default.facility',facility.name())
               try :
                   instrument = facility.instrument(new_name)
                   short_name = instrument.shortName()
                   full_name = instrument.name()
                   if len(short_name)>0 :
                       break
               except:
                   pass
           if len(short_name)==0 :
            config.setString('default.facility',old_facility)
            raise KeyError(" Can not find/set-up the instrument: "+new_name+' in any supported facility')

       new_name = short_name
       self.__facility = str(config.getFacility())
       if new_name == self.instr_name:
           return

       self._instr_name = new_name
       config['default.instrument'] = full_name



       if not hasattr(self,'instument') or self.instrument.getName() != instr_name :
            # Load an empty instrument if one isn't already there
            idf_dir = config.getString('instrumentDefinition.directory')
            try:
                idf_file=api.ExperimentInfo.getInstrumentFilename(new_name)
                tmp_ws_name = '__empty_' + new_name
                if not mtd.doesExist(tmp_ws_name):
                    LoadEmptyInstrument(Filename=idf_file,OutputWorkspace=tmp_ws_name)
                self.instrument = mtd[tmp_ws_name].getInstrument()
            except:
                self.instrument = None
                self._instr_name = None
                raise RuntimeError('Cannot load instrument for prefix "%s"' % new_name)


       # Initialise other IDF parameters
       self.init_idf_params(True)




    # Vanadium rmm
    @property
    def van_rmm(self):
      """The rmm of Vanadium is a constant, should not be instrument parameter. Atom not exposed to python :( """
      return 50.9415
    @van_rmm.deleter
    def van_rmm(self):
        pass

    @property
    def facility(self):
        return self.__facility

    # integration range for monochromatic vanadium,
    @property
    def monovan_integr_range(self):
        if not hasattr(self,'_monovan_integr_range') or self._monovan_integr_range is None:
            ei = self.incident_energy
            range = [self.monovan_lo_frac*ei,self.monovan_hi_frac*ei]
            return range
        else:
            return self._monovan_integr_range
    @monovan_integr_range.setter
    def monovan_integr_range(self,value):
        self._monovan_integr_range = value


    # format to save data
    @property
    def save_format(self):
        return self._save_format
    @save_format.setter
    def save_format(self, value):
    # user can clear save formats by setting save_format=None or save_format = [] or save_format=''
    # if empty string or empty list is provided as part of the list, all save_format-s set up earlier are cleared

    # clear format by using None
        if value is None:
            self._save_format = None
            return
    # check string, if it is empty, clear save format, if not -- continue
        if isinstance(value,str):
            if value not in self.__save_formats :
                self.log("Trying to set saving in unknown format: \""+str(value)+"\" No saving will occur for this format")
                if len(value) == 0: # user wants to clear internal save formats
                   self._save_format = None
                return
        elif isinstance(value,list):
            if len(value) > 0 :
                # set single default save format recursively
                for val in value:
                    self.save_format = val
                return;
      # clear all previous formats by providing empty list
            else:
                self._save_format = None;
                return

        # here we came to setting list of save formats
        if self._save_format is None:
            self._save_format = [];

        self._save_format.append(value);

    # bin ranges
    @property
    def energy_bins(self):
        return self._energy_bins;
    @energy_bins.setter
    def energy_bins(self,value):
       if value != None:
          if isinstance(value,str):
             list = str.split(value,',');
             nBlocks = len(list);
             for i in xrange(0,nBlocks,3):
                value = [float(list[i]),float(list[i+1]),float(list[i+2])]
          else:
              nBlocks = len(value);
          if nBlocks%3 != 0:
               raise KeyError("Energy_bin value has to be either list of n-blocks of 3 number each or string representation of this list with numbers separated by commas")
       #TODO: implement single value settings according to rebin
       self._energy_bins= value

    # map file name
    @property
    def map_file(self):
        return self._map_file;
    @map_file.setter
    def map_file(self,value):
        if value != None:
           fileName, fileExtension = os.path.splitext(value)
           if (not fileExtension):
               value=value+'.map'

        self._map_file = value

    # monovanadium map file name
    @property
    def monovan_mapfile(self):
        return self._monovan_mapfile;
    @monovan_mapfile.setter
    def monovan_mapfile(self,value):
        if value != None:
           fileName, fileExtension = os.path.splitext(value)
           if (not fileExtension):
               value=value+'.map'

        self._monovan_mapfile = value
    @property
    def relocate_dets(self) :
        if self.det_cal_file != None:
            return True
        else:
            return False

    @property
    def normalise_method(self):
        return self._normalise_method
    @normalise_method.setter
    def normalise_method(self,value):
        if value is None:
            value = 'none'
        if not isinstance(value,str):
            self.log('Normalization method should be a string or None','error')

        value = value.lower()
        if value in self.__normalization_methods:
            self._normalise_method = value
        else:
            self.log('Attempt to set up unknown normalization method {0}'.format(value),'error')
            raise KeyError('Attempt to set up unknown normalization method {0}'.format(value))

    @property
    def background_test_range(self):
        if not hasattr(self,'_background_test_range') or self._background_test_range is None:
            return self.bkgd_range;
        else:
            return self._background_test_range
    @background_test_range.setter
    def background_test_range(self,value):
        if value is None:
            self._background_test_range=None
            return
        if isinstance(value,str):
            value = str.split(value,',')
        if len(value) != 2:
            raise ValueError("background test range can be set to a 2 element list of floats")

        self._background_test_range=[float(value[0]),float(value[1])]


    def initialise(self, instr_name,reload_instrument=False):
        """
        Initialise the private attributes of the class and the nullify the attributes which expected to be always set-up from a calling script
        """

        # Instrument and default parameter setup
        # fomats availible for saving. As the reducer has to have a method to process one of this, it is private property
        self.__save_formats = {}
        self.__save_formats['.spe']   = lambda workspace,filename : SaveSPE(InputWorkspace=workspace,Filename= filename)
        self.__save_formats['.nxspe'] = lambda workspace,filename : SaveNXSPE(InputWorkspace=workspace,Filename= filename, KiOverKfScaling=self.apply_kikf_correction,Psi=self.psi)
        self.__save_formats['.nxs']   = lambda workspace,filename : SaveNexus(InputWorkspace=workspace,Filename= filename)
        ## Detector diagnosis
        # Diag parameters -- keys used by diag method to pick from default parameters. Diag cuts these keys removing diag_ word
        # and tries to get rest from the correspondent Direct Energy conversion attributes.
        self.__diag_params = ['diag_tiny', 'diag_huge', 'diag_samp_zero', 'diag_samp_lo', 'diag_samp_hi','diag_samp_sig',\
                              'diag_van_out_lo', 'diag_van_out_hi', 'diag_van_lo', 'diag_van_hi', 'diag_van_sig', 'diag_variation',\
                              'diag_bleed_test','diag_bleed_pixels','diag_bleed_maxrate','diag_hard_mask_file','diag_use_hard_mask_only','diag_background_test_range']

        # before starting long run, makes sence to verify if all files requested for the run are in fact availible. Here we specify the properties which describe these files
        self.__file_properties = ['det_cal_file','map_file','hard_mask_file']
        self.__abs_norm_file_properties = ['monovan_mapfile']

        self.__normalization_methods=['none','monitor-1','monitor-2','current'] # 'uamph', peak -- disabled/unknown at the moment

        # list of the parameters which should usually be changed by user and if not, user should be warn about it.
        self.__abs_units_par_to_change=['sample_mass','sample_rmm']
        # list of the parameters which should always be taken from IDF unless explicitly set from elsewhere. These parameters MUST have setters and getters
        self.__instr_par_located =['ei_mon_spectra']

        # set/reset instument name in case if this function is called separately. May call lengthy procedure of changing instrument
        self.instr_name = instr_name

    def setup_mtd_instrument(self, workspace = None,reload_instrument=False):
        if workspace != None:
            # TODO: Check it!
            self.instrument = workspace.getInstrument()
            self.instr_name = self.instrument.getName()


    def init_idf_params(self, reinitialize_parameters=False):
        """
        Initialise some default parameters and add the one from the IDF file
        """
        if self._idf_values_read == True and reinitialize_parameters == False:
            return

        """
        Attach analysis arguments that are particular to the ElasticConversion

        specify some parameters which may be not in IDF Parameters file
        """
       # mandatrory command line parameter. Real value can not vbe negative
        self.incident_energy= -666
        # mandatrory command line parameter
        self.energy_bins = None

          # should come from Mantid
        # Motor names-- SNS stuff -- psi used by nxspe file
        # These should be reconsidered on moving into _Parameters.xml
        self.monitor_workspace = None  # looks like unused parameter
        self.motor = None
        self.motor_offset = None
        self.psi = float('NaN')

        if self.__facility == 'SNS':
            self.normalise_method  = 'current'



        # special property -- synonims -- how to treat external parameters.
        try:
            self.__synonims = self.get_default_parameter("synonims")
        except Exception:
            self.__synonims=dict();

        par_names = self.instrument.getParameterNames()
        if len(par_names) == 0 :
            raise RuntimeError("Can not obtain instrument parameters describing inelastic conversion ")

        # build the dictionary of necessary allowed substitution names and substitute parameters with their values
        self.__synonims = self.build_subst_dictionary(self.__synonims)

        # build the dictionary which allows to process coupled property, namely the property, expressed through other properties values
        self.build_coupled_keys_dict(par_names)

        # Add IDF parameters as properties to the reducer
        self.build_idf_parameters(par_names)

        if reinitialize_parameters:
            # clear Instrument Parameters defined fields:
            for name in self.__instr_par_located:
                setattr(self,name,None);


        # Mark IDF files as read
        self._idf_values_read = True

    def check_abs_norm_defaults_changed(self,changed_param_list) :
        """ Method checks if the parameters mentioned as need to changed were indeed changed from defaults
            If not changed, warn users about it
        """
        n_warnings =0
        for key in self.__abs_units_par_to_change:
            if key not in changed_param_list :
                value = getattr(self,key)
                message = '\n***WARNING!: Absolute units reduction parameter : '+key + ' has its default value: '+str(value)+\
                          '\n             This may need to change for correct absolute units'
                n_warnings += 1
                self.log(message,'warning')


        return n_warnings

    def get_default_parameter(self, name):
        instr = self.instrument;
        if instr is None:
            raise ValueError("Cannot init default parameter, instrument has not been loaded.")

        type_name = instr.getParameterType(name)
        if type_name == "double":
            val = instr.getNumberParameter(name)
        elif type_name == "bool":
            val = instr.getBoolParameter(name)
        elif type_name == "string":
            val = instr.getStringParameter(name)
            if val[0] == "None" :
                return None
        elif type_name == "int" :
              val = instr.getIntParameter(name)
        else :
            raise KeyError(" Can not find property with name "+name)

        return val[0]

    @staticmethod
    def build_subst_dictionary(synonims_list=None) :
        """Method to process the field "synonims_list" in the parameters string

           it takes string of synonyms in the form key1=subs1=subst2=subts3;key2=subst4 and returns the dictionary
           in the form dict[subs1]=key1 ; dict[subst2] = key1 ... dict[subst4]=key2

           e.g. if one wants to use the IDF key word my_detector instead of e.g. norm-mon1-spec, he has to type
           norm-mon1-spec=my_detector in the synonims field of the IDF parameters file.
        """
        if not synonims_list :  # nothing to do
            return dict();
        if type(synonims_list) == dict : # all done
            return synonims_list
        if type(synonims_list) != str :
            raise AttributeError("The synonims field of Reducer object has to be special format string or the dictionary")
        # we are in the right place and going to transform string into dictionary

        subst_lines = synonims_list.split(";")
        rez  = dict()
        for lin in subst_lines :
            lin=lin.strip()
            keys = lin.split("=")
            if len(keys) < 2 :
                raise AttributeError("The pairs in the synonims fields have to have form key1=key2=key3 with at least two values present")
            if len(keys[0]) == 0:
                raise AttributeError("The pairs in the synonims fields have to have form key1=key2=key3 with at least two values present, but the first key is empty")
            for i in xrange(1,len(keys)) :
                if len(keys[i]) == 0 :
                    raise AttributeError("The pairs in the synonims fields have to have form key1=key2=key3 with at least two values present, but the key"+str(i)+" is empty")
                kkk = keys[i].strip();
                rez[kkk]=keys[0].strip()

        return rez;

    def set_input_parameters(self,**kwargs):
        """ Method analyzes input parameters list, substitutes the synonims in this list with predefined synonims
            and sets the existing class parameters with its non-default values taken from input

            returns the list of changed properties.
        """

        properties_changed=[];
        for par_name,value in kwargs.items() :

            if par_name in self.__synonims :
                par_name = self.__synonims[par_name]
            # may be a problem, one tries to set up non-existing value
            if not (hasattr(self,par_name) or hasattr(self,'_'+par_name)):
                # it still can be a composite key which sets parts of the composite property
                if par_name in self.composite_keys_subst :
                    composite_prop_name,index,nc = self.composite_keys_subst[par_name]
                    val = getattr(self,composite_prop_name)
                    val[index] = value;
                    setattr(self,composite_prop_name,val)
                    properties_changed.append(composite_prop_name)
                    continue
                else:
                    raise KeyError("Attempt to set unknown parameter: "+par_name)
            # whole composite key is modified by input parameters
            if par_name in self.composite_keys_set :
               default_value = getattr(self,par_name) # get default value
               if isinstance(value,str) and value.lower()[0:7] == 'default' : # Property changed but default value requesed explicitly
                   value = default_value
               if type(default_value) != type(value):
                   raise KeyError("Attempt to change range property: "+par_name+" of type : "+str(type(default_value))+ " with wrong type value: "+str(type(value)))
               if len(default_value) != len(value) :
                    raise KeyError("Attempt to change range property : "+par_name+" with default value: ["+",".join(str(vv) for vv in val)+
                                   "] to wrong length value: ["+",".join(str(vv) for vv in value)+"]\n")
               else:
                   setattr(self,par_name,value)
                   properties_changed.append(par_name)
                   continue

            # simple case of setting simple value
            if isinstance(value,str) and value.lower()[0:7] == 'default' : # Property changed but default value requesed explicitly
                value = getattr(self,par_name)


            setattr(self,par_name,value)
            properties_changed.append(par_name)

        # some properties have collective meaning
        if self.use_hard_mask_only and self.hard_mask_file is None:
          if self.run_diagnostics:
              self.run_diagnostics=False;
              properties_changed.append('run_diagnostics');

        return properties_changed


    def build_coupled_keys_dict(self,par_names) :
        """Method to build the dictionary of the keys which are expressed through other keys values

          e.g. to substitute key1 = key2,key3  with key = [value[key1],value[key2]]
        """
        instr = self.instrument;
        if instr is None:
            raise ValueError("Cannot init default parameter, instrument has not been loaded.")

        # dictinary used for substituting composite keys values.
        composite_keys_subst = dict();
        # set of keys which are composite keys
        composite_keys_set = set();


        for name in par_names :
            if instr.getParameterType(name)=="string":
               val = self.get_default_parameter(name)
               if val is None :
                   continue
               val = val.strip()
               keys = val.split(":")
               n_keys = len(keys)
               if n_keys>1 : # this is the property we want
                   for i in xrange(0,len(keys)) :
                       key = keys[i];
                       if key in self.__synonims:
                           key = self.__synonims[key];

                       final_name = name
                       if final_name in self.__synonims:
                           final_name = self.__synonims[name];

                       composite_keys_subst[key] = (final_name,i,n_keys);
                       composite_keys_set.add(name)

        self.composite_keys_subst = composite_keys_subst
        self.composite_keys_set   = composite_keys_set

    def build_idf_parameters(self,list_param_names) :
        """Method to process idf parameters, substitute duplicate values and
           add the attributes with the names, defined in IDF to the object

           also sets composite names through component values, e.g creating
           self.key1 = [value[key1],value[key2]] where key1 was defined as key1 = key2,key3

           @param Reducer object with defined coposite_keys dictionary and synonimus dictionary
           @param list of parameter names to transform
        """
        instr = self.instrument;
        if instr is None:
            raise ValueError("Cannot init default parameter, instrument has not been loaded.")

        new_comp_name_set = set();
        # process all default parameters and create property names from them
        for name in list_param_names :

            key_name = name;
            if key_name == 'synonims' : # this is special key we have already dealt with
                continue

            if key_name in self.__synonims: # replace name with its equivalent
                key_name = self.__synonims[name]
            # this key has to be always in IDF
            if key_name in self.__instr_par_located:
                continue;

            # redefine compostie keys set through synonims names for the future usage
            if name in self.composite_keys_set and not(name in self.__instr_par_located):
                new_comp_name_set.add(key_name)
                continue # komposite names are created through their values

             # create or fill in additional values to the composite key
            if key_name in self.composite_keys_subst :
                composite_prop_name,index,nc = self.composite_keys_subst[key_name]
                if composite_prop_name in self.__instr_par_located:
                    continue;

                if not hasattr(self,composite_prop_name): # create new attribute value
                    val = [float('nan')]*nc;
                else:              # key is already created, get its value
                    val = getattr(self,composite_prop_name)
                    if val is None: # some composie property names were set to none. leave them this way.
                        continue

                val[index] = self.get_default_parameter(name)
                setattr(self,composite_prop_name, val);
            else :
                # just new ordinary key, assighn the value to it
            #if hasattr(self,key_name):
            #    raise KeyError(" Dublicate key "+key_name+" found in IDF property file")
            #else:

                setattr(self,key_name, self.get_default_parameter(name));

        # reset the list of composite names defined using synonims
        self.composite_keys_set=new_comp_name_set

    @staticmethod
    def make_ckpt_name(*argi) :
        """ Make the name of the checkpoint from the function arguments
        """
        return ''.join(str(arg) for arg in argi if arg is not None)

    def check_necessary_files(self,monovan_run):
        """ Method verifies if all files necessary for a run are availible.

           usefull for long runs to check if all files necessary for it are present/accessible
        """

        def check_files_list(files_list):
            file_missing = False
            for prop in files_list :
                file = getattr(self,prop)
                if not (file is None) and isinstance(file,str):
                    file_path = FileFinder.getFullPath(file)
                    if len(file_path) == 0:
                        # it still can be run number
                        try:
                            file_path = common.find_file(file)
                        except:
                            file_path=''

                        if len(file_path)==0:
                            self.log(" Can not find file ""{0}"" for property: {1} ".format(file,prop),'error')
                            file_missing=True

            return file_missing

        base_file_missing = check_files_list(self.__file_properties)
        abs_file_missing=False
        if not (monovan_run is None):
            abs_file_missing = check_files_list(self.__abs_norm_file_properties)

        if  base_file_missing or abs_file_missing:
             raise RuntimeError(" Files needed for the run are missing ")




    def log(self, msg,level="notice"):
        """Send a log message to the location defined
        """
        log_options = \
        {"notice" :      lambda (msg):   logger.notice(msg),
         "warning" :     lambda (msg):   logger.warning(msg),
         "error" :       lambda (msg):   logger.error(msg),
         "information" : lambda (msg):   logger.information(msg),
         "debug" :       lambda (msg):   logger.debug(msg)}
        if self._to_stdout:
            print msg
        if self._log_to_mantid:
            log_options[level](msg)


    def help(self,keyword=None) :
        """function returns help on reduction parameters.

           if provided without arguments it returns the list of the parameters availible
        """
        if self.instrument is None:
             raise ValueError("instrument has not been defined, call setup(instrument_name) first.")

        if keyword==None :
            par_names = self.instrument.getParameterNames()
            n_params = len(par_names)
            print "****: ***************************************************************************** "
            print "****: There are ",n_params, " reduction parameters availible to change, namely: "
            for i in xrange(0,n_params,4):
                print "****: ",
                for j in xrange(0,min(n_params-i,4)):
                    print "\t{0}".format(par_names[i+j]),
                print ""
                #print par_names[i],
                #print  type(self.instrument.getParameterType(par_names[i])),
                #print  self.instrument.getParameterType(par_names[i])
            print "****:"
            print "****: type help(parameter_name) to get help on a parameter with the name requested"
            print "****: ***************************************************************************** ";
        else:
            if self.instrument.hasParameter(keyword) :
                print "****: ***************************************************************************** ";
                print "****: IDF value for keyword: ",keyword," is: ",self.get_default_parameter(keyword)
                if keyword in self.__synonims :
                    fieldName = self.__synonims[keyword]
                    print "****: This keyword is known to reducer as: ",fieldName," and its value is: ",self.fieldName
                else:
                    print "****: Its current value in reducer is: ",getattr(self,keyword)

                print "****: help for "+keyword+" is not yet implemented, read "+self.instr_name+"_Parameters.xml\n"\
                      "****: in folder "+config.getString('instrumentDefinition.directory')+" for its description there"
                print "****: ***************************************************************************** ";
            else:
                raise ValueError('Instrument parameter file does not contain a definition for "%s". Cannot continue' % keyword)



#-----------------------------------------------------------------
if __name__=="__main__":
    pass
    #unittest.main()