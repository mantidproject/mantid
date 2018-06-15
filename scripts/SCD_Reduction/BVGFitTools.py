import numpy as np
import matplotlib.pyplot as plt
import ICCFitTools as ICCFT
from mantid.simpleapi import *
from scipy.interpolate import interp1d
from scipy.ndimage.filters import convolve
from matplotlib.mlab import bivariate_normal
import ICConvoluted as ICC
import BivariateGaussian as BivariateGaussian
plt.ion()


def get3DPeak(peak, box, padeCoefficients, qMask, nTheta=150, nPhi=150, fracBoxToHistogram=1.0,
              plotResults=False, zBG=1.96, bgPolyOrder=1, fICCParams=None, oldICCFit=None,
              strongPeakParams=None, forceCutoff=250, edgeCutoff=15, predCoefficients=None,
              neigh_length_m=3, q_frame='sample', dtSpread=0.03, pplmin_frac=0.8, pplmax_frac=1.5, mindtBinWidth=1,
              figureNumber=2):
    n_events = box.getNumEventsArray()

    if q_frame == 'lab':
        q0 = peak.getQLabFrame()
    elif q_frame == 'sample':
        q0 = peak.getQSampleFrame()
    else:
        raise ValueError(
            'BVGFT:get3DPeak - q_frame must be either \'lab\' or \'sample\'; %s was provided' % q_frame)

    if fICCParams is None:
        goodIDX, pp_lambda = ICCFT.getBGRemovedIndices(
                    n_events, peak=peak, box=box, qMask=qMask, calc_pp_lambda=True, padeCoefficients=padeCoefficients,
                    predCoefficients=predCoefficients, neigh_length_m=neigh_length_m, pp_lambda=None, pplmin_frac=pplmin_frac,
                    pplmax_frac=pplmax_frac, mindtBinWidth=mindtBinWidth)

        YTOF, fICC, x_lims = fitTOFCoordinate(
                    box, peak, padeCoefficients, dtSpread=dtSpread, qMask=qMask, bgPolyOrder=bgPolyOrder, zBG=zBG,
                    plotResults=plotResults, pp_lambda=pp_lambda, neigh_length_m=neigh_length_m, pplmin_frac=pplmin_frac,
                    pplmax_frac=pplmax_frac, mindtBinWidth=mindtBinWidth)

    else:  # we already did I-C profile, so we'll just read the parameters
        pp_lambda = fICCParams[-1]
        fICC = ICC.IkedaCarpenterConvoluted()
        fICC.init()
        fICC['A'] = fICCParams[5]
        fICC['B'] = fICCParams[6]
        fICC['R'] = fICCParams[7]
        fICC['T0'] = fICCParams[8]
        fICC['Scale'] = fICCParams[9]
        fICC['HatWidth'] = fICCParams[10]
        fICC['KConv'] = fICCParams[11]
        goodIDX, _ = ICCFT.getBGRemovedIndices(
            n_events, pp_lambda=pp_lambda, qMask=qMask)

        # Get the 3D TOF component, YTOF
        if oldICCFit is not None:
            x_lims = [np.min(oldICCFit[0]), np.max(oldICCFit[0])]
            tofxx = oldICCFit[0]
            tofyy = oldICCFit[2]
        else:
            dtSpread = 0.03
            x_lims = [(1 - dtSpread) * peak.getTOF(),
                      (1 + dtSpread) * peak.getTOF()]
            tofxx = np.arange(x_lims[0], x_lims[1], 5)
            tofyy = fICC.function1D(tofxx)
        ftof = interp1d(tofxx, tofyy, bounds_error=False, fill_value=0.0)
        XTOF = boxToTOFThetaPhi(box, peak)[:, :, :, 0]
        YTOF = ftof(XTOF)

    # Get YBVG - the detector component
    if goodIDX is not None:
        goodIDX *= qMask
    X = boxToTOFThetaPhi(box, peak)
    dEdge = edgeCutoff
    useForceParams = peak.getIntensity() < forceCutoff or peak.getRow() <= dEdge or peak.getRow(
    ) >= 255 - dEdge or peak.getCol() <= dEdge or peak.getCol() >= 255 - dEdge
    if strongPeakParams is not None and useForceParams:  # We will force parameters on this fit
        ph = np.arctan2(q0[1], q0[0])
        th = np.arctan2(q0[2], np.hypot(q0[0], q0[1]))
        phthPeak = np.array([ph, th])
        tmp = strongPeakParams[:, :2] - phthPeak
        distSq = tmp[:, 0]**2 + tmp[:, 1]**2
        nnIDX = np.argmin(distSq)
        #print 'Using [ph, th] =', strongPeakParams[nnIDX,
        #                                           :2], 'for ', phthPeak, '; nnIDX = ', nnIDX
        params, h, t, p = doBVGFit(box, nTheta=nTheta, nPhi=nPhi, fracBoxToHistogram=fracBoxToHistogram,
                                   goodIDX=goodIDX, forceParams=strongPeakParams[nnIDX])
    else:  # Just do the fit - no nearest neighbor assumptions
        params, h, t, p = doBVGFit(
            box, nTheta=nTheta, nPhi=nPhi, fracBoxToHistogram=fracBoxToHistogram, goodIDX=goodIDX)

    if plotResults:
        compareBVGFitData(
            box, params[0], nTheta, nPhi, fracBoxToHistogram=fracBoxToHistogram, useIDX=goodIDX,
            figNumber=figureNumber)

    # set up the BVG
    # A = params[0][0]  # never used
    mu0 = params[0][1]
    mu1 = params[0][2]
    sigX = params[0][3]
    sigY = params[0][4]
    p = params[0][5]
    bgBVG = params[0][6]
    sigma = np.array([[sigX**2, p * sigX * sigY], [p * sigX * sigY, sigY**2]])
    mu = np.array([mu0, mu1])

    XTOF = X[:, :, :, 0]
    XTHETA = X[:, :, :, 1]
    XPHI = X[:, :, :, 2]

    YBVG = bvg(1.0, mu, sigma, XTHETA, XPHI, 0)

    # Do scaling to the data
    Y, redChiSq, scaleFactor = fitScaling(n_events, box, YTOF, YBVG)
    YBVG2 = bvg(1.0, mu, sigma, XTHETA, XPHI, 0)
    YTOF2 = getYTOF(fICC, XTOF, x_lims)
    Y2 = YTOF2 * YBVG2
    Y2 = scaleFactor * Y2 / Y2.max()

    QX, QY, QZ = ICCFT.getQXQYQZ(box)
    fitMaxIDX = tuple(np.array(np.unravel_index(Y2.argmax(), Y2.shape)))
    newCenter = np.array([QX[fitMaxIDX], QY[fitMaxIDX], QZ[fitMaxIDX]])

    # Set a dictionary with the parameters to return
    retParams = {}
    retParams['Alpha'] = fICC['A']
    retParams['Beta'] = fICC['B']
    retParams['R'] = fICC['R']
    retParams['T0'] = fICC['T0']
    retParams['Scale'] = fICC['Scale']
    retParams['KConv'] = fICC['KConv']
    retParams['MuTH'] = mu0
    retParams['MuPH'] = mu1
    retParams['SigX'] = sigX
    retParams['SigY'] = sigY
    retParams['SigP'] = p
    retParams['bgBVG'] = bgBVG
    retParams['scale3d'] = scaleFactor
    retParams['chiSq3d'] = redChiSq
    retParams['dQ'] = np.linalg.norm(newCenter - q0)
    retParams['newQ'] = newCenter

    return Y2, goodIDX, pp_lambda, retParams


