# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# ruff: noqa: E741  # Ambiguous variable name
"""
Generate UB matrices from measured list of reflections
Identifies upstream and downstream diamonds and assigns matrices to them
"""

# Import all needed libraries
import numpy as np
import itertools as itt

# Function to read the reflections measured


def getMANTIDpkds(filename):
    fileID = filename
    if fileID == 1:
        print("Error opening file: " + filename)
    f = open(fileID, "r")
    lines = f.readlines()
    f.close()

    hkl = np.zeros((len(lines) - 2, 3))
    d = np.zeros((len(lines) - 2))
    q = np.zeros((len(lines) - 2, 3))

    for i in range(1, len(lines) - 1):
        elem = lines[i].split(",")

        hkl[i - 1] = elem[2], elem[3], elem[4]
        d[i - 1] = elem[8]
        q[i - 1] = float(elem[15].replace("[", "")), float(elem[16]), float(elem[17].replace("]", ""))

    return hkl, d, q


# Generate all permutations (including negative) for given hkl


def plusAndMinusPermutations(items):
    for p in itt.permutations(items):
        for signs in itt.product([-1, 1], repeat=len(items)):
            yield [a * sign for a, sign in zip(p, signs)]


# Identify equivalent reflections


def m3mEquiv(hkl_input, tog):
    hkl = hkl_input
    h = float(hkl[0])
    k = float(hkl[1])
    l = float(hkl[2])
    all_equivs = np.asarray(list(plusAndMinusPermutations([h, k, l])))
    # print all_equivs

    # Remove non-unique equivalents
    lab = np.zeros(len(all_equivs))  # label unique reflections
    nlab = 0
    for i in range(len(all_equivs)):
        if lab[i] == 0:  # not checked yet
            nlab += 1
            for j in range(len(all_equivs)):
                diff = all_equivs[i] - all_equivs[j]
                if np.linalg.norm(diff) == 0:  # an identical reflection found
                    lab[j] = nlab

    # print('number of unique reflections is: {0}\n'.format(nlab))

    # red is reduced array with only distinct reflections in it
    red = np.zeros((nlab, 3))
    for i in range(1, nlab + 1):
        k = np.argmax(lab == i)
        red[i - 1] = all_equivs[k]

    if tog == 1:
        k = np.where(red[:, 0] <= 0)[0]
        eqvs = red[k]
    else:
        eqvs = red

    return eqvs


# Match equivalent reflections


def EquivMatch(refh, hkl, gam, tol):
    AllEqv = m3mEquiv(hkl, 1)
    match = np.zeros(len(AllEqv))
    nmatch = 0
    for i in range(len(AllEqv)):
        h1 = AllEqv[i]
        bet = np.degrees(np.arccos(np.dot(refh, h1) / (np.linalg.norm(refh) * np.linalg.norm(h1))))
        dif = np.abs(bet - gam)
        if dif <= tol:
            match[i] = 1
            nmatch += 1

    if nmatch == 0:
        hklmatch = [0, 0, 0]
    else:
        ind = np.where(match == 1)
        hklmatch = AllEqv[ind]

    return hklmatch


"""Jacobsen - Implementation of method articulated in RA Jacobsen
 Zeitschrift fur Krystallographie(1996).

Note!!!! mantidplot scales reciprocal space coordinates with a factor of
2 pi.In contrast, ISAWev does not.This algorithm assumes the mantidplot convention!!!!

h1 and h2 are vertical 3x1 coordinate matrices containing h, k, l for two reflections
Xm_1 and Xm_2 are the corresponding coordinate matrices measured in the (Cartesian)
diffractometer frame Xm(these are known by mantid for a calibrated instrument)

First check indexing of input reflections by checking angle: angle between reciprocal
lattice vectors
"""


