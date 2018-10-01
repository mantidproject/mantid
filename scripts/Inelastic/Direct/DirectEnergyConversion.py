#pylint: disable=too-many-lines
#pylint: disable=invalid-name
#pylind: disable=attribute-defined-outside-init
from __future__ import (absolute_import, division, print_function)
from mantid.simpleapi import *
from mantid.kernel import funcinspect
from mantid import geometry,api

import os.path
import copy
import math
import time
import numpy as np
import collections
from six import iteritems
from six.moves import range

import Direct.CommonFunctions  as common
import Direct.diagnostics      as diagnostics
from Direct.PropertyManager  import PropertyManager
from Direct.RunDescriptor    import RunDescriptor
from Direct.ReductionHelpers import extract_non_system_names,process_prop_list


def setup_reducer(inst_name,reload_instrument=False):
    """
    Given an instrument name or prefix this sets up a converter
    object for the reduction. Deprecated method
    """
    try:
        return DirectEnergyConversion(inst_name,reload_instrument)
    except RuntimeError:
        raise RuntimeError('Unknown instrument "%s" or wrong IDF file for this instrument, cannot continue' % inst_name)

#How could it be that abstract class is not referenced R0921? What it means?
#pylint: disable=too-many-instance-attributes


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
#pylint: disable=too-many-branches
#pylint: disable=too-many-locals

    def diagnose(self, white,diag_sample=None,**kwargs):
        """run diagnostics on the provided workspaces.

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
        # output workspace name.
        try:
            _,r = funcinspect.lhs_info('both')
            out_ws_name = r[0]
#pylint: disable=bare-except
        except:
            out_ws_name = None
        # modify properties using input arguments
        self.prop_man.set_input_parameters(**kwargs)
        # obtain proper run descriptor in case it is not a run descriptor but
        # something else
        white = self.get_run_descriptor(white)

        if self.use_hard_mask_only:
            # build hard mask
            diag_mask = white.get_masking(1)
            if diag_mask is None:
                # in this peculiar way we can obtain working mask which
                # accounts for initial data grouping in the
                # data file.  SNS or 1 to 1 maps may probably avoid this
                # stuff and can load masks directly
                white_data = white.get_ws_clone('white_ws_clone')
                if self.prop_man.mapmask_ref_ws is None:
                    ref_ws = white.get_workspace()
                else:
                    ref_ws = self.prop_man.mapmask_ref_ws
                idf_file = api.ExperimentInfo.getInstrumentFilename(self.instr_name)
                diag_mask = LoadMask(Instrument=idf_file,InputFile=self.hard_mask_file,
                                     OutputWorkspace='hard_mask_ws',RefWorkspace = ref_ws)
                #
                MaskDetectors(Workspace=white_data, MaskedWorkspace=diag_mask)
                white.add_masked_ws(white_data)
                DeleteWorkspace(Workspace='white_ws_clone')
                DeleteWorkspace(Workspace='hard_mask_ws')
                diag_mask = white.get_masking(1)
            if out_ws_name is not None:
                dm = CloneWorkspace(diag_mask,OutputWorkspace=out_ws_name)
                return dm
            else:
                return None

        # Get the white beam vanadium integrals
        white_integrals = self.do_white(white, None, None) # No grouping yet
#pylint: disable=access-member-before-definition
        if self.second_white:
            #TODO: fix THIS DOES NOT WORK!
#pylint: disable=unused-variable
            # second_white = self.second_white
            other_white_integrals = self.do_white(PropertyManager.second_white, None, None) # No grouping yet
#pylint: disable=attribute-defined-outside-init
            self.second_white = other_white_integrals

        # return all diagnostics parameters
        diag_params = self.prop_man.get_diagnostics_parameters()

        # Get the background/total counts from the sample run if present
        name_to_clean = None
        if diag_sample is not None:
            diag_sample = self.get_run_descriptor(diag_sample)
            sample_mask = diag_sample.get_masking(1)
            if sample_mask is None:
                # If the bleed test is requested then we need to pass in the
                # sample_run as well
                if self.bleed_test:
                    # initiate reference to reducer to be able to work with Run
                    # Descriptors
                    diagnostics.__Reducer__ = self
                    diag_params['sample_run'] = diag_sample

                # Set up the background integrals for diagnostic purposes.
                # monitor-2 normalization in multirep mode goes per chunk
                if (PropertyManager.incident_energy.multirep_mode() and self.normalise_method == 'monitor-2')\
                        or self.bleed_test: # bleed test below needs no normalization so normalize cloned workspace
                    result_ws  = diag_sample.get_ws_clone('sample_ws_clone')
                    wb_normalization_method = white_integrals.getRun().getLogData('DirectInelasticReductionNormalisedBy').value
                    result_ws = self.normalise(result_ws, wb_normalization_method)
                    name_to_clean = result_ws.name()
                else:
                    result_ws = self.normalise(diag_sample, self.normalise_method)

                #>>>here result workspace is being processed
                #-- not touching result ws
                bkgd_range = self.background_test_range
                bin_size = 2*(bkgd_range[1]-bkgd_range[0])
                background_int = Rebin(result_ws,
                                       Params=[bkgd_range[0],bin_size,bkgd_range[1]],
                                       PreserveEvents=False,FullBinsOnly=False, IgnoreBinErrors=True)
                total_counts = Integration(result_ws, IncludePartialBins=True)
                background_int = ConvertUnits(background_int, Target="Energy",EMode='Elastic', AlignBins=0)
                self.prop_man.log("Diagnose: finished convertUnits ",'information')

                background_int *= self.scale_factor
                diagnostics.normalise_background(background_int, white_integrals,
                                                 diag_params.get('second_white',None))
                diag_params['background_int'] = background_int
                diag_params['sample_counts'] = total_counts

        # extract existing white mask if one is defined and provide it for
        # diagnose to use instead of constantly diagnosing the same vanadium
        white_mask = white.get_masking(1)
        if white_mask is None or sample_mask is None:
            pass # have to run diagnostics
        else:
            #Sample mask and white masks are defined.
            #nothing to do then
            total_mask = sample_mask + white_mask
            return total_mask

        # Check how we should run diag
        diag_spectra_blocks = self.diag_spectra

        if white_mask is not None:
            diag_params['white_mask'] = white
        # keep white mask workspace for further usage
        if diag_spectra_blocks is None:
            # Do the whole lot at once
            white_masked_ws = diagnostics.diagnose(white_integrals, **diag_params)
            if white_masked_ws:
                white.add_masked_ws(white_masked_ws)
                DeleteWorkspace(white_masked_ws)
        else:
            for bank in diag_spectra_blocks:
                diag_params['start_index'] = bank[0] - 1
                diag_params['end_index'] = bank[1] - 1
                white_masked_ws = diagnostics.diagnose(white_integrals, **diag_params)
                if white_masked_ws:
                    white.add_masked_ws(white_masked_ws)
                    DeleteWorkspace(white_masked_ws)

        if out_ws_name:
            if diag_sample is not None:
                diag_sample.add_masked_ws(white_integrals)
                mask = diag_sample.get_masking(1)
                diag_mask = CloneWorkspace(mask,OutputWorkspace=out_ws_name)
            else: # either WB was diagnosed or WB masks were applied to it
                # Extract the mask workspace
                diag_mask, _ = ExtractMask(InputWorkspace=white_integrals,OutputWorkspace=out_ws_name)
        else:
            diag_mask = None

        self.clean_up(diag_params, name_to_clean, white_integrals)

        return diag_mask

#-------------------------------------------------------------------------------
    # Clean up unrequired workspaces

    def clean_up(self, diag_params, name_to_clean, white_integrals):
        if 'sample_counts' in diag_params:
            DeleteWorkspace(Workspace='background_int')
            DeleteWorkspace(Workspace='total_counts')
        if 'second_white' in diag_params:
            DeleteWorkspace(Workspace=diag_params['second_white'])
        if name_to_clean:
            DeleteWorkspace(name_to_clean)
            if name_to_clean+'_monitors' in mtd:
                DeleteWorkspace(name_to_clean+'_monitors')
        DeleteWorkspace(Workspace=white_integrals)

#-------------------------------------------------------------------------------
#pylint: disable=too-many-arguments
#pylint: disable=too-many-branches
#pylint: disable=too-many-locals
#pylint: disable=W0621

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
            _,r = funcinspect.lhs_info('both')
            out_ws_name = r[0]
#pylint: disable=bare-except
        except:
            out_ws_name = None
        prop_man = self.prop_man

        # check if reducer can find all non-run files necessary for the reduction
        # and verify some other properties which can be wrong before starting a
        # long run.
        prop_man.log("****************************************************************")
        prop_man.log("*** ISIS CONVERT TO ENERGY TRANSFER WORKFLOW STARTED  **********")
        prop_man.validate_properties()
        prop_man.log("*** Loading or retrieving sample run: {0}".format(prop_man.sample_run))
        prop_man.log("****************************************************************")

        # before trying to process new results, let's remove from memory old results
        # if any present and they are not needed any more (user have not renamed them)
        self._clear_old_results()

        start_time = time.time()

        PropertyManager.sample_run.set_action_suffix('')
        sample_ws = PropertyManager.sample_run.get_workspace()

        ei_to_process_available = self.calc_incident_energies(PropertyManager, prop_man, sample_ws)

        # Update reduction properties which may change in the workspace but have
        # not been modified from input parameters.
        # E.g.  detector number have changed
        old_changes = self.prop_man.getChangedProperties()
        all_changes = self.prop_man.update_defaults_from_instrument(sample_ws.getInstrument())
        workspace_defined_prop = all_changes.difference(old_changes)
        if len(workspace_defined_prop) > 0:
            prop_man.log("****************************************************************")
            prop_man.log('*** Sample run {0} properties change default reduction properties: '.
                         format(PropertyManager.sample_run.get_workspace().name()))
            prop_man.log_changed_values('notice',False,old_changes)
            prop_man.log("****************************************************************")
        # inform user on what parameters have changed from script or gui
        # if monovan present, check if abs_norm_ parameters are set
        self.prop_man.log_changed_values('notice')
        if not ei_to_process_available:
            prop_man.log("*** NO GUESS INCIDENT ENERGIES IDENTIFIED FOR THIS RUN *********")
            prop_man.log("*** NOTHING TO REDUCE ******************************************")
            prop_man.log("****************************************************************")
            return None

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
            masking,header = self._run_diagnostics(prop_man)
        else:
            header = '*** Using stored mask file for workspace with {0} spectra and {1} masked spectra'
            masking = self.spectra_masks

        # estimate and report the number of failing detectors
        n_masked_spectra = get_failed_spectra_list_from_masks(masking,prop_man)
        if masking:
            n_spectra = masking.getNumberHistograms()
        else:
            n_spectra = 0
        prop_man.log(header.format(n_spectra,n_masked_spectra),'notice')
#--------------------------------------------------------------------------------------------------
#  now reduction
#--------------------------------------------------------------------------------------------------
        # ISIS or GUI motor stuff
        psi = PropertyManager.psi.read_psi_from_workspace(sample_ws)
        if prop_man.motor_offset is not None and np.isnan(psi):
            #logs have a problem
            prop_man.log("*** Can not retrieve rotation value from sample environment logs: {0}.\n"
                         "     Rotation angle remains undefined".format(prop_man.motor_log_names))
            PropertyManager.psi = None # Just in case
        else:
            # store psi in property not to retrieve it from workspace again
            prop_man.psi = psi
        #end
        #
        if self.monovan_run is not None:
            mono_van_cache_num = PropertyManager.monovan_run.run_number()
        else:
            mono_van_cache_num = None
        #Set or clear monovan run number to use in cash ID to return correct
        #cashed value of monovan integral
        PropertyManager.mono_correction_factor.set_cash_mono_run_number(mono_van_cache_num)

        mono_ws_base = None
        if PropertyManager.incident_energy.multirep_mode():
            self._multirep_mode = True
            ws_base = None
            if self.check_background:
                # find the count rate seen in the regions of the histograms defined
                # as the background regions, if the user defined such region.
                # In multirep mode this has to be done here, as workspace
                # will be cut in chunks and bg regions removed
                bkgd_range = self.bkgd_range
                self._find_or_build_bkgr_ws(PropertyManager.sample_run,bkgd_range[0],bkgd_range[1])
            # initialize list to store resulting workspaces to return
            result = []
        else:
#pylint: disable=W0201
            self._multirep_mode = False

#------------------------------------------------------------------------------------------
# Main loop over incident energies
#------------------------------------------------------------------------------------------
        # do not do enumerate if it generates all sequence at once
        #  -- code below uses current energy state from PropertyManager.incident_energy
        AllEn = PropertyManager.incident_energy.getAllEiList()
        num_ei_cuts = len(AllEn)
        for ind,ei_guess in enumerate(AllEn):
            PropertyManager.incident_energy.set_current_ind(ind)

            cut_ind =ind + 1 # nice printing convention (1 of 1 rather them 0 of 1)
            #---------------
            if self._multirep_mode:
                tof_range = self.find_tof_range_for_multirep(ws_base)
                ws_base = PropertyManager.sample_run.chop_ws_part(ws_base,tof_range,self._do_early_rebinning,
                                                                  cut_ind,num_ei_cuts)
                prop_man.log("*** Processing multirep chunk: #{0}/{1} for provisional energy: {2} meV".
                             format(cut_ind,num_ei_cuts,ei_guess),'notice')
                # do bleed corrections for chunk if necessary
                bleed_mask = self._do_bleed_corrections(PropertyManager.sample_run,cut_ind)
                if bleed_mask is not None:
                    mask_ws_name =  PropertyManager.sample_run.get_workspace().name()+'_bleed_mask'
                    RenameWorkspace(bleed_mask,OutputWorkspace=mask_ws_name)
                    self._old_runs_list.append(mask_ws_name)
            else:
                # single energy uses single workspace and all TOF are used
                tof_range = None
            #---------------
            #
            #Run the conversion first on the sample
            deltaE_ws_sample = self.mono_sample(PropertyManager.sample_run,ei_guess,PropertyManager.wb_run,
                                                self.map_file,masking)
            #

            ei = (deltaE_ws_sample.getRun().getLogData("Ei").value)
            # PropertyManager.incident_energy.set_current(ei) let's not do it --
            # this makes subsequent calls to this method depend on previous calls
            prop_man.log("*** Incident energy found for sample run: {0} meV".format(ei),'notice')
            #
            # calculate absolute units integral and apply it to the workspace
            # or use previously cashed value
            cashed_mono_int = PropertyManager.mono_correction_factor.get_val_from_cash(prop_man)
            if mono_van_cache_num is not None or self.mono_correction_factor or cashed_mono_int:
                deltaE_ws_sample,mono_ws_base = self._do_abs_corrections(deltaE_ws_sample,cashed_mono_int,
                                                                         ei_guess,mono_ws_base,tof_range, cut_ind,num_ei_cuts)
            else:
                pass # no absolute units corrections
            # ensure that the sample_run name is intact with workspace
            PropertyManager.sample_run.synchronize_ws(deltaE_ws_sample)
            #
            #
            self.save_results(deltaE_ws_sample)

            # prepare output workspace
            results_name = deltaE_ws_sample.name()
            if out_ws_name:
                if self._multirep_mode:
                    result.append(deltaE_ws_sample)
                else:
                    if results_name != out_ws_name:
                        RenameWorkspace(InputWorkspace=results_name,OutputWorkspace=out_ws_name)
                        results_name = out_ws_name
                        result = mtd[results_name]
                    else:
                        result = deltaE_ws_sample
            else: # delete workspace if no output is requested
                result = None
            self._old_runs_list.append(results_name)
        #end_for
#------------------------------------------------------------------------------------------
# END Main loop over incident energies
#------------------------------------------------------------------------------------------

        self.clean_up_convert_to_energy(start_time)
        return result


#------------------------------------------------------------------------------------------
    # Handles cleanup of the convert_to_energy method

    def clean_up_convert_to_energy(self, start_time):

        #Must! clear background ws (if present in multirep) to calculate background
        #source for next workspace
        if 'bkgr_ws_source' in mtd:
            DeleteWorkspace('bkgr_ws_source')

        # clear combined mask
        self.spectra_masks = None
        end_time = time.time()
        self.prop_man.log("*** ISIS CONVERT TO ENERGY TRANSFER WORKFLOW FINISHED  *********")
        self.prop_man.log("*** Elapsed time : {0:>9.2f} sec                       *********".
                          format(end_time - start_time),'notice')
        self.prop_man.log("****************************************************************")

#------------------------------------------------------------------------------------------
    # Check auto-ei mode and calculate incident energies if necessary
    # Returns if there is a processible Ei

    def calc_incident_energies(self, PropertyManager, prop_man, sample_ws):
        if PropertyManager.incident_energy.autoEi_mode():
            mon_ws = PropertyManager.sample_run.get_monitors_ws()
            # sum monitor spectra if this is requested
            ei_mon_spec = self.ei_mon_spectra
            if PropertyManager.ei_mon_spectra.need_to_sum_monitors(prop_man):
                ei_mon_spec,mon_ws = self.sum_monitors_spectra(mon_ws,ei_mon_spec)
                sample_ws.setMonitorWorkspace(mon_ws)
            else:
                pass

            try:
                PropertyManager.incident_energy.set_auto_Ei(mon_ws,prop_man,ei_mon_spec)
                return True
            except RuntimeError as er:
                prop_man.log('*** Error while calculating autoEi: {0}. See algorithm log for details.'.
                             format(str(er)))
                return False
        else:
            return True

#------------------------------------------------------------------------------------------
#pylint: disable=too-many-arguments

    def _do_abs_corrections(self,deltaE_ws_sample,cashed_mono_int,ei_guess,
                            mono_ws_base,tof_range, cut_ind,num_ei_cuts):
        """Do absolute corrections using various sources of such corrections
           cashed, provided or calculated from monovan ws
        """
        # do not remove background from vanadium (sample background is not
        # fit for that anyway)
#pylint: disable=E0203
        current_bkg_opt = self.check_background
#pylint: disable=attribute-defined-outside-init
        self.check_background = False
        # what we want to do with absolute units:
#pylint: disable=E0203
        if self.mono_correction_factor: # Correction provided.  Just apply it
            deltaE_ws_sample = self.apply_absolute_normalization(deltaE_ws_sample,PropertyManager.monovan_run,
                                                                 ei_guess,PropertyManager.wb_for_monovan_run,
                                                                 ' provided ')
        elif cashed_mono_int:  # Correction cashed from previous run
#pylint: disable=attribute-defined-outside-init
            self.mono_correction_factor = cashed_mono_int
            deltaE_ws_sample = self.apply_absolute_normalization(deltaE_ws_sample,PropertyManager.monovan_run,
                                                                 ei_guess,PropertyManager.wb_for_monovan_run,
                                                                 ' -cached- ')
#pylint: disable=attribute-defined-outside-init
            self.mono_correction_factor = None
        else:   # Calculate corrections
            if self._multirep_mode:
                mono_ws_base = PropertyManager.monovan_run.chop_ws_part(mono_ws_base,tof_range,
                                                                        self._do_early_rebinning, cut_ind,num_ei_cuts)
                # its pointless to do bleed for monovan run
                #self._do_bleed_corrections(PropertyManager.monovan_run,cut_ind)

            deltaE_ws_sample = self.apply_absolute_normalization(deltaE_ws_sample,PropertyManager.monovan_run,
                                                                 ei_guess,PropertyManager.wb_for_monovan_run,
                                                                 'calculated')
            # monovan workspace has been corrupted in memory after
            # calculations and result placed in cash. Workspace unsuitable
            # for further calculations. Mark it cashed not to verify presence on consecutive runs
            # with the same monovan ws
#pylint: disable=protected-access
            PropertyManager.monovan_run._in_cash = True
#pylint: disable=attribute-defined-outside-init
        self.check_background = current_bkg_opt
        return deltaE_ws_sample,mono_ws_base

    def _run_diagnostics(self,prop_man):
        """Internal diagnostics procedure used over two workspaces, used by convert_to_energy"""

        prop_man.log("======== Run diagnose for sample run ===========================",'notice')
        masking = self.diagnose(PropertyManager.wb_run,PropertyManager.mask_run,
                                second_white=None,print_diag_results=True)
        if prop_man.use_hard_mask_only:
            header = "*** Hard mask file applied to workspace with {0:d} spectra masked {1:d} spectra"
        else:
            header = "*** Diagnostics processed workspace with {0:d} spectra and masked {1:d} bad spectra"

        # diagnose absolute units:
        if self.monovan_run is not None :
            if self.mono_correction_factor is None :
                if self.use_sam_msk_on_monovan:
                    prop_man.log('  Applying sample run mask to mono van')
                else:
                    # in this case the masking2 is different but points to the
                    # same  workspace Should be better solution for that
                    if not self.use_hard_mask_only :
                        prop_man.log("======== Run diagnose for monochromatic vanadium run ===========",'notice')
                        masking2 = self.diagnose(PropertyManager.wb_for_monovan_run,PropertyManager.monovan_run,
                                                 second_white = None,print_diag_results=True)
                        masking +=  masking2
                        DeleteWorkspace(masking2)
            else: # if Reducer.mono_correction_factor != None :
                pass
        else:
            pass
        # Very important statement propagating masks for further usage in
        # convert_to_energy.
        # This property is also directly accessible from GUI.
        self.spectra_masks = masking
        # save mask if it does not exist and has been already loaded
        #if Reducer.save_and_reuse_masks and not masks_done:
        #    SaveMask(InputWorkspace=masking,OutputFile =
        #    mask_file_name,GroupedDetectors=True)
        return masking,header

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

    def _do_bleed_corrections(self,sample_run,nchunk):
        """Calculate TOF-chunk specific bleed corrections, necessary in mutlirep mode
        """
        if not self.prop_man.diag_bleed_test:
            return None

        CUR_bleed_masks, failures = diagnostics.do_bleed_test(sample_run, self.prop_man.bleed_maxrate, self.prop_man.bleed_pixels)
        if failures > 0:
            diagnostics.add_masking(sample_run.get_workspace(),CUR_bleed_masks)
            bleed_mask = CUR_bleed_masks
        else:
            DeleteWorkspace(CUR_bleed_masks)
            bleed_mask = None
        self.prop_man.log("*** Bleeding test for chunk #{0} masked {1} pixels".format(nchunk,failures),'notice')
        return bleed_mask


#pylint: disable=too-many-arguments
    def mono_sample(self, mono_run, ei_guess, white_run=None, map_file=None,
                    spectra_masks=None, result_name=None, Tzero=None):
        """Convert a mono-chromatic sample run to DeltaE.

        """
        mono_run = self.get_run_descriptor(mono_run)
        if white_run:
            white_run = self.get_run_descriptor(white_run)

        mono_s = self._do_mono(mono_run, ei_guess,
                               white_run, map_file, spectra_masks, Tzero)
        # at this stage we would never need monitors for this workspace if they
        # were actually there
        mono_run.clear_monitors()
        return mono_s

#-------------------------------------------------------------------------------
    def sum_monitors_spectra(self,monitor_ws,ei_mon_spectra):
        """Sum monitors spectra for all spectra, specified in the spectra list(s)
           and create monitor workspace containing only the spectra summed and organized
           according to ei_mon_spectra tuple.

           Returns tuple of two spectra numbers, containing in the
           new summed monitors workspace and pointer to the new workspace itself.
        """
        existing_list = process_prop_list(monitor_ws,"CombinedSpectraIDList")
        monitor_ws_name = monitor_ws.name()
        if len(existing_list) == 0:
            spectra_list1=ei_mon_spectra[0]
            spectra_list2=ei_mon_spectra[1]
            if not isinstance(spectra_list1,list):
                spectra_list1 = [spectra_list1]
            spec_id1 = self._process_spectra_list(monitor_ws,spectra_list1,'spectr_ws1')

            if not isinstance(spectra_list2,list):
                spectra_list2 = [spectra_list2]
            spec_id2 = self._process_spectra_list(monitor_ws,spectra_list2,'spectr_ws2')
            # Are other monitors necessary?
            mon_list = self.prop_man.get_used_monitors_list()
            spectra_needed=[]
            monitors_left=[]
            for mon_id in mon_list:
                if not(mon_id in spectra_list1 or mon_id in spectra_list2):
                    wsInd = monitor_ws.getIndexFromSpectrumNumber(int(mon_id))
                    monitors_left.append(int(mon_id))
                    spectra_needed.append(int(wsInd))
            n_other_mon = len(spectra_needed)
            if n_other_mon > 0:
                ExtractSpectra(InputWorkspace=monitor_ws,OutputWorkspace='_OtherMon',
                               WorkspaceIndexList=spectra_needed)
            else:
                pass
            # Deal with summed monitors
            DeleteWorkspace(monitor_ws_name)
            ConjoinWorkspaces(InputWorkspace1='spectr_ws1',InputWorkspace2='spectr_ws2')
            RenameWorkspace(InputWorkspace='spectr_ws1',OutputWorkspace=monitor_ws_name)
            if '_OtherMon' in mtd:
                ConjoinWorkspaces(InputWorkspace1=monitor_ws_name,InputWorkspace2='_OtherMon')
            else:
                pass

            monitor_ws = mtd[monitor_ws_name]
            AddSampleLog(monitor_ws,LogName='CombinedSpectraIDList',
                         LogText=str(monitors_left+spectra_list1+spectra_list2),LogType='String')
        else:
            pass
        # Weird operation. It looks like the spectra numbers obtained from
        # AppendSpectra operation depend on instrument.
        # Looks like a bug in AppendSpectra
        spec_id1 = monitor_ws.getSpectrum(0).getSpectrumNo()
        spec_id2 = monitor_ws.getSpectrum(1).getSpectrumNo()

        return (spec_id1,spec_id2),monitor_ws
    #

    def _process_spectra_list(self,workspace,spectra_list,target_ws_name='SpectraWS'):
        """Method moves all detectors of the spectra list into the same position and
           sums the specified spectra in the workspace"""
        detPos=None
        wsIDs=list()
        for spec_id in spectra_list:
            specID = workspace.getIndexFromSpectrumNumber(spec_id)
            if detPos is None:
                first_detector = workspace.getDetector(specID)
                detPos = first_detector.getPos()
            else:
                psp = workspace.getSpectrum(specID)
                detIDs = psp.getDetectorIDs()
                for detID in detIDs:
                    MoveInstrumentComponent(Workspace=workspace,ComponentName= 'Detector',
                                            DetectorID=detID,X=detPos.getX(),Y=detPos.getY(),
                                            Z=detPos.getZ(),RelativePosition=False)
            wsIDs.append(specID)

        if len(spectra_list) == 1:
            ExtractSingleSpectrum(InputWorkspace=workspace,OutputWorkspace=target_ws_name,
                                  WorkspaceIndex=wsIDs[0])
        else:
            SumSpectra(InputWorkspace=workspace,OutputWorkspace=target_ws_name,
                       ListOfWorkspaceIndices=wsIDs)
        ws = mtd[target_ws_name]
        sp = ws.getSpectrum(0)
        spectrum_num = sp.getSpectrumNo()
        return spectrum_num
#-------------------------------------------------------------------------------

    def get_ei(self, data_run, ei_guess):
        """ Calculate incident energy of neutrons and the time of the of the
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
        if monitor_ws is None:
            raise RuntimeError("Can not find monitors workspace for workspace {0}, run N{1}".
                               format(data_ws.name(),data_ws.getRunNumber()))
        separate_monitors = data_run.is_monws_separate()
        data_run.set_action_suffix('_shifted')
        # sum monitor spectra if this is requested
        if PropertyManager.ei_mon_spectra.need_to_sum_monitors(self.prop_man):
            ei_mon_spectra,monitor_ws = self.sum_monitors_spectra(monitor_ws,ei_mon_spectra)
            data_ws.setMonitorWorkspace(monitor_ws)

        # Calculate the incident energy
        #Returns: ei,mon1_peak,mon1_index,tzero
        ei,mon1_peak,_,_ = \
            GetEi(InputWorkspace=monitor_ws, Monitor1Spec=ei_mon_spectra[0],
                  Monitor2Spec=ei_mon_spectra[1],
                  EnergyEstimate=ei_guess,FixEi=fix_ei)

        # Store found incident energy in the class itself
        if self.prop_man.normalise_method == 'monitor-2' and not separate_monitors:
           # monitor-2 normalization ranges have to be identified before the
           # instrument is shifted in case it is shifted to this monitor (usual
           # case)
           #Find TOF range, correspondent to incident energy monitor peak
            energy_rage = self.mon2_norm_energy_range