def boxToTOFThetaPhi(box, peak):
    QX, QY, QZ = ICCFT.getQXQYQZ(box)
    R, THETA, PHI = ICCFT.cart2sph(QX, QY, QZ)
    flightPath = peak.getL1() + peak.getL2()
    scatteringHalfAngle = 0.5 * peak.getScattering()
    TOF = 3176.507 * flightPath * np.sin(scatteringHalfAngle) / np.abs(R)
    X = np.empty(TOF.shape + (3,))
    X[:, :, :, 0] = TOF
    X[:, :, :, 1] = THETA
    X[:, :, :, 2] = PHI
    return X


def fitScaling(n_events, box, YTOF, YBVG, goodIDX=None, neigh_length_m=3):
    YJOINT = 1.0 * YTOF * YBVG
    YJOINT /= 1.0 * YJOINT.max()

    convBox = 1.0 * \
        np.ones([neigh_length_m, neigh_length_m, neigh_length_m]) / \
        neigh_length_m**3
    conv_n_events = convolve(n_events, convBox)

    QX, QY, QZ = ICCFT.getQXQYQZ(box)
    dP = 8
    fitMaxIDX = tuple(
        np.array(np.unravel_index(YJOINT.argmax(), YJOINT.shape)))
    if goodIDX is None:
        goodIDX = np.zeros_like(YJOINT).astype(np.bool)
        goodIDX[max(fitMaxIDX[0] - dP, 0):min(fitMaxIDX[0] + dP, goodIDX.shape[0]),
                max(fitMaxIDX[1] - dP, 0):min(fitMaxIDX[1] + dP, goodIDX.shape[1]),
                max(fitMaxIDX[2] - dP, 0):min(fitMaxIDX[2] + dP, goodIDX.shape[2])] = True
        goodIDX = np.logical_and(goodIDX, conv_n_events > 0)

    # A1 = slope, A0 = offset
    scaleLinear = Polynomial(n=1)
    scaleLinear.constrain("A1>0")
    scaleX = YJOINT[goodIDX]
    scaleY = n_events[goodIDX]
    # , dataE=np.sqrt(scaleY))
    scaleWS = CreateWorkspace(
        OutputWorkspace='scaleWS', dataX=scaleX, dataY=scaleY)
    fitResultsScaling = Fit(Function=scaleLinear, InputWorkspace=scaleWS,
                            Output='scalefit', CostFunction='Unweighted least squares')
    A0 = fitResultsScaling[3].row(0)['Value']
    A1 = fitResultsScaling[3].row(1)['Value']
    YRET = A1 * YJOINT + A0
    chiSqRed = fitResultsScaling[1]

    #print chiSqRed, 'is chiSqRed'
    return YRET, chiSqRed, A1


