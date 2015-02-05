""" Empty class temporary left for compatibility with previous interfaces """
import DirectEnergyConversion as DRC
import CommonFunctions as common
import time as time
from mantid.simpleapi import *
from mantid import api
from mantid.kernel import funcreturns


# the class which is responsible for data reduction
global Reducer
Reducer = None

# Statement used at debug time to pull changes in DirectEnergyConversion into Mantid
#DRC=reload(DRC)
def getReducer():
    # needed on Linux to adhere to correct reference return
    global Reducer
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
        old_name=Reducer.prop_man.instr_name
        if  old_name.upper()[0:3] == instname.upper()[0:3] :
            if not reload :
               return  # has been already defined

    Reducer = DRC.setup_reducer(instname,reload)


def arb_units(wb_run,sample_run,ei_guess,rebin,map_file='default',monovan_run=None,second_wb=None,**kwargs):
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
    if sample_run:
        Reducer.sample_run = sample_run
    try:
         n,r=funcreturns.lhs_info('both')
         wksp_out=r[0]
    except:
         wksp_out = Reducer.prop_man.get_sample_ws_name()
    #
    res = Reducer.convert_to_energy(wb_run,sample_run,ei_guess,rebin,map_file,monovan_run,second_wb,**kwargs)
    #
    results_name = res.name()
    if results_name != wksp_out:
        RenameWorkspace(InputWorkspace=results_name,OutputWorkspace=wksp_out)


    return res



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
    bleed_maxrate   - If the bleed test is on then this is the maximum frame rate allowed in a tube
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

    mono_correction_factor=float User specified correction factor for absolute units normalization
    """

    kwargs['monovan_mapfile']    = monovan_mapfile
    kwargs['sample_mass']        = samp_mass
    kwargs['sample_rmm']         = samp_rmm

    if sample_run:
        Reducer.sample_run = sample_run
    try:
        n,r=funcreturns.lhs_info('both')
        results_name=r[0]

    except:
        results_name = Reducer.prop_man.get_sample_ws_name()
    if wb_for_run == wb_for_monovanadium: # wb_for_monovanadium property does not accept duplicated workspace
        wb_for_monovanadium = None        # if this value is none, it is constructed to be equal to wb_for_run

    wksp_out = arb_units(wb_for_run,sample_run,ei_guess,rebin,map_file,monovan_run,wb_for_monovanadium,**kwargs)


    if  results_name != wksp_out.getName():
        RenameWorkspace(InputWorkspace=wksp_out,OutputWorkspace=results_name)

    return wksp_out




if __name__=="__main__":
    pass
 #     unittest.main()



    #help()
    #help("rubbish")

    #for attr in dir(Reducer):
    #  print "Reduce.%s = %s" % (attr, getattr(Reducer, attr))



