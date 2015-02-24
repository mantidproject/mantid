#pylint: disable=invalid-name
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

from mantid.api import WorkspaceGroup, MatrixWorkspace
from mantid.kernel import logger
from convert_to_wavelength import ConvertToWavelength
import math
import re
import abc

def enum(**enums):
    return type('Enum', (), enums)

PolarisationCorrection = enum(PNR=1, PA=2, NONE=3)

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
        _corrected = ExponentialCorrection(InputWorkspace=to_correct,C0=self.__c0, C1= self.__c1, Operation='Divide')
        return _corrected

class PolynomialCorrectionStrategy(CorrectionStrategy):
    def __init__(self, poly_string):
        self.__poly_string = poly_string

    def apply(self, to_correct):
        logger.information("Polynomial Correction")
        _corrected = PolynomialCorrection(InputWorkspace=to_correct, Coefficients=self.__poly_string, Operation='Divide')
        return _corrected

class NullCorrectionStrategy(CorrectionStrategy):
    def apply(self, to_correct):
        logger.information("Null Correction")
        _out = to_correct.clone()
        return _out


def quick(run, theta=None, pointdet=True,roi=[0,0], db=[0,0], trans='', polcorr=False, usemon=-1,outputType='pd',
          debug=False, stitch_start_overlap=10, stitch_end_overlap=12, stitch_params=[1.5, 0.02, 17],
          detector_component_name='point-detector', sample_component_name='some-surface-holder',
          correct_positions=True, tof_prefix="_"):
    '''
    Original quick parameters fetched from IDF
    '''
    run_ws = ConvertToWavelength.to_workspace(run, ws_prefix=tof_prefix)
    idf_defaults = get_defaults(run_ws, polcorr)

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
    crho = None
    calpha = None
    cAp = None
    cPp = None
    if polcorr and (polcorr != PolarisationCorrection.NONE):
        crho = idf_defaults['crho']
        calpha = idf_defaults['calpha']
        cAp = idf_defaults['cAp']
        cPp = idf_defaults['cPp']


    return quick_explicit(run=run, i0_monitor_index = i0_monitor_index, lambda_min = lambda_min, lambda_max = lambda_max,\
                   point_detector_start = point_detector_start, point_detector_stop = point_detector_stop,\
                   multi_detector_start = multi_detector_start, background_min = background_min, background_max = background_max,\
                   int_min = int_min, int_max = int_max, theta = theta, pointdet = pointdet, roi = roi, db = db, trans = trans,\
                   debug = debug, correction_strategy = correction_strategy, stitch_start_overlap=stitch_start_overlap,\
                   stitch_end_overlap=stitch_end_overlap, stitch_params=stitch_params, polcorr=polcorr, crho=crho, calpha=calpha, cAp=cAp, cPp=cPp,\
                   detector_component_name=detector_component_name, sample_component_name=sample_component_name, correct_positions=correct_positions)


