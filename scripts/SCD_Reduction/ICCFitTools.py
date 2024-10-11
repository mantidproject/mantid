# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import matplotlib.pyplot as plt
import numpy as np
import sys
from scipy.special import factorial
from scipy.optimize import curve_fit
from mantid.api import mtd
from mantid.simpleapi import BinMD, ConvertToMD, CreateWorkspace, Fit, FunctionWrapper, Load, LoadIsawDetCal, Polynomial
from mantid.kernel import logger, V3D
import ICConvoluted as ICC
import itertools
from functools import reduce
from scipy.ndimage.filters import convolve

plt.ion()


def parseConstraints(peaks_ws):
    """
    returns a dictionary containing parameters for ICC fitting. Parameters
    are derived from instrument parameters files (see MANDI_Parameters.xml
    for an example).
    """
    possibleKeys = ["iccA", "iccB", "iccR", "iccT0", "iccScale0", "iccHatWidth", "iccKConv"]
    d = {}
    for paramName in possibleKeys:
        if peaks_ws.getInstrument().hasParameter(paramName):
            vals = np.array(peaks_ws.getInstrument().getStringParameter(paramName)[0].split(), dtype=float)
            d[paramName] = vals
    return d


def scatFun(x, A, bg):
    """
    scatFun: returns A/x+bg.  Used for background estimation.
    """
    return A / x + bg


def oldScatFun(x, A, k, bg):
    """
    oldScatFun: returns 1.0*A*np.exp(-k*x) + bg.  Used for background estimation.
    """
    return 1.0 * A * np.exp(-1.0 * k * x) + bg


def calcSomeTOF(box, peak, refitIDX=None, q_frame="sample"):
    xaxis = box.getXDimension()
    qx = np.linspace(xaxis.getMinimum(), xaxis.getMaximum(), xaxis.getNBins())
    yaxis = box.getYDimension()
    qy = np.linspace(yaxis.getMinimum(), yaxis.getMaximum(), yaxis.getNBins())
    zaxis = box.getZDimension()
    qz = np.linspace(zaxis.getMinimum(), zaxis.getMaximum(), zaxis.getNBins())
    QX, QY, QZ = getQXQYQZ(box)

    if refitIDX is None:
        refitIDX = np.ones_like(QX).astype(bool)

    if q_frame == "lab":
        qS0 = peak.getQLabFrame()
    elif q_frame == "sample":
        qS0 = peak.getQSampleFrame()
    else:
        raise ValueError("ICCFT:calcSomeTOF - q_frame must be either 'lab' or 'sample'; %s was provided" % q_frame)
    PIXELFACTOR = np.ones_like(QX) * (peak.getL1() + peak.getL2()) * np.sin(0.5 * peak.getScattering())
    for i, x in enumerate(qx):
        for j, y in enumerate(qy):
            for k, z in enumerate(qz):
                if refitIDX[i, j, k]:
                    qNew = V3D(x, y, z)
                    peak.setQSampleFrame(qNew)
                    L = peak.getL1() + peak.getL2()
                    HALFSCAT = 0.5 * peak.getScattering()
                    PIXELFACTOR[i, j, k] = L * np.sin(HALFSCAT)
    peak.setQSampleFrame(qS0)

    tofBox = 3176.507 * PIXELFACTOR * 1.0 / np.sqrt(QX**2 + QY**2 + QZ**2)
    return tofBox


def cart2sph(x, y, z):
    """
    cart2sph takes in spherical coordinates (x,y,z) and returns
    spherical coordinates (r,phi,theta)
    """
    hxy = np.hypot(x, y)
    r = np.hypot(hxy, z)
    el = np.arctan2(z, hxy)
    az = np.arctan2(y, x)
    return r, az, el


def getQXQYQZ(box):
    """
    getQXQYQZ - returns meshed coordinates from an MDHistoWorkspace
    input: box - a 3D MDHistoWorkspace of a*b*c bins
    output: QX,QY,QZ - a*b*c numpy arrays containing evenly spaced coordinates
                        over the range of each axis.
    """
    xaxis = box.getXDimension()
    qx = np.linspace(xaxis.getMinimum(), xaxis.getMaximum(), xaxis.getNBins())
    yaxis = box.getYDimension()
    qy = np.linspace(yaxis.getMinimum(), yaxis.getMaximum(), yaxis.getNBins())
    zaxis = box.getZDimension()
    qz = np.linspace(zaxis.getMinimum(), zaxis.getMaximum(), zaxis.getNBins())
    QX, QY, QZ = np.meshgrid(qx, qy, qz, indexing="ij", copy=False)
    return QX, QY, QZ


def getQuickTOFWS(
    box,
    peak,
    padeCoefficients,
    goodIDX=None,
    dtSpread=0.03,
    qMask=None,
    pp_lambda=None,
    minppl_frac=0.8,
    maxppl_frac=1.5,
    mindtBinWidth=1,
    maxdtBinWidth=50,
    constraintScheme=1,
    peakMaskSize=5,
    iccFitDict=None,
    fitPenalty=None,
):
    """
    getQuickTOFWS - generates a quick-and-dirty TOFWS.  Useful for determining the background.
    Input:
        box: the MDHistoWorkspace for the peak we are fitting
        peak: the peak object we are trying to fit
        padeCoefficients: the dictionary containing coefficients for the Pade coefficients describing thd moderator.
        goodIDX - which indices to consider.  Should be a numpy array with the same shape as box.  Will use all
                    voxels if this is None.
        dtSpread - how far on each side of the peak TOF to consider.
        qMask - which voxels to consider as a result of only keeping (h-eta, k-eta,l-eta) to (h+eta, k+eta, l+eta)
        pp_lambda - nominal background level.  Will be calculated if set to None.
        minppl_frac, maxppl_frac: fraction of the predicted pp_lambda to try if calculating pp_lambda
        mindtBinWidth - the minimum binwidth (in us) that we will allow when histogramming.
        maxdtBinWidth - the maximum binwidth (in us) that we will allow when histogramming.
        constraintScheme - which constraint scheme we use.  Typically set to 1
        iccFitDict - a dictionary containing ICC fit constraints and possibly initial guesses
    Output:
        chiSq - reduced chiSquared from fitting the TOF profile
        h - list of [Y, X], with Y and X being numpy arrays of the Y and X of the tof profile
        intensity - fitted intensity
        sigma - fitted sig(int)
    """
    tof = peak.getTOF()  # in us
    wavelength = peak.getWavelength()  # in Angstrom
    flightPath = peak.getL1() + peak.getL2()  # in m
    scatteringHalfAngle = 0.5 * peak.getScattering()
    energy = 81.804 / wavelength**2 / 1000.0  # in eV
    if qMask is None:
        qMask = np.ones_like(box.getNumEventsArray()).astype(bool)

    calc_pp_lambda = False
    if pp_lambda is None:
        calc_pp_lambda = True

    tofWS, ppl = getTOFWS(
        box,
        flightPath,
        scatteringHalfAngle,
        tof,
        peak,
        qMask,
        dtSpread=dtSpread,
        minFracPixels=0.01,
        neigh_length_m=3,
        zBG=1.96,
        pp_lambda=pp_lambda,
        calc_pp_lambda=calc_pp_lambda,
        pplmin_frac=minppl_frac,
        pplmax_frac=minppl_frac,
        mindtBinWidth=mindtBinWidth,
        maxdtBinWidth=maxdtBinWidth,
        peakMaskSize=peakMaskSize,
        iccFitDict=iccFitDict,
        fitPenalty=fitPenalty,
    )
    fitResults, fICC = doICCFit(
        tofWS,
        energy,
        flightPath,
        padeCoefficients,
        fitOrder=1,
        constraintScheme=constraintScheme,
        iccFitDict=iccFitDict,
        fitPenalty=fitPenalty,
    )
    h = [tofWS.readY(0), tofWS.readX(0)]
    chiSq = fitResults.OutputChi2overDoF

    r = mtd["fit_Workspace"]
    param = mtd["fit_Parameters"]
    n_events = box.getNumEventsArray()

    iii = fICC.numParams() - 1
    fitBG = [param.row(int(iii + i + 1))["Value"] for i in range(1 + 1)]

    # Set the intensity before moving on to the next peak
    icProfile = r.readY(1)
    bgCoefficients = fitBG[::-1]

    intensity, sigma, xStart, xStop = integratePeak(
        r.readX(0),
        icProfile,
        r.readY(0),
        np.polyval(bgCoefficients, r.readX(1)),
        pp_lambda=pp_lambda,
        fracStop=0.01,
        totEvents=np.sum(n_events[goodIDX * qMask]),
        bgEvents=np.sum(goodIDX * qMask) * pp_lambda,
        varFit=chiSq,
    )

    return chiSq, h, intensity, sigma


