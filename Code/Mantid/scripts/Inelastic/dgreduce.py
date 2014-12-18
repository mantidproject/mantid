import DirectEnergyConversion as DRC
import CommonFunctions as common
import time as time
import numpy
from mantid.simpleapi import *
from mantid import api
from mantid.kernel import funcreturns

import unittest

# the class which is responsible for data reduction
global Reducer
Reducer = None

# Statement used at debug time to pull changes in DirectEnergyConversion into Mantid
#DRC=reload(DRC)
def getReducer():
    # needed on Linux to adhere to correct reference return
    global Reducer;
    return Reducer

def setup(instname=None,reload=False):
    """
    setup('mar')
    setup instrument reduction parameters from instname_parameter.xml file

    if the instrument has already been defined, does nothing unless
    reload = True is specified
    """

    global Reducer
    if instname == None :
        instname = config['default.instrument']


    if not (Reducer is None) :
        if  Reducer.instr_name.upper()[0:3] == instname.upper()[0:3] :
            if not reload :
                # reinitialize idf parameters to defaults.
                Reducer.init_idf_params(True);
                return  # has been already defined

    Reducer = DRC.setup_reducer(instname)

def help(keyword=None) :
    """function returns help on reduction parameters.

       Returns the list of the parameters availible if provided without arguments
       or the description and the default value for the key requested
    """
    if Reducer == None:
        raise ValueError("Reducer has not been defined, call setup(instrument_name) first.")

    Reducer.help(keyword)


