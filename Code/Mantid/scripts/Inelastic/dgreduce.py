from DirectEnergyConversion import *
import CommonFunctions as common
import time as time
import numpy
from mantid.simpleapi import *
from mantid.kernel import funcreturns
import unittest

# the class which is responsible for data reduction
global Reducer
Reducer = None

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


    if Reducer != None :
        if  Reducer.instr_name.upper()[0:2] == instname.upper()[0:2] :
            if not reload :
                return  # has been already defined

    Reducer = setup_reducer(instname)

def help(keyword=None) :
    """function returns help on reduction parameters. 
       
       Returns the list of the parameters availible if provided without arguments
       or the description and the default value for the key requested 
    """
    if Reducer == None:
        raise ValueError("Reducer has not been defined, call setup(instrument_name) first.")

    Reducer.help(keyword)


def arb_units(wb_run,sample_run,ei_guess,rebin,map_file=None,monovan_run=None,**kwargs):
    """
    arb_units(wb_run,sample_run,ei_guess,rebin,mapfile,**kwargs)
    
    arb_units(wb_run   Whitebeam run number or file name or workspace
           sample_run  sample run number or file name or workspace
           ei_guess    Ei guess
           rebin       Rebin parameters
           mapfile     Mapfile -- if absent/nonde the defaults from IDF will be used
           monovan_run If present will do the absolute units normalization. Number of additional parameters  
                       specified in **kwargs is usually requested for this. If they are absent, program uses defaults,
                       but the defaults (e.g. sample_mass or sample_rmm ) are usually incorrect for a particular run. 
           kwargs      Additional keyword arguments    

    with run numbers as input:
    dgreduce.arb_units(1000,10001,80,[-10,.1,70],'mari_res', additional keywords as required)
    
    dgreduce.arb_units(1000,10001,80,'-10,.1,70','mari_res',fixei=True)
    
    A detector calibration file must be specified if running the reduction with workspaces as input    
    namely:   
    w2=iliad("wb_wksp","run_wksp",ei,rebin_params,mapfile,det_cal_file=cal_file
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
    Reducer.log('DGreduce run for: '+Reducer.instr_name+' Run number: '+str(sample_run))

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
        Reducer.log('Deleteing previous instance of temp data')
        DeleteWorkspace(Workspace=inst_name+'00000.raw')


    # we may want to run absolute units normalization and this function has been called with monovan run or helper procedure
    abs_units_defaults_check = False
    if monovan_run != None :
       # check if mono-vanadium is provided as multiple files list or just put in brackets ocasionally
        Reducer.log(' Output will be in absolute units of mb/str/mev/fu')
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
    if Reducer.energy_bins[2] >= ei_guess:
        Reducer.log('Error: rebin max rebin range {0:f} exceeds incident energy {1:f}'.format(energy_bins[2],ei_guess),'Error')
        return

    # Process old legacy parameters which are easy to re-define in dgreduce rather then transfer through Mantid
    program_args = process_legacy_parameters(**kwargs)

    # set non-default reducers parameters and check if all optional keys provided as parameters are acceptable and have been defined in IDF 
    changed_Keys=Reducer.set_input_parameters(**program_args);

    # inform user about changed parameters
    for key in changed_Keys:
        val = getattr(Reducer,key);
        Reducer.log("  Value of : {0:<25} is set to : {1:<20} ".format(key,val))


    #do we run absolute units normalization and need to warn users if the parameters needed for that have not changed from defaults
    if abs_units_defaults_check :
        Reducer.check_abs_norm_defaults_changed(changed_Keys);


    # map file given in parameters overrides default map file
    if map_file != None :
        Reducer.map_file = map_file

    # defaults can be None too, but can be a file 
    if  Reducer.map_file == None:      
        Reducer.log('one2one map selected')

    
    #process complex parameters
    if Reducer.mask_run == None :
        mask_run=sample_run
          
    if  Reducer.det_cal_file != None : 
        Reducer.relocate_dets = True
        Reducer.log('Setting detector calibration file to '+reducer.det_cal_file)
    else:
        Reducer.relocate_dets = False
        Reducer.log('Setting detector calibration to detector block info from '+str(sample_run))

    
    if mtd.doesExist(str(sample_run))==True and Reducer.det_cal_file != None:
        print 'For data input type: workspace detector calibration must be specified'
        print 'use Keyword det_cal_file with a valid detctor file or run number'
        return
              
    
    print 'Output will be normalised to', Reducer.normalise_method
    if (numpy.size(sample_run)) > 1 and Reducer.sum_runs:
        #this sums the runs together before passing the summed file to the rest of the reduction
        #this circumvents the inbuilt method of summing which fails to sum the files for diag
        
        sumfilename=str(sample_run[0])+'sum'
        accum=sum_files(sumfilename, sample_run)
        #the D.E.C. tries to be too clever so we have to fool it into thinking the raw file is already exists as a workpsace
        RenameWorkspace(InputWorkspace=accum,OutputWorkspace=inst_name+str(sample_run[0])+'.raw')
        sample_run=sample_run[0]

#-------------------------------------------------------------------------------------------------------------------------------------------------------
#  Here we give control to the Reducer
# --------------------------------------------------------------------------------------------------------    
    # diag the sample and detector vanadium
    if Reducer.use_hard_mask_only: # if it is string, it is treated as bool
        totalmask = Reducer.hard_mask
        
        Reducer.log(' Using hardmask only from: '+totalmask)
        #Return masking workspace
        masking = LoadMask(Instrument=Reducer.instr_name,InputFile=Reducer.hard_mask)
        mask_workspace(wb_run,masking)
        mask_workspace(sample_run,masking)
    else:  
         masking = Reducer.diagnose(wb_run,sample = mask_run,
                                    second_white = None,variation=1.1,print_results=True)

   # Calculate absolute units:    
    if monovan_run != None and Reducer.mono_correction_factor == None :

        if Reducer.use_sam_msk_on_monovan == True or Reducer.use_hard_mask_only:
            Reducer.log('  Applying sample run mask to mono van ')
            mask_workspace(monovan_run,masking)
            if wb_for_monovanadium != wb_run:
                mask_workspace(monovan_run,masking)
        else:
             print '########### Run diagnose for monochromatic vanadium run ##############'

             masking2 = Reducer.diagnose(wb_for_monovanadium,sample=monovan_run,
                                         second_white = None,variation=1.1,print_results=True)
                  
             masking=masking+masking2               
    
    # end monodvan diagnosis    
    Reducer.spectra_masks=masking              
  
    # estimate and report the number of failing detectors
    failed_sp_list,nSpectra = get_failed_spectra_list_from_masks(masking)
    nMaskedSpectra = len(failed_sp_list)
    print 'Diag processed workspace with {0:d} spectra and found {1:d} bad spectra'.format(nSpectra,nMaskedSpectra)

  
  
    
    #Run the conversion first on the sample
    deltaE_wkspace_sample = Reducer.convert_to_energy(sample_run, ei_guess, wb_run)

    # calculate absolute units integral and apply it to the workspace
    if monovan_run != None or Reducer.mono_correction_factor != None :
        deltaE_wkspace_sample = apply_absolute_normalization(Reducer,deltaE_wkspace_sample,monovan_run,ei_guess,wb_run)


    results_name = deltaE_wkspace_sample.name();
    if results_name != wksp_out:
       RenameWorkspace(InputWorkspace=results_name,OutputWorkspace=wksp_out)


    ei= (deltaE_wkspace_sample.getRun().getLogData("Ei").value) 
    print 'Incident energy found for sample run: ',ei,' meV'
    
    end_time=time.time()
    print 'Elapsed time =',end_time-start_time, 's'

    if mtd.doesExist('_wksp.spe-white')==True:
        DeleteWorkspace(Workspace='_wksp.spe-white')

    
    return deltaE_wkspace_sample


def mask_workspace(run_number,mask_ws) :
    """
    mask workspace defined by the run number with masks defined in masking workspace
    """
    run_ws=common.load_run(run_number)

    #if Reducer.map_file != None:
    #    run_ws=GroupDetectors(InputWorkspace=run_ws,OutputWorkspace=run_ws,
    #                          MapFile= Reducer.map_file, KeepUngroupedSpectra=0, Behaviour='Average')

    MaskDetectors(Workspace=run_ws, MaskedWorkspace=mask_ws)

def abs_units(wb_for_run,sample_run,monovan_run,wb_for_monovanadium,samp_rmm,samp_mass,ei_guess,rebin,map_file,monovan_mapfile,**kwargs):
    """     
    dgreduce.abs_units(wb_run          Whitebeam run number or file name or workspace
                  sample_run          Sample run run number or file name or workspace       
                  monovan_run          Monochromatic run run number or file name or workspace
                  wb_mono          White beam for Monochromatic run run number or file name or workspace
                  samp_rmm          Mass of forumula unit of sample
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
    hard_mask       - A file specifying those spectra that should be masked without testing (default=None)
    tiny            - Minimum threshold for acceptance (default = 1e-10)
    large           - Maximum threshold for acceptance (default = 1e10)
    bkgd_range      - A list of two numbers indicating the background range (default=instrument defaults)
    diag_van_median_rate_limit_lo   - Lower bound defining outliers as fraction of median value (default = 0.01)
    diag_van_median_rate_limit_hi   - Upper bound defining outliers as fraction of median value (default = 100.)
    diag_van_median_sigma_lo        - Fraction of median to consider counting low for the white beam diag (default = 0.1)
    diag_van_median_sigma_hi        - Fraction of median to consider counting high for the white beam diag (default = 1.5)
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
         and calculates this factor if nececcary

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
        # TODO: !!!! 
        if Reducer.monovan_integr_range is None: # integration in the range relative to incident energy
            Reducer.monovan_integr_range = [Reducer.monovan_lo_frac*ei_guess,Reducer.monovan_hi_frac*ei_guess]
        Reducer.log('##### Evaluate the integral from the monovan run and calculate the correction factor ######')
        Reducer.log('      Using absolute units vanadion integration range : '+str(Reducer.monovan_integr_range))
       #now on the mono_vanadium run swap the mapping file
        map_file            = Reducer.map_file;
        Reducer.map_file    = Reducer.monovan_mapfile;
        Reducer.save_format = None;
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
    """ The method to deal with old parameters which have logic different from default and easy to process using 
        subprogram. All other parameters just copiet to output 
    """
    params = dict();
    for key,value in kwargs.iteritems():
        if key == 'hardmaskOnly': # legacy key defines other mask file here
            params["hard_mask_file"] = value;
            params["use_hard_mask_only"] = True;
        else:
            params[key]=value;    

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
        print "WB integral has been calculated incorrectrly, look at van_int workspace and input workspace: ",deltaE_wkspaceName
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



def sum_files(accumulator, files):
    if type(files) == list:
         tmp_suffix = '_plus_tmp'

         for filename in files:
              print 'Summing run ',filename,' to workspace ',accumulator
              temp = common.load_run(filename, force=False)
              if mtd.workspaceExists(accumulator)==False:
                   #check for existance of output workpsace if false clone and zero
                   print 'create output file'
                   CloneWorkspace(temp,accumulator)
                   CreateSingleValuedWorkspace(OutputWorkspace="tmp",DataValue="0",ErrorValue="0")
                   Multiply(LHSWorkspace=accumulator,RHSWorkspace="tmp",OutputWorkspace=accumulator)
              Plus(accumulator, temp, accumulator)
         return accumulator



def get_failed_spectra_list(diag_workspace):
    """Compile a list of spectra numbers that are marked as
    masked in the given workspace

    Input:
     diag_workspace  -  A workspace containing masking
    """
    if type(diag_workspace) == str:
        diag_workspace = mtd[diag_workspace]
    
    failed_spectra = []
    for i in range(diag_workspace.getNumberHistograms()):
      try:
        det = diag_workspace.getDetector(i)
      except RuntimeError:
        continue
  
    if det.isMasked():
       spectrum = diag_workspace.getSpectrum(i)
       failed_spectra.append(spectrum.getSpectrumNo())

    return failed_spectra

def get_failed_spectra_list_from_masks(masking_wksp):
    """Compile a list of spectra numbers that are marked as
       masked in the masking workspace

    Input:
     masking_workspace -  A special masking workspace containing masking data
    """
    if type(masking_wksp) == str:
        masking_wksp = mtd[masking_wksp]
    
    failed_spectra = []
    n_spectra = masking_wksp.getNumberHistograms()
    for i in xrange(n_spectra):
        if masking_wksp.readY(i)[0] >0.99 : # spectrum is masked
            failed_spectra.append(masking_wksp.getSpectrum(i).getSpectrumNo())

    return (failed_spectra,n_spectra)



class DgreduceTest(unittest.TestCase):
    def __init__(self, methodName):
        setup("MAPS")
        return super(DgreduceTest, self).__init__(methodName)

    def setUp(self):
        pass

    def tearDown(self):
        pass

    def test_run_help(self):
        self.assertRaises(ValueError,help,'rubbish')
        help("monovan_lo_bound")
    def test_process_legacy_parameters(self):
        kw=dict();
        kw["hardmaskOnly"]="someFileName"
        kw["someKeyword"] ="aaaa"
        params = process_legacy_parameters(**kw);
        self.assertEqual(len(params),3)
        self.assertTrue("someKeyword" in params);
        self.assertTrue("hard_mask_file" in params);
        self.assertTrue("use_hard_mask_only" in params)



if __name__=="__main__":
    unittest.main()



    #help()
    #help("rubbish")

    #for attr in dir(Reducer):
    #  print "Reduce.%s = %s" % (attr, getattr(Reducer, attr))