def getXTOF(box, peak):
    from mantid.kernel import V3D
    QX, QY, QZ = ICCFT.getQXQYQZ(box)
    origQS = peak.getQSampleFrame()
    tList = np.zeros_like(QX)
    for i in xrange(QX.shape[0]):
        for j in xrange(QX.shape[1]):
            for k in xrange(QX.shape[2]):
                newQ = V3D(QX[i, j, k], QY[i, j, k], QZ[i, j, k])
                peak.setQSampleFrame(newQ)
                flightPath = peak.getL1() + peak.getL2()
                scatteringHalfAngle = 0.5 * peak.getScattering()
                # convert to microseconds)
                tList[i, j, k] = 3176.507 * flightPath * \
                    np.sin(scatteringHalfAngle) / np.linalg.norm(newQ)
    peak.setQSampleFrame(origQS)
    return tList


def fitTOFCoordinate(box, peak, padeCoefficients, dtSpread=0.03, minFracPixels=0.01,
                     neigh_length_m=3, zBG=1.96, bgPolyOrder=1, qMask=None, plotResults=False,
                     fracStop=0.01, pp_lambda=None, pplmin_frac=0.8, pplmax_frac=1.5, mindtBinWidth=1):

    # Get info from the peak
    tof = peak.getTOF()  # in us
    wavelength = peak.getWavelength()  # in Angstrom
    flightPath = peak.getL1() + peak.getL2()  # in m
    scatteringHalfAngle = 0.5 * peak.getScattering()
    energy = 81.804 / wavelength**2 / 1000.0  # in eV

    # Set the qMask
    if qMask is None:
        qMask = np.ones_like(box.getNumEventsArray()).astype(np.bool)

    # Calculate the optimal pp_lambda and
    tofWS, ppl = ICCFT.getTOFWS(box, flightPath, scatteringHalfAngle, tof, peak, qMask,
                                dtSpread=dtSpread, minFracPixels=minFracPixels,
                                neigh_length_m=neigh_length_m, zBG=zBG, pp_lambda=pp_lambda,
                                pplmin_frac=pplmin_frac, pplmax_frac=pplmax_frac,
                                mindtBinWidth=mindtBinWidth)

    fitResults, fICC = ICCFT.doICCFit(tofWS, energy, flightPath,
                                      padeCoefficients, fitOrder=bgPolyOrder, constraintScheme=1)

    for i, param in enumerate(['A', 'B', 'R', 'T0', 'Scale', 'HatWidth', 'KConv']):
        fICC[param] = mtd['fit_Parameters'].row(i)['Value']
    bgParamsRows = [7 + i for i in range(bgPolyOrder + 1)]
    bgCoeffs = []
    for bgRow in bgParamsRows[::-1]:  # reverse for numpy order
        bgCoeffs.append(mtd['fit_Parameters'].row(bgRow)['Value'])
    x = tofWS.readX(0)
    yFit = mtd['fit_Workspace'].readY(1)

    interpF = interp1d(x, yFit, kind='cubic')
    tofxx = np.linspace(tofWS.readX(0).min(), tofWS.readX(0).max(), 1000)
    tofyy = interpF(tofxx)
    if plotResults:
        plt.figure(1)
        plt.clf()
        plt.plot(tofxx, tofyy, label='Interpolated')
        plt.plot(tofWS.readX(0), tofWS.readY(0), 'o', label='Data')
        #print 'sum:', np.sum(fICC.function1D(tofWS.readX(0)))
        #print 'bg: ', np.sum(bg[iStart:iStop])
        plt.plot(mtd['fit_Workspace'].readX(1),
                 mtd['fit_Workspace'].readY(1), label='Fit')
        plt.title(fitResults.OutputChi2overDoF)
        plt.legend(loc='best')
    ftof = interp1d(tofxx, tofyy, bounds_error=False, fill_value=0.0)
    XTOF = boxToTOFThetaPhi(box, peak)[:, :, :, 0]
    YTOF = ftof(XTOF)
    return YTOF, fICC, [tofWS.readX(0).min(), tofWS.readX(0).max()]