def quick_explicit(run, i0_monitor_index, lambda_min, lambda_max,  background_min, background_max, int_min, int_max,
                   point_detector_start=0, point_detector_stop=0, multi_detector_start=0, theta=None,
                   pointdet=True,roi=[0,0], db=[0,0], trans='', debug=False, correction_strategy=NullCorrectionStrategy(),
                   stitch_start_overlap=None, stitch_end_overlap=None, stitch_params=None,
                   polcorr=False, crho=None, calpha=None, cAp=None, cPp=None, detector_component_name='point-detector',
                   sample_component_name='some-surface-holder', correct_positions=True ):

    '''
    Version of quick where all parameters are explicitly provided.
    '''

    _sample_ws = ConvertToWavelength.to_single_workspace(run)
    nHist =  _sample_ws.getNumberHistograms()
    to_lam = ConvertToWavelength(run)

    if pointdet:
        detector_index_ranges = (point_detector_start, point_detector_stop)
    else:
        detector_index_ranges = (multi_detector_start, nHist-1)


    _monitor_ws, _detector_ws = to_lam.convert(wavelength_min=lambda_min, wavelength_max=lambda_max, detector_workspace_indexes=detector_index_ranges, monitor_workspace_index=i0_monitor_index, correct_monitor=True, bg_min=background_min, bg_max=background_max )

    inst = _sample_ws.getInstrument()
    # Some beamline constants from IDF

    print i0_monitor_index
    print nHist

    if run=='0':
        RunNumber = '0'
    else:
        RunNumber = groupGet(_sample_ws.getName(),'samp','run_number')

    if not pointdet:
        # Proccess Multi-Detector; assume MD goes to the end:
        # if roi or db are given in the function then sum over the apropriate channels
        print "This is a multidetector run."

        _I0M = RebinToWorkspace(WorkspaceToRebin=_monitor_ws,WorkspaceToMatch=_detector_ws)
        IvsLam = _detector_ws / _I0M
        if (roi != [0,0]) :
            ReflectedBeam = SumSpectra(InputWorkspace=IvsLam, StartWorkspaceIndex=roi[0], EndWorkspaceIndex=roi[1])
        if (db != [0,0]) :
            DirectBeam = SumSpectra(InputWorkspace=_detector_ws, StartWorkspaceIndex=db[0], EndWorkspaceIndex=db[1])
            ReflectedBeam = ReflectedBeam / DirectBeam
        polCorr(polcorr, IvsLam, crho, calpha, cAp, cPp)
        if theta and correct_positions:
            IvsQ = l2q(ReflectedBeam, detector_component_name, theta, sample_component_name)
        else:
            IvsQ = ConvertUnits(InputWorkspace=ReflectedBeam, Target="MomentumTransfer")


    # Single Detector processing-------------------------------------------------------------
    else:
        print "This is a Point-Detector run."
        # handle transmission runs
        # process the point detector reflectivity
        _I0P = RebinToWorkspace(WorkspaceToRebin=_monitor_ws,WorkspaceToMatch=_detector_ws)
        IvsLam = Scale(InputWorkspace=_detector_ws,Factor=1)

        if not trans:
            print "No transmission file. Trying default exponential/polynomial correction..."
            IvsLam = correction_strategy.apply(_detector_ws)
            IvsLam = Divide(LHSWorkspace=IvsLam, RHSWorkspace=_I0P)
        else: # we have a transmission run
            _monInt = Integration(InputWorkspace=_I0P,RangeLower=int_min,RangeUpper=int_max)
            IvsLam = Divide(LHSWorkspace=_detector_ws,RHSWorkspace=_monInt)
            names = mtd.getObjectNames()

            IvsLam = transCorr(trans, IvsLam, lambda_min, lambda_max, background_min, background_max,
                               int_min, int_max, detector_index_ranges, i0_monitor_index, stitch_start_overlap,
                               stitch_end_overlap, stitch_params )


        IvsLam = polCorr(polcorr, IvsLam, crho, calpha, cAp, cPp)



        # Convert to I vs Q
        # check if detector in direct beam
        if theta == None or theta == 0 or theta == '':
            inst = groupGet('IvsLam','inst')
            detLocation=inst.getComponentByName(detector_component_name).getPos()
            sampleLocation=inst.getComponentByName(sample_component_name).getPos()
            detLocation=inst.getComponentByName(detector_component_name).getPos()
            sample2detector=detLocation-sampleLocation    # metres
            source=inst.getSource()
            beamPos = sampleLocation - source.getPos()
            theta = groupGet(str(_sample_ws),'samp','theta')
            if not theta:
                theta = inst.getComponentByName(detector_component_name).getTwoTheta(sampleLocation, beamPos)*180.0/math.pi/2.0
            print "Det location: ", detLocation, "Calculated theta = ",theta
            if correct_positions:  # detector is not in correct place
                # Get detector angle theta from NeXuS
                logger.information('The detectorlocation is not at Y=0')
                print 'Nexus file theta =', theta
                IvsQ = l2q(IvsLam, detector_component_name, theta, sample_component_name)
            else:
                IvsQ = ConvertUnits(InputWorkspace=IvsLam,OutputWorkspace="IvsQ",Target="MomentumTransfer")

        else:
            if correct_positions:
                theta = float(theta)
                try:
                    IvsQ = l2q(IvsLam, detector_component_name, theta, sample_component_name)
                except AttributeError:
                    logger.warning("detector_component_name " + detector_component_name + " is unknown")
                    IvsQ = ConvertUnits(InputWorkspace=IvsLam,OutputWorkspace="IvsQ",Target="MomentumTransfer")
            else:
                IvsQ = ConvertUnits(InputWorkspace=IvsLam,OutputWorkspace="IvsQ",Target="MomentumTransfer")

    RenameWorkspace(InputWorkspace=IvsLam,OutputWorkspace=RunNumber+'_IvsLam')
    if isinstance(IvsLam, WorkspaceGroup):
        counter = 0
        for ws in IvsLam:
            RenameWorkspace(ws, OutputWorkspace=RunNumber+'_IvsLam_'+str(counter))
            counter += 1
    RenameWorkspace(InputWorkspace=IvsQ,OutputWorkspace=RunNumber+'_IvsQ')

    # delete all temporary workspaces unless in debug mode (debug=1)

    if not debug:
        cleanup()
        if mtd.doesExist('IvsLam'):
            DeleteWorkspace('IvsLam')
    return  mtd[RunNumber+'_IvsLam'], mtd[RunNumber+'_IvsQ'], theta


