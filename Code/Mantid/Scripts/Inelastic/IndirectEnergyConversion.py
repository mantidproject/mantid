from mantidsimple import *

try: # mantidplot can only be imported within mantidplot
    from mantidplot import * # we want to be able to run from mantid script
except ImportError:
    pass

import re

def convert_to_energy(rawfiles, grouping, first, last, 
        instrument='', analyser = '', reflection = '', 
        SumFiles=False, bgremove = [0, 0], tempK=-1, 
        calib='', rebinParam='', saveFormats = [], 
        CleanUp = True, Verbose = False):        
    # Get hold of base empty instrument workspace
    wsInst = mtd['__empty_' + instrument]
    if wsInst is None:
        loadInst(instrument)
        wsInst = mtd['__empty_' + instrument]
    fmon, fdet = getFirstMonFirstDet('__empty_'+instrument)
    try: # Get monitor parameters
        inst = wsInst.getInstrument()
        area = inst.getNumberParameter('mon-area')[0]
        thickness = inst.getNumberParameter('mon-thickness')[0]
    except IndexError, message:
        print message
        sys.exit(message)
    isTosca = adjustTOF('__empty_'+instrument)    
    # Get short name of instrument from the config service
    isn = ConfigService().facility().instrument(instrument).shortName().lower()
    if ( calib != '' ): # Only load Calibration file once
        LoadNexusProcessed(calib, '__calibration')    
    # Holds output parameters
    output_workspace_names = []
    runNos = []
    mon_wsl = loadData(rawfiles, Sum=SumFiles, SpecMin=fmon+1, SpecMax=fmon+1,
        Suffix='_mon')
    ws_names = loadData(rawfiles, Sum=SumFiles, SpecMin=first, SpecMax=last)    
    if ( len(mon_wsl) != len(ws_names) ):
        print "Indirect CTE: Error loading data files."
        sys.exit("Indirect CTE: Error loading data files.")
    for i in range(0, len(ws_names)):
        ws = ws_names[i]
        ws_mon = mon_wsl[i]
        invalid = []
        if ( analyser != "" and reflection != "" ):
            applyParameterFile(ws, analyser, reflection)
        if isTosca:
            invalid = getBadDetectorList(ws)
            TofCorrection(ws, ws)
            for i in range(1,141):
                detector = "Detector #" + str(i)
                MoveInstrumentComponent(ws, detector, X=0, Y=0, Z=0,
                    RelativePosition=False)
            factor = 1e9
        else:
            factor = 1e6
        runNo = mtd[ws].getRun().getLogData("run_number").value
        runNos.append(runNo)
        name = isn + runNo + '_' + analyser + reflection + '_red'
        timeRegime(monitor=ws_mon, detectors=ws)
        monitorEfficiency(ws_mon, area, thickness)
        ## start dealing with detector data here
        if ( bgremove != [0, 0] ):
            backgroundRemoval(bgremove[0], bgremove[1], detectors=ws)
        if ( calib != '' ):
            calibrated = useCalib(detectors=ws)
        normToMon(Factor=factor, monitor=ws_mon, detectors=ws)
        # Remove monitor workspace
        DeleteWorkspace(ws_mon)
        conToEnergy(detectors=ws)
        if not CleanUp:
            CloneWorkspace(ws, name+'_intermediate')
        if ( rebinParam != ''): # Rebin data
            rebinData(rebinParam, detectors=ws)
        if ( tempK != -1 ): # "Detailed Balance" correction
            ExponentialCorrection(ws, ws, 1.0, ( 11.606 / ( 2 * tempK ) ) )            
        # Final stage, grouping of detectors - apply name
        if isTosca: # TOSCA's grouping is pre-determined and fixed
            groupTosca(name, invalid, detectors=ws)
        else:
            groupData(grouping, name, detectors=ws)
        output_workspace_names.append(name)
    if ( saveFormats != [] ): # Save data in selected formats
        saveItems(output_workspace_names, saveFormats)
    if ( calib != '' ): # Remove calibration workspace
        DeleteWorkspace('__calibration')
    return output_workspace_names, runNos
        
