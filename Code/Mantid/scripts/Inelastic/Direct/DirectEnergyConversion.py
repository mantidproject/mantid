from mantid.simpleapi import *
from mantid.kernel import funcreturns
from mantid import geometry
from mantid import api

import time as time
import os.path, copy

import Direct.CommonFunctions  as common
import Direct.diagnostics      as diagnostics
from Direct.PropertyManager  import PropertyManager
from Direct.RunDescriptor    import RunDescriptor
from Direct.ReductionHelpers import extract_non_system_names


def setup_reducer(inst_name,reload_instrument=False):
    """
    Given an instrument name or prefix this sets up a converter
    object for the reduction
    """
    try:
        return DirectEnergyConversion(inst_name,reload_instrument)
    except RuntimeError:
        raise RuntimeError('Unknown instrument "%s" or wrong IDF file for this instrument, cannot continue' % inst_name)


class DirectEnergyConversion(object):
    """
    Performs a convert to energy assuming the provided instrument is 
    an direct inelastic geometry instrument

    The class defines various methods to allow users to convert their
    files to Energy transfer

    Usage:
    >>red = DirectEnergyConversion('InstrumentName')
      and then: 
    >>red.convert_to_energy(wb_run,sample_run,ei_guess,rebin)
      or
    >>red.convert_to_energy(wb_run,sample_run,ei_guess,rebin,**arguments)
      or
    >>red.convert_to_energy(wb_run,sample_run,ei_guess,rebin,mapfile,**arguments)
      or
    >>red.prop_man.sample_run = run number
    >>red.prop_man.wb_run     = Whitebeam 
    >>red.prop_man.incident_energy = energy guess
    >>red.prop_man.energy_bins = [min_val,step,max_val]
    >>red.convert_to_energy()

    Where:
    Whitebeam run number or file name or workspace
    sample_run  sample run number or file name or workspace
    ei_guess    suggested value for incident energy of neutrons in direct inelastic instrument
    energy_bins energy binning requested for resulting spe workspace. 
    mapfile     Mapfile -- if absent/'default' the defaults from IDF are used
    monovan_run If present will do the absolute units normalization. Number of additional parameters
                specified in **kwargs is usually requested for this. If they are absent,
                program uses defaults, but the defaults (e.g. sample_mass or sample_rmm )
                are usually incorrect for a particular run.
    arguments   The dictionary containing additional keyword arguments.
                The list of allowed additional arguments is defined in InstrName_Parameters.xml 
                file, located in:
                MantidPlot->View->Preferences->Mantid->Directories->Parameter Definitions

    Usage examples:
    with run numbers as input:
    >>red.convert_to_energy(1000,10001,80,[-10,.1,70])  # will run on default instrument

    >>red.convert_to_energy(1000,10001,80,[-10,.1,70],'mari_res', additional keywords as required)

    >>red.convert_to_energy(1000,10001,80,'-10,.1,70','mari_res',fixei=True)

    A detector calibration file must be specified if running the reduction with workspaces as input
    namely:
    >>w2=cred.onvert_to_energy('wb_wksp','run_wksp',ei,rebin_params,mapfile,det_cal_file=cal_file
                                        ,diag_remove_zero=False,norm_method='current')


     All available keywords are provided in InstName_Parameters.xml file

   Some samples are:
   norm_method =[monitor-1],[monitor-2][Current]
   background  =False , True
   fixei       =False , True
   save_format =['.spe'],['.nxspe'],'none'
   detector_van_range          =[20,40] in mev

   bkgd_range  =[15000,19000]  :integration range for background tests

   second_white     - If provided an additional set of tests is performed on this. 
                         (default = None)
   hardmaskPlus     - A file specifying those spectra that should be masked
                         without testing (default=None)
   tiny             - Minimum threshold for acceptance (default = 1e-10)
   large            - Maximum threshold for acceptance (default = 1e10)
   bkgd_range       - A list of two numbers indicating the background range 
                         (default=instrument defaults)
   diag_van_median_rate_limit_lo  - Lower bound defining outliers as fraction of median value (default = 0.01)
   diag_van_median_rate_limit_hi  - Upper bound defining outliers as fraction of median value (default = 100.)
   diag_van_median_sigma_lo       - Fraction of median to consider counting low for the white beam diag (default = 0.1)
   diag_van_median_sigma_hi       - Fraction of median to consider counting high for the white beam diag (default = 1.5)
   diag_van_sig  - Error criterion as a multiple of error bar i.e. to fail the test, the magnitude of the
                   difference with respect to the median value must also exceed this number of error bars (default=0.0)
   diag_remove_zero                - If true then zeroes in the vanadium data will count as failed (default = True)
   diag_samp_samp_median_sigma_lo  - Fraction of median to consider counting low for the white beam diag (default = 0)
   diag_samp_samp_median_sigma_hi  - Fraction of median to consider counting high for the white beam diag (default = 2.0)
   diag_samp_sig        - Error criterion as a multiple of error bar i.e. to fail the test, the magnitude of the
                          difference with respect to the median value must also exceed this number of
                          error bars (default=3.3)
   variation       - The number of medians the ratio of the first/second white beam can deviate from
                     the average by (default=1.1)
   bleed_test      - If true then the CreatePSDBleedMask algorithm is run
   bleed_maxrate   - If the bleed test is on then this is the maximum framerate allowed in a tube
   bleed_pixels    - If the bleed test is on then this is the number of pixels ignored within the
                       bleed test diagnostic
   print_diag_results - If True then the results are printed to the screen

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
#-------------------------------------------------------------------------------
    def diagnose(self, white,diag_sample=None,**kwargs):
      """ run diagnostics on the provided workspaces.

     this method does some additional processing before moving on to the diagnostics:
      1) Computes the white beam integrals, converting to energy
      2) Computes the background integral using the instrument defined range
      3) Computes a total count from the sample

     these inputs are passed to the diagnostics functions

     required inputs:

      white  - A workspace, run number or filepath of a white beam run. A workspace is assumed to
               have simple been loaded and nothing else.

     optional inputs:

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
      """
      lhs_names = funcreturns.lhs_info('names')
      if len(lhs_names) > 0:
          var_name = lhs_names[0]
      else:
          var_name = "diag_mask"

      # modify properties using input arguments
      self.prop_man.set_input_parameters(**kwargs)
      # obtain proper run descriptor in case it is not a run descriptor but
      # something else
      white = self.get_run_descriptor(white)

      # return all diagnostics parameters
      diag_params = self.prop_man.get_diagnostics_parameters()

      if self.use_hard_mask_only:
         if mtd.doesExist('hard_mask_ws'):
            diag_mask = mtd['hard_mask_ws']
         else: # build hard mask
               # in this peculiar way we can obtain working mask which
               # accounts for initial data grouping in the
               # data file. SNS or 1 to 1 maps may probably avoid this
               # stuff and can load masks directly
            white_data = white.get_ws_clone('white_ws_clone')

            diag_mask = LoadMask(Instrument=self.instr_name,InputFile=self.hard_mask_file,
                               OutputWorkspace='hard_mask_ws')
            MaskDetectors(Workspace=white_data, MaskedWorkspace=diag_mask)
            DeleteWorkspace(diag_mask)
            diag_mask,masked_list = ExtractMask(InputWorkspace=white_data)
            DeleteWorkspace(Workspace='white_ws_clone')

         return diag_mask


      # Get the white beam vanadium integrals
      whiteintegrals = self.do_white(white, None, None) # No grouping yet
      if self.second_white:
         second_white = self.second_white
         other_whiteintegrals = self.do_white(PropertyManager.second_white, None, None) # No grouping yet
         self.second_white = other_whiteintegrals

      # Get the background/total counts from the sample run if present
      if diag_sample: 
         diag_sample = self.get_run_descriptor(diag_sample)
         # If the bleed test is requested then we need to pass in the
         # sample_run as well
         if self.bleed_test:
            # initiate reference to reducer to be able to work with Run
            # Descriptors
            diagnostics.__Reducer__ = self
            diag_params['sample_run'] = diag_sample

         # Set up the background integrals for diagnostic purposes
         result_ws = self.normalise(diag_sample, self.normalise_method)

         #>>> here result workspace is being processed -- not touching
         #result ws
         bkgd_range = self.background_test_range
         background_int = Integration(result_ws,
                           RangeLower=bkgd_range[0],RangeUpper=bkgd_range[1],
                           IncludePartialBins=True)
         total_counts = Integration(result_ws, IncludePartialBins=True)
         background_int = ConvertUnits(background_int, Target="Energy",EMode='Elastic', AlignBins=0)
         self.prop_man.log("Diagnose: finished convertUnits ",'information')

         background_int *= self.scale_factor
         diagnostics.normalise_background(background_int, whiteintegrals,
                                           diag_params.get('second_white',None))
         diag_params['background_int'] = background_int
         diag_params['sample_counts'] = total_counts

      # Check how we should run diag
      diag_spectra_blocks = self.diag_spectra
      if diag_spectra_blocks is None:
         # Do the whole lot at once
         diagnostics.diagnose(whiteintegrals, **diag_params)
      else:
         for index, bank in enumerate(diag_spectra_blocks):
             diag_params['start_index'] = bank[0] - 1
             diag_params['end_index'] = bank[1] - 1
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

#-------------------------------------------------------------------------------
    def convert_to_energy(self,wb_run=None,sample_run=None,ei_guess=None,rebin=None,map_file=None,
                          monovan_run=None,wb_for_monovan_run=None,**kwargs):
      """ One step conversion of run into workspace containing information about energy transfer

      """ 
      # Support for old reduction interface:
      self.prop_man.set_input_parameters_ignore_nan\
           (wb_run=wb_run,sample_run=sample_run,incident_energy=ei_guess,energy_bins=rebin,
            map_file=map_file,monovan_run=monovan_run,wb_for_monovan_run=wb_for_monovan_run)
      #
      self.prop_man.set_input_parameters(**kwargs)

      # output workspace name.
      try:
          n,r = funcreturns.lhs_info('both')
          out_ws_name = r[0]
      except:
          out_ws_name = None



      # inform user on what parameters have changed from script or gui
      # if monovan present, check if abs_norm_ parameters are set
      self.prop_man.log_changed_values('notice')
      prop_man = self.prop_man
      #process complex parameters

      start_time = time.time()

     # check if reducer can find all non-run files necessary for the reduction
     # before starting long run.
      #TODO:
      # Reducer.check_necessary_files(monovan_run)

  
      sample_ws = PropertyManager.sample_run.get_workspace()

      # Update reduction properties which may change in the workspace but have
      # not been modified from input parameters.
      # E.g.  detector number have changed
      oldChanges = self.prop_man.getChangedProperties()
      allChanges = self.prop_man.update_defaults_from_instrument(sample_ws.getInstrument())
      workspace_defined_prop = allChanges.difference(oldChanges)
      if len(workspace_defined_prop) > 0:
          prop_man.log("****************************************************************")
          prop_man.log('*** Sample run {0} properties change default reduction properties: '.\
                       format(PropertyManager.sample_run.get_ws_name()))
          prop_man.log_changed_values('notice',False,oldChanges)
          prop_man.log("****************************************************************")



      masking = None
      masks_done = False
      if not prop_man.run_diagnostics:
          header = "*** Diagnostics including hard masking is skipped "
          masks_done = True
      #if Reducer.save_and_reuse_masks :
      # SAVE AND REUSE MASKS
      if self.spectra_masks:
         masks_done = True
#--------------------------------------------------------------------------------------------------
#  Diagnostics here
# -------------------------------------------------------------------------------------------------
     # diag the sample and detector vanadium.  It will deal with hard mask only
     # if it is set that way
      if not masks_done:
        prop_man.log("======== Run diagnose for sample run ===========================",'notice')
        masking = self.diagnose(PropertyManager.wb_run,PropertyManager.mask_run,
                                second_white=None,print_diag_results=True)
        if prop_man.use_hard_mask_only:
            header = "*** Hard mask file applied to workspace with {0:d} spectra masked {1:d} spectra"
        else:
            header = "*** Diagnostics processed workspace with {0:d} spectra and masked {1:d} bad spectra"


        # diagnose absolute units:
        if self.monovan_run != None :
            if self.mono_correction_factor == None :
                if self.use_sam_msk_on_monovan == True:
                    prop_man.log('  Applying sample run mask to mono van')
                else:
                    if not self.use_hard_mask_only : # in this case the masking2 is different but 
                                                     #points to the same workspace
                                                     # Should be better solution for that.
                        prop_man.log("======== Run diagnose for monochromatic vanadium run ===========",'notice')

                        masking2 = self.diagnose(PropertyManager.wb_for_monovan_run,PropertyManager.monovan_run,
                                         second_white = None,print_diag_results=True)
                        masking +=  masking2
                        DeleteWorkspace(masking2)


            else: # if Reducer.mono_correction_factor != None :
                pass
        # Very important statement propagating masks for further usage in
        # convert_to_energy.
        # This property is also directly accessible from GUI.
        self.spectra_masks = masking
         # save mask if it does not exist and has been already loaded
         #if Reducer.save_and_reuse_masks and not masks_done:
         #    SaveMask(InputWorkspace=masking,OutputFile =
         #    mask_file_name,GroupedDetectors=True)
      else:
          header = '*** Using stored mask file for workspace with  {0} spectra and {1} masked spectra'
          masking = self.spectra_masks
 
      # estimate and report the number of failing detectors
      failed_sp_list,nMaskedSpectra = get_failed_spectra_list_from_masks(masking)
      nSpectra = masking.getNumberHistograms()
      prop_man.log(header.format(nSpectra,nMaskedSpectra),'notice')

      #Run the conversion first on the sample
      deltaE_wkspace_sample = self.mono_sample(PropertyManager.sample_run,self.incident_energy,PropertyManager.wb_run,
                                               self.map_file,masking)

 
      # calculate absolute units integral and apply it to the workspace
      if self.monovan_run != None or self.mono_correction_factor != None :
         deltaE_wkspace_sample = self.apply_absolute_normalization(deltaE_wkspace_sample,PropertyManager.monovan_run,\
                                                                   self.incident_energy,PropertyManager.wb_for_monovan_run)
      # ensure that the sample_run name is intact with workspace
      PropertyManager.sample_run.synchronize_ws(deltaE_wkspace_sample)


      results_name = deltaE_wkspace_sample.name()
      if out_ws_name and results_name != out_ws_name:
         RenameWorkspace(InputWorkspace=results_name,OutputWorkspace=out_ws_name)


      ei = (deltaE_wkspace_sample.getRun().getLogData("Ei").value)
      prop_man.log("*** Incident energy found for sample run: {0} meV".format(ei),'notice')

      end_time = time.time()
      prop_man.log("*** Elapsed time = {0} sec".format(end_time - start_time),'notice')

    # Hack for multirep mode?
#    if mtd.doesExist('hard_mask_ws') == True:
 #       DeleteWorkspace(Workspace='hard_mask_ws')

      # SNS or GUI motor stuff
      self.calculate_rotation(deltaE_wkspace_sample)
      #
      self.save_results(deltaE_wkspace_sample)
      #
      # CLEAN-up (may be worth to do in separate procedure)
      # Currently clear masks unconditionally TODO: cash masks with appropriate
      # precautions
      self.spectra_masks = None
      self.prop_man.sample_run = None # clean up memory of the sample run

      if self._multirep_mode and ('bkgr_ws' in mtd):
         DeleteWorkspace(bkgr_ws)


      return deltaE_wkspace_sample

    def do_white(self, run, spectra_masks=None, map_file=None):
        """
        Create the workspace, which each spectra containing the correspondent white beam integral (single value)

        These integrals are used as estimate for detector efficiency in wide range of energies
        (rather the detector electronic's efficiency as the geuger counters are very different in efficiency)
        and is used to remove the influence of this efficiency to the different detectors.
        """
        white_ws = self._get_wb_inegrals(run)
        # Masks may change in different runs
        # Masking and grouping
        white_ws = self.remap(white_ws, spectra_masks, map_file)

        # White beam scale factor
        white_ws *= self.wb_scale_factor
        return white_ws

    def mono_sample(self, mono_run, ei_guess, white_run=None, map_file=None,
                    spectra_masks=None, result_name=None, Tzero=None):
        """Convert a mono-chromatic sample run to DeltaE.
        If multiple run files are passed to this function, they are summed into a run and then processed
        """
        mono_run = self.get_run_descriptor(mono_run)
        if white_run:
            white_run = self.get_run_descriptor(white_run)


        mono_s = self._do_mono(mono_run, ei_guess,
                             white_run, map_file, spectra_masks, Tzero)
        return mono_s

#-------------------------------------------------------------------------------
    def calculate_rotation(self,sample_wkspace,motor=None, offset=None):
        """calculate psi from sample environment motor and offset
       
           TODO: should probably go to properties
        """

        self.prop_man.set_input_parameters_ignore_nan(motor_name=motor,offset=offset)
        motor = self.prop_man.motor_name
        offset = self.prop_man.motor_offset

        #
        if (offset is None):
            motor_offset = float('nan')
        else:
            motor_offset = float(offset)

        if motor:
        # Check if motor name exists
            if sample_wkspace.getRun().hasProperty(motor):
                motor_rotation = sample_wkspace.getRun()[motor].value[0]
                self.prop_man.log("Motor {0} rotation is {1}".format(motor,motor_rotation))
            else:
                self.prop_man.log("Could not find such sample environment log. Will use psi=motor_offset")
                motor_rotation = 0
        else:
           motor_rotation = float('nan')
        self.prop_man.psi = motor_rotation + motor_offset
#-------------------------------------------------------------------------------
    def get_ei(self, data_run, ei_guess):
        """
        Calculate incident energy of neutrons and the time of the of the
        peak in the monitor spectrum
        The X data is corrected to set the first monitor peak at t=0 by subtracting
            t_mon + t_det_delay
        where the detector delay time is retrieved from the the instrument
        The position of the "source" component is also moved to match the position of
        the first monitor
        """
        data_run = self.get_run_descriptor(data_run)

        fix_ei = self.fix_ei
        ei_mon_spectra = self.ei_mon_spectra

        data_ws = data_run.get_workspace()
        monitor_ws = data_run.get_monitors_ws()
        separate_monitors = data_run.is_monws_separate()
        data_run.set_action_suffix('_shifted')

        ##-------------------------------------------------------------
        ## check if monitors are in the main workspace or provided separately
        #data_ws,monitors_ws =
        #self.check_monitor_ws(data_ws,monitors_ws,ei_mon_spectra)
        ##------------------------------------------------

        # Calculate the incident energy
        ei,mon1_peak,mon1_index,tzero = \
            GetEi(InputWorkspace=monitor_ws, Monitor1Spec=int(ei_mon_spectra[0]),
                  Monitor2Spec=int(ei_mon_spectra[1]),
                  EnergyEstimate=ei_guess,FixEi=fix_ei)

        # Store found incident energy in the class itself
        self.incident_energy = ei
        if self.prop_man.normalise_method == 'monitor-2' and not separate_monitors:
           # monitor-2 normalization ranges have to be identified before the
           # instrument is shifted in case it is shifted to this monitor (usual
           # case)
           #Find TOF range, correspondent to incident energy monitor peak
           energy_rage = self.mon2_norm_energy_range
           self._mon2_norm_time_range = self.get_TOF_for_energies(monitor_ws,energy_rage,
                                                                 [self.mon2_norm_spec],self._debug_mode)
        #end
        if separate_monitors:
            # copy incident energy obtained on monitor workspace to detectors
            # workspace
            AddSampleLog(Workspace=data_ws,LogName='Ei',LogText=str(ei),LogType='Number')


        resultws_name = data_ws.name()
        # Adjust the TOF such that the first monitor peak is at t=0
        ScaleX(InputWorkspace=data_ws,OutputWorkspace=resultws_name,Operation="Add",Factor=-mon1_peak,
               InstrumentParameter="DelayTime",Combine=True)

        # shift to monitor used to calculate energy transfer
        spec_num = monitor_ws.getIndexFromSpectrumNumber(int(ei_mon_spectra[0])) 
        mon1_det = monitor_ws.getDetector(spec_num)
        mon1_pos = mon1_det.getPos()
        src_name = data_ws.getInstrument().getSource().getName()
        MoveInstrumentComponent(Workspace=resultws_name,ComponentName= src_name, X=mon1_pos.getX(), 
                                Y=mon1_pos.getY(), Z=mon1_pos.getZ(), RelativePosition=False)
 
        #
        data_run.synchronize_ws(mtd[resultws_name])
        return ei, mon1_peak

#-------------------------------------------------------------------------------
    def remap(self, result_ws, spec_masks, map_file):
        """
        Mask and group detectors based on input parameters
        """
        ws_name = result_ws.getName()
        if not spec_masks is None:
            MaskDetectors(Workspace=ws_name, MaskedWorkspace=spec_masks)
        if not map_file is None:
            GroupDetectors(InputWorkspace=ws_name,OutputWorkspace=ws_name,
                                       MapFile= map_file, KeepUngroupedSpectra=0, Behaviour='Average')

        return mtd[ws_name]
#-------------------------------------------------------------------------------
    def normalise(self, run, method, range_offset=0.0,external_monitors_ws=None):
        """
        Apply normalization using specified source
        """
        run = self.get_run_descriptor(run)

        data_ws = run.get_workspace()
        old_ws_name = data_ws.name()

        result_name = run.set_action_suffix('_normalized')


        # Make sure we don't call this twice
        done_log = "DirectInelasticReductionNormalisedBy"
        if done_log in data_ws.getRun() or method is None:
            run.synchronize_ws(data_ws)
            output = mtd[result_name]
            return output

        method = method.lower()
        for case in common.switch(method):
            if case('monitor-1'):
               method,old_ws_name = self._normalize_to_monitor1(run,old_ws_name, range_offset,external_monitors_ws)
               break
            if case('monitor-2'):
               method,old_ws_name = self._normalize_to_monitor2(run,old_ws_name, range_offset,external_monitors_ws)
               break
            if case('current'):
                NormaliseByCurrent(InputWorkspace=old_ws_name,OutputWorkspace=old_ws_name)
                break
            if case(): # default
               raise RuntimeError('Normalization method {0} not found. It must be one of monitor-1, monitor-2, current, or None'.format(method))
        #endCase


        # Add a log to the workspace to say that the normalization has been
        # done
        output = mtd[old_ws_name]
        AddSampleLog(Workspace=output, LogName=done_log,LogText=method)
        run.synchronize_ws(output)
        return output
    #
    def _normalize_to_monitor1(self,run,old_name,range_offset=0.0,external_monitor_ws=None):
        """ Helper method implementing  normalize_to_monitor1 """ 

        # get monitor's workspace
        separate_monitors = run.is_monws_separate()
        if external_monitor_ws:
           separate_monitors=True
           mon_ws = external_monitor_ws
        else:
           mon_ws = run.get_monitors_ws()

        if not mon_ws: # no monitors
           if self.__in_white_normalization: # we can normalize wb integrals by current separately as they often do not
                                             # have monitors
              self.normalise(run,'current',range_offset)
              new_name = run.get_ws_name()
              return ('current',new_name)
           else:
              raise RuntimeError('Normalise by monitor-1:: Workspace {0} for run {1} does not have monitors in it'\
                   .format(run.get_ws_name(),run.__get__()))


        range = self.norm_mon_integration_range
        if self._debug_mode:
           kwargs = {'NormFactorWS':'NormMon1_WS' + data_ws.getName()}
        else:
           kwargs = {}

        mon_spect = self.prop_man.mon1_norm_spec
        if separate_monitors:
            kwargs['MonitorWorkspace'] = mon_ws
            kwargs['MonitorWorkspaceIndex'] = int(mon_ws.getIndexFromSpectrumNumber(int(mon_spect)))
            range_min = float(range[0])
            range_max = float(range[1])
        else:
            kwargs['MonitorSpectrum'] = int(mon_spect) # shame TODO: change algorithm
            range_min = float(range[0] + range_offset)
            range_max = float(range[1] + range_offset)



        NormaliseToMonitor(InputWorkspace=old_name,OutputWorkspace=old_name, IntegrationRangeMin=range_min, 
                           IntegrationRangeMax=range_max,IncludePartialBins=True,**kwargs)
        return ('monitor-1',old_name)
    #
    def _normalize_to_monitor2(self,run,old_name, range_offset=0.0,external_monitor_ws=None):
        """ Helper method implementing  normalize_to_monitor_2 """ 

      # get monitor's workspace
        separate_monitors = run.is_monws_separate()
        if external_monitor_ws:
           separate_monitors=True
           mon_ws = external_monitor_ws
        else:
           mon_ws = run.get_monitors_ws()

 
        if not mon_ws: # no monitors
           if self.__in_white_normalization: # we can normalize wb integrals by current separately as they often do not
                                             # have monitors
              self.normalise(run,'current',range_offset)
              new_name = run.get_ws_name()
              return ('current',new_name)
           else:
              raise RuntimeError('Normalize by monitor-2:: Workspace {0} for run {1} does not have monitors in it'\
                   .format(run.get_ws_name(),run.__get__()))
        #
        if self._debug_mode:
           kwargs = {'NormFactorWS':'NormMon2_WS' + mon_ws.getName()}
        else:
           kwargs = {}

        mon_spect = self.prop_man.mon2_norm_spec
        mon_index = int(mon_ws.getIndexFromSpectrumNumber(mon_spect))
        if separate_monitors:
            kwargs['MonitorWorkspace'] = mon_ws
            kwargs['MonitorWorkspaceIndex'] = mon_index
        else:
            kwargs['MonitorSpectrum'] = mon_spect

        #Find TOF range, correspondent to incident energy monitor peak
        if self._mon2_norm_time_range: # range has been found during ei-calculations
           range = self._mon2_norm_time_range
           range_min = range[0] + range_offset
           range_max = range[1] + range_offset
           self._mon2_norm_time_range = None
        else:
           mon_ws_name = mon_ws.name() #monitor's workspace and detector's workspace are e
           if mon_ws_name.find('_shifted') != -1:
              # monitor-2 normalization ranges have to be identified before the
              # instrument is shifted
              raise RuntimeError("Instrument have been shifted but no time range has been identified. Monitor-2 normalization can not be performed ") 
           else:
              # instrument and workspace shifted, so TOF will be calculated wrt shifted instrument
              energy_rage = self.mon2_norm_energy_range
              TOF_range = self.get_TOF_for_energies(mon_ws,energy_rage,[mon_spect],self._debug_mode)
              range_min = TOF_range[0]
              range_max = TOF_range[1]

       # Normalize to monitor 2
        NormaliseToMonitor(InputWorkspace=old_name,OutputWorkspace=old_name,IntegrationRangeMin=range_min, 
                           IntegrationRangeMax=range_max,IncludePartialBins=True,**kwargs)
        return ('monitor-2',old_name)
#-------------------------------------------------------------------------------
#-------------------------------------------------------------------------------
    #
    @staticmethod
    def get_TOF_for_energies(workspace,energy_list,specID_list,debug_mode=False):
        """ Method to find what TOF range corresponds to given energy range             
           for given workspace and detectors.

           Input:
           workspace    pointer to workspace with instrument attached. 
           energy_list  the list of input energies to process
           detID_list   list of detectors to find 

           Returns: 
        """ 
        template_ws_name = '_energy_range_ws'
        range_ws_name = '_TOF_range_ws'
        y = [1] * (len(energy_list) - 1)
        TOF_range = []
        for specID in specID_list:
            ind = workspace.getIndexFromSpectrumNumber(specID)
            ExtractSingleSpectrum(InputWorkspace=workspace, OutputWorkspace=template_ws_name, WorkspaceIndex=ind)
            CreateWorkspace(OutputWorkspace=range_ws_name,NSpec = 1,DataX=energy_list,DataY=y,UnitX='Energy',ParentWorkspace=template_ws_name)
            range_ws = ConvertUnits(InputWorkspace=range_ws_name,OutputWorkspace=range_ws_name,Target='TOF',EMode='Elastic')
            x = range_ws.dataX(0)
            TOF_range.append(x.tolist())

        if not debug_mode:
            DeleteWorkspace(template_ws_name)
            DeleteWorkspace(range_ws_name)
        #
        if len(specID_list) == 1:
            TOF_range = TOF_range[0]

        return TOF_range

    def save_results(self, workspace, save_file=None, formats=None):
        """
        Save the result workspace to the specified filename using the list of formats specified in
        formats. If formats is None then the default list is used
        """
        if formats:
           # clear up existing save formats as one is defined in parameters
            self.prop_man.save_format = None

        self.prop_man.set_input_parameters_ignore_nan(save_file_name=save_file,save_format=formats)

        #TODO: deal with this.  This all should be incorporated in sample_run
        save_file = self.prop_man.save_file_name
        formats = self.prop_man.save_format
        if save_file is None:
            save_file = workspace.getName()
        elif os.path.isdir(save_file):
            save_file = os.path.join(save_file, workspace.getName())
        elif save_file == '':
            raise ValueError('Empty filename is not allowed for saving')
        else:
            pass

        prop_man = self.prop_man 
         
        save_file,ext = os.path.splitext(save_file)
        if len(ext) > 1:
            formats.add(ext[1:])


        for file_format  in formats:
            for case in common.switch(file_format):
                if case('nxspe'):
                   filename = save_file + '.nxspe'
                   SaveNXSPE(InputWorkspace=workspace,Filename= filename, KiOverKfScaling=prop_man.apply_kikf_correction,psi=prop_man.psi)
                   break
                if case('spe'):
                   filename = save_file + '.spe'
                   SaveSPE(InputWorkspace=workspace,Filename= filename)
                   break
                if case('nxs'):
                   filename = save_file + '.nxs'
                   SaveNexus(InputWorkspace=workspace,Filename= filename)
                   break
                if case(): # default, could also just omit condition or 'if True'
                   prop_man.log("Unknown file format {0} requested to save results. No saving performed this format".format(file_format))

    #########
    @property 
    def prop_man(self):
        """ Return property manager containing DirectEnergyConversion parameters """
        return self._propMan

    @prop_man.setter
    def prop_man(self,value):
        """ Assign new instance of direct property manager to provide DirectEnergyConversion parameters """
        if isinstance(value,PropertyManager):
            self._propMan = value
        else:
            raise KeyError("Property manager can be initialized by an instance of ProperyManager only")
    #########
    @property
    def spectra_masks(self):
        """ The property keeps a workspace with masks, stored for further usage """ 

        # check if spectra masks is defined
        if hasattr(self,'_spectra_masks'):
            return self._spectra_masks
        else:
            return None

    @spectra_masks.setter
    def spectra_masks(self,value):
        """ set up spectra masks """ 
        self._spectra_masks = value


    #---------------------------------------------------------------------------
    # Behind the scenes stuff
    #---------------------------------------------------------------------------

    def __init__(self, instr_name=None,reload_instrument=False):
        """
        Constructor
        """
        object.__setattr__(self,'_descriptors',[])
        object.__setattr__(self,'_propMan',None)
        # Debug parameter. Usually True unless investigating a problem
        object.__setattr__(self,'_keep_wb_workspace',True)
        object.__setattr__(self,'_do_ISIS_reduction',True)
        object.__setattr__(self,'_spectra_masks',None)
        # if normalized by monitor-2, range have to be established before
        # shifting the instrument
        object.__setattr__(self,'_mon2_norm_time_range',None)
        object.__setattr__(self,'_debug_mode',False)
        # method used in debug mode and requesting event workspace to be rebinned first 
        object.__setattr__(self,'_do_early_rebinning',False)
        # internal parameter, specifying work in multirep mode. If True, some 
        # auxiliary workspaces should not be deleted until used for each workspace
        # processed
        object.__setattr__(self,'_multirep_mode',False)

        all_methods = dir(self)
        # define list of all existing properties, which have descriptors
        object.__setattr__(self,'_descriptors',extract_non_system_names(all_methods))

        if instr_name:
            self.initialise(instr_name,reload_instrument)
        #end

    def __getattr__(self,attr_name):
       """  overloaded to return values of properties non-existing in the class dictionary 
            from the property manager class except this
            property already have descriptor in self class
       """ 
       if attr_name in self._descriptors:
          return object.__getattr__(self,attr_name)
       else:
          return getattr(self._propMan,attr_name)

    def __setattr__(self,attr_name,attr_value):
        """ overloaded to prohibit adding non-starting with _properties to the class instance
            and add all other properties to property manager except this property already
            have a descriptor
        """
        if attr_name[0] == '_':
            object.__setattr__(self,attr_name,attr_value)
        else:
            if attr_name in self._descriptors:
                object.__setattr__(self,attr_name,attr_value)
            else:
                setattr(self._propMan,attr_name,attr_value)
          
    def initialise(self, instr,reload_instrument=False):
        """
        Initialize the private attributes of the class and the nullify the attributes which expected
        to be always set-up from a calling script
        """
        # Internal properties and keys
        self._keep_wb_workspace = True # for the time being.  May be auto-calculated later but should it?
        self._do_ISIS_reduction = True
        # if normalized by monitor-2, range have to be established before
        # shifting the instrument
        self._mon2_norm_time_range = None
        self._debug_mode = False
        self.spectra_masks = None


        # Instrument and default parameter setup
        # formats available for saving.  As the reducer has to have a method to
        # process one of this, it is private property
        if not hasattr(self,'_propMan') or self._propMan is None:
            if isinstance(instr,PropertyManager):
                self._propMan = instr
            else:
                self._propMan = PropertyManager(instr)
        else:
            old_name = self._propMan.instrument.getName()
            if isinstance(instr,geometry._geometry.Instrument):
                new_name = self._propMan.instrument.getName()
            elif isinstance(instr,PropertyManager):
                new_name = instr.instr_name
            else:
                new_name = instr
            #end if
            if old_name != new_name or reload_instrument:
                self._propMan = PropertyManager(new_name)
            #end if
        #

    def setup_instrument_properties(self, workspace=None,reload_instrument=False):
        if workspace != None:
            instrument = workspace.getInstrument()
            name = instrument.getName()
            if name != self.prop_man.instr_name:
               self.prop_man = PropertyManager(name,workspace)

                
    def get_run_descriptor(self,run):
       """ Spawn temporary run descriptor for input data given in format,
           different from run descriptor. Return existing run descriptor, 
           if it is what provided.
       """
       if not isinstance(run,RunDescriptor):
          tRun = copy.copy(PropertyManager._tmp_run)
          tRun.__set__(None,run)      
          return tRun
       else:
           return run
    #
#-------------------------------------------------------------------------------
    def apply_absolute_normalization(self,sample_ws,monovan_run=None,ei_guess=None,wb_mono=None):
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
        prop_man = self.prop_man
        prop_man.log('*** Running absolute units corrections ****************************************************','notice')

        if prop_man.mono_correction_factor:
            absnorm_factor = float(prop_man.mono_correction_factor)
            prop_man.log('*** Using supplied workspace correction factor                           ******','notice')
            abs_norm_factor_is = 'provided'
        else:
            mvir = prop_man.monovan_integr_range
            prop_man.log('*** Evaluating the integral from the monovan run and calculate the correction factor ******','notice')
            prop_man.log('    Using absolute units vanadium integration range : [{0:8f}:{1:8f}]         ******'.format(mvir[0],mvir[1]),'notice')
            abs_norm_factor_is = 'calculated'

            # convert to monovanadium to energy
            deltaE_wkspace_monovan = self.mono_sample(monovan_run,ei_guess,wb_mono,
                                                      self.monovan_mapfile,self.spectra_masks)


            ei_monovan = deltaE_wkspace_monovan.getRun().getLogData("Ei").value
            prop_man.log('    Incident energy found for monovanadium run: ' + str(ei_monovan) + ' meV','notice')


            (anf_LibISIS,anf_SS2,anf_Puas,anf_TGP) = self.get_abs_normalization_factor(deltaE_wkspace_monovan.getName(),ei_monovan)

            prop_man.log('*** Absolute correction factor(s): S^2: {0:10.4f}\n*** LibISIS: {1:10.4f} Poisson: {2:10.4f}  TGP: {3:10.4f} '\
                .format(anf_LibISIS,anf_SS2,anf_Puas,anf_TGP),'notice')
            prop_man.log('*** If these factors are substantially different, something is wrong                    ***','notice')
            absnorm_factor = anf_TGP
        #end
        prop_man.log('*** Using {0} value : {1} of absolute units correction factor (TGP)'.format(abs_norm_factor_is,absnorm_factor),'notice')
        prop_man.log('*******************************************************************************************','notice')

        sample_ws = sample_ws / absnorm_factor

        return sample_ws
#-------------------------------------------------------------------------------
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

        propman = self.prop_man
        van_mass = propman.van_mass
        # list of two number representing the minimal (ei_monovan[0])
        # and the maximal (ei_monovan[1]) energy to integrate the spectra
        minmax = propman.monovan_integr_range


        data_ws = Integration(InputWorkspace=deltaE_wkspaceName,OutputWorkspace='van_int',RangeLower=minmax[0],RangeUpper=minmax[1],IncludePartialBins='1')
 
        nhist = data_ws.getNumberHistograms()
        # extract wb integrals for combined spectra
        signal = []
        error = []
        izerc = 0
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
           if ((err <= 0) or (sig <= 0)):   # count Inf and negative||zero readings.  Presence of this indicates that
                                            # something went wrong
              izerc+=1
              continue

           signal.append(sig) 
           error.append(err)
        #---------------- Loop finished

        norm_factor = {}
        # Various prior probabilities.
        #-------------------------------------------------------------------------
        # Guess which minimizes the value sum(n_i-n)^2/Sigma_i -- this what
        # Libisis had
        signal_sum = sum(map(lambda s,e: s / e,signal,error))
        weight_sum = sum(map(lambda e: 1. / e, error))
        norm_factor['LibISIS'] = signal_sum / weight_sum
        #-------------------------------------------------------------------------
        # Guess which minimizes the value sum(n_i-n)^2/Sigma_i^2
        signal_sum = sum(map(lambda s,e: s / (e * e),signal,error))
        weight_sum = sum(map(lambda e: 1. / (e * e), error))
        norm_factor['SigSq'] = signal_sum / weight_sum
        #-------------------------------------------------------------------------
        # Guess which assumes Poisson distribution with Err=Sqrt(signal) and
        # calculates
        # the function: N_avrg =
        # 1/(DetEfficiency_avrg^-1)*sum(n_i*DetEfficiency_i^-1)
        # where the DetEfficiency = WB_signal_i/WB_average WB_signal_i is the
        # White Beam Vanadium
        # signal on i-th detector and the WB_average -- average WB vanadium
        # signal.
        # n_i is the modified signal
        signal_sum = sum(map(lambda e: e * e,error))
        weight_sum = sum(map(lambda s,e: e * e / s,signal,error))
        if(weight_sum == 0.0):
            prop_man.log("WB integral has been calculated incorrectly, look at van_int workspace: {0}".format(deltaE_wkspaceName),'error')
            raise ArithmeticError("Division by 0 weight when calculating WB integrals from workspace {0}".format(deltaE_wkspaceName))
        norm_factor['Poisson'] = signal_sum / weight_sum
        #-------------------------------------------------------------------------
        # Guess which estimates value sum(n_i^2/Sigma_i^2)/sum(n_i/Sigma_i^2)
        # TGP suggestion from 12-2012
        signal_sum = sum(map(lambda s,e: s * s / (e * e),signal,error))
        weight_sum = sum(map(lambda s,e: s / (e * e),signal,error))
        if(weight_sum == 0.0):
            prop_man.log("WB integral has been calculated incorrectly, look at van_int workspace: {0}".format(deltaE_wkspaceName),'error')
            raise ArithmeticError("Division by 0 weight when calculating WB integrals from workspace {0}".format(deltaE_wkspaceName))
        norm_factor['TGP'] = signal_sum / weight_sum
        #
        #
        #integral_monovan=signal_sum /(wbVan_sum)
        van_multiplier = (float(propman.van_rmm) / float(van_mass))
        if ei_monovan >= 210.0:
            xsection = 421  # vanadium cross-section in mBarn/sR (402 mBarn/Sr) (!!!modified to fit high
                            # energy limit?!!!)
        else: # old textbook cross-section for vanadium for ei=20mEv
            xsection = 400 + (ei_monovan / 10)
        sample_multiplier = (float(propman.sample_mass) / float(propman.sample_rmm))

        scale_factor = van_multiplier * sample_multiplier / xsection

        for type,val in norm_factor.iteritems():
            norm_factor[type] = val * scale_factor

        # check for NaN
        if (norm_factor['LibISIS'] != norm_factor['LibISIS']) | (izerc != 0):    # It is an error, print diagnostics:
           if (norm_factor['LibISIS'] != norm_factor['LibISIS']):
               log_value = '\n--------> Absolute normalization factor is NaN <----------------------------------------------\n'
           else:
               log_value = '\n--------> Warning, Monovanadium has zero spectra <--------------------------------------------\n'
               log1_value = \
               "--------> Processing workspace: {0}\n"\
               "--------> Monovan Integration range : min={1}, max={2} (meV)\n"\
               "--------> Summed:  {3} spectra with total signal: {4} and error: {5}\n"\
               "--------> Dropped: {6} zero spectra\n"\
               "--------> Using  mBarn/sR*fu normalization factor = {7} resulting in:\n"\
               "--------> Abs norm factors: LibISIS: {8}\n"\
               "--------> Abs norm factors: Sigma^2: {9}\n"\
               "--------> Abs norm factors: Poisson: {10}\n"\
               "--------> Abs norm factors: TGP    : {11}\n"\
               .format(deltaE_wkspaceName,minmax[0],minmax[1],nhist,sum(signal),sum(error),izerc,scale_factor,
                          norm_factor['LibISIS'],norm_factor['SigSq'],norm_factor['Poisson'],norm_factor['TGP'])
           log_value = log_value + log1_value
           propman.log(log_value,'error')
        else:
            if not self._debug_mode:
                DeleteWorkspace(Workspace=deltaE_wkspaceName)
                DeleteWorkspace(Workspace=data_ws)
        return (norm_factor['LibISIS'],norm_factor['SigSq'],norm_factor['Poisson'],norm_factor['TGP'])
# -------------------------------------------------------------------------------------------
#         This actually does the conversion for the mono-sample and
#         mono-vanadium runs
#
# -------------------------------------------------------------------------------------------
    def _do_mono_SNS(self, data_ws, result_name, ei_guess,
                 white_run=None, map_file=None, spectra_masks=None, Tzero=None):
        # does not work -- retrieve from repo and fix
        raise NotImplementedError("Non currently implemented. Retrieve from repository"
                                  " if necessary and fix")
        return
#-------------------------------------------------------------------------------
    def _do_mono_ISIS(self, data_run, ei_guess,
                 white_run=None, map_file=None, spectra_masks=None, Tzero=None):

        # Do ISIS stuff for Ei
        ei_value, mon1_peak = self.get_ei(data_run, ei_guess)
        self.incident_energy = ei_value


        # As we've shifted the TOF so that mon1 is at t=0.0 we need to account
        # for this in CalculateFlatBackground and normalization
        bin_offset = -mon1_peak
        result_name = data_run.set_action_suffix('_spe')

        if self.check_background == True:
            # Remove the count rate seen in the regions of the histograms
            # defined as the background regions, if the user defined such
            # region
            result_ws = data_run.get_workspace()
            bkgd_range = self.bkgd_range
            bkg_range_min = bkgd_range[0]+bin_offset
            bkg_range_max = bkgd_range[1]+bin_offset
            if isinstance(result_ws,api.IEventWorkspace):
                bkgr_ws=self._find_or_build_bkgr_ws(result_ws,bkg_range_min,bkg_range_max)
            else:
                bkgr_ws = None
                CalculateFlatBackground(InputWorkspace=result_ws,OutputWorkspace=result_ws,
                                        StartX= bkg_range_min,EndX= bkg_range_max,
                                        WorkspaceIndexList= '',Mode= 'Mean',SkipMonitors='1')
        else:
            bkgr_ws = None
        data_run.synchronize_ws(result_ws)


        # Normalize using the chosen method+group
        norm_ws = self.normalise(data_run, self.normalise_method, range_offset=bin_offset)
        # normalized workspace can go out with different name, so here we
        # reinforce one expected here
        result_name = data_run.set_action_suffix('_spe')
        data_run.synchronize_ws(norm_ws)

        #
        ConvertUnits(InputWorkspace=result_name,OutputWorkspace=result_name, Target="DeltaE",EMode='Direct')
        self.prop_man.log("_do_mono: finished ConvertUnits for : " + result_name,'information')

        energy_bins = self.energy_bins
        if energy_bins:
           Rebin(InputWorkspace=result_name,OutputWorkspace=result_name,Params= energy_bins,PreserveEvents=False)
           if bkgr_ws: # remove background after converting units and rebinning
              monitors_ws = data_run.get_monitors_ws()
              bkgr_ws = self.normalise(bkgr_ws, self.normalise_method, bin_offset,monitors_ws)
              RemoveBackground(InputWorkspace=result_name,OutputWorkspace=result_name,BkgWorkspace=bkgr_ws,EMode='Direct')
              if not self._multirep_mode:
                 DeleteWorkspace(bkgr_ws)
        else:
            pass # TODO: investigate way of removing background from event workspace if we want result to be an event workspace
            # what to do with event workspace having negative events? will further algorithms work with these events ?

        if self.apply_detector_eff and energy_bins: #should detector efficiency work on event workspace too? At the moment it is not
           DetectorEfficiencyCor(InputWorkspace=result_name,OutputWorkspace=result_name)
           self.prop_man.log("_do_mono: finished DetectorEfficiencyCor for : " + result_name,'information')
        #############
        data_run.synchronize_ws(mtd[result_name])

        return 
#-------------------------------------------------------------------------------
    def _find_or_build_bkgr_ws(self,result_ws,bkg_range_min=None,bkg_range_max=None):
        """ Method calculates  background workspace or restore workspace with 
            the same name as the one produced by this method from ADS
        """ 
        if not bkg_range_min or not bkg_range_max:
            bkg_range_min,bkg_range_max = self.bkgd_range

        if 'bkgr_ws' in mtd: # has to have specific name for this all working
             bkgr_ws = mtd['bkgr_ws']
        else:
             bkgr_ws = Rebin(result_ws,Params=[bkg_range_min,(bkg_range_max-bkg_range_min)*1.001,bkg_range_max],PreserveEvents=False)

        return bkgr_ws
#-------------------------------------------------------------------------------
    def _do_mono(self, run,  ei_guess,
                 white_run=None, map_file=None, spectra_masks=None, Tzero=None):
        """
        Convert units of a given workspace to deltaE, including possible
        normalization to a white-beam vanadium run.
        """

        if (self._do_ISIS_reduction):
           self._do_mono_ISIS(run,ei_guess,
                              white_run, map_file, spectra_masks, Tzero)
        else:
          result_name = run.set_action_suffix('_spe')
          self._do_mono_SNS(run,result_name,ei_guess,
                         white_run, map_file, spectra_masks, Tzero)
          run.synchronize_ws()

        prop_man = self.prop_man
        result_name = run.get_ws_name()
        #######################
        # Ki/Kf Scaling...
        if prop_man.apply_kikf_correction:
            prop_man.log('Start Applying ki/kf corrections to the workspace : ' + result_name,'information')
            CorrectKiKf(InputWorkspace=result_name,OutputWorkspace= result_name, EMode='Direct')
            prop_man.log('finished applying ki/kf corrections for : ' + result_name,'information')

        # Make sure that our binning is consistent
        if prop_man.energy_bins:
           Rebin(InputWorkspace=result_name,OutputWorkspace= result_name,Params= prop_man.energy_bins)

        # Masking and grouping
        result_ws = mtd[result_name]
        result_ws = self.remap(result_ws, spectra_masks, map_file)

        if prop_man.energy_bins: # It should already be a distribution. 
            ConvertToDistribution(Workspace=result_ws)
        # White beam correction
        if white_run is not None:
            white_ws = self.do_white(white_run, spectra_masks, map_file)
            result_ws /= white_ws
            DeleteWorkspace(white_ws)

        # Overall scale factor
        result_ws *= prop_man.scale_factor
        return result_ws
#-------------------------------------------------------------------------------
    def _get_wb_inegrals(self,run):
        """ """
        run = self.get_run_descriptor(run)
        white_ws = run.get_workspace()
        # This both integrates the workspace into one bin spectra and sets up
        # common bin boundaries for all spectra
        done_Log = 'DET_EFFICIENCY_calculated'

        # set action suffix and check if such workspace is already present
        new_ws_name = run.set_action_suffix('_norm_white')

        # Check if the work has been already done
        if new_ws_name in mtd:
            targ_ws = mtd[new_ws_name]
            if done_Log in targ_ws.getRun():
                old_log_val = targ_ws.getRun().getLogData(done_Log).value
                done_log_VAL = self._build_white_tag()
                if old_log_val == done_log_VAL:
                   run.synchronize_ws(targ_ws)
                   if self._keep_wb_workspace:
                        result = run.get_ws_clone()
                   else:
                        result = run.get_workspace()
                   return result 
                else:
                    DeleteWorkspace(Workspace=new_ws_name)
            else:
                DeleteWorkspace(Workspace=new_ws_name)
        #end
        done_log_VAL = self._build_white_tag()

        # Normalize
        self.__in_white_normalization = True
        white_ws = self.normalise(run, self.normalise_method,0.0)
        self.__in_white_normalization = False
        new_ws_name = run.set_action_suffix('_norm_white')
        old_name = white_ws.name()

        # Units conversion
        ConvertUnits(InputWorkspace=old_name,OutputWorkspace=old_name, Target= "Energy", AlignBins=0)
        self.prop_man.log("do_white: finished converting Units",'information')

        low,upp = self.wb_integr_range
        if low > upp:
            raise ValueError("White beam integration range is inconsistent. low=%d, upp=%d" % (low,upp))

        delta = 2.0 * (upp - low)
        white_ws = Rebin(InputWorkspace=old_name,OutputWorkspace=old_name, Params=[low, delta, upp])
        # Why aren't we doing this...
        #Integration(white_ws, white_ws, RangeLower=low, RangeUpper=upp)
        AddSampleLog(white_ws,LogName = done_Log,LogText=done_log_VAL,LogType='String')
        run.synchronize_ws(white_ws)
        if self._keep_wb_workspace:
            result = run.get_ws_clone()
        else:
            result = run.get_workspace()
        return result
#-------------------------------------------------------------------------------
    def _build_white_tag(self):
        """ build tag indicating wb-integration ranges """ 
        low,upp = self.wb_integr_range
        white_tag = 'NormBy:{0}_IntergatedIn:{1:0>10.2f}:{2:0>10.2f}'.format(self.normalise_method,low,upp)
        return white_tag

#-------------------------------------------------------------------------------
#-------------------------------------------------------------------------------
#-------------------------------------------------------------------------------
def get_failed_spectra_list_from_masks(masked_wksp):
    """Compile a list of spectra numbers that are marked as
       masked in the masking workspace

    Input:
     masking_workspace -  A special masking workspace containing masking data
    """
    #TODO: get rid of this and use data, obtained form diagnostics

    failed_spectra = []
    if masked_wksp is None:
       return (failed_spectra,0)

    masking_wksp,sp_list = ExtractMask(masked_wksp)
    DeleteWorkspace(masking_wksp)

    n_spectra = len(sp_list)
    return (sp_list.tolist(),n_spectra)


#-----------------------------------------------------------------
if __name__ == "__main__":
    pass
    #unittest.main()