def make_trans_corr(transrun, stitch_start_overlap, stitch_end_overlap, stitch_params,
                    lambda_min=None, lambda_max=None, background_min=None,
                    background_max=None, int_min=None, int_max=None, detector_index_ranges=None,
                    i0_monitor_index=None):
    '''
    Make the transmission correction workspace.
    '''

    '''
    Check to see whether all optional inputs have been provide. If not we have to get them from the IDF.
    '''
    if not all((lambda_min, lambda_max, background_min, background_max, int_min, int_max, detector_index_ranges, i0_monitor_index)):
        logger.notice("make_trans_corr: Fetching missing arguments from the IDF")
        instrument_source = transrun
        if isinstance(transrun, str):
            instrument_source = transrun.split(',')[0]
        trans_ws = ConvertToWavelength.to_workspace(instrument_source)
        idf_defaults = get_defaults(trans_ws)

        # Fetch defaults for anything not specified
        if not i0_monitor_index:
            i0_monitor_index = idf_defaults['I0MonitorIndex']
        if not lambda_min:
            lambda_min = idf_defaults['LambdaMin']
        if not lambda_max:
            lambda_max = idf_defaults['LambdaMax']
        if not detector_index_ranges:
            point_detector_start = idf_defaults['PointDetectorStart']
            point_detector_stop =  idf_defaults['PointDetectorStop']
            detector_index_ranges = (point_detector_start, point_detector_stop)
        if not background_min:
            background_min = idf_defaults['MonitorBackgroundMin']
        if not background_max:
            background_max = idf_defaults['MonitorBackgroundMax']
        if not int_min:
            int_min = idf_defaults['MonitorIntegralMin']
        if not int_max:
            int_max = idf_defaults['MonitorIntegralMax']


    transWS = None
    if isinstance(transrun, str) and (',' in transrun):
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

        print stitch_start_overlap, stitch_end_overlap, stitch_params
        transWS, outputScaling = Stitch1D(LHSWorkspace=_detector_ws_slam, RHSWorkspace=_detector_ws_llam, StartOverlap=stitch_start_overlap,\
                                           EndOverlap=stitch_end_overlap,  Params=stitch_params)

        transWS = RenameWorkspace(InputWorkspace=transWS, OutputWorkspace="TRANS_" + slam + "_" + llam)
    else:

        to_lam = ConvertToWavelength(transrun)
        _monitor_ws_trans, _detector_ws_trans = to_lam.convert(wavelength_min=lambda_min, wavelength_max=lambda_max, detector_workspace_indexes=detector_index_ranges, monitor_workspace_index=i0_monitor_index, correct_monitor=True, bg_min=background_min, bg_max=background_max )
        _i0p_trans = RebinToWorkspace(WorkspaceToRebin=_monitor_ws_trans, WorkspaceToMatch=_detector_ws_trans)

        _mon_int_trans = Integration( InputWorkspace=_i0p_trans, RangeLower=int_min, RangeUpper=int_max )
        transWS = Divide( LHSWorkspace=_detector_ws_trans, RHSWorkspace=_mon_int_trans )

        transWS = RenameWorkspace(InputWorkspace=transWS, OutputWorkspace="TRANS_" + transrun)
    return transWS


