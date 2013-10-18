# IDA F2PY Absorption Corrections Wrapper
## Handle selection of .pyd files for absorption corrections
import platform, sys
from IndirectImport import *
if is_supported_f2py_platform():
    cylabs = import_f2py("cylabs")
else:
    unsupported_message()

from IndirectCommon import *
from mantid.simpleapi import *
from mantid import config, logger, mtd
import math, os.path, numpy as np
mp = import_mantidplot()

def WaveRange(inWS, efixed):
# create a list of 10 equi-spaced wavelengths spanning the input data
    oWS = '__WaveRange'
    ExtractSingleSpectrum(InputWorkspace=inWS, OutputWorkspace=oWS, WorkspaceIndex=0)
    ConvertUnits(InputWorkspace=oWS, OutputWorkspace=oWS, Target='Wavelength',
        EMode='Indirect', EFixed=efixed)
    Xin = mtd[oWS].readX(0)
    xmin = mtd[oWS].readX(0)[0]
    xmax = mtd[oWS].readX(0)[len(Xin)-1]
    ebin = 0.5
    nw1 = int(xmin/ebin)
    nw2 = int(xmax/ebin)+1
    w1 = nw1*ebin
    w2 = nw2*ebin
    wave = []
    nw = 10
    ebin = (w2-w1)/(nw-1)
    for l in range(0,nw):
        wave.append(w1+l*ebin)
    DeleteWorkspace(oWS)
    return wave

def CheckSize(size,geom,ncan,Verbose):
    if geom == 'cyl':
        if (size[1] - size[0]) < 1e-4:
            error = 'Sample outer radius not > inner radius'			
            logger.notice('ERROR *** '+error)
            sys.exit(error)
        else:
            if Verbose:
                message = 'Sam : inner radius = '+str(size[0])+' ; outer radius = '+str(size[1])
                logger.notice(message)
    if geom == 'flt':
        if size[0] < 1e-4:
            error = 'Sample thickness is zero'			
            logger.notice('ERROR *** '+error)
            sys.exit(error)
        else:
            if Verbose:
                logger.notice('Sam : thickness = '+str(size[0]))
    if ncan == 2:
        if geom == 'cyl':
            if (size[2] - size[1]) < 1e-4:
                error = 'Can inner radius not > sample outer radius'			
                logger.notice('ERROR *** '+error)
                sys.exit(error)
            else:
                if Verbose:
                    message = 'Can : inner radius = '+str(size[1])+' ; outer radius = '+str(size[2])
                    logger.notice(message)
        if geom == 'flt':
            if size[1] < 1e-4:
                error = 'Can thickness is zero'			
                logger.notice('ERROR *** '+error)
                sys.exit(error)
            else:
                if Verbose:
                    logger.notice('Can : thickness = '+str(size[1]))

def CheckDensity(density,ncan):
    if density[0] < 1e-5:
        error = 'Sample density is zero'			
        logger.notice('ERROR *** '+error)
        sys.exit(error)
    if ncan == 2:
        if density[1] < 1e-5:
            error = 'Can density is zero'			
            logger.notice('ERROR *** '+error)
            sys.exit(error)