#pylint: disable=attribute-defined-outside-init
            self._mon2_norm_time_range = self.get_TOF_for_energies(monitor_ws,energy_rage,
                                                                   [self.mon2_norm_spec],None,self._debug_mode)
        #end
        if separate_monitors:
            # copy incident energy obtained on monitor workspace to detectors
            # workspace
            AddSampleLog(Workspace=data_ws,LogName='Ei',LogText='{0:.10f}'.format(ei),LogType='Number')

        resultws_name = data_ws.name()
        # Adjust the TOF such that the first monitor peak is at t=0
        ScaleX(InputWorkspace=data_ws,OutputWorkspace=resultws_name,Operation="Add",Factor=-mon1_peak,
               InstrumentParameter="DelayTime",Combine=True)

        # shift to monitor used to calculate energy transfer
        spec_num = monitor_ws.getIndexFromSpectrumNumber(ei_mon_spectra[0])
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
        ws_name = result_ws.name()
        if spec_masks is not None:
            MaskDetectors(Workspace=ws_name, MaskedWorkspace=spec_masks)
        if map_file is not None:
            GroupDetectors(InputWorkspace=ws_name,OutputWorkspace=ws_name,
                           MapFile= map_file, KeepUngroupedSpectra=0, Behaviour='Average')

        return mtd[ws_name]
