# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""
1. all the functions are defined and built consistently.




Data types:
- Use only numpy arrays to ensure consistency across formatting and type
*** x, x0 = parameters vector - 1D numpy array
*** setang1, setang2 = angles for refinement - 1D numpy arrays, 3 elements
*** hkl1, hkl2 = numpy arrays (3 columns) having all the hkl indices from the 2 diamonds
*** UB1, UB2 = numpy arrays (3x3) holds UB matrices from input files
***
"""

# Import all needed libraries
from matplotlib import pyplot as plt
import numpy as np
import itertools as itt
import UBMatrixGenerator as UBMG
import scipy.optimize as sp

__author__ = "cip"

# Define global variables
global hkl1, hkl2
global UB1, pkcalcint1
global UB2, pkcalcint2
global pktype
global lam, y, e, TOF
global L1
global ttot
global fxsamediam
global neqv1, eqvlab1, neqv2, eqvlab2
global difa, function_verbose
global figure_name_attenuation, run_number


def dlmread(filename):
    """
    Function to read parameters from file after previous fit
    """
    content = []
    with open(filename, "r") as f:
        for line in f.readlines():
            content.append(float(line))
    return np.array(content)


def calcDspacing(a, b, c, alp, bet, gam, h, k, L):
    """
    %CALCDSPACING for general unit cell: a,b,c,alp,bet,gam returns d-spacing for
    %reflection h,k,l
    %
    """
    ca = np.cos(np.radians(alp))
    cb = np.cos(np.radians(bet))
    cg = np.cos(np.radians(gam))
    sa = np.sin(np.radians(alp))
    sb = np.sin(np.radians(bet))
    sg = np.sin(np.radians(gam))

    oneoverdsq = (1.0 - ca**2 - cb**2 - cg**2 + 2 * ca * cb * cg) ** (-1) * (
        (h * sa / a) ** 2
        + (k * sb / b) ** 2
        + (L * sg / c) ** 2
        + (2 * k * L / (b * c)) * (cb * cg - ca)
        + (2 * L * h / (c * a)) * (cg * ca - cb)
        + (2 * h * k / (a * b)) * (ca * cb - cg)
    )

    d = np.sqrt(1.0 / oneoverdsq)
    return d


def genhkl(hmin, hmax, kmin, kmax, lmin, lmax):
    """
    genhkl generates array of hkl values
    total number of points will be (hmax-hmin)
    """
    hvals = np.arange(hmin, hmax + 1, 1)
    kvals = np.arange(kmin, kmax + 1, 1)
    lvals = np.arange(lmin, lmax + 1, 1)

    nh = len(hvals)
    nk = len(kvals)
    nl = len(lvals)

    L = 0
    hkl = np.zeros(shape=(nh * nl * nk, 3))
    for i in range(nh):
        for j in range(nk):
            for k in range(nl):
                hkl[L][0] = hvals[i]
                hkl[L][1] = kvals[j]
                hkl[L][2] = lvals[k]
                L += 1
    return hkl


def mod(a, b):
    return a % b


def forbidden(h, k, L):
    """
    %returns logical positive if this hkl is fobidden according to
    %   diamond reflections conditions....
    """
    ah = abs(h)
    ak = abs(k)
    al = abs(L)

    if (h == 0) and (k == 0) and (L == 0):
        result = 1
        boolresult = bool(result)
        return boolresult
    else:
        result = 0

    if (ah == 2) and (ak == 2) and (al == 2):  # allowed, but vanishingly weak
        result = 1
        boolresult = bool(result)
        return boolresult
    else:
        result = 0

    # condition 1
    if (h != 0) and (k != 0) and (L != 0):  # general hkl
        term1 = h + k
        term2 = h + L  # all have to be even
        term3 = k + L
        if not ((term1 % 2) == 0 and (term2 % 2) == 0 and (term3 % 2) == 0):
            result = 1
            boolresult = bool(result)
            return boolresult
        else:
            result = 0

    # % condition 2
    if (h == 0) and (k != 0) and (L != 0):  # 0kl reflections
        term1 = k + L
        mod4 = mod(term1, 4)
        if not (mod4 == 0 and mod(k, 2) == 0 and mod(L, 2) == 0):
            result = 1
            boolresult = bool(result)
            return boolresult
        else:
            result = 0

    # condition 3
    if h == k:  # hhl reflections
        if not (mod(h + L, 2) == 0):
            result = 1
            boolresult = bool(result)
            return boolresult
        else:
            result = 0

    # condition 4
    if (h == 0) and (k == 0) and (L != 0):  # 00l reflections not including 000
        mod4 = mod(L, 4)
        if not (mod4 == 0):
            result = 1
            boolresult = bool(result)
            return boolresult
        else:
            result = 0

    boolresult = bool(result)
    return boolresult


def allowedDiamRefs(hmin, hmax, kmin, kmax, lmin, lmax):
    """
    %UNTITLED6 generates a list of allowed reflections for diamond between
    %   limits provided sorted descending according to d-spacing
    """
    # obtain all hkl within limits...
    allhkl = genhkl(hmin, hmax, kmin, kmax, lmin, lmax)
    # now purge those violating extinction conditions...

    n = len(allhkl)

    # set all forbidden hkl's to zero
    # hkl or lhk or klh
    for i in range(n):
        h = allhkl[i][0]
        k = allhkl[i][1]
        L = allhkl[i][2]
        if forbidden(h, k, L) or forbidden(L, h, k) or forbidden(k, L, h):
            allhkl[i] = 0  # set equal to zero

    k = 0
    d = []  # np.zeros(0)
    # create new array with all h!=0 k!=0 l!=0
    hkl = np.zeros(shape=(0, 3))
    for i in range(n):
        if not (allhkl[i][0] == 0 and allhkl[i][1] == 0 and allhkl[i][2] == 0):
            hkl = np.vstack((hkl, [allhkl[i][0], allhkl[i][1], allhkl[i][2]]))
            d.append(calcDspacing(3.56683, 3.56683, 3.56683, 90, 90, 90, hkl[k][0], hkl[k][1], hkl[k][2]))
            k += 1
    d = np.array(d)

    # ORDER hkl according to d-spacing
    B = sorted(d)[::-1]  # returns d sorted in descending order
    IX = np.argsort(d)[::-1]  # and corresponding elements

    sorthkl = np.zeros(shape=(k, 3))
    for i in range(k):
        sorthkl[i] = hkl[IX[i]]
        d[i] = B[i]
        # print('hkl: {0:0.3f} {1:0.3f} {2:0.3f} d-spacing: {3:0.3f} A'.format(sorthkl[i][0], sorthkl[i][1],
        #    sorthkl[i][2], d[i]))

    return sorthkl


def getISAWub(fullfilename):
    """
    %getISAWub reads UB determined by ISAW and stored in file "fname"
    %   Detailed explanation goes here


    % [filename pathname ~] = ...
    %     uigetfile('*.dat','Choose UB file (generated by ISAW)');
    % fullfilename = [pathname filename];
    """
    fileID = fullfilename
    if fileID == 1:
        print(("Error opening file: " + fullfilename))
    f = open(fileID, "r")
    lines = f.readlines()
    f.close()

    # Build UB matrix and lattice
    UB = np.zeros(shape=(3, 3))
    lattice = np.zeros(shape=(2, 6))
    for i in range(3):
        UB[i][0], UB[i][1], UB[i][2] = lines[i].split()
    UB = UB.transpose()
    for i in range(3, 5):
        lattice[i - 3][0], lattice[i - 3][1], lattice[i - 3][2], lattice[i - 3][3], lattice[i - 3][4], lattice[i - 3][5], non = lines[
            i
        ].split()

    print("Successfully got UB and lattice")

    return UB, lattice


def pkintread(hkl, loc):
    """
    %reads calculated Fcalc and converts to
    %Fobs using Buras-Gerard Eqn.
    %inputs are hkl(nref,3) and
    % loc(nref,3), which contains, lambda, d-spacing and ttheta for
    % each of the nref reflections.

    % get Fcalcs for diamond, generated by GSAS (using lattice parameter 3.5668
    % and Uiso(C) = 0.0038

    % disp('in pkintread');


    returns pkint = np. array - 1D vector
    """
    # A = np.genfromtxt('diamond_reflist.csv', delimiter=',', skip_header=True)
    # print A
    A = np.array(
        [
            [1.00000000e00, 1.00000000e00, 1.00000000e00, 8.00000000e00, 2.06110000e00, 5.54000000e04],
            [2.00000000e00, 2.00000000e00, 0.00000000e00, 1.20000000e01, 1.26220000e00, 7.52000000e04],
            [3.00000000e00, 1.00000000e00, 1.00000000e00, 2.40000000e01, 1.07640000e00, 2.98000000e04],
            [2.00000000e00, 2.00000000e00, 2.00000000e00, 8.00000000e00, 1.03060000e00, 2.50000000e-25],
            [4.00000000e00, 0.00000000e00, 0.00000000e00, 6.00000000e00, 8.92500000e-01, 4.05000000e04],
            [3.00000000e00, 3.00000000e00, 1.00000000e00, 2.40000000e01, 8.19000000e-01, 1.61000000e04],
            [4.00000000e00, 2.00000000e00, 2.00000000e00, 2.40000000e01, 7.28700000e-01, 2.18000000e04],
            [5.00000000e00, 1.00000000e00, 1.00000000e00, 2.40000000e01, 6.87000000e-01, 8.64000000e03],
            [3.00000000e00, 3.00000000e00, 3.00000000e00, 8.00000000e00, 6.87000000e-01, 8.64000000e03],
            [4.00000000e00, 4.00000000e00, 0.00000000e00, 1.20000000e01, 6.31100000e-01, 1.17000000e04],
            [5.00000000e00, 3.00000000e00, 1.00000000e00, 4.80000000e01, 6.03400000e-01, 4.65000000e03],
            [4.00000000e00, 4.00000000e00, 2.00000000e00, 2.40000000e01, 5.95000000e-01, 1.83000000e-12],
            [6.00000000e00, 2.00000000e00, 0.00000000e00, 2.40000000e01, 5.64500000e-01, 6.31000000e03],
            [5.00000000e00, 3.00000000e00, 3.00000000e00, 2.40000000e01, 5.44400000e-01, 2.50000000e03],
            [6.00000000e00, 2.00000000e00, 2.00000000e00, 2.40000000e01, 5.38200000e-01, 8.80000000e-26],
            [4.00000000e00, 4.00000000e00, 4.00000000e00, 8.00000000e00, 5.15300000e-01, 3.40000000e03],
            [5.00000000e00, 5.00000000e00, 1.00000000e00, 2.40000000e01, 4.99900000e-01, 1.35000000e03],
            [7.00000000e00, 1.00000000e00, 1.00000000e00, 2.40000000e01, 4.99900000e-01, 1.35000000e03],
            [6.00000000e00, 4.00000000e00, 2.00000000e00, 4.80000000e01, 4.77100000e-01, 1.83000000e03],
            [7.00000000e00, 3.00000000e00, 1.00000000e00, 4.80000000e01, 4.64800000e-01, 7.25000000e02],
            [5.00000000e00, 5.00000000e00, 3.00000000e00, 2.40000000e01, 4.64800000e-01, 7.25000000e02],
            [8.00000000e00, 0.00000000e00, 0.00000000e00, 6.00000000e00, 4.46200000e-01, 9.84000000e02],
            [7.00000000e00, 3.00000000e00, 3.00000000e00, 2.40000000e01, 4.36100000e-01, 3.90000000e02],
            [6.00000000e00, 4.00000000e00, 4.00000000e00, 2.40000000e01, 4.32900000e-01, 1.53000000e-13],
            [6.00000000e00, 6.00000000e00, 0.00000000e00, 1.20000000e01, 4.20700000e-01, 5.30000000e02],
            [8.00000000e00, 2.00000000e00, 2.00000000e00, 2.40000000e01, 4.20700000e-01, 5.30000000e02],
            [5.00000000e00, 5.00000000e00, 5.00000000e00, 8.00000000e00, 4.12200000e-01, 2.10000000e02],
            [7.00000000e00, 5.00000000e00, 1.00000000e00, 4.80000000e01, 4.12200000e-01, 2.10000000e02],
            [6.00000000e00, 6.00000000e00, 2.00000000e00, 2.40000000e01, 4.09500000e-01, 1.98000000e-26],
            [8.00000000e00, 4.00000000e00, 0.00000000e00, 2.40000000e01, 3.99100000e-01, 2.85000000e02],
            [7.00000000e00, 5.00000000e00, 3.00000000e00, 4.80000000e01, 3.91900000e-01, 1.13000000e02],
            [9.00000000e00, 1.00000000e00, 1.00000000e00, 2.40000000e01, 3.91900000e-01, 1.13000000e02],
            [8.00000000e00, 4.00000000e00, 2.00000000e00, 4.80000000e01, 3.89500000e-01, 4.44000000e-14],
            [6.00000000e00, 6.00000000e00, 4.00000000e00, 2.40000000e01, 3.80600000e-01, 1.53000000e02],
            [9.00000000e00, 3.00000000e00, 1.00000000e00, 4.80000000e01, 3.74200000e-01, 6.08000000e01],
            [8.00000000e00, 4.00000000e00, 4.00000000e00, 2.40000000e01, 3.64400000e-01, 8.26000000e01],
            [9.00000000e00, 3.00000000e00, 3.00000000e00, 2.40000000e01, 3.58800000e-01, 3.27000000e01],
            [7.00000000e00, 5.00000000e00, 5.00000000e00, 2.40000000e01, 3.58800000e-01, 3.27000000e01],
            [7.00000000e00, 7.00000000e00, 1.00000000e00, 2.40000000e01, 3.58800000e-01, 3.27000000e01],
        ]
    )

    diamd = A[:, 4]
    # diamMult = A[:, 3] # unused variable
    diamFCalcSq = A[:, 5]
    nref = hkl.shape[0]
    # % disp(['there are: ' num2str(nref) ' reflections']);
    # % whos loc

    """
    % [i,j] = size(x);
    % dipspec = zeros(i,j); %array containing dip spectrum
    % difspec = zeros(i,j); %array containing diffraction spectrum
    % d = x/sqrt(2);    %dspacings for this lamda range at 90 degrees

    % In order to calculate the scattered intensity I from the Fcalc^2, need to
    % apply the Buras-Gerward formula:
    %
    % Fcalc^2 = I*2*sin(theta)^2/(lamda^2*A*E*incFlux*detEffic)
    """
    pkint = np.zeros(nref)

    for i in range(nref):
        if loc[i][0] > 0:
            # % satisfies Bragg condition (otherwise ignore)
            Fsq = Fsqcalc(loc[i][1], diamd, diamFCalcSq)
            # % Fsq = 1;
            L = (np.sin(np.radians(loc[i][2] / 2.0))) ** 2  # Lorentz correction
            R = 1.0  # %dipLam(i)^4; %reflectivity correction
            A = 1.0  # %Absorption correction
            Ecor = 1
            pkint[i] = Fsq * R * A / (L * Ecor)  # %scattered intensity

    """
    % whos difspec
    % whos van
    % whos dipspec

    % difspec = difspec.*van;
    % dipspec = dipspec.*van;

    % figure(1)
    % plot(d,difspec)
    """
    return pkint


def Fsqcalc(d, diamd, diamFCalcSq):
    """
    % diamond reflections are identified according to their d-spacing
    % and corresponding calculated Fsq are returned

    % global sf111 sf220 sf311 sf400 sf331
    """
    # n = len(diamd) # unused variable
    ref = d
    dif = abs(diamd - ref)
    i = dif.argmin(0)  # i is index of diamd closest to d
    Fsq = diamFCalcSq[i]
    return Fsq


def pkposcalc(hkl, UB, setang):
    """
    % calculates some useful numbers from (ISAW calculated) UB
    % hkl is a 2D array containing all hkl's
    %
    """

    ome = setang[0]
    phi = setang[1]
    chi = setang[2]
    thkl = hkl.transpose()

    Q = UB.dot(thkl)

    Rx = np.array(
        [[1, 0, 0], [0, np.cos(np.radians(ome)), -np.sin(np.radians(ome))], [0, np.sin(np.radians(ome)), np.cos(np.radians(ome))]]
    )
    Ry = np.array(
        [[np.cos(np.radians(phi)), 0, np.sin(np.radians(phi))], [0, 1, 0], [-np.sin(np.radians(phi)), 0, np.cos(np.radians(phi))]]
    )
    Rz = np.array(
        [[np.cos(np.radians(chi)), -np.sin(np.radians(chi)), 0], [np.sin(np.radians(chi)), np.cos(np.radians(chi)), 0], [0, 0, 1]]
    )
    Rall = Rz.dot(Ry).dot(Rx)  # all three rotations
    # str = sprintf('initial: %6.4f %6.4f %6.4f',Q);
    # disp(str)
    Q = Rall.dot(Q)
    magQ = np.sqrt((Q * Q).sum(axis=0))
    """
    # str = sprintf('Scattering vector: %6.4f %6.4f %6.4f',Q);
    # if show==1
    #    disp(str)
    # end
    % %calculate angle with incident beam i.e. (-1 0 0)
    % beam = [1 0 0];
    % alpha = acosd(dot(Q,beam)/norm(Q));
    % str = sprintf('Angle scat. vect. to beam: %6.4f',alpha);
    % if show==1
    %     disp(str)
    % end
    % beam = [0 1 0];
    % alpha = acosd(dot(Q,beam)/norm(Q));
    % str = sprintf('Angle scat. vect. to y: %6.4f',alpha);
    % if show==1
    %     disp(str)
    % end
    % beam = [0 0 1];
    % alpha = acosd(dot(Q,beam)/norm(Q));
    % str = sprintf('Angle scat. vect. to z: %6.4f',alpha);
    % if show==1
    %     disp(str)
    % end
    % Q is a vector pointing to the reciprocal lattice point corresponding to
    % vector hkl. The coordinate system is in frame I that is right handed with x pointing along
    % the beam direction and z vertical.
    """
    d = 1.0 / magQ  # by definition (note ISAW doesn't use 2pi factor)
    d = d.transpose()
    """
    % In frame I the incident beam vector will be of the form [k 0 0]
    % where k = 1/lambda
    % by considering the scattering relation that Q=k_f-k_i, can show that the dot product of
    % -k_i.Q gives the scattering angle 2theta, thus:
    """
    ttheta = 180 - 2 * np.degrees(np.arccos(-Q[0, :] / magQ))
    ttheta = ttheta.transpose()
    # and Bragg's law gives:
    lambda_1 = 2 * d * np.sin(np.radians(ttheta / 2))
    lambda_1 = lambda_1.transpose()
    """
    %
    %   str = sprintf('for hkl: %3i%3i%3i',hkl(1),hkl(2),hkl(3));
    % disp(str)
    % str = sprintf('d-spacing is: %6.4f',d);
    % disp(str)
    % str = sprintf('ttheta is: %6.4f',ttheta);
    % disp(str)
    % str = sprintf('lambda is: %6.4f',lambda);
    % disp(str)
    """

    return lambda_1, d, ttheta


def getMANTIDdat_keepbinning(csvfile):
    """
    getMANTIDdat reads data from mantid "SaveAscii" output
    %   input file name should be 'csvfilename'.csv
    %   data are returned with binning (xmin:xbin:xmax)

    returns TOF, y , e
    """
    fid = open(csvfile, "r")
    lines = fid.readlines()
    x = []
    y = []
    e = []
    if fid < 0:
        print(("Error opening file: " + csvfile))
    for i in range(1, len(lines)):
        a, b, c = lines[i].split(",")
        x.append(float(a))
        y.append(float(b))
        e.append(float(c))
    fid.close()
    x = np.array(x)
    y = np.array(y)
    e = np.array(e)

    return x, y, e


def findeqvs(hkl):
    """
    FINDEQVS runs through array of hkls and labels those that are equivalent
    %in the m-3m point group.
    %
    % there are n reflections.
    % hkl has dimensions nx3
    % eqvlab has dimensions nx1
    """
    n, m = hkl.shape
    eqvlab = np.zeros(n)
    lab = 1
    for i in range(n):
        if eqvlab[i] == 0:  # then it's not been checked yet, so check it
            eqvlab[i] = lab
            refhkl = np.array([abs(hkl[i][0]), abs(hkl[i][1]), abs(hkl[i][2])])
            for j in range(i + 1, n):  # check remaining indices
                comphkl = np.array([abs(hkl[j][0]), abs(hkl[j][1]), abs(hkl[j][2])])
                # all possible permutations
                permcomphkl = list(itt.permutations(comphkl))
                nperm = len(permcomphkl)
                for k in range(nperm):
                    if refhkl[0] == permcomphkl[k][0] and refhkl[1] == permcomphkl[k][1] and refhkl[2] == permcomphkl[k][2]:
                        eqvlab[j] = lab
        lab += 1

    return eqvlab, lab


def showx3(x):
    """
    %showx displays all parameters for refinement in reasonably intelligible
    %form
    Input : parameter vector and the sets of hkl indices for the diamonds
    """
    global hkl1, hkl2
    global UB1, pkcalcint1
    global UB2, pkcalcint2
    global pktype
    global lam, y, e, TOF
    global L1
    global ttot
    global fxsamediam
    global neqv1, eqvlab1, neqv2, eqvlab2
    global difa, function_verbose

    # nref1 = hkl1.shape[0]  # % number of reflections to integrate over # unused variable
    # nref2 = hkl2.shape[0]  # % number of reflections to integrate over # unused variable
    # % returns array with same dim as input labelling equivs
    eqvlab1, neqv1 = findeqvs(hkl1)
    eqvlab2, neqv2 = findeqvs(hkl2)
    setang1 = x[0:3]
    pkmult1 = x[3 : 4 + neqv1 - 1]
    setang2 = x[4 + neqv1 - 1 : 6 + neqv1]
    pkmult2 = x[6 + neqv1 : 7 + neqv1 + neqv2 - 1]
    sf = x[neqv1 + neqv2 + 7 - 1]
    pkwid1 = x[neqv1 + neqv2 + 8 - 1]
    # bgd = x[neqv1 + neqv2 + 8 - 1:neqv1 + neqv2 + 9 + 2 - 1] # unused variable
    pkwid2 = x[neqv1 + neqv2 + 10]
    # % if diamond intensities the same, allow single scale f
    relsf = x[neqv1 + neqv2 + 11]
    delam = x[neqv1 + neqv2 + 12]
    L2 = x[neqv1 + neqv2 + 13]

    print("_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/\n")
    print(("Setting angles diam {0} : \nalp {1} bet {2} gam {3} \n".format(1, setang1[0], setang1[1], setang1[2])))
    print(("pkmult1: {0}\n".format(pkmult1)))
    print(("Setting angles diam {0} : \nalp {1} bet {2} gam {3} \n".format(2, setang2[0], setang2[1], setang2[2])))
    print(("pkmult2: {0}\n".format(pkmult2)))
    print(("Scale factor: {0}\n".format(sf)))
    print(("pkwid1: {0}\n".format(pkwid1)))
    print(("pkwid2: {0}\n".format(pkwid2)))
    print(("Rel. scale factor : {0}\n".format(relsf)))
    print(("Lambda multiplier: {0}\n".format(delam)))
    print(("L2 sample to detector: {0} m\n".format(L2)))
    print("_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/\n")


def SimTransOutput3(name, x):
    """
    %SimTrans calculates transmission spectrum from two crystals
    %   lam - array containing wavelengths to calc over
    %   hkl - contains all Nref hkl's that calculation is performed for
    %   bgd - array containing coefficients of polynomial for background
    %   sf - overall scale factor
    %   pktype - 1 = gauss; 2 = lorentz; ...
    %   UB1 - UB matrix for first crystal
    %   setang1 - setting angles for first crystal (deviations from ideal UB
    %               location).
    %   pkpars1 - position(lambda), position(d-spacing), position(ttheta), width, intensity for each Nref reflections
    %   UB2,setang2,pkpars2 - as above, for second crystal
    %
    % M. Guthrie 21st Jan 2014
    %
    % calculate background profile
    % determine number of coeffs bgd
    """
    global hkl1, hkl2
    global UB1, pkcalcint1
    global UB2, pkcalcint2
    global pktype
    global lam, y, e, TOF
    global L1
    global ttot
    global fxsamediam
    global neqv1, eqvlab1, neqv2, eqvlab2
    global difa, function_verbose
    global figure_name_attenuation, run_number

    nref1 = hkl1.shape[0]  # % number of reflections to integrate over
    nref2 = hkl2.shape[0]  # % number of reflections to integrate over
    # % returns array with same dim as input labelling equivs
    eqvlab1, neqv1 = findeqvs(hkl1)
    eqvlab2, neqv2 = findeqvs(hkl2)

    setang1 = x[0:3]
    pkmult1 = x[3 : 4 + neqv1 - 1]
    setang2 = x[4 + neqv1 - 1 : 6 + neqv1]
    sf = x[neqv1 + neqv2 + 7 - 1]
    pkwid1 = x[neqv1 + neqv2 + 7]
    bgd = x[neqv1 + neqv2 + 8 : neqv1 + neqv2 + 9 + 2 - 1]
    pkwid2 = x[neqv1 + neqv2 + 10]
    # % if diamond intensities the same, allow single scale f
    relsf = x[neqv1 + neqv2 + 11]
    if fxsamediam == 1:
        x[6 + neqv1 : 7 + neqv1 + neqv2 - 1] = x[3 : 4 + neqv2 - 1] * relsf
        pkmult2 = x[6 + neqv1 : 7 + neqv1 + neqv2 - 1]
    else:
        pkmult2 = x[6 + neqv1 : 7 + neqv1 + neqv2 - 1]
    delam = x[neqv1 + neqv2 + 12]
    L2 = x[neqv1 + neqv2 + 13]

    shftlam = 0.0039558 * TOF / (L1 + L2) + difa * (TOF**2)
    # number of lambda points to calculate over
    npt = shftlam.shape[0]
    # calculate information for peaks for crystal 1 using hkl,UB1, setang,
    # pkpos
    a, b, c = pkposcalc(hkl1, UB1, setang1)
    pkpars1 = np.column_stack((a, b, c))
    # calculate information for peaks for crystal 2 using hkl,UB1, setang,
    # pkpos
    a, b, c = pkposcalc(hkl2, UB2, setang2)
    pkpars2 = np.column_stack((a, b, c))

    # generate nptx,nco array containing, x^0,x^1,x^2,...x^nco for
    # all nonzero background coefficients
    bgdco = np.where(bgd != 0)[0]
    nco = bgdco.shape[0]
    nonzerobgd = np.zeros(nco)

    X = np.ones(shape=(nco, npt))
    for i in range(nco):
        X[i, :] = shftlam ** (bgd[bgdco[i]] - 1)
        nonzerobgd[i] = bgd[bgdco[i]]

    # calculate background profile by multiplying this with coefficients
    # themselves
    bgdprof = nonzerobgd.dot(X)
    # bgdprof = np.outer(nonzerobgd, X)
    # print bgdprof
    # bgdprof = bgdprof[0, :]
    # calculate peaks for crystal 1

    t1 = np.zeros(npt)  # initialise array containing profile
    for i in range(nref1):
        if pktype == 1:
            pkpars1[i][0] = pkpars1[i][0] * delam  # linear lambda shift
            sig = pkwid1 * pkpars1[i][0] + pkwid2 * (pkpars1[i][0] ** 2.0)  # const del(lambda)/lambda
            extScl = pkpars1[i][0] ** 0  # lambda dependent extinction effect
            t1 = t1 - extScl * pkmult1[int(eqvlab1[i])] * pkcalcint1[i] * (np.exp(-((shftlam - pkpars1[i][0]) ** 2.0) / (2 * (sig**2))))

    # calculate peaks for crystal 2
    t2 = np.zeros(npt)  # initialise array containing profile
    for i in range(nref2):
        if pktype == 1:
            pkpars2[i][0] = pkpars2[i][0] * delam  # linear lambda shift
            sig = pkwid1 * pkpars2[i][0] + pkwid2 * (pkpars2[i][0] ** 2.0)  # const del(lambda)/lambda
            extScl = pkpars2[i][0] ** 0  # lambda dependent extinction effect
            t2 = t2 - extScl * pkmult2[int(eqvlab2[i])] * pkcalcint2[i] * (np.exp(-((shftlam - pkpars2[i][0]) ** 2.0) / (2 * (sig**2))))

    # calculate final profile
    ttot = (bgdprof + sf * t1) * (bgdprof + sf * t2)
    # t1 = 1.0;
    # t2 = 1.0;
    # introduce weighting function and calc chi2...
    w = np.ones(len(shftlam))  # equal weighting everywhere
    # i1 = np.where(shftlam > 2.15)[0][0]
    # j1 = np.where(shftlam > 2.65)[0][0]
    # w[i1:j1] = 5 #extra weighting in region of first peaks
    # i1 = find(lam>1.68,1,'first');
    # j1 = find(lam>2.05,1,'first');
    # w(i1:j1)=5; %extra weighting but not too much

    resid = (y - ttot) * w
    chi2 = np.sum(resid**2.0 / (2 * e**2)) / npt

    output = 1
    if output == 1:
        diam1trans = sf * t1 + bgdprof
        diam2trans = sf * t2 + bgdprof
        out = np.column_stack((shftlam, diam1trans, diam2trans, ttot, y))
        np.savetxt(name, out, delimiter=",")

    figure_name_attenuation = "Attenuation " + run_number

    plt.figure(figure_name_attenuation)
    plt.plot(shftlam, ttot, "r", label="Total att.")
    plt.plot(shftlam, diam1trans, "k", label="Diam 1 att.")
    plt.plot(shftlam, diam2trans, "b", label="Diam 2 att.")
    plt.legend(bbox_to_anchor=(0.0, 1.02, 1.0, 0.102), loc=3, ncol=3, mode="expand", borderaxespad=0.0)
    plt.xlabel("Wavelength (A)")
    plt.ylabel("Transmission")
    plt.grid()
    for i in range(len(pkpars1)):
        plt.arrow(pkpars1[i, 0] * delam, 1.1, 0.0, 0.025, fc="k", ec="k", head_width=0, head_length=0)
    for i in range(len(pkpars2)):
        plt.arrow(pkpars2[i, 0] * delam, 1.15, 0.0, 0.025, fc="k", ec="k", head_width=0, head_length=0)
    plt.xlim(1, 2.7)
    plt.ylim(0, 1.2)
    plt.show()

    return chi2


def SimTrans3(x):
    """
    %SimTrans calculates transmission spectrum from two crystals
    %   lam - array containing wavelengths to calc over
    %   hkl - contains all Nref hkl's that calculation is performed for
    %   bgd - array containing coefficients of polynomial for background
    %   sf - overall scale factor
    %   pktype - 1 = gauss; 2 = lorentz; ...
    %   UB1 - UB matrix for first crystal
    %   setang1 - setting angles for first crystal (deviations from ideal UB
    %               location).
    %   pkpars1 - position(lambda), position(d-spacing), position(ttheta), width, intensity for each Nref reflections
    %   UB2,setang2,pkpars2 - as above, for second crystal
    %
    % M. Guthrie 21st Jan 2014
    %
    % calculate background profile
    % determine number of coeffs bgd
    %
    % version 2 constrains intensities for equivalent dips to be the same
    % M. Guthrie 3 April 2014
    %
    % M. Guthrie 7 April 2014, realised I was making an (obvious) mistake by
    % adding the transmissions from the two diamonds. Clearly, they should be
    % multiplied. I've implemented the change...will see what difference it
    % makes.
    %
    % M. Guthrie 9 April 2014, introduced possibility to refine L2 and also a
    % scale factor for calculated dip wavelengths (to account for diamond
    % compressibility).
    """
    global hkl1, hkl2
    global UB1, pkcalcint1
    global UB2, pkcalcint2
    global pktype
    global lam, y, e, TOF
    global L1
    global ttot
    global fxsamediam
    global neqv1, eqvlab1, neqv2, eqvlab2
    global difa, function_verbose

    nref1 = hkl1.shape[0]  # % number of reflections to integrate over
    nref2 = hkl2.shape[0]  # % number of reflections to integrate over
    # % returns array with same dim as input labelling equivs
    eqvlab1, neqv1 = findeqvs(hkl1)
    eqvlab2, neqv2 = findeqvs(hkl2)

    setang1 = x[0:3]
    pkmult1 = x[3 : 4 + neqv1 - 1]
    setang2 = x[4 + neqv1 - 1 : 6 + neqv1]
    sf = x[neqv1 + neqv2 + 7 - 1]
    pkwid1 = x[neqv1 + neqv2 + 7]
    bgd = x[neqv1 + neqv2 + 8 : neqv1 + neqv2 + 9 + 2 - 1]
    pkwid2 = x[neqv1 + neqv2 + 10]
    # % if diamond intensities the same, allow single scale f
    relsf = x[neqv1 + neqv2 + 11]
    if fxsamediam == 1:
        x[6 + neqv1 : 7 + neqv1 + neqv2 - 1] = x[3 : 4 + neqv2 - 1] * relsf
        pkmult2 = x[6 + neqv1 : 7 + neqv1 + neqv2 - 1]
    else:
        pkmult2 = x[6 + neqv1 : 7 + neqv1 + neqv2 - 1]
    delam = x[neqv1 + neqv2 + 12]
    L2 = x[neqv1 + neqv2 + 13]

    shftlam = 0.0039558 * TOF / (L1 + L2) + difa * (TOF**2)
    # number of lambda points to calculate over
    npt = shftlam.shape[0]
    # calculate information for peaks for crystal 1 using hkl,UB1, setang,
    # pkpos
    a, b, c = pkposcalc(hkl1, UB1, setang1)
    pkpars1 = np.column_stack((a, b, c))
    # calculate information for peaks for crystal 2 using hkl,UB1, setang,
    # pkpos
    a, b, c = pkposcalc(hkl2, UB2, setang2)
    pkpars2 = np.column_stack((a, b, c))

    # generate nptx,nco array containing, x^0,x^1,x^2,...x^nco for
    # all nonzero background coefficients
    bgdco = np.where(bgd != 0)[0]
    nco = bgdco.shape[0]
    nonzerobgd = np.zeros(nco)

    X = np.ones(shape=(nco, npt))
    for i in range(nco):
        X[i, :] = shftlam ** (bgd[bgdco[i]] - 1)
        nonzerobgd[i] = bgd[bgdco[i]]

    # calculate background profile by multiplying this with coefficients
    # themselves
    bgdprof = nonzerobgd.dot(X)
    # bgdprof = np.outer(nonzerobgd, X)
    # print bgdprof
    # bgdprof = bgdprof[0, :]
    # calculate peaks for crystal 1

    t1 = np.zeros(npt)  # initialise array containing profile
    for i in range(nref1):
        if pktype == 1:
            pkpars1[i][0] = pkpars1[i][0] * delam  # linear lambda shift
            sig = pkwid1 * pkpars1[i][0] + pkwid2 * (pkpars1[i][0] ** 2.0)  # const del(lambda)/lambda
            extScl = pkpars1[i][0] ** 0  # lambda dependent extinction effect
            t1 = t1 - extScl * pkmult1[int(eqvlab1[i])] * pkcalcint1[i] * (np.exp(-((shftlam - pkpars1[i][0]) ** 2.0) / (2 * (sig**2))))

    # calculate peaks for crystal 2
    t2 = np.zeros(npt)  # initialise array containing profile
    for i in range(nref2):
        if pktype == 1:
            pkpars2[i][0] = pkpars2[i][0] * delam  # linear lambda shift
            sig = pkwid1 * pkpars2[i][0] + pkwid2 * (pkpars2[i][0] ** 2.0)  # const del(lambda)/lambda
            extScl = pkpars2[i][0] ** 0  # lambda dependent extinction effect
            t2 = t2 - extScl * pkmult2[int(eqvlab2[i])] * pkcalcint2[i] * (np.exp(-((shftlam - pkpars2[i][0]) ** 2.0) / (2 * (sig**2))))

    # calculate final profile
    ttot = (bgdprof + sf * t1) * (bgdprof + sf * t2)
    # t1 = 1.0;
    # t2 = 1.0;
    # introduce weighting function and calc chi2...
    w = np.ones(len(shftlam))  # equal weighting everywhere
    # i1 = np.where(shftlam > 2.15)[0][0]
    # j1 = np.where(shftlam > 2.65)[0][0]
    # w[i1:j1] = 5 #extra weighting in region of first peaks
    # i1 = find(lam>1.68,1,'first');
    # j1 = find(lam>2.05,1,'first');
    # w(i1:j1)=5; %extra weighting but not too much

    resid = (y - ttot) * w
    chi2 = np.sum(resid**2.0 / (2 * e**2)) / npt

    # Print if the user wants verbose minimization
    if function_verbose == "y":
        print(("Chi^2 ... " + str(chi2)))

    return chi2


def FitTrans():
    """
    Main part of the program
    """
    global hkl1, hkl2
    global UB1, pkcalcint1
    global UB2, pkcalcint2
    global pktype
    global lam, y, e, TOF
    global L1
    global ttot
    global fxsamediam
    global neqv1, eqvlab1, neqv2, eqvlab2
    global difa, function_verbose
    global run_number

    # Customize constraints
    cnstang = 1  # if set equal to one, setting angles will be constrained between
    # limits defined by anglim1 and anglim2.
    anglim1 = 1.0  # if cnstang ~= 1, setting angles for D2 only move by +/- this amount
    anglim2 = 1.0  # if cnstang ~= 1, setting angles for D2 can only move by +/- this amount
    fxsamediam = 1  # ==1 fix intensities for given hkl to be identical for both diamonds
    fixmult = 0  # if ==1 peak multipliers are fixed during refinement
    initL2 = 0.340  # m dist from centre of instrument to transmission det
    delinitL2 = 0.005  # m variation in det position allowed within refinement
    difa = -1e-10  # of order e-10

    function_verbose = "n"

    # constraint notifications
    if fxsamediam == 0:
        print("*diamonds constrained to have same relative dip intensity*\n")
    else:
        print("*diamonds allowed to have different dip intensities!*")

    if cnstang == 1:
        print(("*Diam {0} setting angles constrained to range of +/- {1} about their current values*".format(1, anglim1)))
        print(("*Diam {0} setting angles constrained to range of +/- {1} about their current values*".format(2, anglim2)))
    else:
        print("no constraint on setting angles")

    if fixmult == 1:
        print("*intensity multipliers fixed*")

    # Get Input Files...
    peaks_file = str(input("Name of file containing diamond peaks: "))

    run_number = str(input("Input run number for transmission data: "))

    # Build input filenames
    # fullfilename_ub1 = str(run_number) + 'UB1.dat' # unused variable
    # fullfilename_ub2 = str(run_number) + 'UB2.dat' # unused variable
    fullfilename_trans = "transNorm" + str(run_number) + ".dat"

    # get both UB's
    UB1, UB2 = UBMG.UBMatrixGen(peaks_file)

    # [filename pathname ~] = ...
    #     uigetfile('*.dat','Choose UB matrix for upstream diamond:');
    # fullfilename = [pathname filename];
    # fullfilename_ub1 = 'snap13108UB1.dat'
    # UB1, remainder = getISAWub(fullfilename_ub1)

    # [filename pathname ~] = ...
    #     uigetfile('*.dat','Choose UB matrix for downstream diamond:');
    # fullfilename = [pathname filename];
    # fullfilename_ub2 = 'snap13108UB2.dat'
    # UB2, remainder = getISAWub(fullfilename_ub2)

    # get transmission data...
    # [filename,pathname,~] = ...
    #     uigetfile('*.csv','Choose transmission datafile:');
    # fullfilename = [pathname filename];
    fullfilename_trans = "transNorm13148.csv"
    TOF, yin, ein = getMANTIDdat_keepbinning(fullfilename_trans)

    print(("Starting refinement for: " + fullfilename_trans))

    # set-up simulation

    L1 = 15.0  # m dist to centre of instrument in m

    # global initial conditions
    sf = 1
    pktype = 1  # 1 = Gaussian, only current working peaktype
    pkwid = 0.003  # peak width 'sig' is quadratic in lamda
    pkwid2 = 2e-4  # sig = pkwid*lam+pkwid2*lam^2

    #####################
    # Start work...
    #####################

    # rebin transmission data
    lam = 0.0039558 * TOF / (L1 + initL2)

    print(("wavelength limits: " + str(lam[0]) + " and " + str(lam[len(lam) - 1])))
    minlam = 0.8
    maxlam = 3.5
    imin = np.where(lam >= minlam)[0][0]
    imax = np.where(lam >= maxlam)[0][0]
    lam = lam[imin : imax + 1]
    TOF = TOF[imin : imax + 1]  # this will be the TOF range used in fit
    y = yin[imin : imax + 1]
    e = ein[imin : imax + 1]
    bgd = np.array([1.0, 0.0])

    # generate all allowed diamond hkls:
    allhkl = allowedDiamRefs(-7, 7, -7, 7, -7, 7)

    # initial conditions for crystal 1
    setang1 = np.zeros(3)
    # setang1[1:3] = 0.0 # rotation angles applied to refined UB
    # use these to calculate resulting peak positions in wavelength
    # pkpars1(:,1) is lambda
    # pkpars1(:,2) is d-spacing
    # pkpars1(:,3) is is 2theta
    a, b, c = pkposcalc(allhkl, UB1, setang1)
    pkpars1 = np.column_stack((a, b, c))

    # initial conditions for crystal 2
    setang2 = np.zeros(3)
    # setang2[1:3][0] = 0.0
    a, b, c = pkposcalc(allhkl, UB2, setang2)
    pkpars2 = np.column_stack((a, b, c))

    # purge all reflections that don't satisfy the Bragg condition and that are
    # out of wavelength calculation range...

    laminlim = lam[0]
    lamaxlim = lam[len(lam) - 1]

    nref = len(allhkl)

    k1 = 0
    k2 = 0
    hkl1 = np.zeros(shape=(0, 3))
    hkl2 = np.zeros(shape=(0, 3))
    for i in range(nref):
        if laminlim <= pkpars1[i][0] <= lamaxlim:  # reflection in range
            hkl1 = np.vstack([hkl1, allhkl[i]])
            k1 += 1

        if laminlim <= pkpars2[i][0] <= lamaxlim:  # reflection in range
            hkl2 = np.vstack([hkl2, allhkl[i]])
            k2 += 1

    print(("There are: " + str(k1) + " expected dips due to Crystal 1"))
    print(("There are: " + str(k2) + " expected dips due to Crystal 2"))

    # determine equivalents
    # returns array with same dim as input labelling equivs
    eqvlab1, neqv1 = findeqvs(hkl1)
    eqvlab2, neqv2 = findeqvs(hkl2)

    # pkpars1 = np.zeros(shape=(k, 6))   #empty array
    a, b, c = pkposcalc(hkl1, UB1, setang1)
    pkpars1 = np.column_stack((a, b, c))
    # Calculated ref intensities
    pkcalcint1 = pkintread(hkl1, (pkpars1[:, 0:3]))
    pkcalcint1 *= 1e-6
    pkmult1 = np.ones(neqv1)  # intensity multiplier for each group of equivs

    # pkpars2 = np.zeros(shape=(l, 6))   #empty array
    a, b, c = pkposcalc(hkl2, UB2, setang2)
    pkpars2 = np.column_stack((a, b, c))
    # Calculated ref intensities
    pkcalcint2 = pkintread(hkl2, (pkpars2[:, 0:3]))
    pkcalcint2 *= 1e-6
    pkmult2 = np.ones(neqv2)  # peak intensity multiplier

    relsf = 1.0  # default value
    delam = 1.0
    L2 = initL2
    tbgd = bgd

    # Either generate, or read variable array from file
    # This is one big array with all the parameters to be refined in it.

    prevf = str(input("Look for pars from a previous run ([y]/n)? "))

    if prevf == "n":
        x0 = np.hstack((setang1, pkmult1, setang2, pkmult2, sf, pkwid, tbgd, pkwid2, relsf, delam, L2))
    else:
        # choose which file to use
        parfilename = str(input("Choose file with starting pars: "))
        parfullfilename = parfilename
        x0 = dlmread(parfullfilename)
        tog = str(input("Got parameters from: \n" + parfilename + "\nUse these ([y]/n)?"))
        if tog == "n":
            x0 = np.hstack((setang1, pkmult1, setang2, pkmult2, sf, pkwid, tbgd, pkwid2, relsf, delam, L2))
            print("discarding pars from previous run")

    print((str(len(x0)) + " parameters will be refined"))

    nvar = len(x0)
    print(("number of variables: " + str(nvar)))
    # nref1 = hkl1.shape[0] # unused variable
    # nref2 = hkl2.shape[0] # unused variable

    # need to apply correction in the case that pars from previous run had
    # fxsamediam==1 and current run also has fxsamediam==1
    # to avoid a double multiplication by relsf

    if fxsamediam == 1 and x0[neqv1 + neqv2 + 11] != 1:
        x0[6 + neqv1 : 7 + neqv1 + neqv2 - 1] = x0[3 : 4 + neqv2 - 1] / x0[neqv1 + neqv2 + 11]
        print(("Diam 2 peak multipliers reset: " + str(x0[neqv1 + neqv2 + 11])))

    # check starting point

    chi2 = SimTrans3(x0)
    fig_name_start = "Starting point " + run_number
    plt.figure(fig_name_start)
    plt.plot(lam, y, label="Observed")
    plt.plot(lam, ttot, label="Calculated")
    plt.plot(lam, (y - ttot), label="Residual")
    plt.xlabel("Wavelength (A)")
    plt.ylabel("Transmission")
    plt.grid()
    plt.legend(bbox_to_anchor=(0.0, 1.02, 1.0, 0.102), loc=3, ncol=3, mode="expand", borderaxespad=0.0)
    plt.show()
    print(("Initial chi^2 is: " + str(chi2)))

    showx3(x0)

    # Prepare minimization of chi^2 for the calculated profile
    # Set-up default constraints...
    # inequalities
    A = np.zeros(len(x0))
    A[0:3] = 0  # setang1 *no constraint
    A[3 : 4 + neqv1 - 1] = -1.0  # pkmult1 Contrains intensities to be positive
    A[4 + neqv1 - 1 : 6 + neqv1] = 0.0  # setang2 *no constraint
    A[6 + neqv1 : 7 + neqv1 + neqv2 - 1] = -1.0  # pkmult2
    A[6 + neqv1 + neqv2] = -1.0  # sf Scale factor must be +ve
    A[7 + neqv1 + neqv2] = -1.0  # pkwid peak width must be +ve
    A[neqv1 + neqv2 + 8 : neqv1 + neqv2 + 9 + 2 - 1] = 0.0  # bgd *no constraint
    A[(neqv1 + neqv2 + 10)] = 0.0  # *no constraint
    A[(neqv1 + neqv2 + 11)] = 0.0  # *no constraint
    A[(neqv1 + neqv2 + 12)] = 0.0  # *no constraint
    A[(neqv1 + neqv2 + 13)] = 0.0  # *no constraint

    # equalities
    Aeq = np.zeros(len(x0))
    Aeq[0:3] = 0.0  # setang1
    Aeq[3 : 4 + neqv1 - 1] = 0.0  # pkmult1
    Aeq[4 + neqv1 - 1 : 6 + neqv1] = 0.0  # setang2
    Aeq[6 + neqv1 : 7 + neqv1 + neqv2 - 1] = 0.0  # pkmult2
    Aeq[6 + neqv1 + neqv2] = 0.0  # sf
    Aeq[7 + neqv1 + neqv2] = 0.0  # pkwid
    Aeq[neqv1 + neqv2 + 8 : neqv1 + neqv2 + 9 + 2 - 1] = 0  # unfixed bgd
    Aeq[neqv1 + neqv2 + 10] = 0
    Aeq[neqv1 + neqv2 + 11] = 0
    Aeq[neqv1 + neqv2 + 12] = 0
    Aeq[neqv1 + neqv2 + 13] = 0

    # beq = 0 # unused variable

    # lower bounds
    lb = np.zeros(len(x0))
    lb[0:3] = -10  # setang1
    lb[3 : 4 + neqv1 - 1] = 0.5  # pkmult1
    lb[4 + neqv1 - 1 : 6 + neqv1] = -10  # setang2
    lb[6 + neqv1 : 7 + neqv1 + neqv2 - 1] = 0.5  # pkmult2
    lb[6 + neqv1 + neqv2] = 0.0  # sf
    lb[7 + neqv1 + neqv2] = 0.0005  # pkwid
    lb[neqv1 + neqv2 + 8 : neqv1 + neqv2 + 9 + 2 - 1] = [0.995, -0.0005]  # bgd
    lb[neqv1 + neqv2 + 10] = 0.5e-4  # 2nd order pkwid
    lb[neqv1 + neqv2 + 11] = 0.9  # rel scale factor must be positive
    lb[neqv1 + neqv2 + 12] = 0.9  # min lambda shift
    # (m) min L2 dist sample to d/stream detector
    lb[neqv1 + neqv2 + 13] = initL2 - delinitL2

    # upper bounds
    ub = np.zeros(len(x0))
    ub[0:3] = 10  # setang1
    ub[3 : 4 + neqv1 - 1] = 50  # pkmult1
    ub[4 + neqv1 - 1 : 6 + neqv1] = 10  # setang2
    ub[6 + neqv1 : 7 + neqv1 + neqv2 - 1] = 50  # pkmult2
    ub[6 + neqv1 + neqv2] = 50  # sf
    ub[7 + neqv1 + neqv2] = 0.01  # pkwid
    ub[neqv1 + neqv2 + 8 : neqv1 + neqv2 + 9 + 2 - 1] = [1.005, 0.0005]  # bgd
    ub[neqv1 + neqv2 + 10] = 1.0e-2  # 2nd order pkwid
    # diamond shouldn't be more than 2 times bigger!
    ub[neqv1 + neqv2 + 11] = 1.1
    ub[neqv1 + neqv2 + 12] = 1.1  # max lambda shift
    # (m) max L2 dist sample to d/stream detector
    ub[neqv1 + neqv2 + 13] = initL2 + delinitL2

    # Customize constraints

    if cnstang == 1:
        # diamond 1
        lb[0] = x0[0] - anglim1
        lb[1] = x0[1] - anglim1
        lb[2] = x0[2] - anglim1
        ub[0] = x0[0] + anglim1
        ub[1] = x0[1] + anglim1
        ub[2] = x0[2] + anglim1
        # diamond 2
        lb[3 + neqv1] = x0[3 + neqv1] - anglim2
        lb[4 + neqv1] = x0[4 + neqv1] - anglim2
        lb[5 + neqv1] = x0[5 + neqv1] - anglim2
        ub[3 + neqv1] = x0[3 + neqv1] + anglim2
        ub[4 + neqv1] = x0[4 + neqv1] + anglim2
        ub[5 + neqv1] = x0[5 + neqv1] + anglim2

    if fixmult == 1:
        lb[3 : 4 + neqv1 - 1] = x0[3 : 4 + neqv1 - 1] - 0.01
        lb[6 + neqv1 : 7 + neqv1 + neqv2 - 1] = x0[6 + neqv1 : 7 + neqv1 + neqv2 - 1] - 0.01
        ub[3 : 4 + neqv1 - 1] = x0[3 : 4 + neqv1 - 1] + 0.01
        ub[6 + neqv1 : 7 + neqv1 + neqv2 - 1] = x0[6 + neqv1 : 7 + neqv1 + neqv2 - 1] + 0.01

    prompt = str(input("Enter anything to begin refinement..."))
    print("Refining...\nMight take quite a long time...")

    max_number_iterations = int(input("Maximum number of iterations for minimization: "))
    function_verbose = str(input("Verbose minimization ([y]/n): "))

    # make dictionary holding constraints for minimization
    # equalities (all must equal 0) and inequalities
    cons = []
    for i in range(len(x0)):
        cons.append({"type": "ineq", "fun": lambda x: -A[i] * x[i]})
    cons = tuple(cons)

    # bounds have to be list of tuples with (lower, upper) for each parameter
    bds = np.vstack((lb, ub)).T
    res = sp.minimize(SimTrans3, x0, method="SLSQP", bounds=bds, constraints=cons, options={"disp": True, "maxiter": max_number_iterations})

    # tolerance limits to put in minimization if you want so : 'ftol': 0.001

    x = np.array(res.x)
    #
    # minimisation...
    #
    # figure(2)
    # options = optimoptions(@fmincon,'Algorithm','interior-point', 'Display','off', 'MaxFunEvals',10000*nvar,'PlotFcns'
    #                                         @optimplotfval, 'MaxIter',4000)
    # x, fval, exitflag, output = fmincon(@SimTrans3,x0,A,b,[],[],Aeq beq
    # lb,ub,[],options)

    # necessary to update these here...
    if fxsamediam == 1:
        # set peak parameters for second diamond to equal those of first
        # but scaled by relsf
        # len(x)
        # neqv1+neqv2+11
        # x[neqv1+neqv2+11]
        x[6 + neqv1 : 7 + neqv1 + neqv2 - 1] = x[3 : 4 + neqv2 - 1] * x[neqv1 + neqv2 + 11]
        print(("Diam 2 peak multipliers reset with factor: " + str(x[neqv1 + neqv2 + 11])))
    else:
        # label ensuring I know that run did not use fxsamediam
        x[neqv1 + neqv2 + 11] = 1.0

    print("AFTER REFINEMENT")
    showx3(x)

    ####
    # output final information
    ####

    # calculate chi2 for best fit
    chi2 = SimTrans3(x)
    print(("Final Chi2 = " + str(chi2)))

    # determine output wavelength range using refined L2 value

    # lamscale = x[neqv1 + neqv2 + 12] # unused variable
    L2 = x[neqv1 + neqv2 + 13]
    outlam = 0.0039558 * TOF / (L1 + L2) + difa * (TOF**2)

    fig_name_final = "Final result " + run_number
    plt.figure(fig_name_final)
    plt.plot(outlam, y, "k", label="Observed")
    plt.plot(outlam, ttot, "r", label="Calculated")
    plt.plot(outlam, (y - ttot), "b", label="Final residuals")
    plt.legend(bbox_to_anchor=(0.0, 1.02, 1.0, 0.102), loc=3, ncol=3, mode="expand", borderaxespad=0.0)
    plt.text(2.1, 0.5, "CHI^2=" + str(chi2))
    plt.grid()
    for i in range(len(pkpars1)):
        plt.arrow(pkpars1[i, 0] * delam, 1.1, 0.0, 0.025, fc="k", ec="k", head_width=0, head_length=0)
    for i in range(len(pkpars2)):
        plt.arrow(pkpars2[i, 0] * delam, 1.15, 0.0, 0.025, fc="k", ec="k", head_width=0, head_length=0)
    plt.xlim(1.0, 2.7)
    plt.ylim(ymax=1.2)
    plt.xlabel("Wavelength (A)")
    plt.ylabel("Transmission")
    plt.show()

    prompt = str(input("output best fit to file ([y]/n): "))
    if prompt == "n":
        print("Ending")
    else:
        fitparname = str(run_number) + ".best_fit_pars3.dat"
        np.savetxt(fitparname, x, delimiter=",")
        print(("output parameters written to file: \n" + fitparname))
        ofilename = str(run_number) + ".fitted3.dat"
        SimTransOutput3(ofilename, x)  # generate output file with fitted data


if __name__ == "__main__":
    FitTrans()