# AbsRun used when user supplies a formula
def AbsRun(inputWS, canWS, geom, beam, ncan, size, avar, Verbose, Save):
    sam = mtd[inputWS].sample        # sample parameters
    if name() in sam:                 # test sample in WS
        if Verbose:
            logger.notice('Sample run : '+inputWS)
            logger.notice('Sample material : '+sam.name())
    else:
        error = 'Sample data does not exist'            
        logger.notice('ERROR *** ' + error)
        sys.exit(error) 
    sam_mat = sam.getMaterial()
    if ncan == 2:
        can = mtd[canWS].sample          # can parameters
        if name() in can:                 # test sample in WS
            if Verbose:
                logger.notice('Can run : '+canWS)
                logger.notice('Can material : '+can.name())
        density = [sam_mat.sampleNumberDensity(), can_mat.sampleNumberDensity()] # number densities
        sigs = [sam_mat.totalScatterXSection(), can_mat.totalScatterXSection()]   #total scat xsec
        siga = [sam_mat.absorptionXSection(), can_mat.absorptionXSection()]       #abs xsec
        else:
            error = 'Can data does not exist'           
            logger.notice('ERROR *** ' + error)
            sys.exit(error) 
            can_mat = can.getMaterial()
    else:
        density = [sam_mat.sampleNumberDensity(), 0.0] # number densities
        sigs = [sam_mat.totalScatterXSection(), 0.0]   #total scat xsec
        siga = [sam_mat.absorptionXSection(), 0.0]       #abs xsec
    CheckDensity(density,ncan)

    AbsRun(inputWS, geom, beam, ncan, size, density, sigs, siga, avar, Verbose, Save)

def AbsRun(inputWS, geom, beam, ncan, size, density, sigs, siga, avar, Verbose, Save):
    workdir = config['defaultsave.directory']
    if Verbose:
        logger.notice('Sample run : '+inputWS)
    Xin = mtd[inputWS].readX(0)
    if len(Xin) == 0:				# check that there is data
        error = 'Sample file has no data'			
        logger.notice('ERROR *** '+error)
        sys.exit(error)
    det = GetWSangles(inputWS)
    ndet = len(det)
    efixed = getEfixed(inputWS)
    wavelas = math.sqrt(81.787/efixed) # elastic wavelength
    waves = WaveRange(inputWS, efixed) # get wavelengths
    nw = len(waves)
    CheckSize(size,geom,ncan,Verbose)
    CheckDensity(density,ncan)
    run_name = getWSprefix(inputWS)
    if Verbose:
        message = 'Sam : sigt = '+str(sigs[0])+' ; siga = '+str(siga[0])+' ; rho = '+str(density[0])
        logger.notice(message)
        if ncan == 2:
            message = 'Can : sigt = '+str(sigs[1])+' ; siga = '+str(siga[1])+' ; rho = '+str(density[1])
            logger.notice(message)
        logger.notice('Elastic lambda : '+str(wavelas))
        message = 'Lambda : '+str(nw)+' values from '+str(waves[0])+' to '+str(waves[nw-1])
        logger.notice(message)
        message = 'Detector angles : '+str(ndet)+' from '+str(det[0])+' to '+str(det[ndet-1])
        logger.notice(message)
    eZ = np.zeros(nw)                  # set errors to zero
    name = run_name + geom
    assWS = name + '_ass'
    asscWS = name + '_assc'
    acscWS = name + '_acsc'
    accWS = name + '_acc'
    fname = name +'_Abs'
    wrk = workdir + run_name
    wrk.ljust(120,' ')
    for n in range(0,ndet):
        if geom == 'flt':
            angles = [avar, det[n]]
            (A1,A2,A3,A4) = FlatAbs(ncan, size, density, sigs, siga, angles, waves)	
            kill = 0
        if geom == 'cyl':
            astep = avar
            if (astep) < 1e-5:
                error = 'Step size is zero'			
                logger.notice('ERROR *** '+error)
                sys.exit(error)
            nstep = int((size[1] - size[0])/astep)
            if nstep < 20:
                error = 'Number of steps ( '+str(nstep)+' ) should be >= 20'			
                logger.notice('ERROR *** '+error)
                sys.exit(error)
            angle = det[n]
            kill, A1, A2, A3, A4 = cylabs.cylabs(astep, beam, ncan, size,
                density, sigs, siga, angle, wavelas, waves, n, wrk, 0)
        if kill == 0:
            if Verbose:
                logger.notice('Detector '+str(n)+' at angle : '+str(det[n])+' * successful')
            if n == 0:
                dataA1 = A1
                dataA2 = A2
                dataA3 = A3
                dataA4 = A4
                eZero =eZ
            else:
                dataA1 = np.append(dataA1,A1)
                dataA2 = np.append(dataA2,A2)
                dataA3 = np.append(dataA3,A3)
                dataA4 = np.append(dataA4,A4)
                eZero = np.append(eZero,eZ)
        else:
            error = 'Detector '+str(n)+' at angle : '+str(det[n])+' *** failed : Error code '+str(kill)
            logger.notice('ERROR *** '+error)
            sys.exit(error)