#-------------------------------------------------------------------------------

    def normalise(self, run, method, range_offset=0.0,external_monitors_ws=None):
        """ Apply normalization using specified source
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
                # NormalizationFactor log has been added by the algorithm themselves.
                break
            if case(): # default
                raise RuntimeError("""Normalization method {0} not found.
                                   It must be one of monitor-1, monitor-2, current, or None""".
                                   format(method))
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
            separate_monitors = True
            mon_ws = external_monitor_ws
        else:
            mon_ws = run.get_monitors_ws()

        if not mon_ws: # no monitors
            if self.__in_white_normalization: # we can normalize wb integrals by current separately as they often do not
#have monitors
                self.normalise(run,'current',range_offset)
                ws = run.get_workspace()
                new_name = ws.name()
                return ('current',new_name)
            else:
                ws = run.get_workspace()
                raise RuntimeError('Normalise by monitor-1:: Workspace {0} for run {1} does not have monitors in it'
                                   .format(ws.name(),run.run_number()))

        int_range = self.norm_mon_integration_range
        if self._debug_mode:
            kwargs = {'NormFactorWS' : 'NormMon1_WS' + data_ws.name()}
        else:
            kwargs = {}
        mon_spect = self.prop_man.mon1_norm_spec
        if separate_monitors:
            kwargs['MonitorWorkspace'] = mon_ws
            kwargs['MonitorWorkspaceIndex'] = int(mon_ws.getIndexFromSpectrumNumber(int(mon_spect)))
            range_min = float(int_range[0])
            range_max = float(int_range[1])
        else:
            kwargs['MonitorSpectrum'] = int(mon_spect) # shame TODO: change c++ algorithm, which need float monitor ID
            range_min = float(int_range[0] + range_offset)
            range_max = float(int_range[1] + range_offset)
        kwargs['NormFactorWS'] = 'Monitor1_norm_ws'

        # Normalize to monitor 1
        NormaliseToMonitor(InputWorkspace=old_name,OutputWorkspace=old_name,IntegrationRangeMin=range_min,
                           IntegrationRangeMax=range_max,IncludePartialBins=True,**kwargs)
        norm_mon1ws= mtd['Monitor1_norm_ws']
        norm_factor = norm_mon1ws.dataY(0)
        if len(norm_factor)>1:
            raise RuntimeError("Can not normalize by monitor spectra. Normalization range necessary")
        AddSampleLog(old_name,LogName='NormalizationFactor',LogText=str(norm_factor[0]),LogType='Number')
        if not self._debug_mode:
            DeleteWorkspace(norm_mon1ws)

        return ('monitor-1',old_name)
    #

    def _normalize_to_monitor2(self,run,old_name, range_offset=0.0,external_monitor_ws=None):
        """ Helper method implementing  normalize_to_monitor_2 """

      # get monitor's workspace
        separate_monitors = run.is_monws_separate()
        if external_monitor_ws:
            separate_monitors = True
            mon_ws = external_monitor_ws
        else:
            mon_ws = run.get_monitors_ws()

        if self.__in_white_normalization: # we normalize wb integrals by current separately as they often do not
            # have monitors or are in fact wb workspace with some special ei
            self.normalise(run,'current',range_offset)
            ws = run.get_workspace()
            new_name = ws.name()
            return ('current',new_name)
        else:
            if not mon_ws: # no monitors
                ws = run.get_workspace()
                raise RuntimeError('Normalize by monitor-2:: Workspace {0} for run {1} does not have monitors in it'
                                   .format(ws.name(),run.run_number()))
        #

        kwargs = {'NormFactorWS':'NormMon2_WS' + mon_ws.name()}

        mon_spect = self.prop_man.mon2_norm_spec
        mon_index = int(mon_ws.getIndexFromSpectrumNumber(mon_spect))
        if separate_monitors:
            kwargs['MonitorWorkspace'] = mon_ws
            kwargs['MonitorWorkspaceIndex'] = mon_index
        else:
            kwargs['MonitorSpectrum'] = mon_spect

        #Find TOF range, correspondent to incident energy monitor peak
        if self._mon2_norm_time_range: # range has been found during ei-calculations
            norm_range = self._mon2_norm_time_range
            range_min = norm_range[0] + range_offset
            range_max = norm_range[1] + range_offset
            self._mon2_norm_time_range = None
        else:
            mon_ws_name = mon_ws.name() #monitor's workspace and detector's workspace are e
            if mon_ws_name.find('_shifted') != -1:
              # monitor-2 normalization ranges have to be identified before the
              # instrument is shifted
                raise RuntimeError("""Instrument have been shifted but no time range has been identified.
                                   Monitor-2 normalization can not be performed """)
            else:
              # instrument and workspace shifted, so TOF will be calculated wrt
              # shifted instrument
                energy_rage = self.mon2_norm_energy_range
                TOF_range = self.get_TOF_for_energies(mon_ws,energy_rage,[mon_spect],None,self._debug_mode)
                range_min = TOF_range[0]
                range_max = TOF_range[1]
       # Normalize to monitor 2
        NormaliseToMonitor(InputWorkspace=old_name,OutputWorkspace=old_name,IntegrationRangeMin=range_min,
                           IntegrationRangeMax=range_max,IncludePartialBins=True,**kwargs)

        norm_ws_name = kwargs['NormFactorWS']
        norm_mon2ws  = mtd[norm_ws_name]
        norm_factor = norm_mon2ws.dataY(0)
        if len(norm_factor)>1:
            raise RuntimeError("Can not normalize by monitor spectra. Normalization range necessary")

        AddSampleLog(old_name,LogName='NormalizationFactor',LogText=str(norm_factor[0]),LogType='Number')
        if not self._debug_mode:
            DeleteWorkspace(norm_ws_name)

        return ('monitor-2',old_name)
#-------------------------------------------------------------------------------
#-------------------------------------------------------------------------------

    def find_tof_range_for_multirep(self,workspace):
        """ Find range of tof-s (and time bin size) corresponding to the
            energy range requested
        """
        if not workspace:
            workspace = PropertyManager.sample_run.get_workspace()

        spectra_id = self.prop_man.multirep_tof_specta_list
        if not spectra_id or len(spectra_id) == 0:
            self.prop_man.log("""*** WARNING! Multirep mode used but no closest and furthest spectra numbers
                                defined in IDF (multirep_tof_specta_list)\n"""
                              "    Using first spectra to identify TOF range for the energy range requested.\n"
                              "    This is correct only if all detectors are equidistant from the sample",
                              'warning')
            spectra_id = [1]

        eMin,dE,eMax = PropertyManager.energy_bins.get_abs_range(self.prop_man)
        ei = PropertyManager.incident_energy.get_current()
        en_list = [eMin,eMin + dE,eMax - dE,eMax]
        TOF_range = self.get_TOF_for_energies(workspace,en_list,spectra_id,ei)

        def process_block(tof_range):
            tof_range = [x for x in tof_range if not(math.isnan(x))]
            dt = list(map(lambda x,y : abs(x - y),tof_range[1:],tof_range[:-1]))
            t_step = min(dt)
            tof_min = min(tof_range)
            tof_max = max(tof_range)
            return (tof_min,t_step,tof_max)

        nBlocks = len(spectra_id)
        if nBlocks > 1:
            tof_min,t_step,tof_max = process_block(TOF_range[0])
            for ind in range(1,nBlocks):
                tof_min1,t_step1,tof_max1 = process_block(TOF_range[ind])
                tof_min = min(tof_min,tof_min1)
                tof_max = max(tof_max,tof_max1)
                t_step = min(t_step,t_step1)
        else:
            tof_min,t_step,tof_max = process_block(TOF_range)
        #end
        # add 5% for detectors specified in Par file are shifted a bit and not min-max det any more
        return (0.95*tof_min,t_step,1.05*tof_max)
        #return (tof_min,t_step,tof_max)
    #
#pylint: disable=too-many-arguments
#pylint: disable=too-many-branches
#pylint: disable=too-many-locals

    def get_TOF_for_energies(self,workspace,energy_list,specID_list,ei=None,debug_mode=False):
        """ Method to find what TOF range corresponds to given energy range
           for given workspace and detectors.

           Input:
           workspace    handler for the workspace with instrument attached.
           energy_list  the list of input energies to process
           detID_list   list of detectors to find
           ei           incident energy. If present, TOF range is calculated in direct mode,
                        if not -- elastic mode

           Returns:
           list of TOF corresponding to input energies list.
        """
        if ei:
            ei_guess = PropertyManager.incident_energy.get_current()
            fix_ei = self.fix_ei
            ei_mon_spectra = self.ei_mon_spectra
            monitor_ws = PropertyManager.sample_run.get_monitors_ws(ei_mon_spectra,workspace)
            if monitor_ws is None: # no shifting to monitor position
                src_name = None
                mon1_peak = 0
            else:
                mon_1_spec_ID = ei_mon_spectra[0]
                if isinstance(mon_1_spec_ID,collections.Iterable):
                    fix_ei = True # This could be a HACK
                    mon_1_spec_ID = mon_1_spec_ID[0]
                #-----------
                mon_2_spec_ID = ei_mon_spectra[1]
                if isinstance(mon_2_spec_ID,collections.Iterable):
                    fix_ei = True # This could be a HACK
                    mon_2_spec_ID = mon_2_spec_ID[1]
                #-----------

                # Calculate the incident energy and TOF when the particles access Monitor1
                try:
                    ei,mon1_peak,mon1_index,_ = \
                        GetEi(InputWorkspace=monitor_ws, Monitor1Spec=mon_1_spec_ID,
                              Monitor2Spec=mon_2_spec_ID,
                              EnergyEstimate=ei_guess,FixEi=fix_ei)
                    mon1_det = monitor_ws.getDetector(mon1_index)
                    mon1_pos = mon1_det.getPos()
                    src_name = monitor_ws.getInstrument().getSource().name()
                #pylint: disable=bare-except
                except:
                    src_name = None
                    mon1_peak = 0
                    en_bin  = [energy_list[0],energy_list[1]-energy_list[0],energy_list[3]]
                    self.prop_man.log("*** WARNING: message from multirep chunking procedure: get_TOF_for_energies:\n"
                                      "    not able to identify energy peak looking for TOF range for incident energy:"
                                      " {0}meV, binning: {1}\n"
                                      "    Continuing under assumption that incident neutrons arrive at source at time=0".
                                      format(ei_guess,en_bin),'warning')
        else:
            mon1_peak = 0
        #end if

        template_ws_name = '_energy_range_ws'
        range_ws_name = '_TOF_range_ws'
        y = [1] * (len(energy_list) - 1)
        TOF_range = []
        for specID in specID_list:
            ind = workspace.getIndexFromSpectrumNumber(specID)
            ExtractSingleSpectrum(InputWorkspace=workspace, OutputWorkspace=template_ws_name, WorkspaceIndex=ind)
            if ei:
                CreateWorkspace(OutputWorkspace=range_ws_name,NSpec = 1,DataX=energy_list,
                                DataY=y,UnitX='DeltaE',ParentWorkspace=template_ws_name)
                if src_name:
                    MoveInstrumentComponent(Workspace=range_ws_name,ComponentName= src_name, X=mon1_pos.getX(),
                                            Y=mon1_pos.getY(), Z=mon1_pos.getZ(), RelativePosition=False)
                range_ws = ConvertUnits(InputWorkspace=range_ws_name,OutputWorkspace=range_ws_name,
                                        Target='TOF',EMode='Direct',EFixed=ei)
            else:
                CreateWorkspace(OutputWorkspace=range_ws_name,NSpec = 1,DataX=energy_list,
                                DataY=y,UnitX='Energy',ParentWorkspace=template_ws_name)
                range_ws = ConvertUnits(InputWorkspace=range_ws_name,OutputWorkspace=range_ws_name,
                                        Target='TOF',EMode='Elastic')
            x = range_ws.dataX(0)+mon1_peak
            TOF_range.append(x.tolist())

        if not debug_mode:
            DeleteWorkspace(template_ws_name)
            DeleteWorkspace(range_ws_name)
        #
        if len(specID_list) == 1:
            TOF_range = TOF_range[0]

        return TOF_range
    #
    #pylint: disable=too-many-branches

    def save_results(self, workspace, save_file=None, formats=None):
        """
        Save the result workspace to the specified filename using the list of formats specified in
        formats. If formats is None then the default list is used
        """
        if formats:
           # clear up existing save formats as one is defined in parameters
            self.prop_man.save_format = None
        # set up internal format variable from method parameters
        self.prop_man.set_input_parameters_ignore_nan(save_file_name=save_file,save_format=formats)
        formats = self.prop_man.save_format

        if save_file:
            save_file,ext = os.path.splitext(save_file)
            if len(ext) > 1:
                formats.add(ext[1:])
        else:
            save_file = self.prop_man.save_file_name

        if save_file is None:
            if workspace is None:
                self.prop_man.log("DirectEnergyConversion:save_results: Nothing to save",
                                  'warning')
                return
            else:
                save_file = workspace.name()
        elif os.path.isdir(save_file):
            save_file = os.path.join(save_file, workspace.name())
        elif save_file == '':
            raise ValueError('Empty filename is not allowed for saving')
        else:
            pass

        prop_man = self.prop_man
        name_orig = workspace.name()
        for file_format  in formats:
            for case in common.switch(file_format):
                if case('nxspe'):
                    filename = save_file + '.nxspe'
                   # nxspe can not write workspace with / in the name
                   # (something to do with folder names inside nxspe)
                    name_supported = name_orig.replace('/','of')
                    if name_supported != name_orig:
                        RenameWorkspace(InputWorkspace=name_orig,OutputWorkspace=name_supported)
                    SaveNXSPE(InputWorkspace=name_supported,Filename= filename,
                              KiOverKfScaling=prop_man.apply_kikf_correction,psi=prop_man.psi)
                    if name_supported != name_orig:
                        RenameWorkspace(InputWorkspace=name_supported,OutputWorkspace=name_orig)
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
                    prop_man.log("Unknown file format {0} requested to save results. No saving performed this format".
                                 format(file_format))
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
        """ The property keeps a workspace with masks workspace name,
            stored for further usage"""

        # check if spectra masks is defined
        if hasattr(self,'_spectra_masks'):
            if self._spectra_masks is not None and self._spectra_masks in mtd:
                return mtd[self._spectra_masks]
            else:
                self._spectra_masks = None
            return None
        else:
            return None

    @spectra_masks.setter
    def spectra_masks(self,value):
        """ set up spectra masks """
        if value is None:
            if hasattr(self,'_spectra_masks') and self._spectra_masks is not None:
                if self._spectra_masks in mtd:
                    DeleteWorkspace(self._spectra_masks)
            self._spectra_masks=None
        elif isinstance(value,api.Workspace):
            self._spectra_masks = value.name()
        elif isinstance(value,str):
            if value in mtd:
                self._spectra_masks = value
            else:
                self._spectra_masks = None
        else:
#pylint: disable=W0201
            self._spectra_masks = None
        return
#-------------------------------------------------------------------------------
#pylint: disable=too-many-arguments

    def apply_absolute_normalization(self,sample_ws,monovan_run=None,ei_guess=None,wb_mono=None,abs_norm_factor_is=None):
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
            if not abs_norm_factor_is:
                abs_norm_factor_is = ' provided '

        else:
            mvir = prop_man.monovan_integr_range
            prop_man.log('*** Evaluating the integral from the monovan run and calculate the correction factor ******',
                         'notice')
            prop_man.log('    Using absolute units vanadium integration range : [{0:8f}:{1:8f}]         ******'.
                         format(mvir[0],mvir[1]),'notice')
            if not abs_norm_factor_is:
                abs_norm_factor_is = 'calculated'

            # convert to monovanadium to energy
            deltaE_wkspace_monovan = self.mono_sample(monovan_run,ei_guess,wb_mono,
                                                      self.monovan_mapfile,self.spectra_masks)

            ei_monovan = deltaE_wkspace_monovan.getRun().getLogData("Ei").value
            prop_man.log('    Incident energy found for monovanadium run: ' + str(ei_monovan) + ' meV','notice')

            (anf_LibISIS,anf_SS2,anf_Puas,anf_TGP) = self.get_abs_normalization_factor(monovan_run,ei_monovan)

            prop_man.log("""*** Absolute correction factor(s): S^2: {0:10.4f}
