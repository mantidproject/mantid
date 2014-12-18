from mantid.simpleapi import *
from IndirectCommon import *
from IndirectImport import import_mantidplot
mp = import_mantidplot()
from mantid import config, logger
import inelastic_indirect_reducer
import sys, os.path, numpy as np

def loadData(rawfiles, outWS='RawFile', Sum=False, SpecMin=-1, SpecMax=-1,
        Suffix=''):
    workspaces = []
    for file in rawfiles:
        ( dir, filename ) = os.path.split(file)
        ( name, ext ) = os.path.splitext(filename)
        try:
            if ( SpecMin == -1 ) and ( SpecMax == -1 ):
                Load(Filename=file, OutputWorkspace=name+Suffix, LoadLogFiles=False)
            else:
                Load(Filename=file, OutputWorkspace=name+Suffix, SpectrumMin=SpecMin,
                    SpectrumMax=SpecMax, LoadLogFiles=False)
            workspaces.append(name+Suffix)
        except ValueError, message:
            logger.notice(message)
            sys.exit(message)
    if Sum and ( len(workspaces) > 1 ):
        MergeRuns(InputWorkspaces=','.join(workspaces), OutputWorkspace=outWS+Suffix)
        factor = 1.0 / len(workspaces)
        Scale(InputWorkspace=outWS+Suffix, OutputWorkspace=outWS+Suffix, Factor=factor)
        for ws in workspaces:
            DeleteWorkspace(ws)
        return [outWS+Suffix]
    else:
        return workspaces

def createMappingFile(groupFile, ngroup, nspec, first):
    if ( ngroup == 1 ): return 'All'
    if ( nspec == 1 ): return 'Individual'
    filename = config['defaultsave.directory']
    filename = os.path.join(filename, groupFile)
    handle = open(filename, 'w')
    handle.write(str(ngroup) +  "\n" )
    for n in range(0, ngroup):
        n1 = n * nspec + first
        handle.write(str(n+1) +  '\n' )
        handle.write(str(nspec) +  '\n')
        for i in range(1, nspec+1):
            n3 = n1 + i - 1
            handle.write(str(n3).center(4) + ' ')
        handle.write('\n')
    handle.close()
    return filename

def resolution(files, iconOpt, rebinParam, bground,
        instrument, analyser, reflection,
        Res=True, factor=None, Plot=False, Verbose=False, Save=False):
    reducer = inelastic_indirect_reducer.IndirectReducer()
    reducer.set_instrument_name(instrument)
    reducer.set_detector_range(iconOpt['first']-1,iconOpt['last']-1)
    for file in files:
        reducer.append_data_file(file)
    parfile = config['instrumentDefinition.directory']
    parfile += instrument +"_"+ analyser +"_"+ reflection +"_Parameters.xml"
    reducer.set_parameter_file(parfile)
    reducer.set_grouping_policy('All')
    reducer.set_sum_files(True)

    try:
        reducer.reduce()
    except Exception, e:
        logger.error(str(e))
        return

    iconWS = reducer.get_result_workspaces()[0]

    if factor != None:
        Scale(InputWorkspace=iconWS, OutputWorkspace=iconWS, Factor = factor)

    if Res:
        name = getWSprefix(iconWS) + 'res'
        CalculateFlatBackground(InputWorkspace=iconWS, OutputWorkspace=name, StartX=bground[0], EndX=bground[1],
            Mode='Mean', OutputMode='Subtract Background')
        Rebin(InputWorkspace=name, OutputWorkspace=name, Params=rebinParam)

        if Save:
            if Verbose:
                logger.notice("Resolution file saved to default save directory.")
            SaveNexusProcessed(InputWorkspace=name, Filename=name+'.nxs')

        if Plot:
            graph = mp.plotSpectrum(name, 0)
        return name
    else:
        if Plot:
            graph = mp.plotSpectrum(iconWS, 0)
        return iconWS

##############################################################################
# Slice Functions
##############################################################################