def transCorr(transrun, i_vs_lam, lambda_min, lambda_max, background_min, background_max, int_min, int_max, detector_index_ranges, i0_monitor_index,
              stitch_start_overlap, stitch_end_overlap, stitch_params ):
    """
    Perform transmission corrections on i_vs_lam.
    return the corrected result.
    """
    if isinstance(transrun, MatrixWorkspace) and transrun.getAxis(0).getUnit().unitID() == "Wavelength" :
        logger.debug("Using existing transmission workspace.")
        _transWS = transrun
    else:
        logger.debug("Creating new transmission correction workspace.")
        # Make the transmission correction workspace.
        _transWS = make_trans_corr(transrun, stitch_start_overlap, stitch_end_overlap, stitch_params,\
                                    lambda_min, lambda_max, background_min, background_max,\
                                    int_min, int_max, detector_index_ranges, i0_monitor_index,)

    #got sometimes very slight binning diferences, so do this again:
    _i_vs_lam_trans = RebinToWorkspace(WorkspaceToRebin=_transWS, WorkspaceToMatch=i_vs_lam, OutputWorkspace=_transWS.name())
    # Normalise by transmission run.
    _i_vs_lam_corrected = i_vs_lam / _i_vs_lam_trans

    return _i_vs_lam_corrected

def polCorr(polcorr, IvsLam, crho, calpha, cAp, cPp):
    '''
    Perform polynomial correction
    '''
    # Treat False like a polarization correction of None.
    if polcorr == PolarisationCorrection.PNR:
        IvsLam = nrPNRCorrection(IvsLam.name(), crho[0],calpha[0],cAp[0],cPp[0])
    elif polcorr == PolarisationCorrection.PA:
        IvsLam = nrPACorrection(IvsLam.name(), crho[0],calpha[0],cAp[0],cPp[0])
    else:
        message = "No Polarisation Correction Requested."
        logger.notice(message)
        print message
    return IvsLam


def cleanup():
    names = mtd.getObjectNames()
    for name in names:
        if re.search("^_", name) and mtd.doesExist(name):
            logger.debug("deleting " + name)
            DeleteWorkspace(name)

def get_defaults(run_ws, polcorr = False):
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
    defaults['PointDetectorStart'] =  int( instrument.getNumberParameter('PointDetectorStart')[0] )
    defaults['PointDetectorStop'] =  int( instrument.getNumberParameter('PointDetectorStop')[0] )
    defaults['MultiDetectorStart'] = int( instrument.getNumberParameter('MultiDetectorStart')[0] )
    defaults['I0MonitorIndex'] = int( instrument.getNumberParameter('I0MonitorIndex')[0] )
    if polcorr and (polcorr != PolarisationCorrection.NONE):

        def str_to_float_list(str):
            str_list = str.split(',')
            float_list = map(float, str_list)
            return float_list

        defaults['crho']  = str_to_float_list(instrument.getStringParameter('crho')[0])
        defaults['calpha']  = str_to_float_list(instrument.getStringParameter('calpha')[0])
        defaults['cAp']  = str_to_float_list(instrument.getStringParameter('cAp')[0])
        defaults['cPp']  = str_to_float_list(instrument.getStringParameter('cPp')[0])


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