*** LibISIS: {1:10.4f} Poisson: {2:10.4f}  TGP: {3:10.4f} """
                         .format(anf_LibISIS,anf_SS2,anf_Puas,anf_TGP),'notice')
            prop_man.log('*** If these factors are substantially different, something is wrong                    ***','notice')
            absnorm_factor = anf_TGP
            # Store the factor for further usage
            PropertyManager.mono_correction_factor.set_val_to_cash(prop_man,anf_TGP)
            # reset current monovan run to run number (if it makes sense) --
            ## workspace is not good for further processing any more
        #end
        prop_man.log('*** Using {0} value : {1} of absolute units correction factor (TGP)'.
                     format(abs_norm_factor_is,absnorm_factor),'notice')
        prop_man.log('*******************************************************************************************','notice')

        sample_ws = sample_ws / absnorm_factor

        return sample_ws
#-------------------------------------------------------------------------------
#pylint: disable=too-many-branches
#pylint: disable=too-many-locals

    def get_abs_normalization_factor(self,monovan_run,ei_monovan):
        """get absolute normalization factor for monochromatic vanadium

          Inputs:
          @param: monvan_run   -- run descriptor of converted to energy monovan workspace
          @param: ei_monovan   -- monovan sample incident energy

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
        mono_vs = monovan_run.get_workspace()
        ws_name = mono_vs.name()

        data_ws = Integration(InputWorkspace=mono_vs,OutputWorkspace='van_int',
                              RangeLower=minmax[0],RangeUpper=minmax[1],IncludePartialBins='1')

        nhist = data_ws.getNumberHistograms()
        # extract wb integrals for combined spectra
        signal = []
        error = []
        izerc = 0
        data_specInfo = data_ws.spectrumInfo()
        for i in range(nhist):
            if not data_specInfo.hasDetectors(i):
                continue
            if data_specInfo.isMasked(i):
                continue
            sig = data_ws.readY(i)[0]
            err = data_ws.readE(i)[0]
            if sig != sig:     #ignore NaN (hopefully it will mean mask some day)
                continue
            if (err <= 0) or (sig <= 0):  # count Inf and negative||zero readings.
                izerc+=1                     # Presence of this indicates that
                continue                     # something went wrong
            signal.append(sig)
            error.append(err)
        #---------------- Loop finished

        norm_factor = {}
        # Various prior probabilities.
        #-------------------------------------------------------------------------
        # Guess which minimizes the value sum(n_i-n)^2/Sigma_i -- this what
        # Libisis had
        signal_sum = sum([ s / e  for s,e in zip(signal,error)])
        weight_sum = sum([1. / e  for e   in error])
        norm_factor['LibISIS'] = signal_sum / weight_sum
        #-------------------------------------------------------------------------
        # Guess which minimizes the value sum(n_i-n)^2/Sigma_i^2
        signal_sum = sum([ s / (e * e)  for s,e in zip(signal,error)])
        weight_sum = sum([1. / (e * e)  for e   in error])
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
        signal_sum = sum([ e * e     for e   in error])
        weight_sum = sum([ e * e / s for s,e in zip(signal,error)])
        if weight_sum == 0.0:
            prop_man.log("WB integral has been calculated incorrectly, look at van_int workspace: {0}".format(ws_name),'error')
            raise ArithmeticError("Division by 0 weight when calculating WB integrals from workspace {0}".format(ws_name))
        norm_factor['Poisson'] = signal_sum / weight_sum
        #-------------------------------------------------------------------------
        # Guess which estimates value sum(n_i^2/Sigma_i^2)/sum(n_i/Sigma_i^2)
        # TGP suggestion from 12-2012
        signal_sum = sum([s * s / (e * e) for s,e in zip(signal,error)])
        weight_sum = sum([s / (e * e)     for s,e in zip(signal,error)])
        if weight_sum == 0.0:
            prop_man.log("WB integral has been calculated incorrectly, look at van_int workspace: {0}".format(ws_name),'error')
            raise ArithmeticError("Division by 0 weight when calculating WB integrals from workspace {0}".format(ws_name))
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

        for norm_type,val in iteritems(norm_factor):
            norm_factor[norm_type] = val * scale_factor

        # check for NaN
        if (norm_factor['LibISIS'] != norm_factor['LibISIS']) | (izerc != 0):    # It is an error, print diagnostics:
            if norm_factor['LibISIS'] != norm_factor['LibISIS']:
                log_value = '\n--------> Absolute normalization factor is NaN <----------------------------------------------\n'
            else:
                log_value = '\n--------> Warning, Monovanadium has zero spectra <--------------------------------------------\n'
                log1_value = """--------> Processing workspace: {0}
--------> Monovan Integration range : min={1}, max={2} (meV)
--------> Summed:  {3} spectra with total signal: {4} and error: {5}
--------> Dropped: {6} zero spectra
--------> Using  mBarn/sR*fu normalization factor = {7} resulting in:
--------> Abs norm factors: LibISIS: {8}
--------> Abs norm factors: Sigma^2: {9}
--------> Abs norm factors: Poisson: {10}
--------> Abs norm factors: TGP    : {11}\n"""\
                .format(ws_name,minmax[0],minmax[1],nhist,sum(signal),sum(error),izerc,scale_factor,
                        norm_factor['LibISIS'],norm_factor['SigSq'],norm_factor['Poisson'],norm_factor['TGP'])
            log_value = log_value + log1_value
            propman.log(log_value,'error')
        else:
            if not self._debug_mode:
                monovan_run.clear_resulting_ws()
                DeleteWorkspace(Workspace=data_ws)
        return (norm_factor['LibISIS'],norm_factor['SigSq'],norm_factor['Poisson'],norm_factor['TGP'])

    #---------------------------------------------------------------------------
    # Behind the scenes stuff
    #---------------------------------------------------------------------------
    def __init__(self, instr_name=None,reload_instrument=False):
        """Constructor """

        object.__setattr__(self,'_descriptors',[])
        object.__setattr__(self,'_propMan',None)
        # Debug parameter.  Usually True unless investigating a problem
        object.__setattr__(self,'_keep_wb_workspace',True)
        object.__setattr__(self,'_do_ISIS_reduction',True)
        object.__setattr__(self,'_spectra_masks',None)
        # if normalized by monitor-2, range have to be established before
        # shifting the instrument
        object.__setattr__(self,'_mon2_norm_time_range',None)
        object.__setattr__(self,'_debug_mode',False)
        # method used in debug mode and requesting event workspace to be
        # rebinned first
        object.__setattr__(self,'_do_early_rebinning',False)
        # internal parameter, specifying work in multirep mode.  If True, some
        # auxiliary workspaces should not be deleted until used for each
        # workspace
        # processed
        object.__setattr__(self,'_multirep_mode',False)
        # list of workspace names, processed earlier
        object.__setattr__(self,'_old_runs_list',[])

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
#pylint: disable=attribute-defined-outside-init
        self._keep_wb_workspace = True # for the time being.  May be auto-calculated later but should it?
