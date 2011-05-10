from mantidsimple import *
from mantidplot import *

import IndirectEnergyConversion as IEC
from IndirectCommon import *

def demon(rawfiles, first, last, instrument, Smooth=False, SumFiles=False,
        grouping='Individual', cal='', CleanUp=True, Verbose=False, 
        Plot='None', Save=True, Real=False, Vanadium='', Monitor=True):
    '''DEMON routine for diffraction reduction on indirect instruments (IRIS /
    OSIRIS).'''
    # Get instrument workspace for gathering parameters and such
    wsInst = mtd['__empty_' + instrument]
    if wsInst is None:
        loadInst(instrument)
        wsInst = mtd['__empty_' + instrument]
    # short name of instrument for saving etc
    isn = ConfigService().facility().instrument(instrument).shortName().lower()
    # parameters to do with monitor
    if Monitor:
        fmon, fdet = getFirstMonFirstDet('__empty_'+instrument)
        ws_mon_l = IEC.loadData(rawfiles, Sum=SumFiles, Suffix='_mon',
            SpecMin=fmon+1, SpecMax=fmon+1)
    ws_det_l = IEC.loadData(rawfiles, Sum=SumFiles, SpecMin=first, 
        SpecMax=last)
    workspaces = []
    for i in range(0, len(ws_det_l)):
        det_ws = ws_det_l[i]
        # Get Run No
        runNo = mtd[det_ws].getRun().getLogData("run_number").value
        savefile = isn + runNo + '_diff'
        if Monitor:
            mon_ws = ws_mon_l[i]
            # Get Monitor WS
            timeRegime(monitor=mon_ws, detectors=det_ws, Smooth=Smooth)
            monitorEfficiency(mon_ws)
            normToMon(monitor=mon_ws, detectors=det_ws)
            # Remove monitor workspace
            DeleteWorkspace(mon_ws)
        if ( cal != '' ): # AlignDetectors and Group by .cal file
            if Monitor:
                ConvertUnits(det_ws, det_ws, 'TOF')
            if ( mtd[det_ws].isDistribution() ):
                ConvertFromDistribution(det_ws)
            useCalFile(det_ws, savefile)
            if ( Vanadium != '' ):
                print "NotImplemented: divide by vanadium."
        else: ## Do it the old fashioned way
            # Convert to dSpacing - need to AlignBins so we can group later
            ConvertUnits(det_ws, det_ws, 'dSpacing', AlignBins=True)
            groupData(grouping, savefile, detectors=det_ws)
        if Save:
            SaveNexusProcessed(savefile, savefile+'.nxs')
        workspaces.append(savefile)
    if ( Plot != 'None' ):
        for demon in workspaces:
            if ( Plot == 'Contour' ):
                importMatrixWorkspace(demon).plotGraph2D()
            else:
                nspec = mtd[demon].getNumberHistograms()
                plotSpectrum(demon, range(0, nspec))
    return workspaces
    
def groupData(grouping, name, detectors=''):
    if (grouping == 'Individual'):
        if ( detectors != name ):
            RenameWorkspace(detectors, name)
        return name
    elif ( grouping == 'All' ):
        nhist = mtd[detectors].getNumberHistograms()
        GroupDetectors(detectors, name, WorkspaceIndexList=range(0,nhist),
            Behaviour='Average')
    else:
        GroupDetectors(detectors, name, MapFile=grouping, Behaviour='Average')
    if ( detectors != name ):
        DeleteWorkspace(detectors)
    return name
    
def useCalFile(workspace, result):
    AlignDetectors(workspace, workspace, cal)
    DiffractionFocussing(workspace, result, cal)
    DeleteWorkspace(workspace)

