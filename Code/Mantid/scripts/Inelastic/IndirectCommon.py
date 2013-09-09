from mantid.simpleapi import *
from mantid import config, logger
import sys, platform, os.path, math, datetime, re
    
def StartTime(prog):
    logger.notice('----------')
    message = 'Program ' + prog +' started @ ' + str(datetime.datetime.now())
    logger.notice(message)

def EndTime(prog):
    message = 'Program ' + prog +' ended @ ' + str(datetime.datetime.now())
    logger.notice(message)
    logger.notice('----------')

def loadInst(instrument):    
    ws = '__empty_' + instrument
    if not mtd.doesExist(ws):
        idf_dir = config['instrumentDefinition.directory']
        idf = idf_dir + instrument + '_Definition.xml'
        LoadEmptyInstrument(Filename=idf, OutputWorkspace=ws)

def loadNexus(filename):
    '''Loads a Nexus file into a workspace with the name based on the
    filename. Convenience function for not having to play around with paths
    in every function.'''
    name = os.path.splitext( os.path.split(filename)[1] )[0]
    LoadNexus(Filename=filename, OutputWorkspace=name)
    return name
    
def getInstrRun(file):
    mo = re.match('([a-zA-Z]+)([0-9]+)',file)
    instr_and_run = mo.group(0)          # instr name + run number
    instr = mo.group(1)                  # instrument prefix
    run = mo.group(2)                    # run number as string
    return instr,run

def getWSprefix(wsname,runfile=None):
    '''Returns a string of the form '<ins><run>_<analyser><refl>_' on which
    all of our other naming conventions are built.
    The workspace is used to get the instrument parameters. If the runfile
    string is given it is expected to be a string with instrument prefix
    and run number. If it is empty then the workspace name is assumed to
    contain this information
    '''
    if wsname == '':
        return ''
    if runfile is None:
        runfile = wsname
    ws = mtd[wsname]
    facility = config['default.facility']
    ws_run = ws.getRun()
    if 'facility' in ws_run:
        facility = ws_run.getLogData('facility').value
    if facility == 'ILL':		
        inst = ws.getInstrument().getName()
        runNo = ws.getRun()['run_number'].value
        run_name = inst + '_'+ runNo
    else:
        (instr, run) = getInstrRun(runfile)
        run_name = instr + run
    try:
        analyser = ws.getInstrument().getStringParameter('analyser')[0]
        reflection = ws.getInstrument().getStringParameter('reflection')[0]
    except IndexError:
        analyser = ''
        reflection = ''
    prefix = run_name + '_' + analyser + reflection + '_'
    return prefix

def getEfixed(workspace, detIndex=0):
    inst = mtd[workspace].getInstrument()
    return inst.getNumberParameter("efixed-val")[0]

def getRunTitle(workspace):
    ws = mtd[workspace]
    title = ws.getRun()['run_title'].value.strip()
    runNo = ws.getRun()['run_number'].value
    inst = ws.getInstrument().getName()
    ins = config.getFacility().instrument(ins).shortName().lower()
    valid = "-_.() %s%s" % (string.ascii_letters, string.digits)
    title = ''.join(ch for ch in title if ch in valid)
    title = ins + runNo + '-' + title
    return title

def createQaxis(inputWS):
    result = []
    ws = mtd[inputWS]
    nHist = ws.getNumberHistograms()
    if ws.getAxis(1).isSpectra():
        inst = ws.getInstrument()
        samplePos = inst.getSample().getPos()
        beamPos = samplePos - inst.getSource().getPos()
        for i in range(0,nHist):
            efixed = getEfixed(inputWS, i)
            detector = ws.getDetector(i)
            theta = detector.getTwoTheta(samplePos, beamPos) / 2
            lamda = math.sqrt(81.787/efixed)
            q = 4 * math.pi * math.sin(theta) / lamda
            result.append(q)
    else:
        axis = ws.getAxis(1)
        msg = 'Creating Axis based on Detector Q value: '
        if not axis.isNumeric():
            msg += 'Input workspace must have either spectra or numeric axis.'
            logger.notice(msg)
            sys.exit(msg)
        if ( axis.getUnit().unitID() != 'MomentumTransfer' ):
            msg += 'Input must have axis values of Q'
            logger.notice(msg)
            sys.exit(msg)
        for i in range(0, nHist):
            result.append(float(axis.label(i)))
    return result