#pylint: disable=attribute-defined-outside-init
        self._do_ISIS_reduction = True
        # if normalized by monitor-2, range have to be established before
        # shifting the instrument
#pylint: disable=attribute-defined-outside-init
        self._mon2_norm_time_range = None
        # WB may not have monitors. In this case the property have to be set to True
        # and WB normalization will not fail but will run normalize by current
        self.__in_white_normalization = False
#pylint: disable=attribute-defined-outside-init
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
            old_name = self._propMan.instrument.name()
#pylint: disable=protected-access
            if isinstance(instr,geometry._geometry.Instrument):
                new_name = self._propMan.instrument.name()
            elif isinstance(instr,PropertyManager):
                new_name = instr.instr_name
            else:
                new_name = instr
            #end if
            if old_name != new_name or reload_instrument:
#pylint: disable=attribute-defined-outside-init
                self._propMan = PropertyManager(new_name)
            #end if
        #
#pylint: disable=unused-argument

    def setup_instrument_properties(self, workspace=None,reload_instrument=False):
        if workspace is not None:
            instrument = workspace.getInstrument()
            name = instrument.name()
            if name != self.prop_man.instr_name:
                self.prop_man = PropertyManager(name,workspace)

    def get_run_descriptor(self,run):
        """ Spawn temporary run descriptor for input data given in format,
           different from run descriptor. Return existing run descriptor,
           if it is what provided.
       """
        if not isinstance(run,RunDescriptor):
