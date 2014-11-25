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
from DirectPropertyManager import DirectPropertyManager;

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

        prop_man = selt.prop_man;
        diag_params = prop_man.getDiagnosticsParameters();
        for key,val in kwargs.iteritems():
            diag_params[key]=val;
        ## Check for any keywords that have not been supplied and put in the values set on reducer
        #for par in self.__diag_params:
        #    arg = par.lstrip('diag_')
        #    if arg not in kwargs:
        #        kwargs[arg] = getattr(self, arg)

        # if input parameter is workspace rather then run, we want to keep it
        self._keep_wb_workspace = False
        if isinstance(white,str) and white in mtd:
         self._keep_wb_workspace = True
        if isinstance(white,api.Workspace) : # it is workspace itself
            self._keep_wb_workspace = True

             
        # TODO: primitive cache is used here. Improve
        if prop_man.use_hard_mask_only:
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

                diag_mask= LoadMask(Instrument=self.instr_name,InputFile=prop_man.hard_mask_file,
                               OutputWorkspace='hard_mask_ws')
                MaskDetectors(Workspace=white_data, MaskedWorkspace=diag_mask)
                diag_mask,masked_list = ExtractMask(InputWorkspace=white_data)
                DeleteWorkspace(Workspace=whitews_name)

            return diag_mask

        # Get the white beam vanadium integrals
        whiteintegrals = self.do_white(white, None, None,None) # No grouping yet
        if prop_man.second_white:
            second_white = prop_man.second_white;
            other_whiteintegrals = self.do_white(second_white, None, None,None) # No grouping yet
            prop_man.second_white = other_whiteintegrals;


        # Get the background/total counts from the sample if present
        if 'sample' in kwargs:
            sample = kwargs['sample']
            del kwargs['sample']
            # If the bleed test is requested then we need to pass in the sample_run as well
            if prop_man.bleed_test:
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
            background_int *= prop_man.scale_factor;
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

        # Normalize
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
        # Check if we need to perform the absolute normalization first
        if not mono_van is None:
            if abs_ei is None:
                abs_ei = ei
            mapping_file = self.abs_map_file
            spectrum_masks = self.spectra_masks
            monovan_wkspace = self.mono_van(mono_van, abs_ei, abs_white_run, mapping_file, spectrum_masks)

            # TODO: Need a better check than this...
            if (abs_white_run is None):
                self.log("Performing Normalization to Mono Vanadium.")
                norm_factor = self.calc_average(monovan_wkspace,ei)
            else:
                self.log("Performing Absolute Units Normalization.")
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
            raise RuntimeError(str.format(" Vanadium weight calculated from {0} spectra out of {1} histograms is equal to 0 and sum: {2} Check vanadium integration ranges or diagnostic settings ",str(ic),str(nhist),str(average_value)))
        return average_value / weight_sum

    def monovan_abs(self, ei_workspace):
        """Calculates the scaling factor required for normalization to absolute units.
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
        Save the result workspace to the specified filename using the list of formats specified in
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

        prop_man = self.prop_man;
        if not(formats is None):
            # clear up existing save formats
            prop_man.save_format=None;
            # parse & set up new save formats 
            prop_man.save_format = formats;

        formats = prop_man.save_format;


          
        save_file,ext = os.path.splitext(save_file)
        if len(ext)>1:
            formats.add(ext[1:]);


        for file_format  in formats:
            for case in switch(file_format):
                if case('nxspe'):
                    filename = save_file +'.nxspe';
                    SaveNXSPE(InputWorkspace=workspace,Filename= filename, KiOverKfScaling=prop_man.apply_kikf_correction,psi=prop_man.psi)
                    break
                if case('spe'):
                    filename = save_file +'.spe';
                    SaveSPE(InputWorkspace=workspace,Filename= filename)
                    break
                if case('nxs'):
                    filename = save_file +'.nxs';
                    SaveNexus(InputWorkspace=workspace,Filename= filename)
                    break
                if case(): # default, could also just omit condition or 'if True'
                    prop_man.log("Unknown file format {0} requested to save results. No saving performed this format".format(file_format));

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

         TODO: As extract single spectrum works only with WorkspaceIndex, we have to assume that WorkspaceIndex=spectraID-1;
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
        self.initialise(instr_name);

    @property 
    def prop_man(self):
        """ Return property manager containing DirectEnergyConversion parameters """
        return self._propMan;
    @prop_man.setter
    def prop_man(self,value):
        """ Assign new instance of direct property manager to provide DirectEnergyConversion parameters """
        if isinstance(value,DirectPropertyManager):
            self._propMan = value;
        else:
            raise KeyError("Property manager can be initialized by an instance of DirectProperyManager only")

    def initialise(self, instr_name,reload_instrument=False):
        """
        Initialize the private attributes of the class and the nullify the attributes which expected to be always set-up from a calling script
        """

        # Instrument and default parameter setup
        # formats available for saving. As the reducer has to have a method to process one of this, it is private property
        ## Detector diagnosis
        # Diag parameters -- keys used by diag method to pick from default parameters. Diag cuts these keys removing diag_ word
        # and tries to get rest from the correspondent Direct Energy conversion attributes.
        self.__diag_params = ['diag_tiny', 'diag_huge', 'diag_samp_zero', 'diag_samp_lo', 'diag_samp_hi','diag_samp_sig',\
                              'diag_van_out_lo', 'diag_van_out_hi', 'diag_van_lo', 'diag_van_hi', 'diag_van_sig', 'diag_variation',\
                              'diag_bleed_test','diag_bleed_pixels','diag_bleed_maxrate','diag_hard_mask_file','diag_use_hard_mask_only','diag_background_test_range']
        if hasattr(self,'_propMan'):
            if self._propMan is None or instr_name != self._propMan.instrument.getName() or reload_instrument:
                self._propMan = DirectPropertyManager(instr_name);
        else:
            self._propMan = DirectPropertyManager(instr_name);

    def setup_mtd_instrument(self, workspace = None,reload_instrument=False):
        if workspace != None:
            # TODO: Check it!
            self.instrument = workspace.getInstrument()
            self.instr_name = self.instrument.getName()



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


 
    @staticmethod
    def make_ckpt_name(*argi) :
        """ Make the name of the checkpoint from the function arguments
        """
        return ''.join(str(arg) for arg in argi if arg is not None)


class switch(object):
    """ Helper class providing nice switch statement""" 
    def __init__(self, value):
        self.value = value
        self.fall = False

    def __iter__(self):
        """Return the match method once, then stop"""
        yield self.match
        raise StopIteration
    
    def match(self, *args):
        """Indicate whether or not to enter a case suite"""
        if self.fall or not args:
            return True
        elif self.value in args: # changed for v1.5, see below
            self.fall = True
            return True
        else:
            return False

#-----------------------------------------------------------------
if __name__=="__main__":
    pass
    #unittest.main()