def GetWSangles(inWS,verbose=False):
    nhist = mtd[inWS].getNumberHistograms()						# get no. of histograms/groups
    sourcePos = mtd[inWS].getInstrument().getSource().getPos()
    samplePos = mtd[inWS].getInstrument().getSample().getPos() 
    beamPos = samplePos - sourcePos
    angles = []										# will be list of angles
    for index in range(0, nhist):
        detector = mtd[inWS].getDetector(index)					# get index
        twoTheta = detector.getTwoTheta(samplePos, beamPos)*180.0/math.pi		# calc angle
        angles.append(twoTheta)						# add angle
    return angles

def GetThetaQ(inWS):
    nhist = mtd[inWS].getNumberHistograms()						# get no. of histograms/groups
    efixed = getEfixed(inWS)
    wavelas = math.sqrt(81.787/efixed)					   # elastic wavelength
    k0 = 4.0*math.pi/wavelas
    d2r = math.pi/180.0
    sourcePos = mtd[inWS].getInstrument().getSource().getPos()
    samplePos = mtd[inWS].getInstrument().getSample().getPos() 
    beamPos = samplePos - sourcePos
    theta = []
    Q = []
    for index in range(0,nhist):
        detector = mtd[inWS].getDetector(index)					# get index
        twoTheta = detector.getTwoTheta(samplePos, beamPos)*180.0/math.pi		# calc angle
        theta.append(twoTheta)						# add angle
        Q.append(k0*math.sin(0.5*twoTheta*d2r))
    return theta,Q

def ExtractFloat(a):                              #extract values from line of ascii
    extracted = []
    elements = a.split()							#split line on spaces
    for n in elements:
        extracted.append(float(n))
    return extracted                                 #values as list

def ExtractInt(a):                              #extract values from line of ascii
    extracted = []
    elements = a.split()							#split line on spaces
    for n in elements:
        extracted.append(int(n))
    return extracted                                 #values as list

def PadArray(inarray,nfixed):                   #pad a list to specified size
	npt=len(inarray)
	padding = nfixed-npt
	outarray=[]
	outarray.extend(inarray)
	outarray +=[0]*padding
	return outarray

def CheckAnalysers(in1WS,in2WS,Verbose):
    ws1 = mtd[in1WS]
    a1 = ws1.getInstrument().getStringParameter('analyser')[0]
    r1 = ws1.getInstrument().getStringParameter('reflection')[0]
    ws2 = mtd[in2WS]
    a2 = ws2.getInstrument().getStringParameter('analyser')[0]
    r2 = ws2.getInstrument().getStringParameter('reflection')[0]
    if a1 != a2:
        error = 'Workspace '+in1WS+' and '+in2WS+' have different analysers'
        logger.notice('ERROR *** '+error)
        sys.exit(error)
    elif r1 != r2:
        error = 'Workspace '+in1WS+' and '+in2WS+' have different reflections'
        logger.notice('ERROR *** '+error)
        sys.exit(error)
    else:
        if Verbose:
            logger.notice('Analyser is '+a1+r1)

def CheckHistZero(inWS):
    nhist = mtd[inWS].getNumberHistograms()       # no. of hist/groups in WS
    if nhist == 0:
        error = 'Workspace '+inWS+' has NO histograms'			
        logger.notice('ERROR *** ' + error)
        sys.exit(error)
    Xin = mtd[inWS].readX(0)
    ntc = len(Xin)-1						# no. points from length of x array
    if ntc == 0:
        error = 'Workspace '+inWS+' has NO points'			
        logger.notice('ERROR *** ' + error)
        sys.exit(error)
    return nhist,ntc