def nrPNRCorrection(Wksp,crho,calpha,cAp,cPp):

    # Constants Based on Runs 18350+18355 and 18351+18356 analyser theta at -0.1deg
    # 2 RF Flippers as the polarising system
    # crho=[1.006831,-0.011467,0.002244,-0.000095]
    # calpha=[1.017526,-0.017183,0.003136,-0.000140]
    # cAp=[0.917940,0.038265,-0.006645,0.000282]
    # cPp=[0.972762,0.001828,-0.000261,0.0]
    message = "Performing PNR correction"
    logger.notice(message)
    print message
    CloneWorkspace(Wksp,OutputWorkspace='_'+Wksp+'_uncorrected')
    if  (not isinstance(mtd[Wksp], WorkspaceGroup))  or (not  mtd[Wksp].size()==2) :
        print "PNR correction works only with exactly 2 periods!"
        return mtd[Wksp]
    else:
        Ip = mtd[Wksp][0]
        Ia = mtd[Wksp][1]

        CloneWorkspace(Ip,OutputWorkspace="PCalpha")
        CropWorkspace(InputWorkspace="PCalpha",OutputWorkspace="PCalpha",StartWorkspaceIndex="0",EndWorkspaceIndex="0")
        PCalpha=(mtd['PCalpha']*0.0)+1.0
        alpha=mtd['PCalpha']
        # a1=alpha.readY(0)
        # for i in range(0,len(a1)):
            # alpha.dataY(0)[i]=0.0
            # alpha.dataE(0)[i]=0.0
        CloneWorkspace("PCalpha",OutputWorkspace="PCrho")
        CloneWorkspace("PCalpha",OutputWorkspace="PCAp")
        CloneWorkspace("PCalpha",OutputWorkspace="PCPp")
        rho=mtd['PCrho']
        Ap=mtd['PCAp']
        Pp=mtd['PCPp']
        # for i in range(0,len(a1)):
            # x=(alpha.dataX(0)[i]+alpha.dataX(0)[i])/2.0
            # for j in range(0,4):
                # alpha.dataY(0)[i]=alpha.dataY(0)[i]+calpha[j]*x**j
                # rho.dataY(0)[i]=rho.dataY(0)[i]+crho[j]*x**j
                # Ap.dataY(0)[i]=Ap.dataY(0)[i]+cAp[j]*x**j
                # Pp.dataY(0)[i]=Pp.dataY(0)[i]+cPp[j]*x**j
        PolynomialCorrection(InputWorkspace="PCalpha",OutputWorkspace="PCalpha",Coefficients=calpha,Operation="Multiply")
        PolynomialCorrection(InputWorkspace="PCrho",OutputWorkspace="PCrho",Coefficients=crho,Operation="Multiply")
        PolynomialCorrection(InputWorkspace="PCAp",OutputWorkspace="PCAp",Coefficients=cAp,Operation="Multiply")
        PolynomialCorrection(InputWorkspace="PCPp",OutputWorkspace="PCPp",Coefficients=cPp,Operation="Multiply")
        D=Pp*(1.0+rho)
        nIp=(Ip*(rho*Pp+1.0)+Ia*(Pp-1.0))/D
        nIa=(Ip*(rho*Pp-1.0)+Ia*(Pp+1.0))/D
        RenameWorkspace(nIp,OutputWorkspace=str(Ip)+"corr")
        RenameWorkspace(nIa,OutputWorkspace=str(Ia)+"corr")

        out = GroupWorkspaces(str(Ip)+"corr"+','+str(Ia)+"corr",OutputWorkspace=Wksp)

        #CloneWorkspace=(InputWorkspace="gr",OutputWorkspace=Wksp)
        iwksp = mtd.getObjectNames()
        list=[str(Ip),str(Ia),"PCalpha","PCrho","PCAp","PCPp","1_p"]
        for i in range(len(iwksp)):
            for j in list:
                lname=len(j)
                if iwksp[i] [0:lname+1] == j+"_":

                    DeleteWorkspace(iwksp[i])

        DeleteWorkspace("PCalpha")
        DeleteWorkspace("PCrho")
        DeleteWorkspace("PCAp")
        DeleteWorkspace("PCPp")
        DeleteWorkspace("D")

        return out
