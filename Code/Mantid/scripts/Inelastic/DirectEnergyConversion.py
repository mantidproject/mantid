"""
Conversion class defined for conversion to deltaE for
'direct' inelastic geometry instruments

The class defines various methods to allow users to convert their
files to DeltaE.

            Usage:
>>red = DirectEnergyConversion('InstrumentName')
and then: 
>>red.convert_to_energy_transfer(wb_run,sample_run,ei_guess,rebin)
or
>>red.convert_to_energy_transfer(wb_run,sample_run,ei_guess,rebin,**arguments)
or
>>red.convert_to_energy_transfer(wb_run,sample_run,ei_guess,rebin,mapfile,**arguments)
or
>>red.convert_to_energy_transfer(wb_run   
Whitebeam run number or file name or workspace
       sample_run  sample run number or file name or workspace
                ei_guess    Ei guess
                rebin       Rebin parameters
                mapfile     Mapfile -- if absent/'default' the defaults from IDF are used
                monovan_run If present will do the absolute units normalization. Number of additional parameters
                            specified in **kwargs is usually requested for this. If they are absent, program uses defaults,
                            but the defaults (e.g. sample_mass or sample_rmm ) are usually incorrect for a particular run.
                arguments   The dictionary containing additional keyword arguments.
                            The list of allowed additional arguments is defined in InstrName_Parameters.xml file, located in
                            MantidPlot->View->Preferences->Mantid->Directories->Parameter Definitions

        with run numbers as input:
       >>red.convert_to_energy_transfer(1000,10001,80,[-10,.1,70])  # will run on default instrument

       >>red.convert_to_energy_transfer(1000,10001,80,[-10,.1,70],'mari_res', additional keywords as required)

       >>red.convert_to_energy_transfer(1000,10001,80,'-10,.1,70','mari_res',fixei=True)

       A detector calibration file must be specified if running the reduction with workspaces as input
       namely:
       >>w2=cred.onvert_to_energy_transfer('wb_wksp','run_wksp',ei,rebin_params,mapfile,det_cal_file=cal_file
               ,diag_remove_zero=False,norm_method='current')


     All available keywords are provided in InstName_Parameters.xml file

     Some samples are:
     norm_method =[monitor-1],[monitor-2][Current]
     background  =False , True
     fixei       =False , True
    save_format =['.spe'],['.nxspe'],'none'
    detector_van_range          =[20,40] in mev

    bkgd_range  =[15000,19000]  :integration range for background tests

      second_white     - If provided an additional set of tests is performed on this. (default = None)
      hardmaskPlus     - A file specifying those spectra that should be masked without testing (default=None)
      tiny             - Minimum threshold for acceptance (default = 1e-10)
      large            - Maximum threshold for acceptance (default = 1e10)
      bkgd_range       - A list of two numbers indicating the background range (default=instrument defaults)
      diag_van_median_rate_limit_lo      - Lower bound defining outliers as fraction of median value (default = 0.01)
      diag_van_median_rate_limit_hi      - Upper bound defining outliers as fraction of median value (default = 100.)
      diag_van_median_sigma_lo           - Fraction of median to consider counting low for the white beam diag (default = 0.1)
      diag_van_median_sigma_hi           - Fraction of median to consider counting high for the white beam diag (default = 1.5)
      diag_van_sig  - Error criterion as a multiple of error bar i.e. to fail the test, the magnitude of the
                    difference with respect to the median value must also exceed this number of error bars (default=0.0)
      diag_remove_zero                - If true then zeroes in the vanadium data will count as failed (default = True)
      diag_samp_samp_median_sigma_lo  - Fraction of median to consider counting low for the white beam diag (default = 0)
      diag_samp_samp_median_sigma_hi  - Fraction of median to consider counting high for the white beam diag (default = 2.0)
      diag_samp_sig                   - Error criterion as a multiple of error bar i.e. to fail the test, the magnitude of the
                                        difference with respect to the median value must also exceed this number of error bars (default=3.3)
      variation       -The number of medians the ratio of the first/second white beam can deviate from
                     the average by (default=1.1)
      bleed_test      - If true then the CreatePSDBleedMask algorithm is run
      bleed_maxrate   - If the bleed test is on then this is the maximum framerate allowed in a tube
      bleed_pixels    - If the bleed test is on then this is the number of pixels ignored within the
                       bleed test diagnostic
      print_results - If True then the results are printed to the screen

      diag_remove_ero =True, False (default):Diag zero counts in background range
      bleed=True , turn bleed correction on and off on by default for Merlin and LET

      sum =True,False(default) , sum multiple files

      det_cal_file= a valid detector block file and path or a raw file. Setting this
                  will use the detector calibraion from the specified file NOT the
                  input raw file
      mask_run = RunNumber to use for diag instead of the input run number

      one2one =True, False :Reduction will not use a mapping file

      hardmaskPlus=Filename :load a hardmarkfile and apply together with diag mask

      hardmaskOnly=Filename :load a hardmask and use as only mask
"""


from mantid.simpleapi import *
from mantid.kernel import funcreturns
from mantid import api
from mantid import geometry

import time as time
import glob,sys,os.path,math
import numpy as np

import CommonFunctions as common
import diagnostics
from DirectPropertyManager import DirectPropertyManager;

def setup_reducer(inst_name,reload_instrument=False):
    """
    Given an instrument name or prefix this sets up a converter
    object for the reduction
    """
    try:
        return DirectEnergyConversion(inst_name,reload_instrument)
    except RuntimeError, exc:
        raise RuntimeError('Unknown instrument "%s" or wrong IDF file for this instrument, cannot continue' % inst_name)