#pylint: disable=protected-access
            tRun = copy.copy(PropertyManager._tmp_run)
            tRun.__set__(None,run)
            return tRun
        else:
            return run
    #
# -------------------------------------------------------------------------------------------
#         This actually does the conversion for the mono-sample and
#         mono-vanadium runs
# -------------------------------------------------------------------------------------------
#pylint: disable=too-many-arguments

    def _do_mono_SNS(self, data_ws, result_name, ei_guess,
                     white_run=None, map_file=None, spectra_masks=None, Tzero=None):
        # does not work -- retrieve from repo and fix if this functionality is needed.
        raise NotImplementedError("Non currently implemented. Retrieve from repository"
                                  " if necessary and fix")
        #return
#-------------------------------------------------------------------------------
#pylint: disable=too-many-arguments
#pylint: disable=unused-argument

    def _do_mono_ISIS(self, data_run, ei_guess,
                      white_run=None, map_file=None, spectra_masks=None, Tzero=None):

        # Do ISIS stuff for Ei
        _, mon1_peak = self.get_ei(data_run, ei_guess)

        # As we've shifted the TOF so that mon1 is at t=0.0 we need to account
        # for this in CalculateFlatBackground and normalization
        bin_offset = -mon1_peak
        result_name = data_run.set_action_suffix('_spe')

        if self.check_background:
            # Remove the count rate seen in the regions of the histograms
            # defined as the background regions, if the user defined such
            # region
            result_ws = data_run.get_workspace()
            bkgd_range = self.bkgd_range
            bkg_range_min = bkgd_range[0] + bin_offset
            bkg_range_max = bkgd_range[1] + bin_offset
            if isinstance(result_ws,api.IEventWorkspace) or PropertyManager.incident_energy.multirep_mode():
                bkgr_ws = self._find_or_build_bkgr_ws(data_run,bkg_range_min,bkg_range_max,bin_offset)
            else:
                bkgr_ws = None
                CalculateFlatBackground(InputWorkspace=result_ws,OutputWorkspace=result_ws,
                                        StartX= bkg_range_min,EndX= bkg_range_max,
                                        WorkspaceIndexList= '',Mode= 'Mean',OutputMode='Subtract Background',
                                        SkipMonitors='1',NullifyNegativeValues='0')
        else:
            bkgr_ws = None
            result_ws = data_run.get_workspace()
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

        energy_bins = PropertyManager.energy_bins.get_abs_range(self.prop_man)
        if energy_bins:
            Rebin(InputWorkspace=result_name,OutputWorkspace=result_name,Params= energy_bins,PreserveEvents=False,
                  IgnoreBinErrors=True)
            if bkgr_ws:
                #apply data ws normalization to background workspace
                data_run.export_normalization(bkgr_ws)
                # remove background after converting units and rebinning
                RemoveBackground(InputWorkspace=result_name,OutputWorkspace=result_name,BkgWorkspace=bkgr_ws,EMode='Direct')
                DeleteWorkspace(bkgr_ws)
        else:
            pass # TODO: investigate way of removing background from event workspace if we want
                 # result to be an event workspace
            # what to do with event workspace having negative events?  will
                            # further algorithms work with these events ?

        if self.apply_detector_eff and energy_bins: #should detector efficiency work on event workspace too?  At the moment it is
                                                    #not (01/02/2015)
            DetectorEfficiencyCor(InputWorkspace=result_name,OutputWorkspace=result_name)
            self.prop_man.log("_do_mono: finished DetectorEfficiencyCor for : " + result_name,'information')
        #############
        data_run.synchronize_ws(mtd[result_name])

        return
