#pylint: disable=invalid-name
# IDA F2PY Absorption Corrections Wrapper
## Handle selection of .pyd files for absorption corrections
import sys
from IndirectImport import *

from IndirectCommon import *
from mantid.simpleapi import *
from mantid import config, logger, mtd
import math, os.path, numpy as np
MTD_PLOT = import_mantidplot()

def WaveRange(inWS, efixed):
# create a list of 10 equi-spaced wavelengths spanning the input data
    oWS = '__WaveRange'
    ExtractSingleSpectrum(InputWorkspace=inWS, OutputWorkspace=oWS, WorkspaceIndex=0)
    if efixed == 0.0:
        ConvertUnits(InputWorkspace=oWS, OutputWorkspace=oWS, Target='Wavelength',
                     EMode='Elastic')
    else:
        ConvertUnits(InputWorkspace=oWS, OutputWorkspace=oWS, Target='Wavelength',
                     EMode='Indirect', EFixed=efixed)
    Xin = mtd[oWS].readX(0)
    xmin = mtd[oWS].readX(0)[0]
    xmax = mtd[oWS].readX(0)[len(Xin) - 1]
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

def CheckSize(size, geom, ncan):
    if geom == 'cyl':
        if (size[1] - size[0]) < 1e-4:
            raise ValueError('Sample outer radius not > inner radius')
        else:
            message = 'Sam : inner radius = ' + str(size[0]) + ' ; outer radius = ' + str(size[1])
            logger.information(message)
    if geom == 'flt':
        if size[0] < 1e-4:
            raise ValueError('Sample thickness is zero')
        else:
            logger.information('Sam : thickness = ' + str(size[0]))
    if ncan == 2:
        if geom == 'cyl':
            if (size[2] - size[1]) < 1e-4:
                raise ValueError('Can inner radius not > sample outer radius')
            else:
                message = 'Can : inner radius = ' + str(size[1]) + ' ; outer radius = ' + str(size[2])
                logger.information(message)
        if geom == 'flt':
            if size[1] < 1e-4:
                raise ValueError('Can thickness is zero')
            else:
                logger.information('Can : thickness = ' + str(size[1]))

def CheckDensity(density, ncan):
    if density[0] < 1e-5:
        raise ValueError('Sample density is zero')

    if ncan == 2:
        if density[1] < 1e-5:
            raise ValueError('Can density is zero')