def sliceReadRawFile(fname, Verbose):

    if Verbose:
        logger.notice('Reading file :'+fname)

    #Load the raw file
    (dir, filename) = os.path.split(fname)
    (root, ext) = os.path.splitext(filename)

    Load(Filename=fname, OutputWorkspace=root, LoadLogFiles=False)

    return root

# returns the number of monitors
# and if they're at the start or end of the file
def countMonitors(rawFile):
    rawFile = mtd[rawFile]
    nhist = rawFile.getNumberHistograms()
    detector = rawFile.getDetector(0)
    monCount = 1

    if detector.isMonitor():
        #monitors are at the start
        for i in range(1,nhist):
            detector = rawFile.getDetector(i)

            if detector.isMonitor():
                monCount += 1
            else:
                break

        return monCount, True
    else:
        #monitors are at the end
        detector = rawFile.getDetector(nhist)

        if not detector.isMonitor():
            #if it's not, we don't have any monitors!
            return 0, True

        for i in range(nhist,0,-1):
            detector = rawFile.getDetector(i)

            if detector.isMonitor():
                monCount += 1
            else:
                break

        return monCount, False

# Run the calibration file with the raw file workspace
def sliceProcessCalib(rawFile, calibWsName, spec):
    calibSpecMin, calibSpecMax = spec

    if calibSpecMax-calibSpecMin > mtd[calibWsName].getNumberHistograms():
        raise IndexError("Number of spectra used is greater than the number of spectra in the calibration file.")

    #offset cropping range to account for monitors
    (monCount, atStart) = countMonitors(rawFile)

    if atStart:
        calibSpecMin -= monCount+1
        calibSpecMax -= monCount+1

    #Crop the calibration workspace, excluding the monitors
    CropWorkspace(InputWorkspace=calibWsName, OutputWorkspace=calibWsName,
        StartWorkspaceIndex=calibSpecMin, EndWorkspaceIndex=calibSpecMax)

def sliceProcessRawFile(rawFile, calibWsName, useCalib, xRange, useTwoRanges, spec, suffix, Verbose):

    #Crop the raw file to use the desired number of spectra
    #less one because CropWorkspace is zero based
    CropWorkspace(InputWorkspace=rawFile, OutputWorkspace=rawFile,
        StartWorkspaceIndex=spec[0]-1, EndWorkspaceIndex=spec[1]-1)

    nhist,ntc = CheckHistZero(rawFile)

    #use calibration file if desired
    if useCalib:
        Divide(LHSWorkspace=rawFile, RHSWorkspace=calibWsName, OutputWorkspace=rawFile)

    #construct output workspace name
    run = mtd[rawFile].getRun().getLogData("run_number").value
    sfile = rawFile[:3].lower() + run + '_' + suffix + '_slice'

    if not useTwoRanges:
        Integration(InputWorkspace=rawFile, OutputWorkspace=sfile, RangeLower=xRange[0], RangeUpper=xRange[1],
            StartWorkspaceIndex=0, EndWorkspaceIndex=nhist-1)
    else:
        CalculateFlatBackground(InputWorkspace=rawFile, OutputWorkspace=sfile, StartX=xRange[2], EndX=xRange[3],
                Mode='Mean')
        Integration(InputWorkspace=sfile, OutputWorkspace=sfile, RangeLower=xRange[0], RangeUpper=xRange[1],
            StartWorkspaceIndex=0, EndWorkspaceIndex=nhist-1)

    return sfile