def loadData(rawfiles, outWS='RawFile', Sum=False, SpecMin=-1, SpecMax=-1,
        Suffix=''):
    workspaces = []
    for file in rawfiles:
        ( dir, filename ) = os.path.split(file)
        ( name, ext ) = os.path.splitext(filename)
        try:
            if ( SpecMin == -1 ) and ( SpecMax == -1 ):
                LoadRaw(file, name+Suffix, LoadLogFiles=False)
            else:
                LoadRaw(file, name+Suffix, SpectrumMin=SpecMin, 
                    SpectrumMax=SpecMax, LoadLogFiles=False)
            workspaces.append(name+Suffix)
        except ValueError, message:
            print message
            sys.exit(message)
    if Sum and ( len(workspaces) > 1 ):
        MergeRuns(','.join(workspaces), outWS+Suffix)
        factor = 1.0 / len(workspaces)
        Scale(outWS+Suffix, outWS+Suffix, factor)
        for ws in workspaces:
            DeleteWorkspace(ws)
        return [outWS+Suffix]
    else:
        return workspaces

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

def monitorEfficiency(inWS, area, thickness):
    OneMinusExponentialCor(inWS, inWS, (8.3 * thickness), area)
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

def useCalib(detectors=''):
    tmp = mtd[detectors]
    shist = tmp.getNumberHistograms()
    tmp = mtd['__calibration']
    chist = tmp.getNumberHistograms()
    if chist != shist:
        msg = 'Number of spectra in calibration file does not match number \
                that exist in the data file.'
        print msg
        sys.exit(msg)
    else:
        Divide(detectors,'__calibration',detectors)
    return detectors

def normToMon(Factor=1e6, monitor='', detectors=''):
    ConvertUnits(detectors,detectors, 'Wavelength', EMode='Indirect')
    RebinToWorkspace(detectors,monitor,detectors)
    Divide(detectors,monitor,detectors)
    Scale(detectors, detectors, Factor)
    return detectors

def conToEnergy(detectors=''):
    ConvertUnits(detectors, detectors, 'DeltaE', 'Indirect')
    CorrectKiKf(detectors, detectors, 'Indirect')
    return detectors

def rebinData(rebinParam, detectors=''):
    Rebin(detectors, detectors, rebinParam)
    return detectors

def groupData(grouping, name, detectors=''):
    if (grouping == 'Individual'):
        RenameWorkspace(detectors, name)
        return name
    elif ( grouping == 'All' ):
        nhist = mtd[detectors].getNumberHistograms()
        GroupDetectors(detectors, name, WorkspaceIndexList=range(0,nhist),
            Behaviour='Average')
    else:
        GroupDetectors(detectors, name, MapFile=grouping, Behaviour='Average')
    if detectors != name:
        DeleteWorkspace(detectors)
    return name

def groupTosca(wsname, invalid, detectors=''):
    grp = range(0,70)
    for i in invalid:
        try:
            grp.remove(i)
        except ValueError:
            pass
    GroupDetectors(detectors, wsname, WorkspaceIndexList=grp, 
        Behaviour='Average')
    grp = range(70,140)
    for i in invalid:
        try:
            grp.remove(i)
        except ValueError:
            pass
    GroupDetectors(detectors, '__front', WorkspaceIndexList=grp, 
        Behaviour='Average')
    DeleteWorkspace(detectors)
    ConjoinWorkspaces(wsname, '__front')
    return wsname

def getBadDetectorList(workspace):
    IdentifyNoisyDetectors(workspace, '__temp_tsc_noise')
    ws = mtd['__temp_tsc_noise']
    nhist = ws.getNumberHistograms()
    invalid = []
    for i in range(0, nhist):
        if ( ws.readY(i)[0] == 0.0 ):
            invalid.append(i)
    DeleteWorkspace('__temp_tsc_noise')
    return invalid

def backgroundRemoval(tofStart, tofEnd, detectors=''):
    ConvertToDistribution(detectors)
    FlatBackground(detectors, detectors, tofStart, tofEnd, Mode = 'Mean')
    ConvertFromDistribution(detectors)
    return detectors