#-------------------------------------------------------------------------------

    def _find_or_build_bkgr_ws(self,run,bkg_range_min=None,bkg_range_max=None,time_shift=0):
        """ Method calculates  background workspace or restore workspace with
            the same name as the one produced by this method from ADS
        """
        if not bkg_range_min or not bkg_range_max:
            bkg_range_min,bkg_range_max = self.bkgd_range
        bkg_range_min += time_shift
        bkg_range_max += time_shift
        run = self.get_run_descriptor(run)
        result_ws = run.get_workspace()

        # has to have specific name for this working. The ws is build at
        # the beginning of multirep run.
        if 'bkgr_ws_source' in mtd:
            #TODO: This is questionable operation, which may be unnecessary if remove background
            # uses time interval only. (and it probably does)
            # need to check if bkgr_ws =mtd['bkgr_ws_source'] is enough here.
            # (and not delete it after bkg removal)
            bkgr_ws = CloneWorkspace(InputWorkspace='bkgr_ws_source',OutputWorkspace='bkgr_ws')
            if time_shift != 0: # Workspace has probably been shifted, so to have
                                # one needs to do appropriate shift here
                                #correct units conversion as well
                CopyInstrumentParameters(result_ws,bkgr_ws)
             # Adjust the TOF such that the first monitor peak is at t=0
                ScaleX(InputWorkspace=bkgr_ws,OutputWorkspace='bkgr_ws',Operation="Add",Factor=time_shift,
                       InstrumentParameter="DelayTime",Combine=True)
        else: # calculate background workspace for future usage
            bkgr_ws = Rebin(result_ws,Params=[bkg_range_min,(bkg_range_max - bkg_range_min) * 1.001,bkg_range_max],PreserveEvents=False,
                            IgnoreBinErrors=True)
            RenameWorkspace(InputWorkspace=bkgr_ws, OutputWorkspace='bkgr_ws_source')
            bkgr_ws = mtd['bkgr_ws_source']

        return bkgr_ws