def arb_units(wb_run,sample_run,ei_guess,rebin,map_file='default',monovan_run=None,**kwargs):
    """ One step conversion of run into workspace containing information about energy transfer
    Usage:
    >>arb_units(wb_run,sample_run,ei_guess,rebin)

    >>arb_units(wb_run,sample_run,ei_guess,rebin,**arguments)

    >>arb_units(wb_run,sample_run,ei_guess,rebin,mapfile,**arguments)

    >>arb_units(wb_run   Whitebeam run number or file name or workspace
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
    >>dgreduce.arb_units(1000,10001,80,[-10,.1,70])  # will run on default instrument

    >>dgreduce.arb_units(1000,10001,80,[-10,.1,70],'mari_res', additional keywords as required)

    >>dgreduce.arb_units(1000,10001,80,'-10,.1,70','mari_res',fixei=True)

    A detector calibration file must be specified if running the reduction with workspaces as input
    namely:
    >>w2=iliad("wb_wksp","run_wksp",ei,rebin_params,mapfile,det_cal_file=cal_file
               ,diag_remove_zero=False,norm_method='current')


    type help() for the list of all available keywords. All availible keywords are provided in InstName_Parameters.xml file


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
    diag_samp_sig                   - Error criterion as a multiple of error bar i.e. to fail the test, the magnitude of the"
                                      difference with respect to the median value must also exceed this number of error bars (default=3.3)
    variation       -The number of medians the ratio of the first/second white beam can deviate from
                     the average by (default=1.1)
    bleed_test      - If true then the CreatePSDBleedMask algorithm is run
    bleed_maxrate   - If the bleed test is on then this is the maximum framerate allowed in a tube
    bleed_pixels    - If the bleed test is on then this is the number of pixels ignored within the
                       bleed test diagnostic
    print_results - If True then the results are printed to the screen

    diag_remove_zero =True, False (default):Diag zero counts in background range
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
    global Reducer
    if Reducer is None or Reducer.instrument is None:
        raise ValueError("instrument has not been defined, call setup(instrument_name) first.")
# --------------------------------------------------------------------------------------------------------
#    Deal with mandatory parameters for this and may be some top level procedures
# --------------------------------------------------------------------------------------------------------
    Reducer.log("****************************************************************");
    if isinstance(sample_run,api.Workspace) or (isinstance(sample_run,str) and sample_run in mtd):
        Reducer.log('*** DGreduce run for: {0:>20} :  Workspace name: {1:<20} '.format(Reducer.instr_name,str(sample_run)))
    else:
        Reducer.log('*** DGreduce run for: {0:>20} :  Run number/s : {1:<20} '.format(Reducer.instr_name,str(sample_run)))

    try:
        n,r=funcreturns.lhs_info('both')
        wksp_out=r[0]
    except:
        if sample_run == 0:
            #deal with the current run being parsed as 0 rather than 00000
            sample_run='00000'
        wksp_out=Reducer.instr_name+str(sample_run)+'.spe'
        if kwargs.has_key('sum') and kwargs.get('sum')==True:
            wksp_out=inst_name+str(sample_run[0])+'sum'+'.spe'

    start_time=time.time()

    if sample_run=='00000' and mtd.doesExist(inst_name+'00000.raw')==True:
        Reducer.log('Deleting previous instance of temp data')
        DeleteWorkspace(Workspace=inst_name+'00000.raw')


    # we may want to run absolute units normalization and this function has been called with monovan run or helper procedure
    abs_units_defaults_check = False
    if monovan_run != None :
       # check if mono-vanadium is provided as multiple files list or just put in brackets occasionally
        Reducer.log("****************************************************************");
        Reducer.log('*** Output will be in absolute units of mb/str/mev/fu')
        if isinstance(monovan_run,list):
                if len(monovan_run)>1:
                    raise IOError(' Can currently work only with single monovan file but list supplied')
                else:
                    monovan_run = monovan_run[0];
        abs_units_defaults_check =True
        if '_defaults_have_changed' in kwargs:
            del kwargs['_defaults_have_changed']
            abs_units_defaults_check =False
    if "wb_for_monovanadium" in kwargs :
         wb_for_monovanadium = kwargs['wb_for_monovanadium']
         del kwargs['wb_for_monovanadium']
    else:
         wb_for_monovanadium = wb_run;



    if isinstance(ei_guess,str):
        ei_guess = float(ei_guess)

    # set rebinning range
    Reducer.energy_bins = rebin
    Reducer.incident_energy = ei_guess;
    if Reducer.energy_bins[2] > ei_guess:
        Reducer.log('Error: rebin max rebin range {0:f} exceeds incident energy {1:f}'.format(Reducer.energy_bins[2],ei_guess),'Error')
        return

    # Process old legacy parameters which are easy to re-define in dgreduce rather then transfer through Mantid
    program_args = process_legacy_parameters(**kwargs)

    # set non-default reducers parameters and check if all optional keys provided as parameters are acceptable and have been defined in IDF
    changed_Keys=Reducer.set_input_parameters(**program_args);

    # inform user about changed parameters

    Reducer.log("*** Provisional Incident energy: {0:>12.3f} mEv".format(ei_guess))
    Reducer.log("****************************************************************");
    for key in changed_Keys:
        val = getattr(Reducer,key);
        Reducer.log("  Value of : {0:<25} is set to : {1:<20} ".format(key,val))


    save_dir = config.getString('defaultsave.directory')
    Reducer.log("****************************************************************");
    if monovan_run != None and not('van_mass' in changed_Keys or 'vanadium-mass' in changed_Keys) :
         Reducer.log("*** Monochromatic vanadium mass used : {0} ".format(Reducer.van_mass))
    Reducer.log("*** By default results are saved into: {0}".format(save_dir));
    Reducer.log("****************************************************************");
    #do we run absolute units normalization and need to warn users if the parameters needed for that have not changed from defaults
    if abs_units_defaults_check :
        Reducer.check_abs_norm_defaults_changed(changed_Keys);

    #process complex parameters


    # map file given in parameters overrides default map file
    if map_file != 'default' :
        Reducer.map_file = map_file
    # defaults can be None too, but can be a file
    if  Reducer.map_file == None:
        Reducer.log('one2one map selected')


    if  Reducer.det_cal_file != None :
        if isinstance(Reducer.det_cal_file,str) and not Reducer.det_cal_file in mtd : # it is a file
            Reducer.log('Setting detector calibration file to '+Reducer.det_cal_file)
        else:
           Reducer.log('Setting detector calibration to {0}, which is probably a workspace '.format(str(Reducer.det_cal_file)))
    else:
        Reducer.log('Setting detector calibration to detector block info from '+str(sample_run))

    # check if reducer can find all non-run files necessary for the reduction before starting long run.
    Reducer.check_necessary_files(monovan_run);

    print 'Output will be normalized to', Reducer.normalise_method
    if (numpy.size(sample_run)) > 1 and Reducer.sum_runs:
        #this sums the runs together before passing the summed file to the rest of the reduction
        #this circumvents the inbuilt method of summing which fails to sum the files for diag

        #the D.E.C. tries to be too clever so we have to fool it into thinking the raw file is already exists as a workpsace
        sumfilename=Reducer.instr_name+str(sample_run[0])+'.raw'
        sample_run =sum_files(Reducer.instr_name,sumfilename, sample_run)
        common.apply_calibration(Reducer.instr_name,sample_run,Reducer.det_cal_file)

        #sample_run = RenameWorkspace(InputWorkspace=accum,OutputWorkspace=inst_name+str(sample_run[0])+'.raw')


    if Reducer.mask_run == None :
        mask_run=sample_run

    masking = None;
    masks_done=False
    if not Reducer.run_diagnostics:
       header="Diagnostics including hard masking is skipped "
       masks_done = True;
    if Reducer.save_and_reuse_masks :
        raise NotImplementedError("Save and reuse masks option is not yet implemented")
        mask_file_name = common.create_resultname(str(mask_run),Reducer.instr_name,'_masks.xml')
        mask_full_file = FileFinder.getFullPath(mask_file_name)
        if len(mask_full_file) > 0 :
            masking = LoadMask(Instrument=Reducer.instr_name,InputFile=mask_full_file,OutputWorkspace=mask_file_name)
            #Reducer.hard_mask_file = mask_full_file;
            #Reducer.use_hard_mask_only = True
            masks_done=True
            header="Masking fully skipped and processed {0} spectra and  {1} bad spectra "
        else:
            pass
#-------------------------------------------------------------------------------------------------------------------------------------------------------
#  Here we give control to the Reducer
# --------------------------------------------------------------------------------------------------------
     # diag the sample and detector vanadium. It will deal with hard mask only if it is set that way
    if not   masks_done:
        print '########### Run diagnose for sample run ##############################'
        masking = Reducer.diagnose(wb_run,sample = mask_run,
                                    second_white = None,print_results=True)
        header = "Diag Processed workspace with {0:d} spectra and masked {1:d} bad spectra"


   # Calculate absolute units:
        if monovan_run != None :
            if Reducer.mono_correction_factor == None :
                if Reducer.use_sam_msk_on_monovan == True:
                    Reducer.log('  Applying sample run mask to mono van')
                else:
                    if not Reducer.use_hard_mask_only : # in this case the masking2 is different but points to the same workspace Should be better soulution for that.
                        print '########### Run diagnose for monochromatic vanadium run ##############'
                        masking2 = Reducer.diagnose(wb_for_monovanadium,sample=monovan_run,
                                         second_white = None,rint_results=True)
                        masking +=  masking2
                        DeleteWorkspace(masking2)


            else: # if Reducer.mono_correction_factor != None :
                pass

    # save mask if it does not exist and has been already loaded
    if Reducer.save_and_reuse_masks and not masks_done:
        SaveMask(InputWorkspace=masking,OutputFile = mask_file_name,GroupedDetectors=True)

    # Very important statement propagating masks for further usage in convert_to_energy
    Reducer.spectra_masks=masking
    # estimate and report the number of failing detectors
    failed_sp_list,nSpectra = get_failed_spectra_list_from_masks(masking)
    nMaskedSpectra = len(failed_sp_list)
    # this tells turkey in case of hard mask only but everything else semens work fine
    print header.format(nSpectra,nMaskedSpectra)
     #Run the conversion first on the sample
    deltaE_wkspace_sample = Reducer.convert_to_energy(sample_run, ei_guess, wb_run)


    # calculate absolute units integral and apply it to the workspace
    if monovan_run != None or Reducer.mono_correction_factor != None :
        deltaE_wkspace_sample = apply_absolute_normalization(Reducer,deltaE_wkspace_sample,monovan_run,ei_guess,wb_run)
        # Hack for multirep
        #if isinstance(monovan_run,int):
        #    filename = common.find_file(monovan_run)
        #    output_name = common.create_dataname(filename);
       #     DeleteWorkspace(output_name);


    results_name = deltaE_wkspace_sample.name();
    if results_name != wksp_out:
       RenameWorkspace(InputWorkspace=results_name,OutputWorkspace=wksp_out)


    ei= (deltaE_wkspace_sample.getRun().getLogData("Ei").value)
    print 'Incident energy found for sample run: ',ei,' meV'

    end_time=time.time()
    print 'Elapsed time =',end_time-start_time, 's'

    if mtd.doesExist('_wksp.spe-white')==True:
        DeleteWorkspace(Workspace='_wksp.spe-white')
    # Hack for multirep mode?
    if mtd.doesExist('hard_mask_ws') == True:
        DeleteWorkspace(Workspace='hard_mask_ws')

    return deltaE_wkspace_sample



def abs_units(wb_for_run,sample_run,monovan_run,wb_for_monovanadium,samp_rmm,samp_mass,ei_guess,rebin,map_file='default',monovan_mapfile='default',**kwargs):
    """
    dgreduce.abs_units(wb_run          Whitebeam run number or file name or workspace
                  sample_run          Sample run run number or file name or workspace
                  monovan_run          Monochromatic run run number or file name or workspace
                  wb_mono          White beam for Monochromatic run run number or file name or workspace
                  samp_rmm          Mass of formula unit of sample
                  samp_mass          Actual sample mass
                  ei_guess          Ei guess of run
                  rebin          Rebin parameters for output data
                  map_file          Mapfile for sample run
                  monovan_mapfile     Mapfile for mono van run
                  keyword arguments     Any specified additional keyword arguments

    Example with run numbers
    abs_units(11001,11002,11003,10098,250.1,5.2,80,'-10,.1,75','mari_res','mari_res')

    A detector calibration file must be specified if running the reduction with workspace inputs

    Example with workspace inputs

    abs_units('wb_run','sam_run','mono_run','wb_for_mono',250.1,5.2,80,'-10,.1,75','mari_res','mari_res',
                   det_cal_file=10001,diag_remove_zero=False,norm_method='current')


    A detector calibration file must be specified if running the reduction with workspace inputs
    Available keywords
    norm_method =[monitor-1],[monitor-2][Current]
    background  =False , True
    fixei       =False , True
    save_format =['.spe'],['.nxspe'],'none'
    detector_van_range          =[20,40] in mev

    bkgd_range  =[15000,19000]  :integration range for background tests

    second_white    - If provided an additional set of tests is performed on this. (default = None)
    hard_mask_file       - A file specifying those spectra that should be masked without testing (default=None)
    tiny            - Minimum threshold for acceptance (default = 1e-10)
    large           - Maximum threshold for acceptance (default = 1e10)
    bkgd_range      - A list of two numbers indicating the background range (default=instrument defaults)
    diag_van_median_rate_limit_lo   - Lower bound defining outliers as fraction of median value (default = 0.01)
    diag_van_median_rate_limit_hi   - Upper bound defining outliers as fraction of median value (default = 100.)
    diag_van_median_sigma_lo        - Fraction of median to consider counting low for the white beam diag (default = 0.1)
    diag_van_median_sigma_hi        - Fraction of median to consider counting high for the white beam diag (default = 1.5)
    diag_van_sig  - Error criterion as a multiple of error bar i.e. to fail the test, the magnitude of the
                    difference with respect to the median value must also exceed this number of error bars (default=0.0)
    diag_remove_zero                - If true then zeros in the vanadium data will count as failed (default = True)
    diag_samp_samp_median_sigma_lo  - Fraction of median to consider counting low for the white beam diag (default = 0)
    diag_samp_samp_median_sigma_hi  - Fraction of median to consider counting high for the white beam diag (default = 2.0)
    diag_samp_sig                   - Error criterion as a multiple of error bar i.e. to fail the test, the magnitude of the"
                                      difference with respect to the median value must also exceed this number of error bars (default=3.3)
    variation       -The number of medians the ratio of the first/second white beam can deviate from
                    the average by (default=1.1)
    bleed_test      - If true then the CreatePSDBleedMask algorithm is run
    bleed_maxrate   - If the bleed test is on then this is the maximum framerate allowed in a tube
    bleed_pixels    - If the bleed test is on then this is the number of pixels ignored within the
                    bleed test diagnostic
    print_results - If True then the results are printed to the screen

    diag_remove_zero =True, False (default):Diag zero counts in background range

    bleed=True , turn bleed correction on and off on by default for Merlin and LET

    sum =True,False(default) , sum multiple files

    det_cal_file= a valid detector block file and path or a raw file. Setting this
                     will use the detector calibration from the specified file NOT the
                     input raw file
    mask_run = RunNumber to use for diag instead of the input run number

    one2one =True, False :Reduction will not use a mapping file

    hardmaskPlus=Filename :load a hardmarkfile and apply together with diag mask

    hardmaskOnly=Filename :load a hardmask and use as only mask

    use_sam_msk_on_monovan=False This will set the total mask to be that of the sample run

    abs_units_van_range=[-40,40] integral range for absolute vanadium data

    mono_correction_factor=float User specified correction factor for absolute units normalisation
    """

    kwargs['monovan_mapfile']    = monovan_mapfile
    kwargs['wb_for_monovanadium']= wb_for_monovanadium
    kwargs['sample_mass']        = samp_mass
    kwargs['sample_rmm']         = samp_rmm

    # service property, which tells arb_units that here defaults have changed and no need to check they are changed
    kwargs['_defaults_have_changed']  = True
    try:
        n,r=funcreturns.lhs_info('both')
        wksp_out=r[0]

    except:
        if sample_run == 0:
            #deal with the current run being parsed as 0 rather than 00000
            sample_run='00000'
        wksp_out=Reducer.instr_name+str(sample_run)+'.spe'
        if kwargs.has_key('sum') and kwargs.get('sum')==True:
            wksp_out=inst_name+str(sample_run[0])+'sum'+'.spe'


    results_name = wksp_out
    wksp_out = arb_units(wb_for_run,sample_run,ei_guess,rebin,map_file,monovan_run,**kwargs)


    if  results_name != wksp_out.getName():
        RenameWorkspace(InputWorkspace=wksp_out,OutputWorkspace=results_name)

    return wksp_out



def apply_absolute_normalization(Reducer,deltaE_wkspace_sample,monovan_run,ei_guess,wb_mono):
    """  Function applies absolute normalization factor to the target workspace
         and calculates this factor if necessary

         Inputs:
         Reducer           --    properly initialized class which performs reduction
         deltaE_wkspace_sample-- the workspace which should be modified
         monovan_run          -- run number for monochromatic vanadium sample at current energy
         ei_guess             -- estimated neutrons incident energy
         wb_mono              -- white bean vanadium run number.
    """
    if Reducer.mono_correction_factor != None :
         absnorm_factor=float(Reducer.mono_correction_factor)
         Reducer.log('##### Using supplied workspace correction factor                          ######')
         Reducer.log('      Value : '+str(absnorm_factor))

    else:
        Reducer.log('##### Evaluate the integral from the monovan run and calculate the correction factor ######')
        Reducer.log('      Using absolute units vanadium integration range : '+str(Reducer.monovan_integr_range))
        #
        result_ws_name = common.create_resultname(monovan_run)
        # check the case when the sample is monovan itself (for testing purposes)
        if result_ws_name == deltaE_wkspace_sample.name() :
            deltaE_wkspace_monovan = CloneWorkspace(InputWorkspace=deltaE_wkspace_sample,OutputWorkspace=result_ws_name+'-monovan');
            deltaE_wkspace_monovan=Reducer.remap(deltaE_wkspace_monovan,None,Reducer.monovan_mapfile)
        else:
            # convert to monovan to energy
            map_file            = Reducer.map_file;
            Reducer.map_file    = Reducer.monovan_mapfile;
            deltaE_wkspace_monovan = Reducer.convert_to_energy(monovan_run, ei_guess, wb_mono)
            Reducer.map_file = map_file

        ei_monovan = deltaE_wkspace_monovan.getRun().getLogData("Ei").value
        Reducer.log('      Incident energy found for monovanadium run: '+str(ei_monovan)+' meV')


        (absnorm_factorL,absnorm_factorSS,absnorm_factorP,absnorm_factTGP) = get_abs_normalization_factor(Reducer,deltaE_wkspace_monovan.getName(),ei_monovan)

        Reducer.log('Absolute correction factor S^2: {0:10.4f} Libisis: {1:10.4f} Puasonian: {2:10.4f}  TGP: {3:10.4f} '.format(absnorm_factorSS,absnorm_factorL,absnorm_factorP,absnorm_factTGP))
        absnorm_factor = absnorm_factTGP;


    deltaE_wkspace_sample = deltaE_wkspace_sample/absnorm_factor;


    return deltaE_wkspace_sample


def process_legacy_parameters(**kwargs) :
    """ The method to deal with old parameters which have logi c different from default and easy to process using
        subprogram. All other parameters just copiet to output
    """
    params = dict();
    for key,value in kwargs.iteritems():
        if key == 'hardmaskOnly': # legacy key defines other mask file here
            params["hard_mask_file"] = value;
            params["use_hard_mask_only"] = True;
        elif key == 'hardmaskPlus': # legacy key defines other mask file here
            params["hard_mask_file"] = value;
            params["use_hard_mask_only"] = False;
        else:
            params[key]=value;

    # Check all possible ways to define hard mask file:
    if 'hard_mask_file' in params and not params['hard_mask_file'] is None:
        if type(params['hard_mask_file']) == str and params['hard_mask_file']=="None":
            params['hard_mask_file'] = None;
        elif type(params['hard_mask_file']) == bool:
           if  params['hard_mask_file']:
               raise  TypeError("hard_mask_file has to be a file name or None. It can not be boolean True")
           else:
               params['hard_mask_file'] = None;
        elif len(params['hard_mask_file']) == 0:
             params['hard_mask_file'] = None;


    return params


def get_abs_normalization_factor(Reducer,deltaE_wkspaceName,ei_monovan) :
    """get absolute normalization factor for monochromatic vanadium

    Inputs:
    @param: deltaE_wkspace  -- the name (string) of monovan workspace, converted to energy
    @param: min             -- the string representing the minimal energy to integrate the spectra
    @param: max             -- the string representing the maximal energy to integrate the spectra

    @returns the value of monovan absolute normalization factor.
             deletes monovan workspace (deltaE_wkspace) if abs norm factor was calculated successfully

    Detailed explanation:
     The algorithm takes the monochromatic vanadium workspace normalized by WB vanadium and calculates
     average modified monochromatic vanadium (MV) integral considering that the efficiency of detectors
     are different and accounted for by dividing each MV value by corresponding WBV value,
     the signal on a detector has poison distribution and the error for a detector is the square
     root of correspondent signal on a detector.
     Error for WBV considered negligebly small wrt the error on MV
    """

    van_mass=Reducer.van_mass;
    min  = Reducer.monovan_integr_range[0];
    max  = Reducer.monovan_integr_range[1];

    data_ws=Integration(InputWorkspace=deltaE_wkspaceName,OutputWorkspace='van_int',RangeLower=min,RangeUpper=max,IncludePartialBins='1')
    input_ws = mtd[deltaE_wkspaceName]


    nhist = data_ws.getNumberHistograms()
   #print nhist

    signal1_sum = 0.0
    weight1_sum = 0.0
    signal2_sum = 0.0
    weight2_sum = 0.0
    signal3_sum = 0.0
    weight3_sum = 0.0
    signal4_sum = 0.0
    weight4_sum = 0.0


    ic=0;
    izerc=0;
    for i in range(nhist):
        try:
            det = data_ws.getDetector(i)
        except Exception:
            continue
        if det.isMasked():
            continue

        signal = data_ws.readY(i)[0]
        error = data_ws.readE(i)[0]

        if signal != signal:     #ignore NaN
            continue
        if ((error<=0) or (signal<=0)):          # ignore Inf (0 in error are probably 0 in sign
            izerc+=1
            continue
        # Guess which minimizes the value sum(n_i-n)^2/Sigma_i -- this what Libisis had
        weight = 1.0/error
        signal1_sum += signal * weight
        weight1_sum += weight
        # Guess which minimizes the value sum(n_i-n)^2/Sigma_i^2
        weight2 = 1.0/(error*error)
        signal2_sum += signal * weight2
        weight2_sum += weight2
        # Guess which assumes puassonian distribution with Err=Sqrt(signal) and calculates
        # the function: N_avrg = 1/(DetEfficiency_avrg^-1)*sum(n_i*DetEfficiency_i^-1)
        # where the DetEfficiency = WB_signal_i/WB_average WB_signal_i is the White Beam Vanadium
        # signal on i-th detector and the WB_average -- average WB vanadium signal.
        # n_i is the modified signal
        err_sq      = error*error
        weight      = err_sq/signal
        signal3_sum += err_sq
        weight3_sum += weight
        # Guess which estimatnes value sum(n_i^2/Sigma_i^2)/sum(n_i/Sigma_i^2) TGP suggestion from 12-2012
        signal4_sum += signal*signal/err_sq
        weight4_sum += signal/err_sq

        ic += 1
        #print 'signal value =' ,signal
        #print 'error value =' ,error
        #print 'average ',signal_sum
   #---------------- Loop finished

    if( weight1_sum==0.0 or weight2_sum == 0.0 or weight3_sum == 0.0 or weight4_sum == 0.0) :
        print "WB integral has been calculated incorrectrly, look at van_int workspace in the input workspace: ",deltaE_wkspaceName
        raise IOError(" divided by 0 weight")

    integral_monovanLibISIS=signal1_sum / weight1_sum
    integral_monovanSigSq  =signal2_sum / weight2_sum
    integral_monovanPuason =signal3_sum / weight3_sum
    integral_monovanTGP    =signal4_sum / weight4_sum
    #integral_monovan=signal_sum /(wbVan_sum)
    van_multiplier = (float(Reducer.van_rmm)/float(van_mass))
    absnorm_factorLibISIS = integral_monovanLibISIS * van_multiplier
    absnorm_factorSigSq   = integral_monovanSigSq   * van_multiplier
    absnorm_factorPuason  = integral_monovanPuason  * van_multiplier
    absnorm_factorTGP     = integral_monovanTGP     * van_multiplier
    #print 'Monovan integral :' ,integral_monovan

    if ei_monovan >= 210.0:
        xsection = 421  # vanadium cross-section in mBarn/sR (402 mBarn/Sr) (!!!modified to fit high energy limit?!!!)
    else: # old textbook cross-section for vanadium for ei=20mEv
        xsection = 400 + (ei_monovan/10)

    absnorm_factorLibISIS /= xsection
    absnorm_factorSigSq  /= xsection
    absnorm_factorPuason /= xsection
    absnorm_factorTGP    /= xsection

    sample_multiplier = (float(Reducer.sample_mass)/float(Reducer.sample_rmm))
    absnorm_factorLibISIS= absnorm_factorLibISIS *sample_multiplier
    absnorm_factorSigSq  = absnorm_factorSigSq *sample_multiplier
    absnorm_factorPuason = absnorm_factorPuason *sample_multiplier
    absnorm_factorTGP    = absnorm_factorTGP *sample_multiplier

    # check for NaN
    if (absnorm_factorLibISIS !=absnorm_factorLibISIS)|(izerc!=0):    # It is an error, print diagnostics:
        if (absnorm_factorLibISIS !=absnorm_factorLibISIS):
            print '--------> Absolute normalization factor is NaN <----------------------------------------------'
        else:
            print '--------> Warning, Monovanadium has zero spectra <--------------------------------------------'
        print '--------> Processing workspace: ',deltaE_wkspaceName
        print '--------> Monovan Integration range : min=',min,' max=',max
        print '--------> Summarized: ',ic,' spectra with total value: ',signal2_sum, 'and total weight: ',weight2_sum
        print '--------> Dropped: ',izerc,' empty spectra'
        print '--------> Van multiplier: ',van_multiplier,'  sample multiplier: ',sample_multiplier, 'and xsection: ',xsection
        print '--------> Abs norm factors: LibISIS: ',absnorm_factorLibISIS,' Sigma^2: ',absnorm_factorSigSq
        print '--------> Abs norm factors: Puasonian: ',absnorm_factorPuason, ' TGP: ',absnorm_factorTGP
        print '----------------------------------------------------------------------------------------------'
    else:
        DeleteWorkspace(Workspace=deltaE_wkspaceName)
    DeleteWorkspace(Workspace=data_ws)
    return (absnorm_factorLibISIS,absnorm_factorSigSq,absnorm_factorPuason,absnorm_factorTGP)



def sum_files(inst_name, accumulator, files):
    """ Custom sum for multiple runs

        Left for compartibility as internal summation had some unspecified problems.
        Will go in a future
    """
    accum_name = accumulator
    if isinstance(accum_name,api.Workspace): # it is actually workspace
        accum_name  = accumulator.name()


    if type(files) == list:
         #tmp_suffix = '_plus_tmp'

         for filename in files:
              print 'Summing run ',filename,' to workspace ',accumulator
              temp = common.load_run(inst_name,filename, force=False,load_with_workspace=Reducer.load_monitors_with_workspace)

              if accum_name in mtd: # add current workspace to the existing one
                  if not isinstance(accumulator,api.Workspace):
                      accumulator = mtd[accum_name]
                  accumulator+=  temp
                  DeleteWorkspace(Workspace=temp)
              else:
                   print 'Create output workspace: '
                   accumulator=RenameWorkspace(InputWorkspace=temp,OutputWorkspace=accum_name)

         return accumulator
    else:
        temp = common.load_run(inst_name,files, force=False,load_with_workspace=Reducer.load_monitors_with_workspace)
        accumulator=RenameWorkspace(InputWorkspace=temp,OutputWorkspace=accum_name)
        return accumulator;




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






if __name__=="__main__":
    pass
 #     unittest.main()



    #help()
    #help("rubbish")

    #for attr in dir(Reducer):
    #  print "Reduce.%s = %s" % (attr, getattr(Reducer, attr))



