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


def arb_units(wb_run,sample_run,ei_guess,rebin,map_file=None,**kwargs):
    """
    arb_units(wb_run,sample_run,ei_guess,rebin,mapfile,**kwargs)
    
    arb_units(wb_run   Whitebeam run number or file name or workspace
           sample_run  sample run number or file name or workspace
           ei_guess    Ei guess
           rebin       Rebin parameters
           mapfile     Mapfile
           kwargs      Additional keyword arguments    
     with run numbers as input

    dgreduce.arb_units(1000,10001,80,'-10,.1,70','mari_res', additional keywords as required)
    
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

    # Deal with mandatory parameters
    print 'DGreduce run for ',Reducer.instr_name,'run number ',sample_run
    try:
        n,r=funcreturns.lhs_info('both')
        wksp_out=r[0]
    except:
        if sample_run == 0:
            #deal with the current run being parsed as 0 rather than 00000
            sample_run='00000'
        wksp_out=inst_name+str(sample_run)+'.spe'
        if kwargs.has_key('sum') and kwargs.get('sum')==True:
            wksp_out=inst_name+str(sample_run[0])+'sum'+'.spe'
        
    start_time=time.time()
    
    if sample_run=='00000' and mtd.doesExist(inst_name+'00000.raw')==True:
        print 'Deleteing previous instance of temp data'
        DeleteWorkspace(Workspace=inst_name+'00000.raw')


    # set rebinning range
    Reducer.energy_bins = rebin
    if float(str.split(rebin,',')[2])>=float(ei_guess):
        print 'Error: rebin max range {0} exceeds incident energy {1}'.format(str.split(rebin,',')[2],ei_guess)
        return

    # Process old legacy parameters which are easy to define in dgrecue then transfer through Mantid
    program_args = process_legacy_parameters(**kwargs)


    # set non-default reducers parameters and check if all optional keys provided as parameters are acceptable and have been defined in IDF 
    changed_Keys=Reducer.set_input_parameters(**program_args);


    # map file given in parameters overrides default map file
    if map_file != None :
        Reducer.map_file = map_file
        fileName, fileExtension = os.path.splitext(map_file)
        if (not fileExtension):
            map_file=map_file+'.map'    
        Reducer.map_file = map_file

    # defaults can be None too, but can be a file 
    if  Reducer.map_file == None:      
       print 'one2one map selected'       

    
    #process complex parameters
    if Reducer.mask_run == None :
        mask_run=sample_run
          
    if  Reducer.det_cal_file != None : 
        Reducer.relocate_dets = True
        print 'Setting detector calibration file to ',reducer.det_cal_file
    else:
        Reducer.relocate_dets = False
        print 'Setting detector calibration to detector block info from ', sample_run

    
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
 # Here we give control to the Reducer
    

    if Reducer.use_hard_mask_only: # if it is string, it is treated as bool
        totalmask = Reducer.hard_mask_file
        
        print 'Using hardmask from: ', totalmask
        #next stable version can replace this with loadmask algoritum
        specs=diag_load_mask(totalmask)
        CloneWorkspace(InputWorkspace=sample_run,OutputWorkspace='mask_wksp')
        MaskDetectors(Workspace='mask_wksp',SpectraList=specs)
        masking=mtd['mask_wksp']
    else:    
         masking = Reducer.diagnose(wb_run,sample = mask_run,
                                    second_white = None,variation=1.1,print_results=True)

    # provide masking workspace for the reducer              
    Reducer.spectra_masks=masking
    sumSp = SumSpectra(InputWorkspace=masking,OutputWorkspace="sumSpectra");

    nSpectra       = sumSp.getRun().getLogData("NumAllSpectra").value
    nMaskedSpectra = int(sumSp.readY(0)[0])
    
    print 'Diag processed workspace with {0:d} spectra and found {1:d} bad spectra'.format(nSpectra,nMaskedSpectra)
    DeleteWorkspace(sumSp);
    
    #Run the conversion
    deltaE_wkspace = Reducer.convert_to_energy(sample_run, ei_guess, wb_run)
    end_time=time.time()
    results_name=str(sample_run)+'.spe'
    
    ei= (deltaE_wkspace.getRun().getLogData("Ei").value)
    
    if mtd.doesExist('_wksp.spe-white')==True:
        DeleteWorkspace(Workspace='_wksp.spe-white')
    
    if mtd.doesExist(results_name)==False:
        RenameWorkspace(InputWorkspace=deltaE_wkspace,OutputWorkspace=results_name)
    
    print 'Incident energy found ',ei,' meV'
    print 'Elapsed time =',end_time-start_time, 's'
    #get the name that convert to energy will use
    
    
    RenameWorkspace(InputWorkspace=results_name,OutputWorkspace=wksp_out)
    
    return mtd[wksp_out]

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

def abs_units(wb_run,sample_run,mono_van,wb_mono,samp_rmm,samp_mass,ei_guess,rebin,map_file,monovan_mapfile,**kwargs):
    pass


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



