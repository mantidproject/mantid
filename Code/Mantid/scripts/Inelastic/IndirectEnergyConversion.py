from mantidsimple import *

try: # mantidplot can only be imported within mantidplot
    from mantidplot import * # we want to be able to run from mantid script
except ImportError:
    print "Could not import MantidPlot module. Plotting is disabled."
    pass

from IndirectCommon import *
import inelastic_indirect_reducer

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

def createMappingFile(groupFile, ngroup, nspec, first):
    if ( ngroup == 1 ): return 'All'
    if ( nspec == 1 ): return 'Individual'
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
    DeleteWorkspace(cwsn)
    return savefile

def resolution(files, iconOpt, rebinParam, bground, 
        instrument, analyser, reflection,
        plotOpt=False, Res=True):
    reducer = inelastic_indirect_reducer.IndirectReducer()
    reducer.set_instrument_name('IRIS')
    reducer.set_detector_range(iconOpt['first']-1,iconOpt['last']-1)
    for file in files:
        reducer.append_data_file(file)
    reducer.set_parameter_file(instrument + '_' + analyser + '_' + reflection
        + '_Parameters.xml')
    reducer.set_grouping_policy('All')
    reducer.set_sum(True)
    reducer.reduce()
    iconWS = reducer.get_result_workspaces()[0]
    if Res:
        name = getWSprefix(iconWS) + 'res'
        FlatBackground(iconWS, '__background', bground[0], bground[1], 
            Mode='Mean', OutputMode='Return Background')
        Rebin(iconWS, iconWS, rebinParam)
        RebinToWorkspace('__background', iconWS, '__background')
        Minus(iconWS, '__background', name)
        DeleteWorkspace(iconWS)
        DeleteWorkspace('__background')
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

def slice(inputfiles, calib, xrange, spec, suffix, Save=False, Verbose=False,
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
        DeleteWorkspace(root)
    if Plot:
        graph = plotBin(outWSlist, 0)
        
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

def applyParameterFile(workspace, analyser, refl):
    inst = mtd[workspace].getInstrument().getName()
    idf_dir = mantid.getConfigProperty('instrumentDefinition.directory')
    ipf = idf_dir + inst + '_' + analyser + '_' + refl + '_Parameters.xml'
    LoadParameterFile(workspace, ipf)