#
#
def nrPACorrection(Wksp,crho,calpha,cAp,cPp):#UpUpWksp,UpDownWksp,DownUpWksp,DownDownWksp):
#    crho=[0.941893,0.0234006,-0.00210536,0.0]
#    calpha=[0.945088,0.0242861,-0.00213624,0.0]
#    cAp=[1.00079,-0.0186778,0.00131546,0.0]
#    cPp=[1.01649,-0.0228172,0.00214626,0.0]
    # Constants Based on Runs 18350+18355 and 18351+18356 analyser theta at -0.1deg
    # 2 RF Flippers as the polarising system
    # Ipa and Iap appear to be swapped in the sequence on CRISP 4 perido data!
    message = "Performing PA correction"
    logger.notice(message)
    print message
    CloneWorkspace(Wksp,OutputWorkspace='_'+Wksp+'_uncorrected')
    if  (not isinstance(mtd[Wksp], WorkspaceGroup)) or (not mtd[Wksp].size()==4) :
        print "PNR correction works only with exactly 4 periods (uu,ud,du,dd)!"
        return mtd[Wksp]
    else:
        Ipp = mtd[Wksp][0]
        Ipa = mtd[Wksp][1]
        Iap = mtd[Wksp][2]
        Iaa = mtd[Wksp][3]

        CloneWorkspace(Ipp,OutputWorkspace="PCalpha")
        CropWorkspace(InputWorkspace="PCalpha",OutputWorkspace="PCalpha",StartWorkspaceIndex=0,EndWorkspaceIndex=0)
        PCalpha=(mtd['PCalpha']*0.0)+1.0
        alpha=mtd['PCalpha']
        CloneWorkspace("PCalpha",OutputWorkspace="PCrho")
        CloneWorkspace("PCalpha",OutputWorkspace="PCAp")
        CloneWorkspace("PCalpha",OutputWorkspace="PCPp")
        rho=mtd['PCrho']
        Ap=mtd['PCAp']
        Pp=mtd['PCPp']

        # Use the polynomial corretion fn instead
        PolynomialCorrection(InputWorkspace="PCalpha",OutputWorkspace="PCalpha",Coefficients=calpha,Operation="Multiply")
        PolynomialCorrection(InputWorkspace="PCrho",OutputWorkspace="PCrho",Coefficients=crho,Operation="Multiply")
        PolynomialCorrection(InputWorkspace="PCAp",OutputWorkspace="PCAp",Coefficients=cAp,Operation="Multiply")
        PolynomialCorrection(InputWorkspace="PCPp",OutputWorkspace="PCPp",Coefficients=cPp,Operation="Multiply")

        A0 = (Iaa * Pp * Ap) + (Ap * Ipa * rho * Pp) + (Ap * Iap * Pp * alpha) + (Ipp * Ap * alpha * rho * Pp)
        A1 = Pp * Iaa
        A2 = Pp * Iap
        A3 = Ap * Iaa
        A4 = Ap * Ipa
        A5 = Ap * alpha * Ipp
        A6 = Ap * alpha * Iap
        A7 = Pp * rho  * Ipp
        A8 = Pp * rho  * Ipa
        D = Pp * Ap *( 1.0 + rho + alpha + (rho * alpha) )
        nIpp = (A0 - A1 + A2 - A3 + A4 + A5 - A6 + A7 - A8 + Ipp + Iaa - Ipa - Iap) / D
        nIaa = (A0 + A1 - A2 + A3 - A4 - A5 + A6 - A7 + A8 + Ipp + Iaa - Ipa - Iap) / D
        nIpa = (A0 - A1 + A2 + A3 - A4 - A5 + A6 + A7 - A8 - Ipp - Iaa + Ipa + Iap) / D
        nIap = (A0 + A1 - A2 - A3 + A4 + A5 - A6 - A7 + A8 - Ipp - Iaa + Ipa + Iap) / D
        ipp_corr = RenameWorkspace(nIpp,OutputWorkspace=str(Ipp)+"corr")
        ipa_corr = RenameWorkspace(nIpa,OutputWorkspace=str(Ipa)+"corr")
        iap_corr = RenameWorkspace(nIap,OutputWorkspace=str(Iap)+"corr")
        iaa_corr = RenameWorkspace(nIaa,OutputWorkspace=str(Iaa)+"corr")
        ReplaceSpecialValues(str(Ipp)+"corr",OutputWorkspace=str(Ipp)+"corr",NaNValue="0.0",NaNError="0.0",InfinityValue="0.0",InfinityError="0.0")
        ReplaceSpecialValues(str(Ipp)+"corr",OutputWorkspace=str(Ipp)+"corr",NaNValue="0.0",NaNError="0.0",InfinityValue="0.0",InfinityError="0.0")
        ReplaceSpecialValues(str(Ipp)+"corr",OutputWorkspace=str(Ipp)+"corr",NaNValue="0.0",NaNError="0.0",InfinityValue="0.0",InfinityError="0.0")
        ReplaceSpecialValues(str(Ipp)+"corr",OutputWorkspace=str(Ipp)+"corr",NaNValue="0.0",NaNError="0.0",InfinityValue="0.0",InfinityError="0.0")
        iwksp=mtd.getObjectNames()
        list=[str(Ipp),str(Ipa),str(Iap),str(Iaa),"PCalpha","PCrho","PCAp","PCPp","1_p"]
        for i in range(len(iwksp)):
            for j in list:
                lname=len(j)
                if iwksp[i] [0:lname+1] == j+"_":
                    DeleteWorkspace(iwksp[i])
        DeleteWorkspace("PCalpha")
        DeleteWorkspace("PCrho")
        DeleteWorkspace("PCAp")
        DeleteWorkspace("PCPp")
        DeleteWorkspace('A0')
        DeleteWorkspace('A1')
        DeleteWorkspace('A2')
        DeleteWorkspace('A3')
        DeleteWorkspace('A4')
        DeleteWorkspace('A5')
        DeleteWorkspace('A6')
        DeleteWorkspace('A7')
        DeleteWorkspace('A8')
        DeleteWorkspace('D')
        out = GroupWorkspaces("%s, %s, %s, %s" % (ipp_corr.name(), ipa_corr.name(), iap_corr.name(), iaa_corr.name()))

        return out

def groupGet(wksp,whattoget,field=''):
    '''
    returns information about instrument or sample details for a given workspace wksp,
    also if the workspace is a group (info from first group element)
    '''
    if whattoget == 'inst':
        if isinstance(mtd[wksp], WorkspaceGroup):
            return mtd[wksp+'_1'].getInstrument()
        else:
            return mtd[wksp].getInstrument()

    elif whattoget == 'samp' and field != '':
        if isinstance(mtd[wksp], WorkspaceGroup):
            try:
                log = mtd[wksp + '_1'].getRun().getLogData(field).value
                if type(log) is int or type(log) is str:
                    res=log
                else:
                    res = log[len(log)-1]
            except RuntimeError:
                res = 0
                print "Block "+field+" not found."
        else:
            try:
                log = mtd[wksp].getRun().getLogData(field).value
                if type(log) is int or type(log) is str:
                    res=log
                else:
                    res = log[len(log)-1]
            except RuntimeError:
                res = 0
                print "Block "+field+" not found."
        return res
    elif whattoget == 'wksp':
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
