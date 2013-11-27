''' SVN Info:      The variables below will only get subsituted at svn checkout if
        the repository is configured for variable subsitution. 

    $Id$
    $HeadURL$
|=============================================================================|=======|    
1                                                                            80   <tab>
'''
#these need to be moved into one NR folder or so
#from ReflectometerCors import *
from l2q import *
from combineMulti import *
from mantid.simpleapi import *  # New API
from mantid.api import WorkspaceGroup
from mantid.kernel import logger
from convert_to_wavelength import ConvertToWavelength
import math
import re
import abc

class CorrectionStrategy(object):
    __metaclass__ = abc.ABCMeta # Mark as an abstract class
    
    @abc.abstractmethod
    def apply(self, to_correct):
        pass
    
class ExponentialCorrectionStrategy(CorrectionStrategy):

    def __init__(self, c0, c1):
        self.__c0 = c0
        self.__c1 = c1
        
    def apply(self, to_correct):
        logger.information("Exponential Correction")
        corrected = ExponentialCorrection(InputWorkspace=to_correct,C0=self.__c0, C1= self.__c1, Operation='Divide')
        return corrected
    
class PolynomialCorrectionStrategy(CorrectionStrategy):
    def __init__(self, poly_string):
        self.__poly_string = poly_string
    
    def apply(self, to_correct):
        logger.information("Polynomial Correction")
        corrected = PolynomialCorrection(InputWorkspace=to_correct, Coefficients=self.__poly_string, Operation='Divide')
        return corrected
       
class NullCorrectionStrategy(CorrectionStrategy):
    def apply(self, to_correct):
        logger.information("Null Correction")
        out = to_correct.clone()
        return out
        

def quick(run, theta=0, pointdet=True,roi=[0,0], db=[0,0], trans='', polcorr=0, usemon=-1,outputType='pd', debug=False):
    '''
    Original quick parameters fetched from IDF
    '''
    run_ws = ConvertToWavelength.to_workspace(run)
    idf_defaults = get_defaults(run_ws)
    
    i0_monitor_index = idf_defaults['I0MonitorIndex']
    multi_detector_start = idf_defaults['MultiDetectorStart']
    lambda_min = idf_defaults['LambdaMin']
    lambda_max = idf_defaults['LambdaMax']
    point_detector_start = idf_defaults['PointDetectorStart']
    point_detector_stop =  idf_defaults['PointDetectorStop']
    multi_detector_start = idf_defaults['MultiDetectorStart']
    background_min = idf_defaults['MonitorBackgroundMin']
    background_max = idf_defaults['MonitorBackgroundMax']
    int_min = idf_defaults['MonitorIntegralMin']
    int_max = idf_defaults['MonitorIntegralMax']
    correction_strategy = idf_defaults['AlgoritmicCorrection']
    
    return quick_explicit(run=run, i0_monitor_index = i0_monitor_index, lambda_min = lambda_min, lambda_max = lambda_max, 
                   point_detector_start = point_detector_start, point_detector_stop = point_detector_stop, 
                   multi_detector_start = multi_detector_start, background_min = background_min, background_max = background_max, 
                   int_min = int_min, int_max = int_max, theta = theta, pointdet = pointdet, roi = roi, db = db, trans = trans, 
                   debug = debug, correction_strategy = correction_strategy )
    
    