## Create the workspaces
    dataX = waves * ndet
    qAxis = createQaxis(inputWS)
    CreateWorkspace(OutputWorkspace=assWS, DataX=dataX, DataY=dataA1, DataE=eZero,
        NSpec=ndet, UnitX='Wavelength',
        VerticalAxisUnit='MomentumTransfer', VerticalAxisValues=qAxis)
    CreateWorkspace(OutputWorkspace=asscWS, DataX=dataX, DataY=dataA2, DataE=eZero,
        NSpec=ndet, UnitX='Wavelength',
        VerticalAxisUnit='MomentumTransfer', VerticalAxisValues=qAxis)
    CreateWorkspace(OutputWorkspace=acscWS, DataX=dataX, DataY=dataA3, DataE=eZero,
        NSpec=ndet, UnitX='Wavelength',
        VerticalAxisUnit='MomentumTransfer', VerticalAxisValues=qAxis)
    CreateWorkspace(OutputWorkspace=accWS, DataX=dataX, DataY=dataA4, DataE=eZero,
        NSpec=ndet, UnitX='Wavelength',
        VerticalAxisUnit='MomentumTransfer', VerticalAxisValues=qAxis)
    ## Save output
    group = assWS +','+ asscWS +','+ acscWS +','+ accWS
    GroupWorkspaces(InputWorkspaces=group,OutputWorkspace=fname)
    if Save:
        opath = os.path.join(workdir,fname+'.nxs')
        SaveNexusProcessed(InputWorkspace=fname, Filename=opath)
        if Verbose:
            logger.notice('Output file created : '+opath)
    if ncan > 1:
        return [assWS, asscWS, acscWS, accWS]
    else:
        return [assWS]

def AbsRunFeeder(inputWS, geom, beam, ncan, size, density, sigs, siga, avar,
        plotOpt='None', Verbose=False,Save=False):
    StartTime('CalculateCorrections')
    '''Handles the feeding of input and plotting of output for the F2PY
    absorption correction routine.'''
    workspaces = AbsRun(inputWS, geom, beam, ncan, size, density,
        sigs, siga, avar, Verbose, Save)
    EndTime('CalculateCorrections')
    if ( plotOpt == 'None' ):
        return
    if ( plotOpt == 'Wavelength' or plotOpt == 'Both' ):
        graph = mp.plotSpectrum(workspaces, 0)
    if ( plotOpt == 'Angle' or plotOpt == 'Both' ):
        graph = mp.plotTimeBin(workspaces, 0)
        graph.activeLayer().setAxisTitle(mp.Layer.Bottom, 'Angle')


# FlatAbs - CALCULATE FLAT PLATE ABSORPTION FACTORS
#
#  Input parameters :
#  sigs - list of scattering  cross-sections
#  siga - list of absorption cross-sections
#  density - list of density
#  ncan - =0 no can, >1 with can
#  thick - list of thicknesses ts,t1,t2
#  angles - list of angles
#  waves - list of wavelengths
#  Output parameters :
#  A1 - Ass ; A2 - Assc ; A3 - Acsc ; A4 - Acc

def Fact(AMU,T,SEC1,SEC2):
    S = AMU*T*(SEC1-SEC2)
    F = 1.0
    if (S == 0.):
        F = T
    else:
        S = (1-math.exp(-S))/S
        F = T*S
    return F