def slice(inputfiles, calib, xRange, spec, suffix, Save=False, Verbose=False, Plot=False):

    StartTime('Slice')

    CheckXrange(xRange,'Time')

    workdir = config['defaultsave.directory']

    outWSlist = []
    useTwoRanges = (len(xRange) != 2)
    useCalib = (calib != '')
    calibWsName = '__calibration'

    #load the calibration file
    if useCalib:
        Load(Filename=calib, OutputWorkspace=calibWsName)
        if Verbose:
            logger.notice('Using Calibration file: %s' % calib)

    for index, file in enumerate(inputfiles):
        rawFile = sliceReadRawFile(file, Verbose)

        #only need to process the calib file once
        if(index == 0 and useCalib):
            sliceProcessCalib(rawFile, calibWsName, spec)

        sfile = sliceProcessRawFile(rawFile, calibWsName, useCalib, xRange, useTwoRanges, spec, suffix, Verbose)
        Transpose(InputWorkspace=sfile, OutputWorkspace=sfile)
        unit = mtd[sfile].getAxis(0).setUnit("Label")
        unit.setLabel("Spectrum Number", "")

        outWSlist.append(sfile)
        DeleteWorkspace(rawFile)

        if Save:
            # path name for nxs file
            o_path = os.path.join(workdir, sfile+'.nxs')
            SaveNexusProcessed(InputWorkspace=sfile, Filename=o_path)

            if Verbose:
                logger.notice('Output file :'+o_path)

    if useCalib:
        DeleteWorkspace(Workspace=calibWsName)

    if Plot:
        try:
            graph = mp.plotSpectrum(sfile, 0)
        except RuntimeError, e:
            #User clicked cancel on plot so don't do anything
            pass

    EndTime('Slice')

def getInstrumentDetails(instrument):
    instr_name = '__empty_' + instrument
    if mtd.doesExist(instr_name):
        workspace = mtd[instr_name]
    else:
        idf_dir = config['instrumentDefinition.directory']
        idf = idf_dir + instrument + '_Definition.xml'
        LoadEmptyInstrument(Filename=idf, OutputWorkspace=instr_name)
        workspace = mtd[instr_name]
    instrument = workspace.getInstrument()
    ana_list_param = instrument.getStringParameter('analysers')
    if len(ana_list_param) != 1:
        return ""
    ana_list_split = ana_list_param[0].split(',')
    reflections = []
    result = ''
    for analyser in ana_list_split:
        list = []
        name = 'refl-' + analyser
        list.append( analyser )
        try:
            item = instrument.getStringParameter(name)[0]
        except IndexError:
            item = ''
        refl = item.split(',')
        list.append( refl )
        reflections.append(list)
    for i in range(0, len(reflections)):
        message = reflections[i][0] + '-'
        for j in range(0,len(reflections[i][1])):
            message += str(reflections[i][1][j])
            if j < ( len(reflections[i][1]) -1 ):
                message += ','
        result += message
        if ( i < ( len(reflections) - 1) ):
            result += '\n'
    return result

def getReflectionDetails(inst, analyser, refl):
    idf_dir = config['instrumentDefinition.directory']
    ws = '__empty_' + inst
    if not mtd.doesExist(ws):
        idf_file = inst + '_Definition.xml'
        idf = os.path.join(idf_dir, idf_file)
        LoadEmptyInstrument(Filename=idf, OutputWorkspace=ws)
    ipf_file = inst + '_' + analyser + '_' + refl + '_Parameters.xml'
    ipf = os.path.join(idf_dir, ipf_file)
    LoadParameterFile(Workspace=ws, Filename=ipf)
    inst = mtd[ws].getInstrument()
    result = ''
    try:
        result += str( inst.getStringParameter('analysis-type')[0] ) + '\n'
        result += str( int(inst.getNumberParameter('spectra-min')[0]) ) + '\n'
        result += str( int(inst.getNumberParameter('spectra-max')[0]) ) + '\n'
        result += str( inst.getNumberParameter('efixed-val')[0] ) + '\n'
        result += str( int(inst.getNumberParameter('peak-start')[0]) ) + '\n'
        result += str( int(inst.getNumberParameter('peak-end')[0]) ) + '\n'
        result += str( int(inst.getNumberParameter('back-start')[0]) ) + '\n'
        result += str( int(inst.getNumberParameter('back-end')[0]) ) + '\n'
        result += inst.getStringParameter('rebin-default')[0]
    except IndexError:
        pass
    return result

##############################################################################
# Transmission
##############################################################################

def UnwrapMon(inWS):
# Unwrap monitor - inWS contains M1,M2,S1  - outWS contains unwrapped Mon
#Unwrap s1>2 to L of S2 (M2) ie 38.76  Ouput is in wavelength
    out, join = UnwrapMonitor(InputWorkspace=inWS,LRef='37.86')
    outWS = 'out'