def getYTOF(fICC, XTOF, xlims):
    tofxx = np.linspace(xlims[0], xlims[1], 1000)
    tofyy = fICC.function1D(tofxx)
    ftof = interp1d(tofxx, tofyy, bounds_error=False, fill_value=0.0)
    YTOF = ftof(XTOF)
    return YTOF


def getAngularHistogram(box, useIDX=None, nTheta=200, nPhi=200, zBG=1.96, neigh_length_m=3, fracBoxToHistogram=1.0):
    n_events = box.getNumEventsArray()
    hasEventsIDX = n_events > 0
    if useIDX is None:
        if zBG >= 0:
            goodIDX, pp_lambda = ICCFT.getBGRemovedIndices(n_events)
        else:
            goodIDX = hasEventsIDX

        useIDX = goodIDX

    # Setup our coordinates
    QX, QY, QZ = ICCFT.getQXQYQZ(box)
    R, THETA, PHI = ICCFT.cart2sph(QX, QY, QZ)

    thetaMin = np.min(THETA)
    thetaMax = np.max(THETA)
    dTheta = thetaMax - thetaMin
    thetaMid = 0.5 * (thetaMin + thetaMax)
    thetaMin = max(thetaMin, thetaMid - dTheta * fracBoxToHistogram / 2.0)
    thetaMax = min(thetaMax, thetaMid + dTheta * fracBoxToHistogram / 2.0)

    phiMin = np.min(PHI)
    phiMax = np.max(PHI)
    dPhi = phiMax - phiMin
    phiMid = 0.5 * (phiMin + phiMax)
    phiMin = max(phiMin, phiMid - dPhi * fracBoxToHistogram / 2.0)
    phiMax = min(phiMax, phiMid + dPhi * fracBoxToHistogram / 2.0)

    thetaBins = np.linspace(thetaMin, thetaMax, nTheta)
    phiBins = np.linspace(phiMin, phiMax, nPhi)
    thetaVect = THETA[useIDX]
    phiVect = PHI[useIDX]
    nVect = n_events[useIDX]

    # Do the histogram
    h, thBins, phBins = np.histogram2d(
        thetaVect, phiVect, weights=nVect, bins=[thetaBins, phiBins])
    return h, thBins, phBins