def processVanadium(vanadium, empty, cal):
    van = 'vanadium'
    LoadRaw(vanadium, van)    
    ## Mask any bins? Possibly option in Instrument Parameter File
    # MaskBins(van, van, XMin=?, XMax=?)
    ## I think this is only necessary for HRPD ?
    NormaliseByCurrent(van, van)
    ## Here we diverge a little bit
    vs1 = vanaStage1(van)
    
    ## Load the "Empty"
    emp = '__empty'
    LoadRaw(empty, emp)
    ## Mask Bins on Empty ?
    NormaliseByCurrent(emp, emp)
    
    Minus(van, emp, van)
    DeleteWorkspace(emp)
    AlignDetectors(van, van, cal)
    Divide(van, vs1, van)
    DeleteWorkspace(vs1)
    ConvertUnits(van, van, 'Wavelength')
    CylinderAbsorption(van, '_tran', AttenuationXSection="5.1",
        ScatteringXSection="5.08",SampleNumberDensity="0.072",
        NumberOfWavelengthPoints="100",CylinderSampleHeight="2",
        CylinderSampleRadius="0.4",NumberOfSlices="10", NumberOfAnnuli="10")
    Divide(van, '__tran', van)
    DeleteWorkspace('__tran')
    ConvertUnits(van, van, 'dSpacing')
    DiffractionFocussing(van, van, cal)
    ReplaceSpecialValues(van, van, NaNValue="0",InfinityValue="0",
        BigNumberThreshold="99999999.999999985")
    CropWorkspace(van, van, EndWorkspaceIndex=0)
    SmoothData(van, van, 100)    
    ## RemoveBins ?
    return van

def vanaStage1(van):
    result = '__vanStage1'
    SolidAngle(van, '__cor')
    Scale('__cor', '__cor', 100, 'Multiply')
    Divide(van, '__cor', result)
    ConvertUnits(result, result, 'Wavelength')
    ## Need to find the integration ranges from somewhere !?!
    lower = 1.4
    upper = 3.0
    Integration(result, result, RangeLower=lower, RangeUpper=upper)
    Multiply('__cor', result, result)
    DeleteWorkspace('__cor')
    Scale(result, result, (1.0/100000), 'Multiply')
    return result
    
def getFirstMonFirstDet(inWS):
    workspace = mtd[inWS]
    FirstDet = FirstMon = -1
    nhist = workspace.getNumberHistograms()
    for counter in range(0, nhist):
        try:
            detector = workspace.getDetector(counter)
        except RuntimeError: # This causes problems when encountering some
            pass             # incomplete instrument definition files (TOSCA)
        if detector.isMonitor():
            if (FirstMon == -1):
                FirstMon = counter
        else:
            if (FirstDet == -1):
                FirstDet = counter
        if ( FirstMon != -1 and FirstDet != -1 ):
            break
    return FirstMon, FirstDet
    
def timeRegime(Smooth=True, monitor='', detectors=''):
    SpecMon = mtd[monitor].readX(0)[0]
    SpecDet = mtd[detectors].readX(0)[0]
    if ( SpecMon == SpecDet ):
        LRef = getReferenceLength(detectors, 0)
        alg = Unwrap(monitor, monitor, LRef = LRef)
        join = float(alg.getPropertyValue('JoinWavelength'))
        RemoveBins(monitor, monitor, join-0.001, join+0.001, 
                Interpolation='Linear')
        if Smooth:
            FFTSmooth(monitor, monitor, 0)
    else:
        ConvertUnits(monitor, monitor, 'Wavelength')
    return monitor

def monitorEfficiency(inWS):
    inst = mtd[inWS].getInstrument()
    try:
        area = inst.getNumberParameter('Workflow.MonitorArea')[0]
        thickness = inst.getNumberParameter('Workflow.MonitorThickness')[0]
        OneMinusExponentialCor(inWS, inWS, (8.3 * thickness), area)
    except IndexError:
        print "Unable to take Monitor Area and Thickness from Paremeter File."
    return inWS

def getReferenceLength(inWS, fdi):
    workspace = mtd[inWS]
    instrument = workspace.getInstrument()
    sample = instrument.getSample()
    source = instrument.getSource()
    detector = workspace.getDetector(fdi)
    sample_to_source = sample.getPos() - source.getPos()
    r = detector.getDistance(sample)
    x = sample_to_source.getZ()
    LRef = x + r
    return LRef
    
def normToMon(monitor='', detectors=''):
    ConvertUnits(detectors, detectors, 'Wavelength')
    RebinToWorkspace(detectors,monitor,detectors)
    Divide(detectors,monitor,detectors)
    try:
        factor = mtd[detectors].getInstrument().getNumberParameter(
            'Workflow.MonitorScalingFactor')[0]
        Scale(detectors, detectors, factor)
    except IndexError:
        pass
    return detectors