def getPoissionGoodIDX(n_events, zBG=1.96, neigh_length_m=3):
    """
    getPoissionGoodIDX - returns a numpy arrays which is true if the voxel contains events at
            the zBG z level (1.96=95%CI).  This is based only on Poission statistics.
    Input:
        n_events: 3D numpy array containing counts
        zBG: the z value at which we will set the cutoff
        neigh_lengh_m: the number of voxels we will smooth over (via np.convolve)
    Output:
        goodIDX: a numpy arrays the same size as n_events that is True of False for if it contains
                counts at a statistically significant level.

        pp_lambda: the most likely number of background events
    """
    hasEventsIDX = n_events > 0
    # Set up some things to only consider good pixels
    # Set to zero for "this pixel only" mode - performance is optimized for neigh_length_m=0
    neigh_length_m = neigh_length_m

    # Get the most probably number of events
    pp_lambda = get_pp_lambda(n_events, hasEventsIDX)
    found_pp_lambda = False
    convBox = 1.0 * np.ones([neigh_length_m, neigh_length_m, neigh_length_m]) / neigh_length_m**3
    conv_n_events = convolve(n_events, convBox)
    allEvents = np.sum(n_events[hasEventsIDX])
    if allEvents > 0:
        while not found_pp_lambda:
            goodIDX = np.logical_and(hasEventsIDX, conv_n_events > pp_lambda + zBG * np.sqrt(pp_lambda / (2 * neigh_length_m + 1) ** 3))
            boxMean = n_events[goodIDX]
            if allEvents > np.sum(boxMean):
                found_pp_lambda = True
            else:
                pp_lambda *= 1.05
    return goodIDX, pp_lambda


def getOptimizedGoodIDX(
    n_events,
    padeCoefficients,
    zBG=1.96,
    neigh_length_m=3,
    qMask=None,
    peak=None,
    box=None,
    pp_lambda=None,
    peakNumber=-1,
    minppl_frac=0.8,
    maxppl_frac=1.5,
    mindtBinWidth=1,
    maxdtBinWidth=50,
    constraintScheme=1,
    peakMaskSize=5,
    iccFitDict=None,
    fitPenalty=None,
):
    """
    getOptimizedGoodIDX - returns a numpy arrays which is true if the voxel contains events at
            the zBG z level (1.96=95%CI).  Rather than using Poission statistics, this function
            will determine the optimal background level by trying to fit the TOF profile
            at each possible background level and returning the one which is best fit by the
            predicted moderator output (as described by the padeCoefficients.)
    Input:
        n_events - 3D numpy array containing counts
        padeCoefficients - A dictionary of Pade coefficients describing moderator emission.
        zBG - the z value at which we will set the cutoff
        neigh_lengh_m: the number of voxels we will smooth over (via np.convolve)
        qMask - the voxels of n_events to consider based on keeping only a fraction around (h,k,l)
        peak - the IPeak object for the peak we're trying to find the local background of.
        box - a 3D MDHisto workspace containing the intensity of each voxel for the peak we're fitting.
        pp_lambda - Currently unused.  Leave as None. TODO: remove this.
        peakNumber - currently unused.  TODO: Remove this.
        minppl_frac, maxppl_frac; range around predicted pp_lambda to check.
        mindtBinWidth - the smallest dt (in us) allowed for constructing the TOF profile.
        mindtBinWidth - the largest dt (in us) allowed for constructing the TOF profile.
        constraintScheme - sets the constraints for TOF profile fitting.  Leave as 1 if you're
                not sure how to modify this.
        iccFitDict - a dictionary containing ICC fit constraints and possibly initial guesses

    Output:
        goodIDX: a numpy arrays the same size as n_events that is True of False for if it contains
                counts to be included in the TOF profile.
        pp_lambda: the most likely number of background events
    """
    # Set up some things to only consider good pixels
    hasEventsIDX = n_events > 0
    # Set to zero for "this pixel only" mode - performance is optimized for neigh_length_m=0
    neigh_length_m = neigh_length_m
    convBox = 1.0 * np.ones([neigh_length_m, neigh_length_m, neigh_length_m]) / neigh_length_m**3
    conv_n_events = convolve(n_events, convBox)
    # Get the most probable number of events
    pp_lambda = get_pp_lambda(n_events, hasEventsIDX)

    pp_lambda_toCheck = np.unique(conv_n_events)
    pp_lambda_toCheck = pp_lambda_toCheck[1:][np.diff(pp_lambda_toCheck) > 0.001]

    # Get the average background level
    nX, nY, nZ = n_events.shape
    cX = nX // 2
    cY = nY // 2
    cZ = nZ // 2
    dP = peakMaskSize

    peakMask = qMask.copy()
    peakMask[cX - dP : cX + dP, cY - dP : cY + dP, cZ - dP : cZ + dP] = 0
    neigh_length_m = 3
    convBox = 1.0 * np.ones([neigh_length_m, neigh_length_m, neigh_length_m]) / neigh_length_m**3
    conv_n_events = convolve(n_events, convBox)
    bgMask = np.logical_and(conv_n_events > 0, peakMask > 0)
    meanBG = np.mean(n_events[bgMask])

    # We set it at slightly lower than 1:1 because we don't
    # want to fit a peak where most of the counts have been removed.
    # The factor of 1.96 comes from a historical definition of pp_lambda
    # where background was considered at the 95% confidence interval.
    pred_ppl = np.polyval([0.98, 0], meanBG) * 1.96
    minppl = minppl_frac * pred_ppl
    maxppl = maxppl_frac * pred_ppl
    pp_lambda_toCheck = pp_lambda_toCheck[pp_lambda_toCheck > minppl]
    pp_lambda_toCheck = pp_lambda_toCheck[pp_lambda_toCheck < maxppl]
    if len(pp_lambda_toCheck) == 0:
        pp_lambda_toCheck = [meanBG * 1.96]
        print("Cannot find suitable background.  Consider adjusting MinpplFrac or MaxpplFrac")

    chiSqList = 1.0e30 * np.ones_like(pp_lambda_toCheck)
    ISIGList = 1.0e-30 * np.ones_like(pp_lambda_toCheck)
    IList = 1.0e-30 * np.ones_like(pp_lambda_toCheck)
    oldGoodIDXSum = -1.0
    for i, pp_lambda in enumerate(pp_lambda_toCheck):
        try:
            goodIDX = np.logical_and(hasEventsIDX, conv_n_events > pp_lambda + zBG * np.sqrt(pp_lambda / (2 * neigh_length_m + 1) ** 3))
            if np.sum(goodIDX) == oldGoodIDXSum:  # No new points removed, we skip this
                continue
            else:
                oldGoodIDXSum = np.sum(goodIDX)
            try:
                chiSq, h, intens, sigma = getQuickTOFWS(
                    box,
                    peak,
                    padeCoefficients,
                    goodIDX=goodIDX,
                    qMask=qMask,
                    pp_lambda=pp_lambda,
                    minppl_frac=minppl_frac,
                    maxppl_frac=maxppl_frac,
                    mindtBinWidth=mindtBinWidth,
                    maxdtBinWidth=maxdtBinWidth,
                    constraintScheme=constraintScheme,
                    peakMaskSize=peakMaskSize,
                    iccFitDict=iccFitDict,
                    fitPenalty=fitPenalty,
                )
            except:
                # raise
                break
            chiSqList[i] = chiSq
            ISIGList[i] = intens / sigma
            IList[i] = intens
            # hList.append((pp_lambda, chiSq, h))
            # or np.sum(h[0])<10: #or (chiSq > 100 and np.min(chiSqList)<5):
            if len(h[0]) < 10:
                break
        except RuntimeError:
            # This is caused by there being fewer datapoints remaining than parameters.  For now, we just hope
            # we found a satisfactory answer.
            raise
            break
        except KeyboardInterrupt:
            sys.exit()
    use_ppl = np.argmin(np.abs(chiSqList[: i + 1] - 1.0))
    pp_lambda = pp_lambda_toCheck[use_ppl]
    goodIDX, _ = getBGRemovedIndices(n_events, pp_lambda=pp_lambda)
    chiSq, h, intens, sigma = getQuickTOFWS(
        box,
        peak,
        padeCoefficients,
        goodIDX=goodIDX,
        qMask=qMask,
        pp_lambda=pp_lambda,
        minppl_frac=minppl_frac,
        maxppl_frac=maxppl_frac,
        mindtBinWidth=mindtBinWidth,
        maxdtBinWidth=maxdtBinWidth,
        peakMaskSize=peakMaskSize,
        iccFitDict=iccFitDict,
        fitPenalty=fitPenalty,
    )
    if qMask is not None:
        return goodIDX * qMask, pp_lambda
    return goodIDX, pp_lambda