def AbsRun(inputWS, geom, beam, ncan, size, density, sigs, siga, avar, Save):
    workdir = getDefaultWorkingDirectory()

    logger.information('Sample run : ' + inputWS)

    # check that there is data
    Xin = mtd[inputWS].readX(0)
    if len(Xin) == 0:
        raise ValueError('Sample file has no data')

    CheckSize(size, geom, ncan)
    CheckDensity(density,ncan)

    diffraction_run = checkUnitIs(inputWS, 'dSpacing')
    logger.information('Is diffraction run: %s' % str(diffraction_run))

    if diffraction_run:
        det = GetWSangles(inputWS)
        efixed = 0.0
    else:
        det, _ = GetThetaQ(inputWS)
        efixed = getEfixed(inputWS)

    ndet = len(det)
    waves = WaveRange(inputWS, efixed) # get wavelengths
    nw = len(waves)

    if diffraction_run:
        wavelas = waves[int(nw / 2)]
    else:
        wavelas = math.sqrt(81.787/efixed) # elastic wavelength

    run_name = getWSprefix(inputWS)

    message = 'Sam : sigt = ' + str(sigs[0]) + ' ; siga = ' + str(siga[0]) + ' ; rho = ' + str(density[0])
    logger.information(message)

    if ncan == 2:
        message = 'Can : sigt = ' + str(sigs[1]) + ' ; siga = ' + str(siga[1]) + ' ; rho = ' + str(density[1])
        logger.information(message)

    logger.information('Elastic lambda : ' + str(wavelas))

    message = 'Lambda : ' + str(nw) + ' values from ' + str(waves[0]) + ' to ' + str(waves[nw - 1])
    logger.information(message)

    message = 'Detector angles : ' + str(ndet) + ' from ' + str(det[0]) + ' to ' + str(det[ndet - 1])
    logger.information(message)

    name = run_name + geom
    wrk = workdir + run_name
    wrk.ljust(120,' ')

    dataA1 = []
    dataA2 = []
    dataA3 = []
    dataA4 = []

    #initially set errors to zero
    eZero = np.zeros(nw)

    # F2Py only needed for cylinder
    if geom == 'cyl':
        if is_supported_f2py_platform():
            cylabs = import_f2py("cylabs")
        else:
            unsupported_message()
            return

    for n in range(ndet):
        #geometry is flat
        if geom == 'flt':
            sample_logs = {'sample_shape': 'flat',
                           'sample_thickness': size[0],
                           'sample_angle': avar}

            angles = [avar, det[n]]
            (A1, A2, A3, A4) = FlatAbs(ncan, size, density, sigs, siga, angles, waves)
            kill = 0

        #geometry is a cylinder
        elif geom == 'cyl':
            sample_logs = {'sample_shape': 'cylinder',
                           'sample_radius1': size[0],
                           'sample_radius2': size[1]}

            astep = avar
            if (astep) < 1e-5:
                raise ValueError('Step size is zero')

            nstep = int((size[1] - size[0]) / astep)
            if nstep < 20:
                raise ValueError('Number of steps ( ' + str(nstep) + ' ) should be >= 20')

            angle = det[n]
            kill, A1, A2, A3, A4 = cylabs.cylabs(astep, beam, ncan, size,\
                density, sigs, siga, angle, wavelas, waves, n, wrk, 0)

        if kill == 0:
            logger.information('Detector ' + str(n) + ' at angle : ' + str(det[n]) + ' * successful')

            dataA1 = np.append(dataA1, A1)
            dataA2 = np.append(dataA2, A2)
            dataA3 = np.append(dataA3, A3)
            dataA4 = np.append(dataA4, A4)
        else:
            raise ValueError('Detector ' + str(n) + ' at angle : ' + str(det[n]) + ' *** failed : Error code ' + str(kill))

    dataX = waves * ndet

    if diffraction_run:
        v_axis_unit = 'dSpacing'
        v_axis_values = [1.0]
    else:
        v_axis_unit = 'MomentumTransfer'
        v_axis_values = createQaxis(inputWS)

    # Create the output workspaces
    assWS = name + '_ass'
    asscWS = name + '_assc'
    acscWS = name + '_acsc'
    accWS = name + '_acc'
    fname = name + '_abs'

    CreateWorkspace(OutputWorkspace=assWS, DataX=dataX, DataY=dataA1,
                    NSpec=ndet, UnitX='Wavelength',
                    VerticalAxisUnit=v_axis_unit, VerticalAxisValues=v_axis_values)
    addSampleLogs(assWS, sample_logs)

    CreateWorkspace(OutputWorkspace=asscWS, DataX=dataX, DataY=dataA2,
                    NSpec=ndet, UnitX='Wavelength',
                    VerticalAxisUnit=v_axis_unit, VerticalAxisValues=v_axis_values)
    addSampleLogs(asscWS, sample_logs)

    CreateWorkspace(OutputWorkspace=acscWS, DataX=dataX, DataY=dataA3,
                    NSpec=ndet, UnitX='Wavelength',
                    VerticalAxisUnit=v_axis_unit, VerticalAxisValues=v_axis_values)
    addSampleLogs(acscWS, sample_logs)

    CreateWorkspace(OutputWorkspace=accWS, DataX=dataX, DataY=dataA4,
                    NSpec=ndet, UnitX='Wavelength',
                    VerticalAxisUnit=v_axis_unit, VerticalAxisValues=v_axis_values)
    addSampleLogs(accWS, sample_logs)

    group = assWS + ',' + asscWS + ',' + acscWS + ',' + accWS
    GroupWorkspaces(InputWorkspaces=group, OutputWorkspace=fname)

    # save output to file if required
    if Save:
        opath = os.path.join(workdir, fname + '.nxs')
        SaveNexusProcessed(InputWorkspace=fname, Filename=opath)
        logger.information('Output file created : ' + opath)

    if ncan > 1:
        return [fname, assWS, asscWS, acscWS, accWS]
    else:
        return [fname, assWS]