#Fill bad (dip) in spectrum
    RemoveBins(InputWorkspace=outWS, OutputWorkspace=outWS, Xmin=join-0.001, Xmax=join+0.001,
        Interpolation="Linear")
    FFTSmooth(InputWorkspace=outWS, OutputWorkspace=outWS, WorkspaceIndex=0, IgnoreXBins=True) # Smooth - FFT
    DeleteWorkspace(inWS)    # delete monWS
    return outWS

def TransMon(inst, type,file,verbose):
    if verbose:
        logger.notice('Raw file : '+file)
    LoadRaw(Filename=file,OutputWorkspace='__m1',SpectrumMin=1,SpectrumMax=1)
    LoadRaw(Filename=file,OutputWorkspace='__m2',SpectrumMin=2,SpectrumMax=2)
    LoadRaw(Filename=file,OutputWorkspace='__det',SpectrumMin=3,SpectrumMax=3)
# Check for single or multiple time regimes
    MonTCBstart = mtd['__m1'].readX(0)[0]
    SpecTCBstart = mtd['__det'].readX(0)[0]
    DeleteWorkspace('__det')    							# delete monWS
    monWS = '__Mon'
    if (SpecTCBstart == MonTCBstart):
        monWS = UnwrapMon('__m1')    # unwrap the monitor spectrum and convert to wavelength
        RenameWorkspace(InputWorkspace=monWS, OutputWorkspace='__Mon1')
    else:
        ConvertUnits(InputWorkspace='__m1', OutputWorkspace='__Mon1', Target="Wavelength")
    ConvertUnits(InputWorkspace='__m2', OutputWorkspace='__Mon2', Target="Wavelength")
    DeleteWorkspace('__m2')    							# delete monWS
    Xin = mtd['__Mon1'].readX(0)
    xmin1 = mtd['__Mon1'].readX(0)[0]
    xmax1 = mtd['__Mon1'].readX(0)[len(Xin)-1]
    Xin = mtd['__Mon2'].readX(0)
    xmin2 = mtd['__Mon2'].readX(0)[0]
    xmax2 = mtd['__Mon2'].readX(0)[len(Xin)-1]
    wmin = max(xmin1,xmin2)
    wmax = min(xmax1,xmax2)
    CropWorkspace(InputWorkspace='__Mon1', OutputWorkspace='__Mon1', XMin=wmin, XMax=wmax)
    RebinToWorkspace(WorkspaceToRebin='__Mon2', WorkspaceToMatch='__Mon1', OutputWorkspace='__Mon2')
    monWS = inst +'_'+ type
    Divide(LHSWorkspace='__Mon2', RHSWorkspace='__Mon1', OutputWorkspace=monWS)
    DeleteWorkspace('__Mon1')    							# delete monWS
    DeleteWorkspace('__Mon2')    							# delete monWS

def TransPlot(inputWS):
    tr_plot=mp.plotSpectrum(inputWS,0)

def IndirectTrans(inst, sfile,cfile,Verbose=False,Plot=False,Save=False):
    StartTime('Transmission')
    TransMon(inst,'Sam',sfile,Verbose)
    TransMon(inst,'Can',cfile,Verbose)
    samWS = inst + '_Sam'
    canWS = inst + '_Can'
    trWS = inst + '_Trans'
    Divide(LHSWorkspace=samWS, RHSWorkspace=canWS, OutputWorkspace=trWS)
    trans = np.average(mtd[trWS].readY(0))
    transWS = inst + '_Transmission'
    workdir = config['defaultsave.directory']
    group = samWS +','+ canWS +','+ trWS
    GroupWorkspaces(InputWorkspaces=group,OutputWorkspace=transWS)
    if Verbose:
        logger.notice('Transmission : '+str(trans))
    path = os.path.join(workdir,transWS+'.nxs')

    if Save:
        SaveNexusProcessed(InputWorkspace=transWS, Filename=path)
        if Verbose:
            logger.notice('Output file created : '+path)

    if Plot:
        TransPlot(transWS)
    EndTime('Transmission')