def Jacobsen(h1, Xm_1, h2, Xm_2):
    alp = np.degrees(np.arccos(np.dot(h1, h2) / (np.linalg.norm(h1) * np.linalg.norm(h2))))
    bet = np.degrees(np.arccos(np.dot(Xm_1, Xm_2) / (np.linalg.norm(Xm_1) * np.linalg.norm(Xm_2))))
    if ((alp - bet) ** 2) > 1:
        print("check your indexing!")

    a = 3.567  # diamond lattice parameter
    # recip lattice par(note this is the mantid convention: no 2 pi)
    ast = (2 * np.pi) / a
    B = np.array([[ast, 0, 0], [0, ast, 0], [0, 0, ast]])
    Xm_g = np.cross(Xm_1, Xm_2)
    Xm = np.column_stack([Xm_1, Xm_2, Xm_g])

    # Vector Q1 is described in reciprocal space by its coordinate matrix h1
    Xa_1 = B.dot(h1)
    Xa_2 = B.dot(h2)
    Xa_g = np.cross(Xa_1, Xa_2)
    Xa = np.column_stack((Xa_1, Xa_2, Xa_g))

    R = Xa.dot(np.linalg.inv(Xm))
    U = np.linalg.inv(R)

    UB = U.dot(B)

    return UB


# Guess Miller indices based on d-spacing


def guessIndx(d, tol):
    # guessIndx accepts n d-spacings and returns guess at indexing class for
    # diamond
    dref = np.array([2.0593, 1.2611, 1.0754, 0.8917, 0.8183, 0.7281])
    # note, by default assume that diamond a / a * axis is close to parallel to beam, therefore, h index
    # will be negative for all reflections
    href = np.array([[-1, 1, 1], [-2, 2, 0], [-3, 1, 1], [-4, 0, 0], [-3, 3, 1], [-4, 2, 2]])

    h = np.zeros((len(d), 3))
    for i in range(len(d)):
        delta = np.abs(dref - d[i])
        hit = np.where(delta < tol)
        h[i] = href[hit]

    return h