def plotAbs(workspaces, plotOpt):
    if plotOpt == 'None':
        return

    if plotOpt == 'Wavelength' or plotOpt == 'Both':
        graph = MTD_PLOT.plotSpectrum(workspaces, 0)

    if plotOpt == 'Angle' or plotOpt == 'Both':
        graph = MTD_PLOT.plotTimeBin(workspaces, 0)
        graph.activeLayer().setAxisTitle(MTD_PLOT.Layer.Bottom, 'Angle')


def AbsRunFeeder(input_ws, can_ws, geom, ncan, size, avar, density, beam_width=None, sample_formula=None, can_formula=None, sigs=None, siga=None,
                 plot_opt='None', save=False):
    """
    Handles the feeding of input and plotting of output for the F2PY
    absorption correction routine.

    @param input_ws - workspace to generate corrections for
    @param geom - type of geometry used (flat plate or cylinder)
    @param beam_width - width of the beam used. If None this will be taken from the IPF
    @param ncan - number of cans used.
    @param size - sample & can thickness
    @param sample_formula - optional, chemical formula for the sample
    @param cam_formula - optional, chemical formula for the can
    @param density - density of the sample and cans(s)
    @param sigs - scattering for sample and can(s)
    @param siga - absorption for sample and can(s)
    @param avar - sample angle
    @param plot_opt - whether to plot output
    @param save - whether to save the output to file
    @return The result workspace group
    """

    StartTime('CalculateCorrections')
    CheckDensity(density, ncan)

    #attempt to find beam width if none given
    if beam_width is None:
        beam_width = getInstrumentParameter(input_ws, 'Workflow.beam-width')
        beam_width = float(beam_width)

    #attempt to find beam height from parameter file
    try:
        beam_height = getInstrumentParameter(input_ws, 'Workflow.beam-height')
        beam_height = float(beam_height)
    except ValueError:
        # fall back on default value for beam height
        beam_height = 3.0

    # beam[0]    height         overall height of sample
    # beam[1:2]  a,b            beam width parameters (a>b)
    # beam[3:4]  a1,b1          scattered beam width parameters (a1 > b1)
    # beam[5:6]  hdown,hup      bottom and top of beam from sample bottom
    # beam[7:8]  hsdown,hsup    bottom and top of scattered beam from sample b.
    beam = [beam_height, 0.5 * beam_width, -0.5 * beam_width, (beam_width / 2), -(beam_width / 2), 0.0, beam_height, 0.0, beam_height]

    if sample_formula is None and (sigs is None or siga is None):
        raise ValueError("Either a formula for the sample or values for the cross sections must be supplied.")

    #set sample material based on input or formula
    if sample_formula is not None:
        SetSampleMaterial(InputWorkspace=input_ws, ChemicalFormula=sample_formula,
                          SampleNumberDensity=density[0])

        sample = mtd[input_ws].sample()
        sam_mat = sample.getMaterial()

        # total scattering x-section
        sigs[0] = sam_mat.totalScatterXSection()
        # absorption x-section
        siga[0] = sam_mat.absorbXSection()

    if can_formula is not None and ncan == 2:
        #set can material based on input or formula
        SetSampleMaterial(InputWorkspace=can_ws, ChemicalFormula=can_formula,
                          SampleNumberDensity=density[1])

        can_sample = mtd[can_ws].sample()
        can_mat = can_sample.getMaterial()

        # total scattering x-section for can
        sigs[1] = can_mat.totalScatterXSection()
        sigs[2] = can_mat.totalScatterXSection()
        # absorption x-section for can
        siga[1] = can_mat.absorbXSection()
        siga[2] = can_mat.absorbXSection()

    workspaces = AbsRun(input_ws, geom, beam, ncan, size, density,
                        sigs, siga, avar, save)

    EndTime('CalculateCorrections')
    plotAbs(workspaces[1:], plot_opt)

    return workspaces[0]