def getBGRemovedIndices(
    n_events,
    zBG=1.96,
    calc_pp_lambda=False,
    neigh_length_m=3,
    qMask=None,
    peak=None,
    box=None,
    pp_lambda=None,
    peakNumber=-1,
    padeCoefficients=None,
    pplmin_frac=0.8,
    pplmax_frac=1.5,
    mindtBinWidth=1,
    maxdtBinWidth=50,
    constraintScheme=1,
    peakMaskSize=5,
    iccFitDict=None,
    fitPenalty=None,
):
    """
    getBGRemovedIndices - A wrapper for getOptimizedGoodIDX
    Input:
        n_events - 3D numpy array containing counts
        padeCoefficients - A dictionary of Pade coefficients describing moderator emission.
        zBG - the z value at which we will set the cutoff
        calc_pp_lambda - a boolean for if pp_lambda should be calculated
        neigh_lengh_m: the number of voxels we will smooth over (via np.convolve)
        qMask - the voxels of n_events to consider based on keeping only a fraction around (h,k,l)
        peak - the IPeak object for the peak we're trying to find the local background of.
        box - a 3D MDHisto workspace containing the intensity of each voxel for the peak we're fitting.
        pp_lambda - Currently unused.  Leave as None. TODO: remove this.
        peakNumber - currently unused.  TODO: Remove this.
        minppl_frac, maxppl_frac; range around predicted pp_lambda to check.
        mindtBinWidth - the smallest dt (in us) allowed for constructing the TOF profile.
        maxdtBinWidth - the largest dt (in us) allowed for constructing the TOF profile.
        constraintScheme - sets the constraints for TOF profile fitting.  Leave as 1 if you're
                not sure how to modify this.
        iccFitDict - a dictionary containing ICC fit constraints and possibly initial guesses

    Output:
        goodIDX: a numpy arrays the same size as n_events that is True of False for if it contains
                counts to be included in the TOF profile.
        pp_lambda: the most likely number of background events
    """

    if calc_pp_lambda is True and pp_lambda is not None:
        sys.exit("Error in ICCFT:getBGRemovedIndices: You should not calculate and specify pp_lambda.")

    if calc_pp_lambda is True and padeCoefficients is None:
        sys.exit("Error in ICCFT:getBGRemovedIndices: calc_pp_lambda is True, but no moderator coefficients are provided.")

    if pp_lambda is not None:
        # Set up some things to only consider good pixels
        hasEventsIDX = n_events > 0
        # Set to zero for "this pixel only" mode - performance is optimized for neigh_length_m=0
        neigh_length_m = neigh_length_m
        convBox = 1.0 * np.ones([neigh_length_m, neigh_length_m, neigh_length_m]) / neigh_length_m**3
        conv_n_events = convolve(n_events, convBox)
        goodIDX = np.logical_and(hasEventsIDX, conv_n_events > pp_lambda + zBG * np.sqrt(pp_lambda / (2 * neigh_length_m + 1) ** 3))
        return goodIDX, pp_lambda

    if calc_pp_lambda is False:
        return getPoissionGoodIDX(n_events, zBG=zBG, neigh_length_m=neigh_length_m)

    if peak is not None and box is not None and padeCoefficients is not None:
        while pplmin_frac >= 0.0:
            try:
                return getOptimizedGoodIDX(
                    n_events,
                    padeCoefficients,
                    zBG=1.96,
                    neigh_length_m=neigh_length_m,
                    minppl_frac=pplmin_frac,
                    maxppl_frac=pplmax_frac,
                    qMask=qMask,
                    peak=peak,
                    box=box,
                    pp_lambda=pp_lambda,
                    peakNumber=peakNumber,
                    mindtBinWidth=mindtBinWidth,
                    maxdtBinWidth=maxdtBinWidth,
                    constraintScheme=constraintScheme,
                    peakMaskSize=peakMaskSize,
                    iccFitDict=iccFitDict,
                    fitPenalty=fitPenalty,
                )
            except KeyboardInterrupt:
                sys.exit()
            except:
                # raise
                pplmin_frac -= 0.4
    logger.warning("ERROR WITH ICCFT:getBGRemovedIndices!")