def FlatAbs(ncan, thick, density, sigs, siga, angles, waves):
    PICONV = math.pi/180.
    ssigs = sigs[0]                             #sam scatt x-sect
    ssiga = siga[0]                             #sam abs x-sect
    rhos = density[0]                           #sam density
    TS = thick[0]                               #sam thicknes
    T1 = thick[1]                               #can thickness 1
    T2 = thick[2]                               #can thickness 2
    csigs = sigs[1]                             #can scatt x-sect
    csiga = siga[1]                             #can abs x-sect
    rhoc = density[1]                           #can density
    TCAN1 = angles[0]                           #angle can to beam
    TCAN = TCAN1*PICONV
    THETA1 = angles[1]                          #THETAB value - detector angle
    THETA = PICONV*THETA1

    AmuS1 = []                                # sample & can cross sections
    AmuC1 = []
    nlam = len(waves)
    for n in range(0,nlam):
       AS1 = ssigs + ssiga*waves[n]/1.8
       AmuS1.append(AS1*rhos)
    if (ncan > 1):
        for n in range(0,nlam):
            AC1 = csigs + csiga*waves[n]/1.8
            AmuC1.append(AC1*rhoc)
    else:
        rhoc=0.

    SEC1 = 1./math.cos(TCAN)
    TSEC=THETA1-TCAN1    # TSEC IS THE ANGLE THE SCATTERED BEAM MAKES WITH THE NORMAL TO THE SAMPLE SURFACE.
    A1 = []
    A2 = []
    A3 = []
    A4 = []
    if (abs(abs(TSEC)-90.0) < 1.0):            # case where TSEC is close to 90. CALCULATION IS UNRELIABLE
        ASS = 1.0
        for n in range(0,nlam):                #start loop over wavelengths
            A1.append(ASS)
            A2.append(ASS)
            A3.append(ASS)
            A4.append(ASS)
    else:
        TSEC = TSEC*PICONV
        SEC2 = 1./math.cos(TSEC)
        for n in range(0,nlam):                   #start loop over wavelengths
            AMUS = AmuS1[n]
            FS = Fact(AMUS,TS,SEC1,SEC2)
            ES1=AMUS*TS*SEC1
            ES2=AMUS*TS*SEC2
            if (ncan > 1):
                AMUC = AmuC1[n]
                F1 = Fact(AMUC,T1,SEC1,SEC2)
                F2 = Fact(AMUC,T2,SEC1,SEC2)
                E11 = AMUC*T1*SEC1
                E12 = AMUC*T1*SEC2
                E21 = AMUC*T2*SEC1
                E22 = AMUC*T2*SEC2
            if (SEC2 < 0.):
                ASS=FS/TS
                if(ncan > 1):
                    ASSC = ASS*math.exp(-(E11-E12))
                    ACC1 = F1
                    ACC2 = F2*math.exp(-(E11-E12))
                    ACSC1 = ACC1
                    ACSC2 = ACC2*math.exp(-(ES1-ES2))
                else:
                    ASSC = 1.0
                    ACSC = 1.0
                    ACC = 1.0
            else:
                ASS=math.exp(-ES2)*FS/TS
                if(ncan > 1):
                    ASSC = math.exp(-(E11+E22))*ASS
                    ACC1 = math.exp(-(E12+E22))*F1
                    ACC2 = math.exp(-(E11+E22))*F2
                    ACSC1 = ACC1*math.exp(-ES2)
                    ACSC2 = ACC2*math.exp(-ES1)
                else:
                    ASSC = 1.0
                    ACSC = 1.0
                    ACC = 1.0
            tsum = T1+T2
            if(tsum > 0.):
                ACC = (ACC1+ACC2)/tsum
                ACSC = (ACSC1+ACSC2)/tsum
            else:
                ACC = 1.0
                ACSC = 1.0
            A1.append(ASS)
            A2.append(ASSC)
            A3.append(ACSC)
            A4.append(ACC)
	return A1, A2, A3, A4