def cte_rebin(mapfile, tempK, rebinParam, analyser, reflection, instrument, 
        savesuffix, saveFormats, CleanUp=False, Verbose=False):
    ws_list = mantid.getWorkspaceNames()
    energy = re.compile('_'+analyser+reflection+r'_intermediate$')
    int_list = []
    if ( len(int_list) == 0 ):
        message = "No intermediate workspaces were found. Run with \
                'Keep Intermediate Workspaces' checked."
        print message
        sys.exit(message)
    output_workspace_names = []
    runNos = []
    for workspace in ws_list:
        if energy.search(workspace):
            int_list.append(workspace)
    for cte in int_list:
        runNo = mtd[cte].getRun().getLogData("run_number").value
        runNos.append(runNo)
        if ( rebinParam != ''):
            rebin = rebinData(rebinParam, inWS_n=cte)
            if CleanUp:
                mantid.deleteWorkspace(cte)
        else:
            if CleanUp:
                RenameWorkspace(cte, 'Energy')
            else:
                CloneWorkspace(cte, 'Energy')
        if ( tempK != -1 ):
            db = detailedBalance(tempK)
        scale = scaleAndGroup(mapfile, outWS_n=cte[:-13])
        output_workspace_names.append(scale)
    if ( saveFormats != [] ):
        saveItems(output_workspace_names, saveFormats)

def createMappingFile(groupFile, ngroup, nspec, first):
    filename = mtd.getConfigProperty('defaultsave.directory')
    filename += groupFile
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

def createCalibFile(rawfiles, suffix, peakMin, peakMax, backMin, backMax, 
        specMin, specMax, PlotOpt=False):
    savepath = mantid.getConfigProperty('defaultsave.directory')
    runs = []
    for file in rawfiles:
        (direct, filename) = os.path.split(file)
        (root, ext) = os.path.splitext(filename)
        try:
            LoadRaw(file, root, SpectrumMin=specMin, SpectrumMax=specMax,
                LoadLogFiles=False)
            runs.append(root)
        except:
            sys.exit('Indirect-Could not load raw file: ' + file)
    cwsn = '__calibration'
    if ( len(runs) > 1 ):
        MergeRuns(",".join(runs), cwsn)
        factor = 1.0 / len(runs)
        Scale(cwsn, cwsn, factor)
    else:
        cwsn = runs[0]
    tmp = mtd[cwsn]
    nhist = tmp.getNumberHistograms()
    FlatBackground(cwsn, cwsn, StartX=backMin, EndX=backMax, Mode='Mean')
    Integration(cwsn, cwsn, peakMin, peakMax)
    cal_ws = mtd[cwsn]
    sum = 0
    for i in range(0, nhist):
        sum += cal_ws.readY(i)[0]
    value = sum / nhist
    CreateSingleValuedWorkspace('__cal_avg', value)
    runNo = mtd[cwsn].getRun().getLogData("run_number").value
    outWS_n = runs[0][:3] + runNo + suffix
    Divide(cwsn, '__cal_avg', outWS_n)
    DeleteWorkspace('__cal_avg')
    savefile = os.path.join(savepath, outWS_n+'.nxs')
    SaveNexusProcessed(outWS_n, savefile, 'Calibration')
    if PlotOpt:
        graph = plotTimeBin(outWS_n, 0)
    ## Tidyup
    DeleteWorkspace(cwsn)
    return savefile

def resolution(files, iconOpt, rebinParam, bground, 
        instrument, analyser, reflection,
        plotOpt=False, Res=True):
    workspace_list, runNos = convert_to_energy(files, 'All', 
        iconOpt['first'], iconOpt['last'], SumFiles=True, 
        instrument=instrument, analyser=analyser, reflection=reflection)
    iconWS = workspace_list[0]
    if Res:
        run = mtd[iconWS].getRun().getLogData("run_number").value
        name = iconWS[:3].lower() + run + '_' + analyser + reflection + '_res'
        Rebin(iconWS, iconWS, rebinParam)
        FFTSmooth(iconWS,iconWS,0)
        FlatBackground(iconWS, name, bground[0], bground[1], Mode='Mean')
        DeleteWorkspace(iconWS)
        SaveNexusProcessed(name, name+'.nxs')
        if plotOpt:
            graph = plotSpectrum(name, 0)
        return name
    else:
        if plotOpt:
            graph = plotSpectrum(iconWS, 0)
        return iconWS