def getDQFracHKL(UB, frac=0.5):
    """
    UBmatrix as loaded by LoadIsawUB().  Only works in the
    return dQ -  a 3x2 numpy array saying how far in each direction you have
        to go to get all of (h-frak,k-frac,l-frac; h+frac, k+frac, l+frac_)
    """

    dQ = np.zeros((3, 2))

    q = [2 * np.pi * frac * UB.dot(v) for v in [seq for seq in itertools.product([-1.0, 1.0], repeat=3)]]
    dQ[:, 0] = np.max(q, axis=0)  # TODO THIS CAN BE 1D since it's symmetric
    dQ[:, 1] = np.min(q, axis=0)
    return dQ


def getHKLMask(UB, frac=0.5, dQPixel=0.005, dQ=None):
    """
    getHKLMask returns the qMask used throughout most of the fitting.
    Intput:
        UB: the UB matrix (3x3 numpy array)
        frac: the fraction around (hkl) you want to consider.
        dQPixel: the side length of each voxel
        dQ: how far to go.  Will calculate if set to None
    returns
        mask: the qMask used by many fitting functions.
    """
    if dQ is None:
        dQ = np.abs(getDQFracHKL(UB, frac=frac))
        dQ[dQ > 0.5] = 0.5
    nPtsQ = np.round(np.sum(dQ / dQPixel, axis=1)).astype(int)
    h0 = 1.0
    k0 = 27.0
    l0 = 7.0
    qDummy = 2 * np.pi * UB.dot(np.asarray([h0, k0, l0]))
    qx = np.linspace(qDummy[0] - dQ[0, 0], qDummy[0] + dQ[0, 1], nPtsQ[0])
    qy = np.linspace(qDummy[1] - dQ[1, 0], qDummy[1] + dQ[1, 1], nPtsQ[1])
    qz = np.linspace(qDummy[2] - dQ[2, 0], qDummy[2] + dQ[2, 1], nPtsQ[2])
    QX, QY, QZ = np.meshgrid(qx, qy, qz, indexing="ij", copy=False)
    UBinv = np.linalg.inv(UB)
    tHKL = UBinv.dot([QX.ravel(), QY.ravel(), QZ.ravel()]) / 2 / np.pi
    H = np.reshape(tHKL[0, :], QX.shape)
    K = np.reshape(tHKL[1, :], QX.shape)
    L = np.reshape(tHKL[2, :], QX.shape)
    mask = reduce(np.logical_and, [H > h0 - frac, H < h0 + frac, K > k0 - frac, K < k0 + frac, L > l0 - frac, L < l0 + frac])
    return mask


def padeWrapper(x, a, b, c, d, f, g, h, i, j, k):
    """
    padeWrapper is a wrapper for the pade(c,x) function for compatibility with curve_fit.
    Inputs are x (numpy array) and the 8 coefficients for the approximant.
    Output are the values of the pade approximant at each value of x.
    """
    pArr = np.zeros(10)
    pArr[0] = a
    pArr[1] = b
    pArr[2] = c
    pArr[3] = d
    pArr[4] = f
    pArr[5] = g
    pArr[6] = h
    pArr[7] = i
    pArr[8] = j
    pArr[9] = k
    return pade(pArr, x)


def pade(c, x):
    """
    Stadnard 4th order pade approximant.
    For use with moderator characterization, c are the coefficients
    and x is the energy (in eV)
    """
    return c[0] * x ** c[1] * (1 + c[2] * x + c[3] * x**2 + (x / c[4]) ** c[5]) / (1 + c[6] * x + c[7] * x**2 + (x / c[8]) ** c[9])


def integratePeak(x, yFit, yData, bg, pp_lambda=0, fracStop=0.01, totEvents=1, bgEvents=1, varFit=0.0):
    """
    integratePeak does 1D integration of the peak.  This is currently done by rectangular integration (i.e.
    we just sum the values at each point on the curve for values considered to be the peak.)  The peak is
    defined as all x values with yFit/max(yFit) > fracStop.
    Note that this function will only integrate the 1D TOF peak - for 3D profile fitting, integration is done in
    doBVGFit or (for mantid) in the algorithm itself.)

    Input:
        x - the TOf values for the TOF histogram
        yFit - the fit for the TOF histogram
        yData - the counts for the TOF histogram
        pp_lambda - the pp_lambda background level.  Not used in integration. TODO: Remove this
        fracStop - the threshold at which we consider the peak to be a peak
        totEvents - not used
        bgEvents - total number of background events including events removed by pp_lambda filtering.
        varFit - the variance of the fit (float)
    Output:
        intensity - total number of counts in the peak
        sigma - sigma(I) for the peak
        xStart - the time (in us) the peak started at
        xStop - the time (in us) the peak stopped at

    """
    # Find out start/stop point
    yScaled = (yFit - bg) / np.max(yFit - bg)
    goodIDX = yScaled > fracStop
    if np.sum(goodIDX) > 0:
        iStart = np.min(np.where(goodIDX))
        iStop = np.max(np.where(goodIDX))
        xStart = x[iStart]
        xStop = x[iStop]
    else:
        logger.warning("ICCFITTOOLS:integratePeak - NO GOOD START/STOP POINT!!")
        return 0.0, 1.0, x[0], x[-1]

    # Do the integration
    intensity = np.sum(yFit[iStart:iStop] - bg[iStart:iStop]) - bgEvents

    # Calculate the background sigma = sqrt(var(Fit) + sum(BG))
    # sigma = np.sqrt(totEvents + bgEvents)
    sigma = np.sqrt(intensity + 2.0 * bgEvents + varFit)
    return intensity, sigma, xStart, xStop


def poisson(k, lam):
    return (lam**k) * (np.exp(-lam) / factorial(k))


def normPoiss(k, lam, eventHist):
    pp_dist = poisson(k, lam)
    pp_n = np.dot(pp_dist, eventHist) / np.dot(pp_dist, pp_dist)
    return pp_dist * pp_n


def get_pp_lambda(n_events, hasEventsIDX):
    """
    #Estimate the most likely number of events based on a Poission
    #distribution of the box.  n_events an ND array (N=3 for Qx,Qy,Qz)
    #containing the number of events in each pixel.  Returns pp_lambda,
    #the most likely number of events.
    """
    eventValues = n_events[hasEventsIDX]
    eventHist = np.bincount(eventValues.astype("int"))[1:]
    eventX = np.array(range(len(eventHist))) + 1

    def normPoissL(k, lam):
        return normPoiss(k, lam, eventHist)

    pp_lambda, cov = curve_fit(normPoissL, eventX, eventHist, p0=0.1)
    return pp_lambda