def getBVGResult(box, params, nTheta=200, nPhi=200, fracBoxToHistogram=1.0):
    h, thBins, phBins = getAngularHistogram(
        box, nTheta=nTheta, nPhi=nPhi, fracBoxToHistogram=fracBoxToHistogram)
    thCenters = 0.5 * (thBins[1:] + thBins[:-1])
    phCenters = 0.5 * (phBins[1:] + phBins[:-1])
    TH, PH = np.meshgrid(thCenters, phCenters, indexing='ij', copy=False)

    # Set our initial guess
    m = BivariateGaussian.BivariateGaussian()
    m.init()
    m['A'] = params[0]
    m['MuX'] = params[1]
    m['MuY'] = params[2]
    m['SigX'] = params[3]
    m['SigY'] = params[4]
    m['SigP'] = params[5]
    m['Bg'] = params[6]
    m.setAttributeValue('nX', h.shape[0])
    m.setAttributeValue('nY', h.shape[1])

    pos = np.empty(TH.shape + (2,))
    pos[:, :, 0] = TH
    pos[:, :, 1] = PH

    Y = m.function2D(pos)
    return Y


def compareBVGFitData(box, params, nTheta=200, nPhi=200, figNumber=2, fracBoxToHistogram=1.0, useIDX=None):
    '''
    compareBVGFitData is used for comparing a fit and the histogram.  Useful for debugging.
    '''
    h, thBins, phBins = getAngularHistogram(
        box, nTheta=nTheta, nPhi=nPhi, fracBoxToHistogram=fracBoxToHistogram, useIDX=useIDX)
    Y = getBVGResult(box, params, nTheta=nTheta, nPhi=nPhi,
                     fracBoxToHistogram=fracBoxToHistogram)
    pLow = 0.0
    pHigh = 1.0
    nX, nY = Y.shape
    plt.figure(figNumber)
    plt.clf()
    plt.subplot(2, 2, 1)
    plt.imshow(h, vmin=0, vmax=0.7 * np.max(h), interpolation='None')
    plt.xlim([pLow * nX, pHigh * nX])
    plt.ylim([pLow * nY, pHigh * nY])
    if useIDX is None:
        plt.title('Measured Peak')
    else:
        plt.title('BG Removed Measured Peak')
    plt.colorbar()
    plt.subplot(2, 2, 2)
    plt.imshow(Y, vmin=0, vmax=0.7 * np.max(h), interpolation='None')
    plt.title('Modeled Peak')
    plt.xlim([pLow * nX, pHigh * nX])
    plt.ylim([pLow * nY, pHigh * nY])
    plt.colorbar()
    plt.subplot(2, 2, 3)
    plt.imshow(h - Y, interpolation='None')
    plt.xlim([pLow * nX, pHigh * nX])
    plt.ylim([pLow * nY, pHigh * nY])
    plt.xlabel('Difference')
    plt.colorbar()

    if useIDX is not None:
        h0, thBins, phBins = getAngularHistogram(
            box, nTheta=nTheta, nPhi=nPhi, fracBoxToHistogram=fracBoxToHistogram, useIDX=None)
        plt.subplot(2, 2, 4)
        plt.imshow(h0, vmin=0, vmax=1.0 * np.max(h0), interpolation='None')
        plt.xlim([pLow * nX, pHigh * nX])
        plt.ylim([pLow * nY, pHigh * nY])
        plt.xlabel('Measured Peak')
        plt.colorbar()