def quick_explicit(run, i0_monitor_index, lambda_min, lambda_max,  background_min, background_max, int_min, int_max,
                   point_detector_start=0, point_detector_stop=0, multi_detector_start=0, theta=0, pointdet=True,roi=[0,0], db=[0,0], trans='', debug=False, correction_strategy=NullCorrectionStrategy):
    '''
    Version of quick where all parameters are explicitly provided.
    '''

    run_ws = ConvertToWavelength.to_workspace(run)
    to_lam = ConvertToWavelength(run_ws)
    nHist = run_ws.getNumberHistograms()
    
    if pointdet:
        detector_index_ranges = (point_detector_start, point_detector_stop)
    else:
        detector_index_ranges = (multi_detector_start, nHist-1)
    
    
    _monitor_ws, _detector_ws = to_lam.convert(wavelength_min=lambda_min, wavelength_max=lambda_max, detector_workspace_indexes=detector_index_ranges, monitor_workspace_index=i0_monitor_index, correct_monitor=True, bg_min=background_min, bg_max=background_max )

    inst = run_ws.getInstrument()
    # Some beamline constants from IDF
   
    print i0_monitor_index
    print nHist
    if not pointdet:
        # Proccess Multi-Detector; assume MD goes to the end:
        # if roi or db are given in the function then sum over the apropriate channels
        print "This is a multidetector run."
        try:
            _I0M = RebinToWorkspace(WorkspaceToRebin=_monitor_ws,WorkspaceToMatch=_detector_ws)
            IvsLam = _detector_ws / _I0M
            if (roi != [0,0]) :
                ReflectedBeam = SumSpectra(InputWorkspace=IvsLam, StartWorkspaceIndex=roi[0], EndWorkspaceIndex=roi[1])
            if (db != [0,0]) :
                DirectBeam = SumSpectra(InputWorkspace=_detector_ws, StartWorkspaceIndex=db[0], EndWorkspaceIndex=db[1])
        except SystemExit:
            print "Point-Detector only run."
        RunNumber = groupGet(IvsLam.getName(),'samp','run_number')
        if (theta):
            IvsQ = l2q(ReflectedBeam, 'linear-detector', theta) # TODO: possible to get here and an invalid state if roi == [0,0] see above.
        else:
            IvsQ = ConvertUnits(InputWorkspace=ReflectedBeam, Target="MomentumTransfer")
                
    # Single Detector processing-------------------------------------------------------------
    else:
        print "This is a Point-Detector run."
        # handle transmission runs
        # process the point detector reflectivity  
        _I0P = RebinToWorkspace(WorkspaceToRebin=_monitor_ws,WorkspaceToMatch=_detector_ws)
        IvsLam = Scale(InputWorkspace=_detector_ws,Factor=1)
        #  Normalise by good frames
        GoodFrames = groupGet(IvsLam.getName(),'samp','goodfrm')
        print "run frames: ", GoodFrames
        if (run=='0'):
            RunNumber = '0'
        else:
            RunNumber = groupGet(IvsLam.getName(),'samp','run_number') 
        if (trans==''):  
            print "No transmission file. Trying default exponential/polynomial correction..."
            IvsLam = correction_strategy.apply(_detector_ws)
            IvsLam = Divide(LHSWorkspace=IvsLam, RHSWorkspace=_I0P)
        else: # we have a transmission run
            _monInt = Integration(InputWorkspace=_I0P,RangeLower=int_min,RangeUpper=int_max)
            IvsLam = Divide(LHSWorkspace=_detector_ws,RHSWorkspace=_monInt)
            names = mtd.getObjectNames()
            if trans in names:
                trans = RebinToWorkspace(WorkspaceToRebin=trans,WorkspaceToMatch=IvsLam,OutputWorkspace=trans)
                IvsLam = Divide(LHSWorkspace=IvsLam,RHSWorkspace=trans,OutputWorkspace="IvsLam") # TODO: Hardcoded names are bad
            else:
                IvsLam = transCorr(trans, IvsLam, lambda_min, lambda_max, background_min, background_max, 
                                   int_min, int_max, detector_index_ranges, i0_monitor_index)
                print type(IvsLam)
                RenameWorkspace(InputWorkspace=IvsLam, OutputWorkspace="IvsLam") # TODO: Hardcoded names are bad
                
        
        # Convert to I vs Q
        # check if detector in direct beam
        if (theta == 0 or theta == ''):
            if (theta == ''):
                theta = 0
            print "given theta = ",theta
            inst = groupGet('IvsLam','inst')
            detLocation=inst.getComponentByName('point-detector').getPos()
            sampleLocation=inst.getComponentByName('some-surface-holder').getPos()
            detLocation=inst.getComponentByName('point-detector').getPos()
            sample2detector=detLocation-sampleLocation    # metres
            source=inst.getSource()
            beamPos = sampleLocation - source.getPos()
            PI = 3.1415926535
            theta = inst.getComponentByName('point-detector').getTwoTheta(sampleLocation, beamPos)*180.0/PI/2.0
            print "Det location: ", detLocation, "Calculated theta = ",theta
            if detLocation.getY() == 0:  # detector is not in correct place
                print "det at 0"
                # Load corresponding NeXuS file
                runno = '_' + str(run)
                #templist.append(runno)
                if type(run)==type(int()):
                    LoadNexus(Filename=run,OutputWorkspace=runno)
                else:
                    LoadNexus(Filename=run.replace("raw","nxs",1),OutputWorkspace=runno)
                # Get detector angle theta from NeXuS
                theta = groupGet(runno,'samp','theta')
                print 'Nexus file theta =', theta
                IvsQ = l2q(mtd['IvsLam'], 'point-detector', theta)
            else:
                ConvertUnits(InputWorkspace='IvsLam',OutputWorkspace="IvsQ",Target="MomentumTransfer")
            
        else:
            theta = float(theta)
            IvsQ = l2q(mtd['IvsLam'], 'point-detector', theta)        
    
    RenameWorkspace(InputWorkspace='IvsLam',OutputWorkspace=RunNumber+'_IvsLam')
    RenameWorkspace(InputWorkspace='IvsQ',OutputWorkspace=RunNumber+'_IvsQ')
        
    # delete all temporary workspaces unless in debug mode (debug=1)
    
    if debug == 0:
        pass
        cleanup()
    return  mtd[RunNumber+'_IvsLam'], mtd[RunNumber+'_IvsQ'], theta