def CheckHistSame(in1WS,name1,in2WS,name2):
    nhist1 = mtd[in1WS].getNumberHistograms()       # no. of hist/groups in WS1
    X1 = mtd[in1WS].readX(0)
    xlen1 = len(X1)
    nhist2 = mtd[in2WS].getNumberHistograms()       # no. of hist/groups in WS2
    X2 = mtd[in2WS].readX(0)
    xlen2 = len(X2)
    if nhist1 != nhist2:				# check that no. groups are the same
        e1 = name1+' ('+in1WS+') histograms (' +str(nhist1) + ')'
        e2 = name2+' ('+in2WS+') histograms (' +str(nhist2) + ')'
        error = e1 + ' not = ' + e2
        logger.notice('ERROR *** ' + error)
        sys.exit(error)
    elif xlen1 != xlen2:
        e1 = name1+' ('+in1WS+') array length (' +str(xlen1) + ')'
        e2 = name2+' ('+in2WS+') array length (' +str(xlen2) + ')'
        error = e1 + ' not = ' + e2
        logger.notice('ERROR *** ' + error)
        sys.exit(error)

def CheckXrange(xrange,type):
    if  not ( ( len(xrange) == 2 ) or ( len(xrange) == 4 ) ):
        error = type + ' - Range must contain either 2 or 4 numbers'
        logger.notice(error)
        sys.exit(error)
    if math.fabs(xrange[0]) < 1e-5:
        error = type + ' - input minimum ('+str(xrange[0])+') is Zero'			
        logger.notice('ERROR *** ' + error)
        sys.exit(error)
    if math.fabs(xrange[1]) < 1e-5:
        error = type + ' - input maximum ('+str(xrange[1])+') is Zero'			
        logger.notice('ERROR *** ' + error)
        sys.exit(error)
    if xrange[1] < xrange[0]:
        error = type + ' - input max ('+str(xrange[1])+') < min ('+xrange[0]+')'			
        logger.notice('ERROR *** ' + error)
        sys.exit(error)
    if len(xrange) >2:
        if math.fabs(xrange[2]) < 1e-5:
            error = type + '2 - input minimum ('+str(xrange[2])+') is Zero'			
            logger.notice('ERROR *** ' + error)
            sys.exit(error)
        if math.fabs(xrange[3]) < 1e-5:
            error = type + '2 - input maximum ('+str(xrange[3])+') is Zero'			
            logger.notice('ERROR *** ' + error)
            sys.exit(error)
        if xrange[3] < xrange[2]:
            error = type + '2 - input max ('+str(xrange[3])+') < min ('+xrange[2]+')'			
            logger.notice('ERROR *** ' + error)
            sys.exit(error)

def CheckElimits(erange,Xin):
    nx = len(Xin)-1
    if math.fabs(erange[0]) < 1e-5:
        error = 'Elimits - input emin ( '+str(erange[0])+' ) is Zero'			
        logger.notice('ERROR *** ' + error)
        sys.exit(error)
    if erange[0] < Xin[0]:
        error = 'Elimits - input emin ( '+str(erange[0])+' ) < data emin ( '+str(Xin[0])+' )'		
        logger.notice('ERROR *** ' + error)
        sys.exit(error)
    if math.fabs(erange[1]) < 1e-5:
        error = 'Elimits - input emax ( '+str(erange[1])+' ) is Zero'			
        logger.notice('ERROR *** ' + error)
        sys.exit(error)
    if erange[1] > Xin[nx]:
        error = 'Elimits - input emax ( '+str(erange[1])+' ) > data emax ( '+str(Xin[nx])+' )'			
        logger.notice('ERROR *** ' + error)
        sys.exit(error)
    if erange[1] < erange[0]:
        error = 'Elimits - input emax ( '+str(erange[1])+' ) < emin ( '+erange[0]+' )'			
        logger.notice('ERROR *** ' + error)
        sys.exit(error)