def saveItems(workspaces, fileFormats, Verbose=False):
    for workspace in workspaces:
        for j in fileFormats:
            if j == 'spe':
                SaveSPE(workspace, workspace+'.spe')
            elif j == 'nxs':
                SaveNexusProcessed(workspace, workspace+'.nxs')
            elif j == 'nxspe':
                SaveNXSPE(workspace, workspace+'.nxspe')
            else:
                print 'Save: unknown file type.'
                system.exit('Save: unknown file type.')

def slice(inputfiles, calib, xrange, spec,  suffix, Save=False, Verbose=False,
        Plot=False):
    outWSlist = []
    if  not ( ( len(xrange) == 2 ) or ( len(xrange) == 4 ) ):
        mantid.sendLogMessage('>> TOF Range must contain either 2 or 4 \
                numbers.')
        sys.exit(1)
    for file in inputfiles:
        (direct, filename) = os.path.split(file)
        (root, ext) = os.path.splitext(filename)
        if spec == [0, 0]:
            LoadRaw(file, root, LoadLogFiles=False)
        else:
            LoadRaw(file, root, SpectrumMin=spec[0], SpectrumMax=spec[1],
                LoadLogFiles=False)
        nhist = mtd[root].getNumberHistograms()
        if calib != '':
            useCalib(calib, inWS_n=root, outWS_n=root)
        run = mtd[root].getRun().getLogData("run_number").value
        sfile = root[:3].lower() + run + '_' + suffix + '_slt'
        if (len(xrange) == 2):
            Integration(root, sfile, xrange[0], xrange[1], 0, nhist-1)
        else:
            FlatBackground(root, sfile, StartX=xrange[2], EndX=xrange[3], 
                    Mode='Mean')
            Integration(sfile, sfile, xrange[0], xrange[1], 0, nhist-1)
        if Save:
            SaveNexusProcessed(sfile, sfile+'.nxs')
        outWSlist.append(sfile)
        mantid.deleteWorkspace(root)
    if Plot:
        graph = plotBin(outWSlist, 0)

def getInstrumentDetails(instrument):
    workspace = mtd['__empty_' + instrument]
    if ( workspace == None ):
        idf_dir = mantid.getConfigProperty('instrumentDefinition.directory')
        idf = idf_dir + instrument + '_Definition.xml'
        LoadEmptyInstrument(idf, '__empty_'+instrument)
        workspace = mtd['__empty_'+instrument]
    instrument = workspace.getInstrument()
    ana_list_split = instrument.getStringParameter('analysers')[0].split(',')
    reflections = []
    result = ''
    for i in range(0,len(ana_list_split)):
        list = []
        name = 'refl-' + ana_list_split[i]
        list.append( ana_list_split[i] )
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
    idf_dir = mantid.getConfigProperty('instrumentDefinition.directory')
    ws = '__empty_' + inst
    if (mtd[ws] == None):
        idf = idf_dir + inst + '_Definition.xml'
        LoadEmptyInstrument(idf, ws)
    ipf = idf_dir + inst + '_' + analyser + '_' + refl + '_Parameters.xml'
    LoadParameterFile(ws, ipf)
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

def loadInst(instrument):    
    ws = '__empty_' + instrument
    if (mtd[ws] == None):
        idf_dir = mantid.getConfigProperty('instrumentDefinition.directory')
        idf = idf_dir + instrument + '_Definition.xml'
        LoadEmptyInstrument(idf, ws)

def adjustTOF(ws='', inst=''):
    if ( ws != '' ):
        ins = mtd[ws].getInstrument()
    elif ( inst != ''):
        idf_dir = mantid.getConfigProperty('instrumentDefinition.directory')
        idf = idf_dir + inst + '_Definition.xml'
        LoadEmptyInstrument(idf, 'ins')
        ins = mtd['ins'].getInstrument()
    try:
        val = ins.getNumberParameter('adjustTOF')[0]
    except IndexError:
        val = 0
    if ( val == 1 ):
        return True
    else:
        return False

def applyParameterFile(workspace, analyser, refl):
    inst = mtd[workspace].getInstrument().getName()
    idf_dir = mantid.getConfigProperty('instrumentDefinition.directory')
    ipf = idf_dir + inst + '_' + analyser + '_' + refl + '_Parameters.xml'
    LoadParameterFile(workspace, ipf)
