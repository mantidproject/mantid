from mantid.simpleapi import *
from IndirectCommon import *
from IndirectImport import import_mantidplot
mp = import_mantidplot()
from mantid import config, logger
import inelastic_indirect_reducer
import sys, os.path

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

def resolution(files, iconOpt, rebinParam, bground, 
        instrument, analyser, reflection,
        plotOpt=False, Res=True):
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
    reducer.reduce()
    iconWS = reducer.get_result_workspaces()[0]
    if Res:
        name = getWSprefix(iconWS) + 'res'
        FlatBackground(InputWorkspace=iconWS, OutputWorkspace='__background', StartX=bground[0], EndX=bground[1], 
            Mode='Mean', OutputMode='Return Background')
        Rebin(InputWorkspace=iconWS, OutputWorkspace=iconWS, Params=rebinParam)
        RebinToWorkspace(WorkspaceToRebin='__background', WorkspaceToMatch=iconWS, OutputWorkspace='__background')
        Minus(LHSWorkspace=iconWS, RHSWorkspace='__background', OutputWorkspace=name)
        DeleteWorkspace(iconWS)
        DeleteWorkspace('__background')
        SaveNexusProcessed(InputWorkspace=name, Filename=name+'.nxs')
        if plotOpt:
            graph = mp.plotSpectrum(name, 0)
        return name
    else:
        if plotOpt:
            graph = mp.plotSpectrum(iconWS, 0)
        return iconWS

def slice(inputfiles, calib, xrange, spec, suffix, Save=False, Verbose=True,
        Plot=False):
    StartTime('Slice')
    workdir = config['defaultsave.directory']
    outWSlist = []
    CheckXrange(xrange,'Time')
    for file in inputfiles:
        if Verbose:
            logger.notice('Reading file :'+file)
        (direct, filename) = os.path.split(file)
        (root, ext) = os.path.splitext(filename)
        if spec == [0, 0]:
            Load(Filename=file, OutputWorkspace=root, LoadLogFiles=False)
        else:
            Load(Filename=file, OutputWorkspace=root, SpectrumMin=spec[0], SpectrumMax=spec[1],
                LoadLogFiles=False)
        nhist,ntc = CheckHistZero(root)
        if calib != '':
            if Verbose:
                logger.notice('Using Calibration file :'+calib)
            useCalib(detectors=root)
        run = mtd[root].getRun().getLogData("run_number").value
        sfile = root[:3].lower() + run + '_' + suffix + '_slice'
        if (len(xrange) == 2):
            Integration(InputWorkspace=root, OutputWorkspace=sfile, RangeLower=xrange[0], RangeUpper=xrange[1],
                StartWorkspaceIndex=0, EndWorkspaceIndex=nhist-1)
        else:
            FlatBackground(root, sfile, StartX=xrange[2], EndX=xrange[3], 
                    Mode='Mean')
            Integration(InputWorkspace=sfile, OutputWorkspace=sfile, RangeLower=xrange[0], RangeUpper=xrange[1],
                StartWorkspaceIndex=0, EndWorkspaceIndex=nhist-1)
        if Save:
            o_path = os.path.join(workdir, sfile+'.nxs')					# path name for nxs file
            SaveNexusProcessed(InputWorkspace=sfile, Filename=o_path)
            if Verbose:
                logger.notice('Output file :'+o_path)
        outWSlist.append(sfile)
        DeleteWorkspace(root)
    if Plot:
        graph = mp.plotBin(outWSlist, 0)
    EndTime('Slice')
        
def useCalib(detectors):
    CheckHistSame(detectors,'Detectors','__calibration','Calibration')
    Divide(LHSWorkspace=detectors, RHSWorkspace='__calibration', OutputWorkspace=detectors)
    return detectors
    
def getInstrumentDetails(instrument):
    workspace = mtd['__empty_' + instrument]
    if ( workspace == None ):
        idf_dir = config['instrumentDefinition.directory']
        idf = idf_dir + instrument + '_Definition.xml'
        LoadEmptyInstrument(Filename=idf, OutputWorkspace='__empty_'+instrument)
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
    idf_dir = config['instrumentDefinition.directory']
    ws = '__empty_' + inst
    if (mtd[ws] == None):
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
