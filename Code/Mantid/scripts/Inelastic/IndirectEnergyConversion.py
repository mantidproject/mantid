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
        CalculateFlatBackground(InputWorkspace=iconWS, OutputWorkspace=name, StartX=bground[0], EndX=bground[1], 
            Mode='Mean', OutputMode='Subtract Background')
        Rebin(InputWorkspace=name, OutputWorkspace=name, Params=rebinParam)
        DeleteWorkspace(iconWS)
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
    calib_wsname = None
    if calib != '':
        calib_wsname = '__calibration'
        Load(Filename=calib,OutputWorkspace=calib_wsname)
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
            CalculateFlatBackground(InputWorkspace=root, OutputWorkspace=sfile, StartX=xrange[2], EndX=xrange[3], 
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
    if calib_wsname is not None:
        DeleteWorkspace(Workspace=calib_wsname)
    EndTime('Slice')
        
def useCalib(detectors):
    Divide(LHSWorkspace=detectors, RHSWorkspace='__calibration', OutputWorkspace=detectors)
    return detectors
    
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
    FFTSmooth(InputWorkspace=outWS, OutputWorkspace=outWS, WorkspaceIndex=0)									# Smooth - FFT
    DeleteWorkspace(inWS)								# delete monWS
    return outWS

def TransMon(type,file,verbose):
    if verbose:
        logger.notice('Raw file : '+file)
    LoadRaw(Filename=file,OutputWorkspace='__m1',SpectrumMin=1,SpectrumMax=1)
    LoadRaw(Filename=file,OutputWorkspace='__m2',SpectrumMin=2,SpectrumMax=2)		
    LoadRaw(Filename=file,OutputWorkspace='__det',SpectrumMin=3,SpectrumMax=3)	
# Check for single or multiple time regimes
    MonTCBstart = mtd['__m1'].readX(0)[0]
    SpecTCBstart = mtd['__det'].readX(0)[0]	
    DeleteWorkspace('__det')								# delete monWS
    monWS = '__Mon'
    if (SpecTCBstart == MonTCBstart):
        monWS = UnwrapMon('__m1')	# unwrap the monitor spectrum and convert to wavelength
        RenameWorkspace(InputWorkspace=monWS, OutputWorkspace='__Mon1')		
    else:
        ConvertUnits(InputWorkspace='__m1', OutputWorkspace='__Mon1', Target="Wavelength")
    ConvertUnits(InputWorkspace='__m2', OutputWorkspace='__Mon2', Target="Wavelength")		
    DeleteWorkspace('__m2')								# delete monWS
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
    monWS = file[0:8] +'_'+ type
    Divide(LHSWorkspace='__Mon2', RHSWorkspace='__Mon1', OutputWorkspace=monWS)
    DeleteWorkspace('__Mon1')								# delete monWS
    DeleteWorkspace('__Mon2')								# delete monWS
	
def TransPlot(inputWS):
    tr_plot=mp.plotSpectrum(inputWS,0)

def IndirectTrans(sfile,cfile,Verbose=False,Plot=True,Save=False):
    StartTime('Transmission')
    TransMon('Sam',sfile,Verbose)
    TransMon('Can',cfile,Verbose)
    samWS = sfile[0:8] + '_Sam'
    canWS =cfile[0:8] + '_Can'
    trWS = sfile[0:8] + '_Trans'
    Divide(LHSWorkspace=samWS, RHSWorkspace=canWS, OutputWorkspace=trWS)
    trans = np.average(mtd[trWS].readY(0))
    transWS = sfile[0:8] + '_Transmission'
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

##############################################################################
# Moments
##############################################################################

def SqwMoments(samWS,erange,factor,Verbose,Plot,Save):
    StartTime('Moments')
    workdir = config['defaultsave.directory']
    nq,nw = CheckHistZero(samWS)
    if Verbose:
        logger.notice('Sample '+samWS+' has '+str(nq)+' Q values & '+str(nw)+' w values')
    axis = mtd[samWS].getAxis(1)
    Q = []
    e0 = []
    for i in range(0,nq):
        Q.append(float(axis.label(i)))
        e0.append(0.0)
    Xin = mtd[samWS].readX(0)
    CheckElimits(erange,Xin)
    CropWorkspace(InputWorkspace=samWS, OutputWorkspace=samWS, XMin=erange[0], XMax=erange[1])
    Xin = mtd[samWS].readX(0)
    nw = len(Xin)-1
    if Verbose:
        logger.notice('Energy range is '+str(Xin[0])+' to '+str(Xin[nw]))
    if factor > 0.0:
        Scale(InputWorkspace=samWS, OutputWorkspace=samWS, Factor=factor, Operation='Multiply')
        if Verbose:
            logger.notice('S(q,w) scaled by '+str(factor))
    w = mtd[samWS].readX(0)
    yM0 = []
    yM1 = []
    yM2 = []
    yM4 = []
    xdel = []
    for n in range(0,nw):
        xdel.append(w[n+1]-w[n])
    xdel.append(w[nw-1])
    for m in range(0,nq):
        if Verbose:
            logger.notice('Group '+str(m+1)+' at Q = '+str(Q[m]))
        S = mtd[samWS].readY(m)
        m0 = 0.0
        m1 = 0.0
        m2 = 0.0
        m3 = 0.0
        m4 = 0.0
        for n in range(0,nw):
            S0 = S[n]*xdel[n]
            m0 += S0
            S1 = Xin[n]*S0
            m1 += S1
            S2 = Xin[n]*S1
            m2 += S2
            S3 = Xin[n]*S2
            m3 += S3
            S4 = Xin[n]*S3
            m4 += S4
        m1 = m1/m0
        m2 = m2/m0
        m3 = m3/m0
        m4 = m4/m0
        text = 'M0 = '+str(m0)+' ; M2 = '+str(m2)+' ; M4 = '+str(m4)
        logger.notice(text)
        yM0.append(m0)
        yM1.append(m1)
        yM2.append(m2)
        yM4.append(m4)
    fname = samWS[:-3] + 'Moments'
    CreateWorkspace(OutputWorkspace=fname+'_M0', DataX=Q, DataY=yM0, DataE=e0,
        Nspec=1, UnitX='MomentumTransfer')
    CreateWorkspace(OutputWorkspace=fname+'_M1', DataX=Q, DataY=yM1, DataE=e0,
        Nspec=1, UnitX='MomentumTransfer')
    CreateWorkspace(OutputWorkspace=fname+'_M2', DataX=Q, DataY=yM2, DataE=e0,
        Nspec=1, UnitX='MomentumTransfer')
    CreateWorkspace(OutputWorkspace=fname+'_M4', DataX=Q, DataY=yM4, DataE=e0,
        Nspec=1, UnitX='MomentumTransfer')
    group = fname+'_M0,'+fname+'_M1,'+fname+'_M2,'+fname+'_M4'
    GroupWorkspaces(InputWorkspaces=group,OutputWorkspace=fname)
    if Save:
        opath = os.path.join(workdir,fname+'.nxs')
        SaveNexusProcessed(InputWorkspace=fname, Filename=opath)
        if Verbose:
			logger.notice('Output file : ' + opath)
    if (Plot != 'None'):
        SqwMomentsPlot(fname,Plot)
    EndTime('Moments')	
	
def SqwMomentsPlot(inputWS,Plot):
    m0_plot=mp.plotSpectrum(inputWS+'_M0',0)
    m2_plot=mp.plotSpectrum([inputWS+'_M2',inputWS+'_M4'],0)