def transCorr(transrun, i_vs_lam, lambda_min, lambda_max, background_min, background_max, int_min, int_max, detector_index_ranges, i0_monitor_index):
    """
    Perform transmission corrections on i_vs_lam.
    return the corrected result.
    """
    
    transWS = None
    if ',' in transrun:
        slam = transrun.split(',')[0]
        llam = transrun.split(',')[1]
        print "Transmission runs: ", transrun
        
        to_lam = ConvertToWavelength(slam)
        _monitor_ws_slam, _detector_ws_slam = to_lam.convert(wavelength_min=lambda_min, wavelength_max=lambda_max, detector_workspace_indexes=detector_index_ranges, monitor_workspace_index=i0_monitor_index, correct_monitor=True, bg_min=background_min, bg_max=background_max )
        
        _i0p_slam = RebinToWorkspace(WorkspaceToRebin=_monitor_ws_slam, WorkspaceToMatch=_detector_ws_slam)
        _mon_int_trans = Integration(InputWorkspace=_i0p_slam, RangeLower=int_min, RangeUpper=int_max)
        _detector_ws_slam = Divide(LHSWorkspace=_detector_ws_slam, RHSWorkspace=_mon_int_trans)
        
        to_lam = ConvertToWavelength(llam)
        _monitor_ws_llam, _detector_ws_llam = to_lam.convert(wavelength_min=lambda_min, wavelength_max=lambda_max, detector_workspace_indexes=detector_index_ranges, monitor_workspace_index=i0_monitor_index, correct_monitor=True, bg_min=background_min, bg_max=background_max )
        
        _i0p_llam = RebinToWorkspace(WorkspaceToRebin=_monitor_ws_llam, WorkspaceToMatch=_detector_ws_llam)
        _mon_int_trans = Integration(InputWorkspace=_i0p_llam, RangeLower=int_min,RangeUpper=int_max)
        _detector_ws_llam = Divide(LHSWorkspace=_detector_ws_llam, RHSWorkspace=_mon_int_trans)
        
        # TODO: HARDCODED STITCHING VALUES!!!!!
        _transWS, outputScaling = Stitch1D(LHSWorkspace=_detector_ws_slam, RHSWorkspace=_detector_ws_llam, StartOverlap=10, EndOverlap=12,  Params="%f,%f,%f" % (1.5, 0.02, 17))

    else:
        
        to_lam = ConvertToWavelength(transrun)
        _monitor_ws_trans, _detector_ws_trans = to_lam.convert(wavelength_min=lambda_min, wavelength_max=lambda_max, detector_workspace_indexes=detector_index_ranges, monitor_workspace_index=i0_monitor_index, correct_monitor=True, bg_min=background_min, bg_max=background_max )
        _i0p_trans = RebinToWorkspace(WorkspaceToRebin=_monitor_ws_trans, WorkspaceToMatch=_detector_ws_trans)

        _mon_int_trans = Integration( InputWorkspace=_i0p_trans, RangeLower=int_min, RangeUpper=int_max )
        _transWS = Divide( LHSWorkspace=_detector_ws_trans, RHSWorkspace=_mon_int_trans )
    
   
    #got sometimes very slight binning diferences, so do this again:
    _i_vs_lam_trans = RebinToWorkspace(WorkspaceToRebin=_transWS, WorkspaceToMatch=i_vs_lam)
    # Normalise by transmission run.    
    _i_vs_lam_corrected = i_vs_lam / _i_vs_lam_trans
    
    return _i_vs_lam_corrected


def cleanup():
    names = mtd.getObjectNames()
    for name in names:
        if re.search("^_", name):
            DeleteWorkspace(name)
    