def FlatAbs(ncan, thick, density, sigs, siga, angles, waves):
    """
    FlatAbs - calculate flat plate absorption factors

    For more information See:
      - MODES User Guide: http://www.isis.stfc.ac.uk/instruments/iris/data-analysis/modes-v3-user-guide-6962.pdf
      - C J Carlile, Rutherford Laboratory report, RL-74-103 (1974)

    @param sigs - list of scattering  cross-sections
    @param siga - list of absorption cross-sections
    @param density - list of density
    @param ncan - =0 no can, >1 with can
    @param thick - list of thicknesses: sample thickness, can thickness1, can thickness2
    @param angles - list of angles
    @param waves - list of wavelengths
    """
    PICONV = math.pi/180.

    #can angle and detector angle
    tcan1, theta1 = angles
    canAngle = tcan1 * PICONV
    theta = theta1 * PICONV

    # tsec is the angle the scattered beam makes with the normal to the sample surface.
    tsec = theta1-tcan1

    nlam = len(waves)

    ass = np.ones(nlam)
    assc = np.ones(nlam)
    acsc = np.ones(nlam)
    acc = np.ones(nlam)

    # case where tsec is close to 90 degrees. CALCULATION IS UNRELIABLE
    if abs(abs(tsec)-90.0) < 1.0:
        #default to 1 for everything
        return ass, assc, acsc, acc
    else:
        #sample & can scattering x-section
        sampleScatt, canScatt = sigs[:2]
        #sample & can absorption x-section
        sampleAbs, canAbs = siga[:2]
        #sample & can density
        sampleDensity, canDensity = density[:2]
        #thickness of the sample and can
        samThickness, canThickness1, canThickness2 = thick

        tsec = tsec*PICONV

        sec1 = 1. / math.cos(canAngle)
        sec2 = 1. / math.cos(tsec)

        #list of wavelengths
        waves = np.array(waves)

        #sample cross section
        sampleXSection = (sampleScatt + sampleAbs * waves / 1.8) * sampleDensity

        #vector version of fact
        vecFact = np.vectorize(Fact)
        fs = vecFact(sampleXSection, samThickness, sec1, sec2)

        sampleSec1, sampleSec2 = calcThicknessAtSec(sampleXSection, samThickness, [sec1, sec2])


        if sec2 < 0.0:
            ass = fs / samThickness
        else:
            ass= np.exp(-sampleSec2) * fs / samThickness

        useCan = (ncan > 1)
        if useCan:
            #calculate can cross section
            canXSection = (canScatt + canAbs * waves / 1.8) * canDensity
            assc, acsc, acc = calcFlatAbsCan(ass, canXSection, canThickness1, canThickness2,
                                             sampleSec1, sampleSec2, [sec1, sec2])

    return ass, assc, acsc, acc

def Fact(xSection, thickness, sec1, sec2):
    S = xSection * thickness * (sec1 - sec2)
    F = 1.0
    if S == 0.:
        F = thickness
    else:
        S = (1 - math.exp(-S)) / S
        F = thickness*S
    return F

def calcThicknessAtSec(xSection, thickness, sec):
    sec1, sec2 = sec

    thickSec1 = xSection * thickness * sec1
    thickSec2 = xSection * thickness * sec2

    return thickSec1, thickSec2

def calcFlatAbsCan(ass, canXSection, canThickness1, canThickness2, sampleSec1, sampleSec2, sec):
    assc = np.ones(ass.size)
    acsc = np.ones(ass.size)
    acc = np.ones(ass.size)

    sec1, sec2 = sec

    #vector version of fact
    vecFact = np.vectorize(Fact)
    f1 = vecFact(canXSection,canThickness1,sec1,sec2)
    f2 = vecFact(canXSection,canThickness2,sec1,sec2)

    canThick1Sec1, canThick1Sec2 = calcThicknessAtSec(canXSection, canThickness1, sec)
    canThick2Sec2 = calcThicknessAtSec(canXSection, canThickness2, sec)[1]

    if sec2 < 0.0:
        val = np.exp(-(canThick1Sec1-canThick1Sec2))
        assc = ass * val

        acc1 = f1
        acc2 = f2 * val

        acsc1 = acc1
        acsc2 = acc2 * np.exp(-(sampleSec1 - sampleSec2))
    else:
        val = np.exp(-(canThick1Sec1 + canThick2Sec2))
        assc = ass * val

        acc1 = f1 * np.exp(-(canThick1Sec2 + canThick2Sec2))
        acc2 = f2 * val

        acsc1 = acc1 * np.exp(-sampleSec2)
        acsc2 = acc2 * np.exp(-sampleSec1)

    canThickness = canThickness1 + canThickness2

    if canThickness > 0.0:
        acc = (acc1 + acc2) / canThickness
        acsc = (acsc1 + acsc2) / canThickness

    return assc, acsc, acc
