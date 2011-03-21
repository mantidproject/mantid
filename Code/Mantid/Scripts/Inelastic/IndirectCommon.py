from mantidsimple import *
import os.path
import math

def loadNexus(filename):
    '''Loads a Nexus file into a workspace with the name based on the
    filename. Convenience function for not having to play around with paths
    in every function.'''
    name = os.path.splitext( os.path.split(filename)[1] )[0]
    LoadNexus(filename, name)
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
    det = mtd[workspace].getDetector(detIndex)
    try:
        efixed = det.getNumberParameter('Efixed')[0]
    except AttributeError:
        ids = det.getDetectorIDs()
        det = mtd[workspace].getInstrument().getDetector(ids[0])
        efixed = det.getNumberParameter('Efixed')[0]
    return efixed

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
            print msg
            sys.exit(msg)
        if ( axis.getUnit().name() != 'MomentumTransfer' ):
            msg += 'Input must have axis values of Q'
            print msg
            sys.exit(msg)
        for i in range(0, nHist):
            result.append(float(axis.label(i)))
    return result