def doBVGFit(box, nTheta=200, nPhi=200, zBG=1.96, fracBoxToHistogram=1.0, goodIDX=None,
             forceParams=None, forceTolerance=0.1, dth=10, dph=10):
    """
    doBVGFit takes a binned MDbox and returns the fit of the peak shape along the non-TOF direction.  This is done in one of two ways:
        1) Standard least squares fit of the 2D histogram.
        2) Forcing a set of parameters.  Under this, parameters are tightly constrained.  The peak center may move by (dth, dph) from
        predicted position (in units of histogram pixels) and sigma parameters can change by a factor of forceTolerance.
    Input:
        box: a binned 'MDbox'.
        nTheta, nPhi: integer, number of bins to use when creating 2D BVG histogram
        zBG: Z score at which we consider events to be above BG #TODO: I think this can be removed since we pass in goodIDX?
        fracBoxToHistrogram: Leave at 1.0 to histogram whole box.  Any values lower will remove the edges of box before
                histogramming.
        goodIDX: a numpy array of shape box.getNumEventsArray().shape.  True for voxels we will histogram (i.e. False if the
                events in this voxel are background.)
        forceParams: set of parameters to force.  These are the same format as a row in strongPeaksParams
        forceTolerance: the factor we allow sigX, sigY, sigP to change when forcing peaks.  Not used if forceParams is None.
        dth, dph: The peak center may move by (dth, dph) from predicted position (in units of histogram pixels).

    """
    h, thBins, phBins = getAngularHistogram(
        box, nTheta=nTheta, nPhi=nPhi, zBG=zBG, fracBoxToHistogram=fracBoxToHistogram, useIDX=goodIDX)
    thCenters = 0.5 * (thBins[1:] + thBins[:-1])
    phCenters = 0.5 * (phBins[1:] + phBins[:-1])
    TH, PH = np.meshgrid(thCenters, phCenters, indexing='ij', copy=False)

    weights = np.sqrt(h)
    weights[weights < 1] = 1

    pos = np.empty(TH.shape + (2,))
    pos[:, :, 0] = TH
    pos[:, :, 1] = PH

    H = np.empty(h.shape + (2,))
    H[:, :, 0] = h
    H[:, :, 1] = h

    def fSigP(x, a, k, phi, b):
        return a * np.sin((k * x) - phi) + b * x

    if forceParams is None:
        meanTH = TH.mean()
        meanPH = PH.mean()
        # sigX0 = 0.0018
        # sigX0 = 0.002#ICCFT.oldScatFun(meanPH, 1.71151521e-02,   6.37218400e+00,   3.39439675e-03)
        sigX0 = ICCFT.oldScatFun(
            meanPH, 1.71151521e-02, 6.37218400e+00, 3.39439675e-03)
        sigY0 = 0.0025
        sigP0 = fSigP(meanTH, 0.1460775, 1.85816592,
                      0.26850086, -0.00725352)

        # Set some constraints
        boundsDict = {}
        boundsDict['A'] = [0.0, np.inf]
        boundsDict['MuX'] = [thBins[thBins.size // 2 - dth],
                             thBins[thBins.size // 2 + dth]]
        boundsDict['MuY'] = [phBins[phBins.size // 2 - dph],
                             phBins[phBins.size // 2 + dph]]
        # boundsDict['sigX'] = [0.7*sigX0, 1.3*sigX0]
        boundsDict['SigX'] = [0., 0.02]
        boundsDict['SigY'] = [0., 0.02]
        boundsDict['SigP'] = [-1., 1.]
        boundsDict['Bg'] = [0, np.inf]

        # Set our initial guess
        m = BivariateGaussian.BivariateGaussian()
        m.init()
        m['A'] = 1.
        #m['MuX'] = meanTH
        #m['MuY'] = meanPH
        m['MuX'] = TH[np.unravel_index(h.argmax(), h.shape)]
        m['MuY'] = PH[np.unravel_index(h.argmax(), h.shape)]
        m['SigX'] = sigX0
        m['SigY'] = sigY0
        m['SigP'] = sigP0
        m.setAttributeValue('nX', h.shape[0])
        m.setAttributeValue('nY', h.shape[1])
        m.setConstraints(boundsDict)
        # print 'before: '
        # print(m)
        # Do the fit
        bvgWS = CreateWorkspace(OutputWorkspace='bvgWS', DataX=pos.ravel(
        ), DataY=H.ravel(), DataE=np.sqrt(H.ravel()))

        fitResults = Fit(Function=m, InputWorkspace='bvgWS', Output='bvgfit',
                         Minimizer='Levenberg-MarquardtMD')

        #print 'after'
        #print m
    elif forceParams is not None:
        p0 = np.zeros(7)
        p0[0] = np.max(h)
        p0[1] = TH.mean()
        p0[2] = PH.mean()
        p0[3] = forceParams[5]
        p0[4] = forceParams[6]
        p0[5] = forceParams[7]

        # Set some constraints
        isPos = np.sign(p0)
        bounds = ((1.0 - isPos * forceTolerance) * p0, (1.0 + isPos * forceTolerance) * p0)
        bounds[0][0] = 0.0
        bounds[1][0] = np.inf  # Amplitude
        bounds[0][1] = min(thBins[thBins.size // 2 - dth],
                           thBins[thBins.size // 2 + dth])
        bounds[1][1] = max(thBins[thBins.size // 2 - dth],
                           thBins[thBins.size // 2 + dth])
        bounds[0][2] = min(phBins[phBins.size // 2 - dph],
                           phBins[phBins.size // 2 + dph])
        bounds[1][2] = max(phBins[phBins.size // 2 - dph],
                           phBins[phBins.size // 2 + dph])
        bounds[1][-1] = np.inf

        boundsDict = {}
        boundsDict['A'] = [0.0, np.inf]
        boundsDict['MuX'] = [thBins[thBins.size // 2 - dth],
                             thBins[thBins.size // 2 + dth]]
        boundsDict['MuY'] = [phBins[phBins.size // 2 - dph],
                             phBins[phBins.size // 2 + dph]]
        boundsDict['SigX'] = [bounds[0][3], bounds[1][3]]
        boundsDict['SigY'] = [bounds[0][4], bounds[1][4]]
        boundsDict['SigP'] = [bounds[0][5], bounds[1][5]]
        # Set our initial guess
        m = BivariateGaussian.BivariateGaussian()
        m.init()
        m['A'] = 0.1
        #m['muX'] = np.average(thCenters,weights=np.sum(h,axis=1))
        #m['muY'] = np.average(phCenters,weights=np.sum(h,axis=0))

        #m['muX'] = TH.mean()
        #m['muY'] = PH.mean()
        m['MuX'] = TH[np.unravel_index(h.argmax(), h.shape)]
        m['MuY'] = PH[np.unravel_index(h.argmax(), h.shape)]
        m['SigX'] = forceParams[5]
        m['SigY'] = forceParams[6]
        m['SigP'] = forceParams[7]
        m.setAttributeValue('nX', h.shape[0])
        m.setAttributeValue('nY', h.shape[1])
        m.setConstraints(boundsDict)
        #print 'before:'
        #print m

        # Do the fit
        #plt.figure(18); plt.clf(); plt.imshow(m.function2D(pos)); plt.title('BVG Initial guess')
        bvgWS = CreateWorkspace(OutputWorkspace='bvgWS', DataX=pos.ravel(), DataY=H.ravel(), DataE=np.sqrt(H.ravel()))
        fitFun = m
        fitResults = Fit(Function=fitFun, InputWorkspace=bvgWS,
                         Output='bvgfit', Minimizer='Levenberg-MarquardtMD')

        #print 'after:'
        #print m
    # Recover the result
    m = BivariateGaussian.BivariateGaussian()
    m.init()
    m['A'] = mtd['bvgfit_Parameters'].row(0)['Value']
    m['MuX'] = mtd['bvgfit_Parameters'].row(1)['Value']
    m['MuY'] = mtd['bvgfit_Parameters'].row(2)['Value']
    m['SigX'] = mtd['bvgfit_Parameters'].row(3)['Value']
    m['SigY'] = mtd['bvgfit_Parameters'].row(4)['Value']
    m['SigP'] = mtd['bvgfit_Parameters'].row(5)['Value']
    m['Bg'] = mtd['bvgfit_Parameters'].row(6)['Value']

    m.setAttributeValue('nX', h.shape[0])
    m.setAttributeValue('nY', h.shape[1])
    chiSq = fitResults[1]
    params = [[m['A'], m['MuX'], m['MuY'], m['SigX'],
               m['SigY'], m['SigP'], m['Bg']], chiSq]
    # print params
    return params, h, thBins, phBins


def is_pos_def(x):  # Checks if matrix x is positive definite
    return np.all(np.linalg.eigvals(x) > 0)


def bvg(A, mu, sigma, x, y, bg):
    """
    bvg is the bivariate gaussian.  This function is a convenient wrapper for
    multivariate_normal.
    Intput:
        A: amplitude
        mu: 2 element array containing [muX, muY]
        sigma: SIGMA matrix [[sigX**2, sigX*sigY*sigP],[sigX*sigY*sigP,sigY**2]]
        x: numy array containing the x coordinates (e.g. theta for detector space)
        y: numy array containing the y coordinates (e.g. phi_az for detector space)
        bg: constant for the background
    Output:
        a numpy array with the same shape as x.  If sigma is not positive-definite,
        this array will contain all zeros.  Otherwise, the BVG will be evaluated
        at each point at the value is returned.
    """

    if is_pos_def(sigma):
        f = bivariate_normal(x, y, sigmax=np.sqrt(sigma[0, 0]), sigmay=np.sqrt(sigma[1, 1]),
                             sigmaxy=sigma[1, 0], mux=mu[0], muy=mu[1])
        return A * f + bg
    else:
        print '   BVGFT:bvg:not PSD Matrix'
        return 0.0 * np.ones_like(x)