def getTOFWS(
    box,
    flightPath,
    scatteringHalfAngle,
    tofPeak,
    peak,
    qMask,
    zBG=-1.0,
    dtSpread=0.02,
    minFracPixels=0.005,
    workspaceNumber=None,
    neigh_length_m=0,
    pp_lambda=None,
    calc_pp_lambda=False,
    padeCoefficients=None,
    pplmin_frac=0.8,
    pplmax_frac=1.5,
    peakMaskSize=5,
    mindtBinWidth=1,
    maxdtBinWidth=50,
    constraintScheme=1,
    iccFitDict=None,
    fitPenalty=None,
):
    """
    Builds a TOF profile from the data in box which is nominally centered around a peak.
    Input:
        box - in a binned MD box.
        flightPath - L1+L2 (units: m)
        scatteringHalfAngle - the scattering half angle (units: rad)
        tofPeak - the nominal TOF of the peak (units: us)
        peak - the IPeak object for the peak we're fitting
        qMask - a numpy array telling which voxels to use within a fraction of some (hkl).  True for use.
        zBG - the z score at which we will accept pixels (i.e. 1.96 for 95% CI).  If zBG<0
             then we will not remove background and will instead just consider each pixel
             individually
        dtSpread - the fraction of t around tofPeak we will consider
        minFracPixels -
        workspaceNumber - None of not doing multiple fits.  Otherwise it will append an integer in the mtd[] object so
            not to overwrite.  Probably not needed in most cases.
        neigh_length_m - integer; how large of a convolution box to use.
        pp_lambda - the most likely background level; set to None if want to calculate
        calc_pp_lambda - boolean; True if you want to calculate pp_lambda using TOF profile fitting.  If you do not
            want to, you can feed the value in as pp_lambda (calculated elsewhere).
        minppl_frac, maxppl_frac; range around predicted pp_lambda to check.
        mindtBinWidth - the small dt (in us) allowed for constructing the TOF profile.
        maxdtBinWidth - the largest dt (in us) allowed for constructing the TOF profile.
        constraintScheme - sets the constraints for TOF profile fitting.  Leave as 1 if you're
                not sure how to modify this.
        iccFitDict - a dictionary containing ICC fit constraints and possibly initial guesses

    Output:
        tofWS - a mantid containing the TOF profile.  X-axis is TOF (units: us) and
               Y-axis is the number of events.
        pp_lambda - the most likey number of bg events.
    """
    # Find the qVoxels to use
    n_events = box.getNumEventsArray()
    hasEventsIDX = np.logical_and(n_events > 0, qMask)
    # Set up some things to only consider good pixels
    if zBG >= 0:
        if pp_lambda is None:
            calc_pp_lambda = True
        goodIDX, pp_lambda = getBGRemovedIndices(
            n_events,
            box=box,
            qMask=qMask,
            peak=peak,
            pp_lambda=pp_lambda,
            calc_pp_lambda=calc_pp_lambda,
            padeCoefficients=padeCoefficients,
            pplmin_frac=pplmin_frac,
            pplmax_frac=pplmax_frac,
            mindtBinWidth=mindtBinWidth,
            maxdtBinWidth=maxdtBinWidth,
            constraintScheme=constraintScheme,
            peakMaskSize=peakMaskSize,
            iccFitDict=iccFitDict,
            fitPenalty=fitPenalty,
        )
        hasEventsIDX = np.logical_and(goodIDX, qMask)
        boxMeanIDX = np.where(hasEventsIDX)
    else:  # don't do background removal - just consider one pixel at a time
        pp_lambda = 0
        boxMeanIDX = np.where(hasEventsIDX)
    boxMeanIDX = np.asarray(boxMeanIDX)

    # Setup our axes -- ask if there is a way to just get this
    xaxis = box.getXDimension()
    qx = np.linspace(xaxis.getMinimum(), xaxis.getMaximum(), xaxis.getNBins())
    yaxis = box.getYDimension()
    qy = np.linspace(yaxis.getMinimum(), yaxis.getMaximum(), yaxis.getNBins())
    zaxis = box.getZDimension()
    qz = np.linspace(zaxis.getMinimum(), zaxis.getMaximum(), zaxis.getNBins())
    QX, QY, QZ = np.meshgrid(qx, qy, qz, indexing="ij", copy=False)

    # Create our TOF distribution from bg corrected data
    tList = 1.0 / np.sqrt(QX[hasEventsIDX] ** 2 + QY[hasEventsIDX] ** 2 + QZ[hasEventsIDX] ** 2)
    tList = 3176.507 * flightPath * np.sin(scatteringHalfAngle) * tList  # convert to microseconds

    # Set up our bins for histogramming
    tMin = np.min(tList)
    tMax = np.max(tList)
    dt = tofPeak * dtSpread  # time in us on either side of the peak position to consider
    tMin = min(tMin, tofPeak - dt)
    tMax = max(tMax, tofPeak + dt)

    qCorners = np.array([[qx[v[0]], qy[v[1]], qz[v[2]]] for v in itertools.product((1, -1), repeat=3)])
    qMagCorn = np.array([np.sqrt(q[0] * q[0] + q[1] * q[1] + q[2] * q[2]) for q in qCorners])
    tofCorners = 3176.507 * flightPath * np.sin(scatteringHalfAngle) / qMagCorn
    tMin = np.min(tofCorners)
    tMax = np.max(tofCorners)
    # this will set dtBinWidth as the resolution at the center of the box
    tC = (
        3176.507
        * flightPath
        * np.sin(scatteringHalfAngle)
        / np.linalg.norm([qx[qx.shape[0] // 2], qy[qy.shape[0] // 2], qz[qz.shape[0] // 2]])
    )
    tD = (
        3176.507
        * flightPath
        * np.sin(scatteringHalfAngle)
        / np.linalg.norm([qx[qx.shape[0] // 2 + 1], qy[qy.shape[0] // 2 + 1], qz[qz.shape[0] // 2 + 1]])
    )
    dtBinWidth = np.abs(tD - tC)
    dtBinWidth = max(mindtBinWidth, dtBinWidth)
    dtBinWidth = min(maxdtBinWidth, dtBinWidth)
    tBins = np.arange(tMin, tMax, dtBinWidth)
    weightList = n_events[hasEventsIDX]  # - pp_lambda
    h = np.histogram(tList, tBins, weights=weightList)
    # For and plot the TOF distribution
    tPoints = 0.5 * (h[1][1:] + h[1][:-1])
    yPoints = h[0]

    if workspaceNumber is None:
        tofWS = CreateWorkspace(OutputWorkspace="__tofWS", DataX=tPoints, DataY=yPoints, DataE=np.sqrt(yPoints))
    else:
        tofWS = CreateWorkspace(OutputWorkspace="tofWS%i" % workspaceNumber, DataX=tPoints, DataY=yPoints, DataE=np.sqrt(yPoints))
    return tofWS, float(pp_lambda)


def getT0Shift(E, L):
    """
    getT0Shift(E,L) returns the time it takes a neutron of energy E
    (in eV) to go a distance L (in m).  Time is returned in us.
    """
    # E is energy in eV, L is flight path in m
    mn = 1.674929e-27  # neutron mass, kg
    E = E * 1.60218e-19  # convert eV to J
    t0Shift = L * np.sqrt(mn / 2 / E)  # units = s
    t0Shift = t0Shift * 1.0e6  # units =us
    return t0Shift


def getModeratorCoefficients(fileName):
    """
    getModeratorCoefficients takes coefficients saved via numpy and loads
    them as a dictionary for use in other functions.
    """
    r = np.loadtxt(fileName)
    d = dict()
    d["A"] = r[0]
    d["B"] = r[1]
    d["R"] = r[2]
    d["T0"] = r[3]
    return d


def oneOverXSquared(x, A, bg):
    return A / np.sqrt(x) + bg


def getInitialGuess(tofWS, paramNames, energy, flightPath, padeCoefficients):
    """
    Returns initial parameters for fitting based on a few quickly derived TOF
     profile parameters.  tofWS is a worskapce containng the TOF profile,
     paramNames is the list of parameter names
     energy is the energy of the peak (units: eV)
     flightPath is L = L1 + L2 (units: m)
    """
    x0 = np.zeros(len(paramNames))
    x = tofWS.readX(0)
    y = tofWS.readY(0)
    x0[0] = pade(padeCoefficients["A"], energy)
    x0[1] = pade(padeCoefficients["B"], energy)
    x0[2] = pade(padeCoefficients["R"], energy)
    x0[3] = pade(padeCoefficients["T0"], energy) + getT0Shift(energy, flightPath)  # extra is for ~18m downstream we are

    # These are still phenomenological
    # x0[0] /= 1.2
    # x0[2] += 0.05
    # x0[3] -= 10 #This is lazy - we can do it detector-by-detector
    x0[3] = np.mean(x[np.argsort(y)[::-1][: min(5, np.sum(y > 0))]])
    x0[4] = (np.max(y)) / x0[0] * 2 * 2.5  # Amplitude - gets rescaled later anyway

    x0[5] = 0.5  # hat width in IDX units
    x0[6] = 120.0  # Exponential decay rate for convolution
    return x0


def getSample(run, DetCalFile, workDir, fileName, qLow=-25, qHigh=25, q_frame="sample"):
    """
    getSample loads the event workspace, converts it to reciprocal space as an MDWorkspace.
        The event workspace will be removed from memory.
    Input:
        run - int; the run number
        DetCalFile - string; path to the DetCalFile.  If None, default calibration is used.
        workDir - not used.  TODO - remove this.
        fileName - str; the events file to load.  Should probably be an absolute path.
        qLow, qHigh - the returned MDWorkspace will range from qLow to qHigh in all 3 directions.
        q_frame - either 'sample' or 'lab'.  Whether to return in the lab or sample coordinate system.
    Returns:
        MDdata - a handle for the mtd['MDdata'] object, which contains the loaded run in reciprocal space.
    """

    # data
    logger.information("Loading file" + fileName)
    data = Load(Filename=fileName)
    if DetCalFile is not None:
        LoadIsawDetCal(InputWorkspace=data, Filename=DetCalFile)

    if q_frame == "lab":
        Q3DFrame = "Q_lab"
    elif q_frame == "sample":
        Q3DFrame = "Q_sample"
    else:
        raise ValueError("ICCFT:calcSomeTOF - q_frame must be either 'lab' or 'sample'; %s was provided" % q_frame)

    MDdata = ConvertToMD(
        InputWorkspace=data,
        QDimensions="Q3D",
        dEAnalysisMode="Elastic",
        Q3DFrames=Q3DFrame,
        QConversionScales="Q in A^-1",
        MinValues="%f, %f, %f" % (qLow, qLow, qLow),
        Maxvalues="%f, %f, %f" % (qHigh, qHigh, qHigh),
        MaxRecursionDepth=10,
        LorentzCorrection=False,
    )
    mtd.remove("data")
    return MDdata


def plotFit(filenameFormat, r, tofWS, fICC, runNumber, peakNumber, energy, chiSq, bgFinal, xStart, xStop, bgx0=None):
    """
    Function to make and save plots of the fits. bgx0=polynomial coefficients (polyfit order)
    for the initial guess
    """
    plt.figure(1)
    plt.clf()
    plt.plot(r.readX(0), r.readY(0), "o", label="Data")
    if bgx0 is not None:
        plt.plot(tofWS.readX(0), fICC.function1D(tofWS.readX(0)) + np.polyval(bgx0, tofWS.readX(0)), "b", label="Initial Guess")
    else:
        plt.plot(tofWS.readX(0), fICC.function1D(tofWS.readX(0)), "b", label="Initial Guess")

    plt.plot(r.readX(1), r.readY(1), ".-", label="Fit")
    plt.plot(r.readX(1), np.polyval(bgFinal, r.readX(1)), "r", label="Background")
    yLims = plt.ylim()
    plt.plot([xStart, xStart], yLims, "k")
    plt.plot([xStop, xStop], yLims, "k")
    plt.title("E0=%4.4f meV, redChiSq=%4.4e" % (energy * 1000, chiSq))
    plt.legend(loc="best")
    plt.savefig(filenameFormat % (runNumber, peakNumber))


def getBoxFracHKL(peak, peaks_ws, MDdata, UBMatrix, peakNumber, dQ, dQPixel=0.005, fracHKL=0.5, fracHKLRefine=0.2, q_frame="sample"):
    """
    getBoxFracHKL returns the binned MDbox going from (x,y,z) - (dq_x, dq_y, dq_z) to (x,y,z) + (dq_x, dq_y, dq_z)
     Inputs:
        peak: peak object to be analyzed.  HKL and peak centers must be defined
        peaks_ws - not used; TODO: remove this.
        MDdata: the MD events workspace to be binned
                only the minmium number of constants is necessary for simpler systems (e.g.
                one value for cubic crystals).
        UBMatrix - not used; TODO: remove this.
        peakNumber - not used.  TODO: remove this
        dQ - a (3x2) numpy array stating how far to bin MDdata around the peak
        fracHKL - not used; TODO: remove this
        fracHKLRefine - not used;  TODO: remove this
        q_frame - str; either 'sample' or 'lab'
      Returns:
          Box, an MDWorkspace with histogrammed events around the peak
    """

    if q_frame == "lab":
        q0 = peak.getQLabFrame()
    elif q_frame == "sample":
        q0 = peak.getQSampleFrame()
    else:
        raise ValueError("ICCFT:calcSomeTOF - q_frame must be either 'lab' or 'sample'; %s was provided" % q_frame)
    Qx = q0[0]
    Qy = q0[1]
    Qz = q0[2]
    dQ = np.abs(dQ)
    dQ[dQ > 0.5] = 0.5
    nPtsQ = np.round(np.sum(dQ / dQPixel, axis=1)).astype(int)

    Box = BinMD(
        InputWorkspace="MDdata",
        AlignedDim0="Q_%s_x," % q_frame + str(Qx - dQ[0, 0]) + "," + str(Qx + dQ[0, 1]) + "," + str(nPtsQ[0]),
        AlignedDim1="Q_%s_y," % q_frame + str(Qy - dQ[1, 0]) + "," + str(Qy + dQ[1, 1]) + "," + str(nPtsQ[1]),
        AlignedDim2="Q_%s_z," % q_frame + str(Qz - dQ[2, 0]) + "," + str(Qz + dQ[2, 1]) + "," + str(nPtsQ[2]),
        OutputWorkspace="MDbox",
    )
    return Box


def doICCFit(
    tofWS, energy, flightPath, padeCoefficients, constraintScheme=None, outputWSName="fit", fitOrder=1, iccFitDict=None, fitPenalty=None
):
    """
    doICCFit - Carries out the actual least squares fit for the TOF workspace.
    Intput:
        tofWS - a workspace with x being timepoints and y being the TOF profile
        energy - peak energy
        flightPath - L1 + L2 (in m)
        padeCoefficients - the dictinoary containing coefficients for the pade
            approximant describing moderator output.
        constraintScheme - defines the sets of constraints to use for fitting.
            None will force no constraints
            1 will set constraints to 50% of the initial guess either high or low.
            2 sets a very large range of physically possible (though not necessarily
            correct) values
            Other schemes can be implemented in this code by following the template
            below.
        outputWSName - the base name for output workspaces.  Leave as 'fit' unless you are
            doing multiple fits.
        fitOrder - the background polynomial order
        iccFitDict - a dictionary containing ICC fit constraints and possibly initial guesses
    Returns:
        fitResults - the output from Mantid's Fit() routine
        fICC - an IkedaCarpenterConvoluted function with parameters set to the fit values.
    """
    # Set up our initial guess
    fICC = ICC.IkedaCarpenterConvoluted()
    fICC.init()
    paramNames = [fICC.getParamName(x) for x in range(fICC.numParams())]
    x0 = getInitialGuess(tofWS, paramNames, energy, flightPath, padeCoefficients)
    [fICC.setParameter(iii, v) for iii, v in enumerate(x0[: fICC.numParams()])]
    x = tofWS.readX(0)
    y = tofWS.readY(0)
    bgx0 = np.polyfit(x[np.r_[0:15, -15:0]], y[np.r_[0:15, -15:0]], fitOrder)

    nPts = x.size
    scaleFactor = np.max((y - np.polyval(bgx0, x))[nPts // 3 : 2 * nPts // 3]) / np.max(fICC.function1D(x)[nPts // 3 : 2 * nPts // 3])
    x0[4] = x0[4] * scaleFactor
    fICC.setParameter(4, x0[4])
    # fICC.setPenalizedConstraints(A0=[0.01, 1.0], B0=[0.005, 1.5], R0=[0.01, 1.0], T00=[0,1.0e10], KConv0=[10,500],penalty=1.0e20)
    if constraintScheme == 1:
        # Set these bounds as defaults - they can be changed for each instrument
        # They can be changed by setting parameters in the INSTRUMENT_Parameters.xml file.
        A0 = [0.5 * x0[0], 1.5 * x0[0]]
        B0 = [0.5 * x0[1], 1.5 * x0[1]]
        R0 = [0.5 * x0[2], 1.5 * x0[2]]
        T00 = [0.0, 1.0e10]
        HatWidth0 = [0.0, 5.0]
        Scale0 = [0.0, np.inf]
        KConv0 = [100, 140]
        # Now we see what instrument specific parameters we have
        if iccFitDict is not None:
            possibleKeys = ["iccA", "iccB", "iccR", "iccT0", "iccScale0", "iccHatWidth", "iccKConv"]
            for keyIDX, (key, bounds) in enumerate(zip(possibleKeys, [A0, B0, R0, T00, Scale0, HatWidth0, KConv0])):
                if key in iccFitDict:
                    bounds[0] = iccFitDict[key][0]
                    bounds[1] = iccFitDict[key][1]
                    if len(iccFitDict[key] == 3):
                        x0[keyIDX] = iccFitDict[key][2]
                        fICC.setParameter(keyIDX, x0[keyIDX])
        fICC.setPenalizedConstraints(A0=A0, B0=B0, R0=R0, T00=T00, KConv0=KConv0, penalty=fitPenalty)
    if constraintScheme == 2:
        fICC.setPenalizedConstraints(
            A0=[0.0001, 1.0],
            B0=[0.005, 1.5],
            R0=[0.00, 1.0],
            Scale0=[0.0, 1.0e10],
            T00=[0, 1.0e10],
            KConv0=[100.0, 140.0],
            penalty=fitPenalty,
        )
    f = FunctionWrapper(fICC)
    bg = Polynomial(n=fitOrder)
    for i in range(fitOrder + 1):
        bg["A" + str(fitOrder - i)] = bgx0[i]
    bg.constrain("-1.0 < A%i < 1.0" % fitOrder)
    fitFun = f + bg
    fitResults = Fit(Function=fitFun, InputWorkspace="__tofWS", Output=outputWSName)
    return fitResults, fICC


def integrateSample(
    run,
    MDdata,
    peaks_ws,
    paramList,
    UBMatrix,
    dQ,
    qMask,
    padeCoefficients,
    figsFormat=None,
    dtSpread=0.02,
    fracHKL=0.5,
    minFracPixels=0.0000,
    fracStop=0.01,
    dQPixel=0.005,
    p=None,
    neigh_length_m=0,
    zBG=-1.0,
    bgPolyOrder=1,
    doIterativeBackgroundFitting=False,
    q_frame="sample",
    progressFile=None,
    minpplfrac=0.8,
    maxpplfrac=1.5,
    mindtBinWidth=1,
    maxdtBinWidth=50,
    keepFitDict=False,
    constraintScheme=1,
    peakMaskSize=5,
    iccFitDict=None,
    fitPenalty=None,
):
    """
        integrateSample contains the loop that integrates over all of the peaks in a run and saves the results.  Importantly, it also
        handles errors (mostly by passing and recording special values for failed fits.)
        Input:
            run - int; the run number to process
            MDdata - MDWorkspace; the MDWorkspace from this run in q_frame coordinates
            peaks_ws - a Mantid peaks workspace containing at least one peak to be fit from run number run
            paramList - used to save TOF fit parameters.  Can pass an empty list [] or a list you want to append to.
            UBMatrix - the UBMatrix for the sample
            dQ - how far to extend the box
            qMask - mask stating which voxels to use to stay within a fraction of (h,k,l)
            padeCoefficients - dictionary containing the coefficients for pade approximants for the moderator output
            figsFormat - fileName format to save figure for each fit.  Will not save if set to None.
            dtSpread - how far on each side of the nominal peak TOF to consider.
            fracHKL - the fraction for generating qMask
            minFracPixels -
            fracStop - the minimum intensity (as a ratio of the maximum intensity) of bins we include in the peak
            dQPixel - the side length of each voxel for fitting
            p - array of peak numbers to fit.  Useful for troubleshooting.
            neigh_length_m - the number of voxels we will smooth over (via np.convolve)
            zBG - the z score at which we consider events to be signal.  Set to negative to not use.
    `       bgPolyOrder - the polynomial order the background is fit to for the TOF profile.  Typically
                the background is removed by only keeping signal, so linear is sufficient to take care of
                any small residual bakcground.
            doIterativeBackgroundFitting - do not use; leave as False.  TODO: Remove this
            q_frame - str; either 'sample' or 'lab'
            progressFile - the name of a file which will write the current peak number every 100 peaks.  Useful
                for monitoring batch jobs.  Set to None to not write file.
            minpplfrac, maxpplfrac - the range of pp_lambdas to check around the predicted pp_lambda as a fraction
                of pp_lambda
            mindtBinWidth - the smallest dt bin width (in us) allowed for TOF profile construction
            mindtBinWidth - the largest dt bin width (in us) allowed for TOF profile construction
            keepFitDict= bool; if True then each fit will be saved in a dictionary and returned.  For large peak sets,
                this can take a lot of memory.
            constraintScheme - which constraint scheme we will use - leave as 1 if you're not sure what this does.
            iccFitDict - a dictionary containing ICC fit constraints and possibly initial guesses
        Returns:
            peaks_ws - the peaks_ws with updated I, sig(I)
            paramList - a list of fit parameters for each peak.  Parameters are in the order:
                [peakNumber, energy (eV), sum(fitIntensities), 0.0, redChiSq, alpha, beta, R, T0, amplitude/scale, hat_width,
                k_conv, Ai (the background coefficients, A0 and A1 for linear), reducedChiSquared, pp_lambda]
            fitDict - if keepFitDict is False, an empty dictionary.  If keepFitDict is true, a dictionary (integer peak number as key)
                containing the x, yData, yFit for each peak.
    """
    if p is None:
        p = range(peaks_ws.getNumberPeaks())
    fitDict = {}
    for i in p:
        if progressFile is not None and i % 100 == 0:
            with open(progressFile, "w") as f:
                f.write("%i\n" % (i))
        peak = peaks_ws.getPeak(i)
        if peak.getRunNumber() == run:
            try:  # for ppppp in [3]:#try:
                Box = getBoxFracHKL(peak, peaks_ws, MDdata, UBMatrix, i, dQ, fracHKL=fracHKL, dQPixel=dQPixel, q_frame=q_frame)
                wavelength = peak.getWavelength()  # in Angstrom
                energy = 81.804 / wavelength**2 / 1000.0  # in eV
                flightPath = peak.getL1() + peak.getL2()  # in m
                logger.information("---fitting peak {:d}".format(i))
                if Box.getNEvents() < 1 or np.all(np.abs(peak.getHKL()) == 0):
                    logger.information("Peak {:d} has 0 events or is HKL=000. Skipping!".format(p))
                    peak.setIntensity(0)
                    peak.setSigmaIntensity(1)
                    paramLisg.append([i, energy, 0.0, 1.0e10, 1.0e10] + [0 for i in range(mtd["fit_parameters"].rowCount())] + [0])

                    mtd.remove("MDbox_" + str(run) + "_" + str(i))
                    continue
                n_events = Box.getNumEventsArray()
                goodIDX, pp_lambda = getBGRemovedIndices(
                    n_events,
                    peak=peak,
                    box=Box,
                    qMask=qMask,
                    calc_pp_lambda=True,
                    padeCoefficients=padeCoefficients,
                    mindtBinWidth=mindtBinWidth,
                    maxdtBinWidth=maxdtBinWidth,
                    pplmin_frac=minpplfrac,
                    pplmax_frac=maxpplfrac,
                    constraintScheme=constraintScheme,
                    peakMaskSize=peakMaskSize,
                    iccFitDict=iccFitDict,
                    fitPenalty=fitPenalty,
                )
                tofWS = mtd["__tofWS"]

                fitResults, fICC = doICCFit(
                    tofWS,
                    energy,
                    flightPath,
                    padeCoefficients,
                    fitOrder=bgPolyOrder,
                    constraintScheme=constraintScheme,
                    iccFitDict=iccFitDict,
                    fitPenalty=fitPenalty,
                )
                chiSq = fitResults.OutputChi2overDoF

                r = mtd["fit_Workspace"]
                param = mtd["fit_Parameters"]
                tofWS = mtd["__tofWS"]

                iii = fICC.numParams() - 1
                fitBG = [param.row(int(iii + bgIDX + 1))["Value"] for bgIDX in range(bgPolyOrder + 1)]

                # Set the intensity before moving on to the next peak
                icProfile = r.readY(1)
                bgCoefficients = fitBG[::-1]
                # peak.setSigmaIntensity(np.sqrt(np.sum(icProfile)))i

                convBox = 1.0 * np.ones([neigh_length_m, neigh_length_m, neigh_length_m]) / neigh_length_m**3
                conv_n_events = convolve(n_events, convBox)

                totEvents = np.sum(n_events[goodIDX * qMask])
                bgIDX = reduce(np.logical_and, [~goodIDX, qMask, conv_n_events > 0])
                bgEvents = np.mean(n_events[bgIDX]) * np.sum(goodIDX * qMask)
                intensity, sigma, xStart, xStop = integratePeak(
                    r.readX(0),
                    icProfile,
                    r.readY(0),
                    np.polyval(bgCoefficients, r.readX(1)),
                    pp_lambda=pp_lambda,
                    fracStop=fracStop,
                    totEvents=totEvents,
                    bgEvents=bgEvents,
                    varFit=chiSq,
                )
                # subtract background
                icProfile = icProfile - np.polyval(bgCoefficients, r.readX(1))
                peak.setIntensity(intensity)
                peak.setSigmaIntensity(sigma)
                if figsFormat is not None:
                    plotFit(figsFormat, r, tofWS, fICC, peak.getRunNumber(), i, energy, chiSq, fitBG, xStart, xStop, bgx0=None)
                if keepFitDict:
                    fitDict[i] = np.array([r.readX(0), r.readY(0), r.readY(1), r.readY(2)])
                paramList.append(
                    [i, energy, np.sum(icProfile), 0.0, chiSq] + [param.row(i)["Value"] for i in range(param.rowCount())] + [pp_lambda]
                )
                mtd.remove("MDbox_" + str(run) + "_" + str(i))

            except KeyboardInterrupt:
                logger.warning("KeyboardInterrupt: Exiting Program!!!!!!!")
                sys.exit()
            except:  # Error with fitting
                # raise
                peak.setIntensity(0)
                peak.setSigmaIntensity(1)
                logger.warning("Error with peak " + str(i))
                paramList.append([i, energy, 0.0, 1.0e10, 1.0e10] + [0 for i in range(10)] + [0])
                # paramList.append([i, energy, 0.0, 1.0e10,1.0e10] + [0 for i in range(mtd['fit_parameters'].rowCount())]+[0])
                continue
        mtd.remove("MDbox_" + str(run) + "_" + str(i))
    return peaks_ws, paramList, fitDict