def UBMatrixGen(fileName):
    # read ascii file with mantidplot peaks list
    h, d, q = getMANTIDpkds(fileName)
    # sort peaks according to d-spacing
    dsort = sorted(d)[::-1]
    sortindx = np.argsort(d)[::-1]
    N = len(d)
    ublab = np.zeros(N)  # will be label for UB/diamond

    hsort = np.zeros((N, 3))
    qsort = np.zeros((N, 3))
    for i in range(N):
        hsort[i] = h[sortindx[i]]
        qsort[i] = q[sortindx[i]]
    d = dsort
    h = hsort
    q = qsort
    h = guessIndx(d, 0.05)  # overwrites what's in original file with guess
    # based on d-spacing

    # display all reflections with guessed indices, allow choice of one
    # reflection as reference reflection
    print("First guess at indexing\n")
    print("REF | h k l | d-spac(A)")
    for i in range(N):
        print("{0:0.0f}    {1:0.0f} {2:0.0f} {3:0.0f}     {4:0.3f}".format(i, h[i][0], h[i][1], h[i][2], d[i]))

    nref1 = int(input("Choose one reference reflection: "))

    print("REF | h k l | obs|  calc")
    beta = np.zeros(N)
    for i in range(N):
        beta[i] = np.degrees(np.arccos(np.dot(q[nref1], q[i]) / (np.linalg.norm(q[nref1]) * np.linalg.norm(q[i]))))
        if i == nref1:
            print("{0:0.0f} |   {1:0.0f} {2:0.0f} {3:0.0f}   |  {4:0.3f}|  0.0| REFERENCE".format(i, h[i][0], h[i][1], h[i][2], beta[i]))
        else:
            # check for possible index suggestion
            hklA = h[nref1]
            hklB = h[i]
            hklhit = EquivMatch(hklA, hklB, beta[i], 1.0)
            if np.linalg.norm(hklhit) == 0:
                print("{0:0.0f} |   {1:0.0f} {2:0.0f} {3:0.0f} |    {4:0.3f}".format(i, h[i][0], h[i][1], h[i][2], beta[i]))
            else:  # % there is an hkl at a matching angle
                calcang = np.degrees(np.arccos(np.dot(hklA, hklhit[0]) / (np.linalg.norm(hklA) * np.linalg.norm(hklhit[0]))))
                h[i] = hklhit[0]
                print(
                    "{0:0.0f} |   {1:0.0f} {2:0.0f} {3:0.0f} |    {4:0.3f} | {5:0.3f}".format(
                        i, h[i][0], h[i][1], h[i][2], beta[i], calcang
                    )
                )

    nref2 = int(input("Choose a second reflection to use for indexing: "))

    h1 = h[nref1]
    q1 = q[nref1]
    q2 = q[nref2]
    h2 = h[nref2]

    UB1 = Jacobsen(h1, q1, h2, q2)

    # Re-index all input reflections using this UB
    hindx = (np.linalg.inv(UB1).dot(q.transpose())).transpose()
    print("Reflections will be re-indexed using this UB")
    tol = float(input("Enter tolerance for accepting index: "))
    print("REF | h k l | obs|  calc")
    nindexed1 = 0
    for i in range(N):  # decide if reflection is indexed to being within an integer by less then the tolerance
        # difference with nearst integer
        dif = np.abs(hindx[i] - np.round(hindx[i]))
        if np.sum(dif) <= 3 * tol:  # all indices within tolerance
            h[i] = np.round(hindx[i])
            print("{0:0.0f} |   {1:0.0f} {2:0.0f} {3:0.0f} | ".format(i, hindx[i][0], hindx[i][1], hindx[i][2]))
            nindexed1 += 1
            ublab[i] = 1
        else:
            print("{0:0.0f} |   {1:0.0f} {2:0.0f} {3:0.0f} | ".format(i, hindx[i][0], hindx[i][1], hindx[i][2]))
            ublab[i] = 2
    print("{0:0.0f} reflections indexed!".format(nindexed1))
    nsub = N - nindexed1

    if nsub < 2:
        print("not enough remaining reflections to index second diamond :(")
    else:
        k = np.where(ublab == 2)
        hsub = np.array(h[k])  # a list of unindexed h
        qsub = np.array(q[k])  # and their q - vectors
        d = np.array(d)
        dsub = d[k]

        print("Now find UB for second diamond")
        print("Remaining unindexed reflections:")
        print(" REF|  h  k  l| d-spac(A)")
        for i in range(nsub):
            print("{0:0.0f}    {1:0.0f} {2:0.0f} {3:0.0f}     {4:0.3f}".format(i, hsub[i][0], hsub[i][1], hsub[i][2], dsub[i]))

        nref1 = int(input("Choose one reference reflection: "))

        for i in range(nsub):
            beta[i] = np.degrees(np.arccos(np.dot(qsub[nref1], qsub[i]) / (np.linalg.norm(qsub[nref1]) * np.linalg.norm(qsub[i]))))
            if i == nref1:
                print(
                    "{0:0.0f} |   {1:0.0f} {2:0.0f} {3:0.0f}   |  {4:0.3f}|  0.0| REFERENCE".format(
                        i, hsub[i][0], hsub[i][1], hsub[i][2], beta[i]
                    )
                )
            else:
                # check for possible index suggestion
                hklA = hsub[nref1]
                hklB = hsub[i]
                hklhit = EquivMatch(hklA, hklB, beta[i], 1.0)
                if np.linalg.norm(hklhit) == 0:
                    print("{0:0.0f} |   {1:0.0f} {2:0.0f} {3:0.0f} |    {4:0.3f}".format(i, hsub[i][0], hsub[i][1], hsub[i][2], beta[i]))
                else:  # % there is an hkl at a matching angle
                    calcang = np.degrees(np.arccos(np.dot(hklA, hklhit[0]) / (np.linalg.norm(hklA) * np.linalg.norm(hklhit[0]))))
                    h[i] = hklhit[0]
                    print(
                        "{0:0.0f} |   {1:0.0f} {2:0.0f} {3:0.0f} |    {4:0.3f} | {5:0.3f}".format(
                            i, hsub[i][0], hsub[i][1], hsub[i][2], beta[i], calcang
                        )
                    )
        nref2 = int(input("Choose a second reflection to use for indexing: "))

    h1 = hsub[nref1]
    q1 = qsub[nref1]
    q2 = qsub[nref2]
    h2 = hsub[nref2]

    UB2 = Jacobsen(h1, q1, h2, q2)
    print("UB1 = ", UB1)
    print("UB2 = ", UB2)
    return UB1, UB2