def get_defaults(run_ws):
    '''
    Fetch out instrument level defaults.
    '''
    defaults = dict()
    if isinstance(run_ws, WorkspaceGroup):
        instrument = run_ws[0].getInstrument()
    else:
        instrument = run_ws.getInstrument()    
    defaults['LambdaMin'] = float( instrument.getNumberParameter('LambdaMin')[0] )
    defaults['LambdaMax'] = float( instrument.getNumberParameter('LambdaMax')[0] )
    defaults['MonitorBackgroundMin'] =  float( instrument.getNumberParameter('MonitorBackgroundMin')[0] )
    defaults['MonitorBackgroundMax'] =float( instrument.getNumberParameter('MonitorBackgroundMax')[0] ) 
    defaults['MonitorIntegralMin'] =  float( instrument.getNumberParameter('MonitorIntegralMin')[0] )
    defaults['MonitorIntegralMax'] = float( instrument.getNumberParameter('MonitorIntegralMax')[0] )
    defaults['MonitorsToCorrect'] = int( instrument.getNumberParameter('MonitorsToCorrect')[0] )
    defaults['PointDetectorStart'] =  int( instrument.getNumberParameter('PointDetectorStart')[0] )
    defaults['PointDetectorStop'] =  int( instrument.getNumberParameter('PointDetectorStop')[0] )
    defaults['MultiDetectorStart'] = int( instrument.getNumberParameter('MultiDetectorStart')[0] )
    defaults['I0MonitorIndex'] = int( instrument.getNumberParameter('I0MonitorIndex')[0] ) 
    
    correction = NullCorrectionStrategy()
    corrType=instrument.getStringParameter('correction')[0]
    if corrType == 'polynomial':
        poly_string = instrument.getStringParameter('polystring')[0]
        correction = PolynomialCorrectionStrategy(poly_string)
    elif corrType == 'exponential':
        c0=instrument.getNumberParameter('C0')[0]
        c1=instrument.getNumberParameter('C1')[0]
        correction = ExponentialCorrectionStrategy(c0, c1)
        
    defaults['AlgoritmicCorrection'] = correction
    return defaults
    
def groupGet(wksp,whattoget,field=''):
    '''
    returns information about instrument or sample details for a given workspace wksp,
    also if the workspace is a group (info from first group element)
    '''
    if (whattoget == 'inst'):
        if isinstance(mtd[wksp], WorkspaceGroup):
            return mtd[wksp+'_1'].getInstrument()
        else:
            return mtd[wksp].getInstrument()
            
    elif (whattoget == 'samp' and field != ''):
        if isinstance(mtd[wksp], WorkspaceGroup):
            try:
                log = mtd[wksp + '_1'].getRun().getLogData(field).value
                if (type(log) is int or type(log) is str):
                    res=log
                else:
                    res = log[len(log)-1]
            except RuntimeError:
                res = 0
                print "Block "+field+" not found."            
        else:
            try:
                log = mtd[wksp].getRun().getLogData(field).value
                if (type(log) is int or type(log) is str):
                    res=log
                else:
                    res = log[len(log)-1]
            except RuntimeError:        
                res = 0
                print "Block "+field+" not found."
        return res
    elif (whattoget == 'wksp'):
        if isinstance(mtd[wksp], WorkspaceGroup):
            return mtd[wksp+'_1'].getNumberHistograms()
        else:
            return mtd[wksp].getNumberHistograms()


""" =====================  Testing stuff =====================
    
    ToDo: 
        1) Make the test below more rigorous.
        2) Each test should either print out Success or Failure
        or describe in words what the tester should see on the screen.     
        3) Each function should be accompanied by a test function
        4) all test functions to be run in a give file should be called in doAllTests().
        The reason for this is that the normal python if __name__ == '__main__':  
        testing location is heavily used during script development and debugging. 

"""
    
def _testQuick():
    config['default.instrument'] = "SURF"
    [w1lam,w1q,th] = quick(94511,theta=0.25,trans='94504')
    [w2lam,w2q,th] = quick(94512,theta=0.65,trans='94504')
    [w3lam,w3q,th] = quick(94513,theta=1.5,trans='94504')
    g1=plotSpectrum("94511_IvsQ",0)
    g2=plotSpectrum("94512_IvsQ",0)
    g3=plotSpectrum("94513_IvsQ",0)
    
    return True

def  _doAllTests():
    _testQuick()
    return True


if __name__ == '__main__':
    ''' This is the debugging and testing area of the file.  The code below is run when ever the 
       script is called directly from a shell command line or the execute all menu option in mantid. 
    '''
    #Debugging = True  # Turn the debugging on and the testing code off
    Debugging = False # Turn the debugging off and the testing on
    
    if Debugging == False:
        _doAllTests()  
    else:    #Debugging code goes below
        rr = quick("N:/SRF93080.raw")
    

#  x=quick(95266)