class DirectEnergyConversion(object):
    """
    Performs a convert to energy assuming the provided instrument is an elastic instrument
    """

    def diagnose(self, white,diag_sample=None,**kwargs):
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
              diag_sample - A workspace, run number or filepath of additional (sample) run used for diagnostics.
                            A workspace is assumed to have simple been loaded and nothing else. (default = None)

              second_white - If provided an additional set of tests is performed on this. (default = None)
              hard_mask    - A file specifying those spectra that should be masked without testing (default=None)

              # IDF-based diagnostics parameters:
              tiny        - Minimum threshold for acceptance (default = 1e-10)
              huge        - Maximum threshold for acceptance (default = 1e10)
              background_test_range - A list of two numbers indicating the background range (default=instrument defaults)
              van_out_lo  - Lower bound defining outliers as fraction of median value (default = 0.01)
              van_out_hi  - Upper bound defining outliers as fraction of median value (default = 100.)
              van_lo      - Fraction of median to consider counting low for the white beam diag (default = 0.1)
              van_hi      - Fraction of median to consider counting high for the white beam diag (default = 1.5)
              van_sig  - Error criterion as a multiple of error bar i.e. to fail the test, the magnitude of the\n"
                          "difference with respect to the median value must also exceed this number of error bars (default=0.0)
              samp_zero    - If true then zeros in the vanadium data will count as failed (default = True)
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

        prop_man = self.prop_man;
        # modify properties using input arguments
        prop_man.set_input_parameters(**kwargs);
        # return all diagnostics parameters
        diag_params = prop_man.get_diagnostics_parameters();

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
                white_data,mon_ws = self.load_data(white,whitews_name,self._keep_wb_workspace)

                diag_mask= LoadMask(Instrument=prop_man.instr_name,InputFile=prop_man.hard_mask_file,
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


        # Get the background/total counts from the sample run if present
        if diag_sample: 
            # If the bleed test is requested then we need to pass in the sample_run as well
            if prop_man.bleed_test:
                diag_params['sample_run'] = diag_sample

            # Set up the background integrals
            result_ws,mon_ws = self.load_data(diag_sample)
            result_ws        = self.normalise(result_ws, result_ws.name(), prop_man.normalise_method)

            bkgd_range = prop_man.background_test_range
            background_int = Integration(result_ws,
                                         RangeLower=bkgd_range[0],RangeUpper=bkgd_range[1],
                                         IncludePartialBins=True)
            total_counts = Integration(result_ws, IncludePartialBins=True)
            background_int = ConvertUnits(background_int, "Energy", AlignBins=0)
            prop_man.log("Diagnose: finished convertUnits ",'information')

            background_int *= prop_man.scale_factor;
            diagnostics.normalise_background(background_int, whiteintegrals, diag_params.get('second_white',None))
            diag_params['background_int'] = background_int
            diag_params['sample_counts'] = total_counts

        # Check how we should run diag
        diag_spectra_blocks = prop_man.diag_spectra;
        if diag_spectra_blocks  is None:
            # Do the whole lot at once
            diagnostics.diagnose(whiteintegrals, **diag_params)
        else:
            for index, bank in enumerate(diag_spectra_blocks):
                diag_params['start_index'] = bank[0] - 1
                diag_params['end_index']   = bank[1] - 1
                diagnostics.diagnose(whiteintegrals, **diag_params)

        if 'sample_counts' in diag_params:
            DeleteWorkspace(Workspace='background_int')
            DeleteWorkspace(Workspace='total_counts')
        if 'second_white' in diag_params:
            DeleteWorkspace(Workspace=diag_params['second_white'])
        # Extract a mask workspace
        diag_mask, det_ids = ExtractMask(InputWorkspace=whiteintegrals,OutputWorkspace=var_name)

        DeleteWorkspace(Workspace=whiteintegrals)
        #TODO do we need this?
        #self.spectra_masks = diag_mask
        return diag_mask


    def do_white(self, white_run, spectra_masks, map_file,mon_number=None):
        """
        Create the workspace, which each spectra containing the correspondent white beam integral (single value)

        These integrals are used as estimate for detector efficiency in wide range of energies
        (rather the detector electronic's efficiency as the geuger counters are very different in efficiency)
        and is used to remove the influence of this efficiency to the different detectors.
        """
        propman = self.prop_man;
        instr_name = propman.instr_name;

        whitews_name = common.create_resultname(white_run, prefix = instr_name,suffix='-white')
        if whitews_name in mtd:
            DeleteWorkspace(Workspace=whitews_name)
        # Load
        white_data,mon_ws = self.load_data(white_run,whitews_name,self._keep_wb_workspace)

        # Normalize
        self.__in_white_normalization = True;
        white_ws = self.normalise(white_data, whitews_name, propman.normalise_method,0.0,mon_number)
        self.__in_white_normalization = False;

        # Units conversion
        white_ws = ConvertUnits(InputWorkspace=white_ws,OutputWorkspace=whitews_name, Target= "Energy", AlignBins=0)
        propman.log("do_white: finished converting Units",'information')
        # This both integrates the workspace into one bin spectra and sets up common bin boundaries for all spectra
        low,upp=propman.wb_integr_range;
        if low > upp:
            raise ValueError("White beam integration range is inconsistent. low=%d, upp=%d" % (low,upp))
        delta = 2.0*(upp - low)
        white_ws = Rebin(InputWorkspace=white_ws,OutputWorkspace=whitews_name, Params=[low, delta, upp])
        # Why aren't we doing this...
        #Integration(white_ws, white_ws, RangeLower=low, RangeUpper=upp)

        # Masking and grouping
        white_ws = self.remap(white_ws, spectra_masks, map_file)

        # White beam scale factor
        white_ws *= propman.wb_scale_factor
        return white_ws

    def mono_van(self, mono_van, ei_guess, white_run=None, map_file=None,
                 spectra_masks=None, result_name=None, Tzero=None):
        """Convert a mono vanadium run to DeltaE.
        If multiple run files are passed to this function, they are summed into a run and then processed
        """
        # Load data
        sample_data,mon_ws = self.load_data(mono_van)
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
        instr_name = self.prop_man.instr_name
        if result_name is None:
            result_name = common.create_resultname(mono_run, instr_name)

        # Load data
        sample_data,mon_ws = self.load_data(mono_run,result_name)
        # Create the result name if necessary
        mono_s=self._do_mono(sample_data, mon_ws, result_name, ei_guess,
                                  white_run, map_file, spectra_masks, Tzero)
        return mono_s


# -------------------------------------------------------------------------------------------
#         This actually does the conversion for the mono-sample and mono-vanadium runs
#
# -------------------------------------------------------------------------------------------
    def _do_mono_SNS(self, data_ws, monitor_ws, result_name, ei_guess,
                 white_run=None, map_file=None, spectra_masks=None, Tzero=None):

        # Special load monitor stuff.
        propman = self.prop_man;
        if (propman.instr_name == "CNCS" or propman.instr_name == "HYSPEC"):
            propman.fix_ei = True
            ei_value = ei_guess
            if (propman.instr_name == "HYSPEC"):
                Tzero=4.0 + 107.0 / (1+math.pow((ei_value/31.0),3.0))
                propman.log("Determined T0 of %s for HYSPEC" % str(Tzero))
            if (Tzero is None):
                tzero = (0.1982*(1+ei_value)**(-0.84098))*1000.0
            else:
                tzero = Tzero
            # apply T0 shift
            ScaleX(InputWorkspace=data_ws,OutputWorkspace=result_name,Operation="Add",Factor=-tzero)
            mon1_peak = 0.0
        elif (propman.instr_name == "ARCS" or propman.instr_name == "SEQUOIA"):
            if 'Filename' in data_ws.getRun(): mono_run = data_ws.getRun()['Filename'].value
            else: raise RuntimeError('Cannot load monitors for event reduction. Unable to determine Filename from mono workspace, it should have been added as a run log.')

            propman.log("mono_run = %s (%s)" % (mono_run,type(mono_run)),'debug')

            if mono_run.endswith("_event.nxs"):
                monitor_ws=LoadNexusMonitors(Filename=mono_run)
            elif mono_run.endswith("_event.dat"):
                InfoFilename = mono_run.replace("_neutron_event.dat", "_runinfo.xml")
                monitor_ws=LoadPreNexusMonitors(RunInfoFilename=InfoFilename)

            argi = {};
            argi['Monitor1Spec']=int(propman.ei_mon_spectra[0]);
            argi['Monitor2Spec']=int(propman.ei_mon_spectra[1]);
            argi['EnergyEstimate']=ei_guess;
            argi['FixEi'] =propman.fix_ei
            if hasattr(self, 'ei_mon_peak_search_range'):
                argi['PeakSearchRange']=self.ei_mon_peak_search_range;

            try:
                ei_calc,firstmon_peak,firstmon_index,TzeroCalculated = \
                    GetEi(InputWorkspace=monitor_ws,**argi)
            except:
                propman.log("Error in GetEi. Using entered values.")
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
            if (propman.fix_ei):
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
        else:
            # Do ISIS stuff for Ei
            # Both are these should be run properties really
            ei_value, mon1_peak = self.get_ei(data_ws,monitor_ws, result_name, ei_guess)
        self.prop_man.incident_energy = ei_value

        # As we've shifted the TOF so that mon1 is at t=0.0 we need to account for this in CalculateFlatBackground and normalisation
        bin_offset = -mon1_peak

        # For event mode, we are going to histogram in energy first, then go back to TOF
        if propman.check_background== True:
           # Extract the time range for the background determination before we throw it away
           background_bins = "%s,%s,%s" % (propman.bkgd_range[0] + bin_offset, (propman.bkgd_range[1]-propman.bkgd_range[0]), propman.bkgd_range[1] + bin_offset)
           Rebin(InputWorkspace=result_name,OutputWorkspace= "background_origin_ws",Params=background_bins)

        # Convert to Et
        ConvertUnits(InputWorkspace=result_name,OutputWorkspace= "_tmp_energy_ws", Target="DeltaE",EMode="Direct", EFixed=ei_value)
        RenameWorkspace(InputWorkspace="_tmp_energy_ws",OutputWorkspace= result_name)
        # Histogram
        Rebin(InputWorkspace=result_name,OutputWorkspace= "_tmp_rebin_ws",Params= propman.energy_bins, PreserveEvents=False)
        RenameWorkspace(InputWorkspace="_tmp_rebin_ws",OutputWorkspace= result_name)
        # Convert back to TOF
        ConvertUnits(InputWorkspace=result_name,OutputWorkspace=result_name, Target="TOF",EMode="Direct", EFixed=ei_value)

        if propman.check_background == True:
            # Remove the count rate seen in the regions of the histograms defined as the background regions, if the user defined such region
            ConvertToDistribution(Workspace=result_name)

            CalculateFlatBackground(InputWorkspace="background_origin_ws",OutputWorkspace= "background_ws",
                               StartX= propman.bkgd_range[0] + bin_offset,EndX= propman.bkgd_range[1] + bin_offset,
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
        self.normalise(mtd[result_name], result_name, propman.normalise_method, range_offset=bin_offset)



        # This next line will fail the SystemTests
        #ConvertUnits(result_ws, result_ws, Target="DeltaE",EMode='Direct', EFixed=ei_value)
        # But this one passes...
        ConvertUnits(InputWorkspace=result_name,OutputWorkspace=result_name, Target="DeltaE",EMode='Direct')
        propman.log("_do_mono: finished ConvertUnits for : "+result_name)



        if propman.energy_bins :
            Rebin(InputWorkspace=result_name,OutputWorkspace=result_name,Params= propman.energy_bins,PreserveEvents=False)

        if propman.apply_detector_eff:
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

        ei_value, mon1_peak = self.get_ei(data_ws,monitor_ws, result_name, ei_guess)
        self.prop_man.incident_energy = ei_value
        prop_man = self.prop_man;

        # As we've shifted the TOF so that mon1 is at t=0.0 we need to account for this in CalculateFlatBackground and normalization
        bin_offset = -mon1_peak

        if prop_man.check_background == True:
            # Remove the count rate seen in the regions of the histograms defined as the background regions, if the user defined such region
            bkgd_range = prop_man.bkgd_range;
            CalculateFlatBackground(InputWorkspace=result_name,OutputWorkspace=result_name,
                                    StartX= bkgd_range[0] + bin_offset,EndX= bkgd_range[1] + bin_offset,
                                     WorkspaceIndexList= '',Mode= 'Mean',SkipMonitors='1')


        # Normalize using the chosen method+group
        self.normalise(mtd[result_name], result_name, prop_man.normalise_method, range_offset=bin_offset)



        # This next line will fail the SystemTests
        #ConvertUnits(result_ws, result_ws, Target="DeltaE",EMode='Direct', EFixed=ei_value)
        # But this one passes...
        ConvertUnits(InputWorkspace=result_name,OutputWorkspace=result_name, Target="DeltaE",EMode='Direct')
        prop_man.log("_do_mono: finished ConvertUnits for : "+result_name,'information')


        energy_bins = prop_man.energy_bins;
        if energy_bins:
            Rebin(InputWorkspace=result_name,OutputWorkspace=result_name,Params= energy_bins,PreserveEvents=False)

        if prop_man.apply_detector_eff:
           DetectorEfficiencyCor(InputWorkspace=result_name,OutputWorkspace=result_name)
           prop_man.log("_do_mono: finished DetectorEfficiencyCor for : "+result_name,'information')
        #############
        return

    def _do_mono(self, data_ws, monitor_ws, result_name, ei_guess,
                 white_run=None, map_file=None, spectra_masks=None, Tzero=None):
        """
        Convert units of a given workspace to deltaE, including possible
        normalization to a white-beam vanadium run.
        """
        if (self._do_ISIS_normalization):
           self._do_mono_ISIS(data_ws,monitor_ws,result_name,ei_guess,
                               white_run, map_file, spectra_masks, Tzero)
        else:
           self._do_mono_SNS(data_ws,monitor_ws,result_name,ei_guess,
                         white_run, map_file, spectra_masks, Tzero)

        prop_man = self.prop_man;
        #######################
        # Ki/Kf Scaling...
        if prop_man.apply_kikf_correction:
            prop_man.log('Start Applying ki/kf corrections to the workspace : '+result_name,'information')
            CorrectKiKf(InputWorkspace=result_name,OutputWorkspace= result_name, EMode='Direct')
            prop_man.log('finished applying ki/kf corrections for : '+result_name,'information')

        # Make sure that our binning is consistent
        if prop_man.energy_bins:
            Rebin(InputWorkspace=result_name,OutputWorkspace= result_name,Params= prop_man.energy_bins)

        # Masking and grouping
        result_ws = mtd[result_name]
        result_ws = self.remap(result_ws, spectra_masks, map_file)


        ConvertToDistribution(Workspace=result_ws)
        # White beam correction
        if white_run is not None:
            white_ws = self.do_white(white_run, spectra_masks, map_file,None)
            result_ws /= white_ws

        # Overall scale factor
        result_ws *= prop_man.scale_factor
        return result_ws

    def convert_to_energy_transfer(self,wb_run=None,sample_run=None,ei_guess=None,rebin=None,map_file=None,monovan_run=None,wb_for_monovan_run=None,**kwargs):
      """ One step conversion of run into workspace containing information about energy transfer

      """ 
      # Support for old reduction interface:
      self.prop_man.set_input_parameters_ignore_nan(wb_run=wb_run,sample_run=sample_run,incident_energy=ei_guess,
                                           energy_bins=rebin,map_file=map_file,monovan_run=monovan_run,wb_for_monovan_run=wb_for_monovan_run)
      # 
      self.prop_man.set_input_parameters(**kwargs);

      # output workspace name.
      try:
          n,r=funcreturns.lhs_info('both')
          wksp_out=r[0]
      except:
          wksp_out = self.prop_man.get_sample_ws_name();


      # Process old legacy parameters which are easy to re-define in dgreduce rather then transfer through Mantid
      #TODO: (verify)
      #program_args = process_legacy_parameters(**kwargs)


      # inform user on what parameters have changed 
      self.prop_man.log_changed_values('notice');
      #process complex parameters

      prop_man = self.prop_man

      start_time=time.time()
      # defaults can be None too, but can be a file

     # check if reducer can find all non-run files necessary for the reduction before starting long run.
      #TODO:
      # Reducer.check_necessary_files(monovan_run);


      # Here was summation in dgreduce. 
      if (np.size(self.prop_man.sample_run)) > 1 and self.prop_man.sum_runs:
        #this sums the runs together before passing the summed file to the rest of the reduction
        #this circumvents the inbuilt method of summing which fails to sum the files for diag
        # TODO: --check out if internal summation works -- it does not. Should be fixed. Old summation meanwhile

        sum_name=self.prop_man.instr_name+str(self.prop_man.sample_run[0])+'-sum'
        sample_run =self.sum_files(sum_name, self.prop_man.sample_run)
        common.apply_calibration(self.prop_man.instr_name,sample_run,self.prop_man.det_cal_file)
      else:
        sample_run,sample_monitors = self.load_data(self.prop_man.sample_run)

      # Update reduction properties which may change in the workspace but have not been modified from input parameters. 
      # E.g. detector number have changed 
      oldChanges = prop_man.getChangedProperties();
      allChanges  =self.prop_man.update_defaults_from_instrument(sample_run.getInstrument())
      workspace_defined_prop=allChanges.difference(oldChanges)
      if len(workspace_defined_prop)>0:
          prop_man.log("****************************************************************")
          prop_man.log('*** Sample run {0} properties change default reduction properties: '.format(sample_run.getName()))
          prop_man.log_changed_values('notice',False,oldChanges)
          prop_man.log("****************************************************************")

      self.prop_man.sample_run = sample_run



      masking = None;
      masks_done=False
      if not prop_man.run_diagnostics:
          header="*** Diagnostics including hard masking is skipped "
          masks_done = True;
      #if Reducer.save_and_reuse_masks :
      # SAVE AND REUSE MASKS
#-------------------------------------------------------------------------------------------------------------------------------------------------------
#  Diagnostics here 
# --------------------------------------------------------------------------------------------------------
     # diag the sample and detector vanadium. It will deal with hard mask only if it is set that way
      if not masks_done:
        prop_man.log("######## Run diagnose for sample run ###########################",'notice');
        masking = self.diagnose(prop_man.wb_run,prop_man.mask_run,
                                second_white=None,print_results=True)
        header = "*** Diagnostics processed workspace with {0:d} spectra and masked {1:d} bad spectra"


        # diagnose absolute units:
        if prop_man.monovan_run != None :
            if prop_man.mono_correction_factor == None :
                if prop_man.use_sam_msk_on_monovan == True:
                    prop_man.log('  Applying sample run mask to mono van')
                else:
                    if not prop_man.use_hard_mask_only : # in this case the masking2 is different but points to the same workspace Should be better solution for that.
                        prop_man.log("######## Run diagnose for monochromatic vanadium run ###########",'notice');

                        masking2 = self.diagnose(prop_man.wb_for_monovan_run,prop_man.monovan_run,
                                         second_white = None,print_results=True)
                        masking +=  masking2
                        DeleteWorkspace(masking2)


            else: # if Reducer.mono_correction_factor != None :
                pass

      # save mask if it does not exist and has been already loaded
      #if Reducer.save_and_reuse_masks and not masks_done:
      #    SaveMask(InputWorkspace=masking,OutputFile = mask_file_name,GroupedDetectors=True)

      # Very important statement propagating masks for further usage in convert_to_energy
      self.spectra_masks=masking
      # estimate and report the number of failing detectors
      failed_sp_list,nSpectra = get_failed_spectra_list_from_masks(masking)
      nMaskedSpectra = len(failed_sp_list)
      # this tells turkey in case of hard mask only but everything else semens work fine
      prop_man.log(header.format(nSpectra,nMaskedSpectra),'notice');

      #Run the conversion first on the sample
      deltaE_wkspace_sample = self.mono_sample(sample_run,prop_man.incident_energy,prop_man.wb_run,
                                               prop_man.map_file,masking)

 
      # calculate absolute units integral and apply it to the workspace
      if prop_man.monovan_run != None or prop_man.mono_correction_factor != None :
         deltaE_wkspace_sample = self.apply_absolute_normalization(deltaE_wkspace_sample,prop_man.monovan_run,\
                                      prop_man.incident_energy,prop_man.wb_for_monovan_run)


      results_name = deltaE_wkspace_sample.name();
      if results_name != wksp_out:
         RenameWorkspace(InputWorkspace=results_name,OutputWorkspace=wksp_out)


      ei= (deltaE_wkspace_sample.getRun().getLogData("Ei").value)
      prop_man.log("*** Incident energy found for sample run: {0} meV".format(ei),'notice');

      end_time=time.time()
      prop_man.log("*** Elapsed time = {0} sec".format(end_time-start_time),'notice');

      if mtd.doesExist('_wksp.spe-white')==True:
        DeleteWorkspace(Workspace='_wksp.spe-white')
    # Hack for multirep mode?
#    if mtd.doesExist('hard_mask_ws') == True:
 #       DeleteWorkspace(Workspace='hard_mask_ws')

      return deltaE_wkspace_sample

  
#----------------------------------------------------------------------------------
#                        Reduction steps
#----------------------------------------------------------------------------------
    def sum_files(self, accumulator, files):
        """ Custom sum for multiple runs

            Left for compatibility as internal summation had some unspecified problems.

            TODO: Should be replaced by internal procedure
            TODO: Monitors are not summed together here. 
            TODO: Should we allow to add to existing accumulator?
        """
        prop_man = self.prop_man
        instr_name = prop_man.instr_name;
        accum_name = accumulator
        if isinstance(accum_name,api.Workspace): # it is actually workspace and is in Mantid
            accum_name  = accumulator.name()
        if accum_name  in mtd:
            DeleteWorkspace(Workspace=accum_name)     


        load_mon_with_ws = prop_man.load_monitors_with_workspace;
        prop_man.log("****************************************************************",'notice');
        prop_man.log("*** Summing multiple runs",'notice')
        if type(files) == list:
             #tmp_suffix = '_plus_tmp'

            for filename in files:
               temp = common.load_run(instr_name,filename, force=False,load_with_workspace = load_mon_with_ws)

               if accum_name in mtd: # add current workspace to the existing one
                  accumulator = mtd[accum_name]
                  accumulator+=  temp
                  DeleteWorkspace(Workspace=temp)
                  prop_man.log("*** Added run: {0} to workspace: {1}".format(filename,accum_name),'notice')
               else:
                   accumulator=RenameWorkspace(InputWorkspace=temp,OutputWorkspace=accum_name)
                   prop_man.log("*** Summing multiple runs: created output workspace: {0} from run {1}".format(accum_name,filename),'notice')

            return accumulator
        else:
            temp = common.load_run(instr_name,files, force=False,load_with_workspace = load_mon_with_ws)
            accumulator=RenameWorkspace(InputWorkspace=temp,OutputWorkspace=accum_name)
        prop_man.log("****************************************************************",'notice');

        return accumulator;


    def apply_absolute_normalization(self,deltaE_wkspace_sample,monovan_run=None,ei_guess=None,wb_mono=None):
        """  Function applies absolute normalization factor to the target workspace
             and calculates this factor if necessary

            Inputs:
            Reducer           --    properly initialized class which performs reduction
            deltaE_wkspace_sample-- the workspace which should be modified
            monovan_run          -- run number for monochromatic vanadium sample at current energy
            ei_guess             -- estimated neutrons incident energy
            wb_mono              -- white bean vanadium run number.
        """
        # Old interface support
        prop_man = self.prop_man;
        prop_man.log('*** Running absolute units corrections ****************************************************','notice')

        if prop_man.mono_correction_factor:
            absnorm_factor=float(prop_man.mono_correction_factor)
            prop_man.log('*** Using supplied workspace correction factor                           ******','notice')
            abs_norm_factor_is='provided'
        else:
            mvir=prop_man.monovan_integr_range;
            prop_man.log('*** Evaluating the integral from the monovan run and calculate the correction factor ******','notice')
            prop_man.log('    Using absolute units vanadium integration range : [{0:8f}:{1:8f}]         ******'.format(mvir[0],mvir[1]),'notice')
            abs_norm_factor_is = 'calculated'
        #
            result_ws_name = common.create_resultname(monovan_run,prop_man.instr_name)
            # check the case when the sample is monovanadium itself (for testing purposes)
            if result_ws_name == deltaE_wkspace_sample.name() :
                deltaE_wkspace_monovan = CloneWorkspace(InputWorkspace=deltaE_wkspace_sample,OutputWorkspace=result_ws_name+'-monovan');
                deltaE_wkspace_monovan=self.remap(deltaE_wkspace_monovan,None,prop_man.monovan_mapfile)
            else:
                # convert to monovanadium to energy
                deltaE_wkspace_monovan  = self.mono_sample(monovan_run,ei_guess,wb_mono,
                                               prop_man.monovan_mapfile,self.spectra_masks)
                #deltaE_wkspace_monovan = self.convert_to_energy(monovan_run, ei_guess, wb_mono)


            ei_monovan = deltaE_wkspace_monovan.getRun().getLogData("Ei").value
            prop_man.log('    Incident energy found for monovanadium run: '+str(ei_monovan)+' meV','notice')


            (anf_LibISIS,anf_SS2,anf_Puas,anf_TGP) = self.get_abs_normalization_factor(deltaE_wkspace_monovan.getName(),ei_monovan)

            prop_man.log('*** Absolute correction factor(s): S^2: {0:10.4f}\n*** LibISIS: {1:10.4f} Poisson: {2:10.4f}  TGP: {3:10.4f} '\
                .format(anf_LibISIS,anf_SS2,anf_Puas,anf_TGP),'notice')
            absnorm_factor = anf_TGP;
        #end
        prop_man.log('*** Using {0} value : {1} of absolute units correction factor (TGP)'.format(abs_norm_factor_is,absnorm_factor),'notice')
        prop_man.log('*******************************************************************************************','notice')

        deltaE_wkspace_sample = deltaE_wkspace_sample/absnorm_factor;


        return deltaE_wkspace_sample
    def get_abs_normalization_factor(self,deltaE_wkspaceName,ei_monovan):
        """get absolute normalization factor for monochromatic vanadium

          Inputs:
          @param: deltaE_wkspace  -- the name (string) of monovan workspace, converted to energy
          @param: ei_monovan      -- monovan sample incident energy

          @returns the value of monovan absolute normalization factor.
                   deletes monovan workspace (deltaE_wkspace) if abs norm factor was calculated successfully

          Detailed explanation:
          The algorithm takes the monochromatic vanadium workspace normalized by WB vanadium and calculates
          average modified monochromatic vanadium (MV) integral considering that the efficiency of detectors
          are different and accounted for by dividing each MV value by corresponding WBV value,
          the signal on a detector has poison distribution and the error for a detector is the square
          root of correspondent signal on a detector.
          Error for WBV considered negligibly small wrt. the error on MV
         """

        propman = self.prop_man;
        van_mass=propman.van_mass;
        # list of two number  representing the minimal (ei_monovan[0])
        # and the maximal (ei_monovan[1]) energy to integrate the spectra
        minmax = propman.monovan_integr_range;


        data_ws=Integration(InputWorkspace=deltaE_wkspaceName,OutputWorkspace='van_int',RangeLower=minmax[0],RangeUpper=minmax[1],IncludePartialBins='1')
 
        nhist = data_ws.getNumberHistograms()
        # extract wb integrals for combined spectra
        signal=[];
        error =[];
        izerc=0;
        for i in range(nhist):
           try:
             det = data_ws.getDetector(i)
           except Exception:
             continue
           if det.isMasked():
             continue
           sig = data_ws.readY(i)[0]
           err = data_ws.readE(i)[0]
           if sig != sig:     #ignore NaN (hopefully it will mean mask some day)
               continue
           if ((err<=0) or (sig<=0)):   # count Inf and negative||zero readings. Presence of this indicates that something went wrong
              izerc+=1;
              continue

           signal.append(sig) 
           error.append(err)
        #---------------- Loop finished

        norm_factor={};
        # Various prior probabilities.
        #-------------------------------------------------------------------------
        # Guess which minimizes the value sum(n_i-n)^2/Sigma_i -- this what Libisis had
        signal_sum = sum(map(lambda s,e: s/e,signal,error));
        weight_sum = sum(map(lambda e: 1./e, error));
        norm_factor['LibISIS']=signal_sum/weight_sum;
        #-------------------------------------------------------------------------
        # Guess which minimizes the value sum(n_i-n)^2/Sigma_i^2
        signal_sum = sum(map(lambda s,e: s/(e*e),signal,error));
        weight_sum = sum(map(lambda e: 1./(e*e), error));
        norm_factor['SigSq']=signal_sum/weight_sum;
        #-------------------------------------------------------------------------
        # Guess which assumes Poisson distribution with Err=Sqrt(signal) and calculates
        # the function: N_avrg = 1/(DetEfficiency_avrg^-1)*sum(n_i*DetEfficiency_i^-1)
        # where the DetEfficiency = WB_signal_i/WB_average WB_signal_i is the White Beam Vanadium
        # signal on i-th detector and the WB_average -- average WB vanadium signal.
        # n_i is the modified signal
        signal_sum = sum(map(lambda e: e*e,error));
        weight_sum = sum(map(lambda s,e: e*e/s,signal,error));
        if( weight_sum==0.0):
            prop_man.log("WB integral has been calculated incorrectly, look at van_int workspace: {0}".format(deltaE_wkspaceName),'error')
            raise ArithmeticError("Division by 0 weight when calculating WB integrals from workspace {0}".format(deltaE_wkspaceName));
        norm_factor['Poisson']=signal_sum/weight_sum;
        #-------------------------------------------------------------------------
        # Guess which estimates value sum(n_i^2/Sigma_i^2)/sum(n_i/Sigma_i^2) TGP suggestion from 12-2012
        signal_sum = sum(map(lambda s,e: s*s/(e*e),signal,error));
        weight_sum = sum(map(lambda s,e: s/(e*e),signal,error));
        if( weight_sum==0.0):
            prop_man.log("WB integral has been calculated incorrectly, look at van_int workspace: {0}".format(deltaE_wkspaceName),'error')
            raise ArithmeticError("Division by 0 weight when calculating WB integrals from workspace {0}".format(deltaE_wkspaceName));
        norm_factor['TGP']=signal_sum/weight_sum;
        #
        #
        #integral_monovan=signal_sum /(wbVan_sum)
        van_multiplier = (float(propman.van_rmm)/float(van_mass))
        if ei_monovan >= 210.0:
            xsection = 421  # vanadium cross-section in mBarn/sR (402 mBarn/Sr) (!!!modified to fit high energy limit?!!!)
        else: # old textbook cross-section for vanadium for ei=20mEv
            xsection = 400 + (ei_monovan/10)
        sample_multiplier = (float(propman.sample_mass)/float(propman.sample_rmm))

        scale_factor = van_multiplier*sample_multiplier/xsection;

        for type,val in norm_factor.iteritems():
            norm_factor[type] = val*scale_factor;

        # check for NaN
        if (norm_factor['LibISIS']!=norm_factor['LibISIS'])|(izerc!=0):    # It is an error, print diagnostics:
           if (norm_factor['LibISIS'] !=norm_factor['LibISIS']):
               log_value = '\n--------> Absolute normalization factor is NaN <----------------------------------------------\n'
           else:
               log_value ='\n--------> Warning, Monovanadium has zero spectra <--------------------------------------------\n'
               log1_value = \
               "--------> Processing workspace: {0}\n"\
               "--------> Monovan Integration range : min={1}, max={2} (meV)\n"\
               "--------> Summed:  {3} spectra with total signal: {4} and error: {5}\n"\
               "--------> Dropped: {6} zero spectra\n"\
               "--------> Using  mBarn/sR*fu normalization factor = {7} resulting in:"\
               "--------> Abs norm factors: LibISIS: {8}\n"\
               "--------> Abs norm factors: Sigma^2: {9}\n"\
               "--------> Abs norm factors: Poisson: {10}"\
               "--------> Abs norm factors: TGP    : {11}\n"\
               .format(deltaE_wkspaceName,minmax[0],minmax[1],nhist,sum(signal),sum(error),izerc,scale_factor,
                          norm_factor['LibISIS'],norm_factor['SigSq'],norm_factor['Poisson'],norm_factor['TGP'])
           log_value = log_value+log1_value;
           propman.log(log_value,'error');
        else:
           DeleteWorkspace(Workspace=deltaE_wkspaceName)
           DeleteWorkspace(Workspace=data_ws)
        return (norm_factor['LibISIS'],norm_factor['SigSq'],norm_factor['Poisson'],norm_factor['TGP'])
#-------------------------------------------------------------------------------
    def set_motors(self,sample_wkspace,motor=None, offset=None):
        """calculate psi from sample environment motor and offset """

        if self._do_ISIS_normalization:
            default_offset=float('NaN')
            if not (motor is None) and sample_wkspace.getRun().hasProperty(motor):
                 default_offset = 0
        else:
            default_offset=0
        #
        if (offset is None):
            motor_offset = default_offset
        else:
            motor_offset = float(offset)

        #self.motor=0
        if not (motor is None):
        # Check if motor name exists
            if sample_wkspace.getRun().hasProperty(motor):
                motor=sample_wkspace.getRun()[motor].value[0]
                self.log("Motor value is %s" % motor)
            else:
                self.log("Could not find such sample environment log. Will use psi=offset")
        self.prop_man.psi = motor+motor_offset



    def convert_to_energy(self, mono_run, ei, white_run=None, mono_van=None,\
                          abs_ei=None, abs_white_run=None, save_path=None, Tzero=None, \
                          motor=None, offset=None):
        """
        One-shot function to convert the given runs to energy
        """

        self.prop_man.incident_energy = ei;      
        prop_man = self.prop_man;
        prop_man.set_input_parameters_ignore_nan(sample_run=mono_run,
                                                 wb_run = white_run,monovan_run=mono_van,
                                                 wb_for_monovan_run =abs_white_run);   # note not-supported second ei for monovan

        # Figure out what to call the workspace
        result_name = mono_run
        if not result_name is None:
            result_name = common.create_resultname(mono_run,prop_man.instr_name)

        # Main run file conversion
        sample_wkspace = self.mono_sample(mono_run, ei, white_run, prop_man.map_file,
                                          self.spectra_masks, result_name, Tzero)
        #if not norm_factor is None:
        #    sample_wkspace /= norm_factor
        
        # Save then finish
        self.save_results(sample_wkspace, save_path)
        # Clear loaded raw data to free up memory
        common.clear_loaded_data()

        return sample_wkspace


    def get_ei(self, data_ws,monitors_ws, resultws_name, ei_guess):
        """
        Calculate incident energy of neutrons and the time of the of the
        peak in the monitor spectrum
        The X data is corrected to set the first monitor peak at t=0 by subtracting
            t_mon + t_det_delay
        where the detector delay time is retrieved from the the instrument
        The position of the "source" component is also moved to match the position of
        the first monitor
        """
        prop_man = self.prop_man;
        fix_ei = prop_man.fix_ei;
        ei_mon_spectra = prop_man.ei_mon_spectra;
        self.prop_man.incident_energy  = ei_guess;

        #-------------------------------------------------------------
        # check if monitors are in the main workspace or provided separately
        data_ws,monitors_ws = self.check_monitor_ws(data_ws,monitors_ws,ei_mon_spectra);
        #------------------------------------------------

        # Calculate the incident energy
        ei,mon1_peak,mon1_index,tzero = \
            GetEi(InputWorkspace=monitors_ws, Monitor1Spec=int(ei_mon_spectra[0]), Monitor2Spec=int(ei_mon_spectra[1]),
                  EnergyEstimate=ei_guess,FixEi=prop_man.fix_ei)

        # Store found incident energy in the class itself
        self.prop_man.incident_energy = ei
        # copy incident energy obtained on monitor workspace to detectors workspace
        if data_ws != monitors_ws:
            AddSampleLog(Workspace=data_ws,LogName='Ei',LogText=str(ei),LogType='Number')
            # if monitors are separated from the input workspace, we need to move them too as this is what happening when monitors are integrated into workspace
            result_mon_name=resultws_name+'_monitors';
            ScaleX(InputWorkspace=monitors_ws,OutputWorkspace=result_mon_name,Operation="Add",Factor=-mon1_peak,
                   InstrumentParameter="DelayTime",Combine=True)


        # Adjust the TOF such that the first monitor peak is at t=0
        ScaleX(InputWorkspace=data_ws,OutputWorkspace=resultws_name,Operation="Add",Factor=-mon1_peak,
               InstrumentParameter="DelayTime",Combine=True)

        mon1_det = monitors_ws.getDetector(mon1_index)
        mon1_pos = mon1_det.getPos()
        src_name = data_ws.getInstrument().getSource().getName()
        MoveInstrumentComponent(Workspace=resultws_name,ComponentName= src_name, X=mon1_pos.getX(), Y=mon1_pos.getY(), Z=mon1_pos.getZ(), RelativePosition=False)
        return ei, mon1_peak

  
    def remap(self, result_ws, spec_masks, map_file):
        """
        Mask and group detectors based on input parameters
        """
        ws_name = result_ws.getName();
        if not spec_masks is None:
            MaskDetectors(Workspace=ws_name, MaskedWorkspace=spec_masks)
        if not map_file is None:
            GroupDetectors(InputWorkspace=ws_name,OutputWorkspace=ws_name,
                                       MapFile= map_file, KeepUngroupedSpectra=0, Behaviour='Average')

        return mtd[ws_name]

    def get_monitors_ws(self,data_ws,method,mon_number=None):
        """ get pointer to a workspace containing monitors. 

           Explores different ways of finding monitor workspace in Mantid and returns the python pointer to the
           workspace which contains monitors.
        """


        if mon_number is None:
            if method == 'monitor-2':
               mon_spectr_num=int(self.prop_man.mon2_norm_spec)
            else:
               mon_spectr_num=int(self.prop_man.mon1_norm_spec)
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

        done_log = "DirectInelasticReductionNormalisedBy"

        if done_log in data_ws.getRun() or method is None:
            if data_ws.name() != result_name:
                CloneWorkspace(InputWorkspace=data_ws, OutputWorkspace=result_name)
            output = mtd[result_name]
            return output;

        # Make sure we don't call this twice
        method = method.lower()


        if method[:7] == 'monitor': # 'monitor-1' or 'monitor-2' :
            # get monitor's workspace
            try:
                mon_ws,mon_index = self.get_monitors_ws(data_ws,method,mon_number)
            except :
                if self.__in_white_normalization: # we can normalize wb integrals by current separately as they often do not have monitors
                    method = 'current'
                else:
                    raise


        if method == 'monitor-1':
            range = self.prop_man.norm_mon_integration_range;
            range_min = range[0] + range_offset
            range_max = range[1] + range_offset


            output=NormaliseToMonitor(InputWorkspace=data_ws, OutputWorkspace=result_name, MonitorWorkspace=mon_ws, MonitorWorkspaceIndex=mon_index,
                                   IntegrationRangeMin=float(str(range_min)), IntegrationRangeMax=float(str(range_max)),IncludePartialBins=True)#,
# debug line:                                   NormalizationFactorWSName='NormMonWS'+data_ws.getName())
        elif method == 'monitor-2':

            # Found TOF range, correspondent to incident energy monitor peak;
            ei = self.prop_man.incident_energy;
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
        propman = self.prop_man;
        calibration = propman.det_cal_file;
        monitors_inWS=propman.load_monitors_with_workspace

        result_ws = common.load_runs(propman.instr_name, runs, calibration=calibration, sum=True,load_with_workspace=monitors_inWS)

        mon_WsName = result_ws.getName()+'_monitors'
        if mon_WsName in mtd  and not monitors_inWS:
            monitor_ws = mtd[mon_WsName];
        else:
            if monitors_inWS:
                monitor_ws = result_ws;
            else:
                monitor_ws = None;

        old_result = result_ws;
        if new_ws_name != None :
            mon_WsName = new_ws_name+'_monitors'
            if keep_previous_ws:
                result_ws = CloneWorkspace(InputWorkspace = result_ws,OutputWorkspace = new_ws_name)
                if monitor_ws and monitor_ws != old_result:
                    monitor_ws = CloneWorkspace(InputWorkspace = monitor_ws,OutputWorkspace = mon_WsName)
            else:
                result_ws = RenameWorkspace(InputWorkspace=result_ws,OutputWorkspace=new_ws_name)
                if monitor_ws:
                    if monitor_ws== old_result:
                        monitor_ws= result_ws;
                    else:
                        monitor_ws=RenameWorkspace(InputWorkspace=monitor_ws,OutputWorkspace=mon_WsName)
                    #
        
        self.setup_instrument_properties(result_ws)
        # TODO: is it possible to have spectra_to_monitors_list defined and monitors loaded with workspace? then all below will fail miserably. 
        spec_to_mon =  propman.spectra_to_monitors_list;
        if monitor_ws and spec_to_mon :
            for specID in spec_to_mon:
                result_ws,monitor_ws=self.copy_spectrum2monitors(result_ws,monitor_ws,specID);

        return (result_ws,monitor_ws)

    @staticmethod
    def check_monitor_ws(data_ws,monitor_ws,ei_mon_spectra):
        """ Check if monitors spectra are indeed located in monitor workspace and if not, try to 
            find them in data workspace

            return tuple of workspaces, with first is data and second -- monitors workspace pointers
            both can point to the same workspace if monitors and data are located in the same workspace

            Raise if can not found monitors in any workspace 
        """ 

        if isinstance(monitor_ws,str):
            monitor_ws = mtd[monitor_ws]
        if isinstance(data_ws,str): 
            data_ws = mtd[data_ws]
        for nspec in ei_mon_spectra:
            # in case it is list of strings
            nsp = int(nspec)
            try:
                # check if the spectra with correspondent number is present in the workspace
                nsp = monitor_ws.getIndexFromSpectrumNumber(nspec);
            except RuntimeError as err:
                mon_ws = data_ws.getName()+'_monitors'
                try:
                    monitor_ws = mtd[mon_ws];
                except:
                    monitor_ws=data_ws
                # no spectra in data workspace

                try:
                    # check if the spectra with correspondent number is present in the data workspace
                    nsp = monitor_ws.getIndexFromSpectrumNumber(nspec);
                except RuntimeError as err:
                    print "**** ERROR while attempting to get spectra {0} from workspace: {1}, error: {2} ".format(nsp,monitor_ws.getName(), err)
                    raise
            #end No spectra in initial monitor ws


        return (data_ws,monitor_ws)

    @staticmethod
    def copy_spectrum2monitors(wsName,monWSName,spectraID):
       """
        this routine copies a spectrum form workspace to monitor workspace and rebins it according to monitor workspace binning

        @param wsName    -- the name of event workspace which detector is considered as monitor or Mantid pointer to this workspace
        @param monWSName -- the name of histogram workspace with monitors where one needs to place the detector's spectra or Mantid pointer to this workspace
        @param spectraID -- the ID of the spectra to copy.

         TODO: As extract single spectrum works only with WorkspaceIndex, we have to assume that WorkspaceIndex=spectraID-1;
         this is not always correct, so it is better to change ExtractSingleSpectrum to accept workspaceID
       """
       if isinstance(wsName,str):
           ws = mtd[wsName]
       else:
           ws = wsName;
       if isinstance(monWSName,str):
            monWS = mtd[monWSName];
       else:
           monWS = monWSName;
       # ----------------------------
       try:
           ws_index = monWS.getIndexFromSpectrumNumber(spectraID)
           # Spectra is already in the monitor workspace
           return (ws,monWS)
       except:
           ws_index = ws.getIndexFromSpectrumNumber(spectraID)



       #
       x_param = monWS.readX(0);
       bins = [x_param[0],x_param[1]-x_param[0],x_param[-1]];
       ExtractSingleSpectrum(InputWorkspace=ws,OutputWorkspace='tmp_mon',WorkspaceIndex=ws_index)
       Rebin(InputWorkspace='tmp_mon',OutputWorkspace='tmp_mon',Params=bins,PreserveEvents='0')
       # should be vice versa but Conjoin invalidate ws pointers and hopefully nothing could happen with workspace during conjoining
       #AddSampleLog(Workspace=monWS,LogName=done_log_name,LogText=str(ws_index),LogType='Number');
       mon_ws_name = monWS.getName();
       ConjoinWorkspaces(InputWorkspace1=monWS,InputWorkspace2='tmp_mon')
       monWS=mtd[mon_ws_name]

       if 'tmp_mon' in mtd:
           DeleteWorkspace(WorkspaceName='tmp_mon');
       return (ws,monWS);

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
    #---------------------------------------------------------------------------
    # Behind the scenes stuff
    #---------------------------------------------------------------------------

    def __init__(self, instr_name=None,reload_instrument=False):
        """
        Constructor
        """
        if instr_name:
            self.initialise(instr_name,reload_instrument);
        else:
            self._propMan = None;
            #
            self._keep_wb_workspace = False;
            self._do_ISIS_normalization = True;
            self.spectra_masks = None;
        #end

 
    def initialise(self, instr,reload_instrument=False):
        """
        Initialize the private attributes of the class and the nullify the attributes which expected to be always set-up from a calling script
        """
        # Internal properties and keys
        self._keep_wb_workspace = False;
        self._do_ISIS_normalization = True;
        self.spectra_masks = None;


        # Instrument and default parameter setup
        # formats available for saving. As the reducer has to have a method to process one of this, it is private property
        if not hasattr(self,'_propMan') or self._propMan is None:
            if isinstance(instr,DirectPropertyManager):
                self._propMan  = instr;
            else:
                self._propMan = DirectPropertyManager(instr);
        else:
            old_name = self._propMan.instrument.getName();
            if isinstance(instr,geometry._geometry.Instrument):
                new_name = self._propMan.instrument.getName();
            elif isinstance(instr,DirectPropertyManager):
                new_name = instr.instr_name;
            else:
                new_name = instr
            #end if
            if old_name != new_name or reload_instrument:
                self._propMan = DirectPropertyManager(new_name);
            #end if
        #

    def setup_instrument_properties(self, workspace = None,reload_instrument=False):
        if workspace != None:
            instrument = workspace.getInstrument();
            name      = instrument.getName();
            if name != self.prop_man.instr_name:
               self.prop_man = DirectPropertyManager(name,workspace);


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

def get_failed_spectra_list_from_masks(masking_wksp):
    """Compile a list of spectra numbers that are marked as
       masked in the masking workspace

    Input:
     masking_workspace -  A special masking workspace containing masking data
    """
    if type(masking_wksp) == str:
        masking_wksp = mtd[masking_wksp]

    failed_spectra = []
    if masking_wksp is None:
       return (failed_spectra,0);

    n_spectra = masking_wksp.getNumberHistograms()
    for i in xrange(n_spectra):
        if masking_wksp.readY(i)[0] >0.99 : # spectrum is masked
            failed_spectra.append(masking_wksp.getSpectrum(i).getSpectrumNo())

    return (failed_spectra,n_spectra)


#-----------------------------------------------------------------
if __name__=="__main__":
    pass
    #unittest.main()