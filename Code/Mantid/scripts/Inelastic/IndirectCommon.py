from mantidsimple import *
from mantid import config, logger
import platform, os.path, math, datetime

def inF2PyCompatibleEnv():
    '''Returns true if we are in an environment where our Fortran2Python 
    files are usable, otherwise returns false.'''
    osEnv = platform.system() + platform.architecture()[0]
    if osEnv == 'Windows32bit':
        return True
    return False

def runF2PyCheck():
    '''Raises exception if not in F2Py compatible env.'''
    if not inF2PyCompatibleEnv():
        raise RuntimeError("F2Py programs NOT available on this platform.")
    
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
    if (mtd[ws] == None):
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
    
def getWSprefix(workspace):
    '''Returns a string of the form '<ins><run>_<analyser><refl>_' on which
    all of our other naming conventions are built.'''
    if workspace == '':
        return ''
    ws = mtd[workspace]
    ins = ws.getInstrument().getName()
    ins = ConfigService().facility().instrument(ins).shortName().lower()
    run = ws.getRun().getLogData('run_number').value
    try:
        analyser = ws.getInstrument().getStringParameter('analyser')[0]
        reflection = ws.getInstrument().getStringParameter('reflection')[0]
    except IndexError:
        analyser = ''
        reflection = ''
    prefix = ins + run + '_' + analyser + reflection + '_'
    return prefix

def getEfixed(workspace, detIndex=0):
    inst = mtd[workspace].getInstrument()
    return inst.getNumberParameter("efixed-val")[0]

def getRunTitle(workspace):
    ws = mtd[workspace]
    title = ws.getRun()['run_title'].value.strip()
    runNo = ws.getRun()['run_number'].value
    inst = ws.getInstrument().getName()
    isn = ConfigService().facility().instrument(inst).shortName().upper()
    valid = "-_.() %s%s" % (string.ascii_letters, string.digits)
    title = ''.join(ch for ch in title if ch in valid)
    title = isn + runNo + '-' + title
    return title

def getDetectorTwoTheta(detector, samplePos, beamPos): #fix 'cos getTwoTheta is incorrectly gettwoTheta in new API
    if hasattr(detector, 'getTwotheta'):
        return detector.getTwotheta(samplePos, beamPos)
    else:
        return detector.getTwoTheta(samplePos, beamPos)

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
            theta = getDetectorTwoTheta(detector, samplePos, beamPos) / 2
            lamda = math.sqrt(81.787/efixed)
            q = 4 * math.pi * math.sin(theta) / lamda
            result.append(q)
    else:
        axis = ws.getAxis(1)
        msg = 'Creating Axis based on Detector Q value: '
        if not axis.isNumeric():
            msg += 'Input workspace must have either spectra or numeric axis.'
            print msg
            sys.exit(msg)
        if ( axis.getUnit().name() != 'MomentumTransfer' ):
            msg += 'Input must have axis values of Q'
            print msg
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
        twoTheta = getDetectorTwoTheta(detector, samplePos, beamPos)*180.0/math.pi		# calc angle
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
        twoTheta = getDetectorTwoTheta(detector, samplePos, beamPos)*180.0/math.pi		# calc angle
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