#-------------------------------------------------------------------------------
#pylint disable=too-many-arguments

    def _do_mono(self, run,  ei_guess,
                 white_run=None, map_file=None, spectra_masks=None, Tzero=None):
        """
        Convert units of a given workspace to deltaE, including possible
        normalization to a white-beam vanadium run.
        """

        if self._do_ISIS_reduction:
            self._do_mono_ISIS(run,ei_guess,
                               white_run, map_file, spectra_masks, Tzero)
        else:
            result_name = run.set_action_suffix('_spe')
            self._do_mono_SNS(run,result_name,ei_guess,
                              white_run, map_file, spectra_masks, Tzero)
            run.synchronize_ws()

        prop_man = self.prop_man
        ws = run.get_workspace()
        result_name = ws.name()
        #######################
        # Ki/Kf Scaling...
        if prop_man.apply_kikf_correction:
            prop_man.log('Start Applying ki/kf corrections to the workspace : ' + result_name,'information')
            CorrectKiKf(InputWorkspace=result_name,OutputWorkspace= result_name, EMode='Direct')
            prop_man.log('finished applying ki/kf corrections for : ' + result_name,'information')

        # Make sure that our binning is consistent
        if prop_man.energy_bins:
            bins = PropertyManager.energy_bins.get_abs_range(prop_man)
            Rebin(InputWorkspace=result_name,OutputWorkspace= result_name,Params=bins,
                  IgnoreBinErrors=True)

        # Masking and grouping
        result_ws = mtd[result_name]
        result_ws = self.remap(result_ws, spectra_masks, map_file)

        if prop_man.energy_bins: # It should already be a distribution.
            ConvertToDistribution(Workspace=result_ws)
        # nullify negarive signals if necessary
        if prop_man.check_background and (hasattr(prop_man,'nullify_negative_signal') and
                                          prop_man.nullify_negative_signal):
            zeroBg = CreateWorkspace(DataX='0,1',DataY=0,DataE=0,UnitX='TOF')
            result_ws=RemoveBackground(result_ws,BkgWorkspace=zeroBg,Emode='Direct',NullifyNegativeValues=True)
            DeleteWorkspace(zeroBg)

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
        """Obtain white bean vanadium integrals either by integrating
           workspace in question or using cashed value
        """
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
#pylint: disable=attribute-defined-outside-init
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
        white_ws = Rebin(InputWorkspace=old_name,OutputWorkspace=old_name, Params=[low, delta, upp],
                         IgnoreBinErrors=True)
        # Why aren't we doing this...-> because integration does not work properly for event workspaces
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
        """build tag indicating wb-integration ranges """
        low,upp = self.wb_integr_range
        white_tag = 'NormBy:{0}_IntergatedIn:{1:0>10.2f}:{2:0>10.2f}'.format(self.normalise_method,low,upp)
        return white_tag
    #

    def _clear_old_results(self):
        """Remove workspaces, processed earlier and not used any more"""
        ws_list = self._old_runs_list
        for ws_name in ws_list:
            if ws_name in mtd:
                DeleteWorkspace(ws_name)
        object.__setattr__(self,'_old_runs_list',[])
    #


def get_failed_spectra_list_from_masks(masked_wksp,prop_man):
    """Compile a list of spectra numbers that are marked as
       masked in the masking workspace

       Input:
       masking_workspace - A special masking workspace containing masking data
    """
    #TODO: get rid of this and use data, obtained form diagnostics
    failed_spectra = []
    if masked_wksp is None:
        return (failed_spectra,0)
    try:
        masked_wksp.name()
#pylint: disable=broad-except
    except Exception:
        prop_man.log("***WARNING: cached mask workspace invalidated. Incorrect masking reported")
        return (failed_spectra,0)

    masking_wksp,sp_list = ExtractMask(masked_wksp)
    DeleteWorkspace(masking_wksp)

    n_spectra = len(sp_list)
    return n_spectra


#-----------------------------------------------------------------
if __name__ == "__main__":
    pass
    #unittest.main()
