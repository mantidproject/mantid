# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name,no-init,too-many-arguments,too-many-branches, unused-variable,too-many-return-statements
# This script creates numerous PeaksWorkspaces for different Crystal Types and Centerings. Random errors
# are also introduced into the peak's.  Each PeaksWorkspace is sent through the algorithm's FindPeaksMD,
# FindUBUsingFFT, and SelectByForm to determine the corresponding Primitive and Conventional cells. These
# results are tested against the theoretical results that should have been gotten

# NOTE; THIS TEST TAKES AN EXTREMELY LONG TIME. DELETE "XXX" IN requiredFiles method to get it to run.
# !!!!!!!!!  REPLACE THE "XXX" OR else !!!!!!!!!!


# import systemtesting
import numpy
from numpy import matrix
import math
import random
import mantid
from mantid.simpleapi import *


# from mantid.simpleapi import *
# TODO premultiply cases, fix up.. Maybe not needed Cause Conv cell was "Nigglied"
# TODO: SWitch cases, if use approx inequality, may get error cause low level code
# [does Not](does) premult but when it [should](should not)
class Peak2ConvCell_Test(object):  # (systemtesting.MantidSystemTest):
    conventionalUB = numpy.zeros(shape=(3, 3))
    Cubic = [1, 3, 5]
    Tetr = [6, 7, 11, 15, 18, 21]
    Orth = [8, 13, 16, 19, 23, 26, 32, 36, 38, 40, 42]
    Hex = [2, 4, 9, 12, 22, 24]
    Tricl = [31, 44]
    Mon = [28, 29, 30, 33, 34, 35, 43]
    MonI = [17, 27]
    MonC = [10, 14, 20, 25, 37, 39, 41]
    CentP = [3, 11, 12, 21, 22, 31, 32, 33, 34, 35, 44]
    CentF = [1, 16, 26]
    CentI = [2, 4, 5, 6, 7, 8, 9, 10, 14, 15, 17, 18, 19, 20, 24, 25, 27, 37, 39, 41, 42, 43]
    CentC = [10, 13, 14, 17, 20, 23, 25, 27, 28, 29, 30, 36, 37, 38, 39, 40, 41]

    def CalcConventionalUB(self, a, b, c, alpha, _beta, _gamma, celltype):
        Res = matrix([[0.0, 0.0, 0.0], [0.0, 0.0, 0.0], [0.0, 0.0, 0.0]])

        if celltype == "O":
            Res[0, 0] = 1.0 / a
            Res[1, 1] = 1.0 / b
            Res[2, 2] = 1.0 / c

        elif celltype == "H":
            Res[0, 0] = a * 1.0
            Res[1, 0] = -a / 2.0
            Res[1, 1] = a * 0.866
            Res[2, 2] = c * 1.0
            Res = Res.I
        else:
            if alpha <= 90:
                self.conventionalUB = None
                return None
            Res[0, 0] = a * 1.0
            Res[1, 1] = b * 1.0
            Alpha = alpha * math.pi / 180
            Res[2, 0] = c * math.cos(Alpha)
            Res[2, 2] = c * math.sin(Alpha)
            # Now Nigglify the matrix( get 3 smallest sides)

            n = 0
            YY = 0
            if a <= c:
                n = (int)(-Res[2, 0] / a)
                YY = Res[2, 0] + n * a

            else:
                n = (int)(-a * Res[2, 0] / (c * c) - 0.5)
                YY = n * Res[2, 0] + a

                # print ["A",YY,n]
            sgn = 1
            if a <= c:
                if math.fabs(YY + a) < math.fabs(YY) and a <= c:
                    YY += a
                    sgn = -1
                    n += 1

            elif (YY + Res[2, 0]) * (YY + Res[2, 0]) + (n + 1) * (n + 1) * Res[2, 2] * Res[2, 2] < a * a:
                YY += Res[2, 0]
                n += 1
                sgn = -1

                # print ["B",YY,sgn,n]

            if n > 0:
                if a <= c:
                    Res[2, 0] = sgn * YY
                    Res[2, 2] *= sgn

                else:
                    if YY * Res[2, 0] + n * Res[2, 2] * Res[2, 2] > 0:
                        sgn = -1

                    else:
                        sgn = 1
                    Res[0, 0] = sgn * YY
                    Res[0, 2] = sgn * n * Res[2, 2]

            Res = Res.I

        self.conventionalUB = Res

        return Res

    def Niggli(self, Res):
        RUB = Res.I
        X = RUB * RUB.T
        done = False

        while not done:
            done = True
            for i in range(2):
                if X[i, i] > X[i + 1, i + 1]:
                    done = False
                    for j in range(3):
                        sav = RUB[i, j]
                        RUB[i, j] = RUB[i + 1, j]
                        RUB[i + 1, j] = sav
                    X = RUB * RUB.T

            if not done:
                continue
                # do bc,ac,then ab
            for kk in range(3):
                jj = 2
                if kk > 1:
                    jj = 1
                    i = 0
                else:
                    i = jj - kk - 1
                if X[i, i] < 2 * math.fabs(X[i, jj]):
                    sgn = 1
                    if X[i, jj] > 0:
                        sgn = -1
                    for j in range(3):
                        RUB[jj, j] += sgn * RUB[i, j]

                    done = False
                    X = RUB * RUB.T

                    break

        if numpy.linalg.det(RUB) < 0:
            for cc in range(3):
                RUB[0, cc] *= -1

        return RUB.I

    @staticmethod
    def _calc_result_center_I(res, res_p):
        s1 = 1
        s2 = 1
        for r in range(0, 3):
            for cc in range(3):
                if cc == 0:
                    if r > 0:
                        s1 = (-1) ** r
                        s2 = -s1

                res[r, cc] = res_p[0, cc] / 2 + s1 * res_p[1, cc] / 2 + s2 * res_p[2, cc] / 2

        return res.I

    @staticmethod
    def _calc_result_center_F(res, res_p):
        ss = [0, 0, 0]

        for r in range(3):
            for cc in range(3):
                ss = [1, 1, 1]
                ss[r] = 0

                res[r, cc] = ss[0] * res_p[0, cc] / 2 + ss[1] * res_p[1, cc] / 2 + ss[2] * res_p[2, cc] / 2

        return res.I

    @staticmethod
    def _calc_result_center_ABC(res, res_p, r):
        k = 0

        res[r, 0] = res_p[r, 0]
        res[r, 1] = res_p[r, 1]
        res[r, 2] = res_p[r, 2]
        for i in range(1, 3):
            if k == r:
                k += 1
            for cc in range(3):
                R = (r + 1) % 3
                s = (-1) ** i

                res[k, cc] = res_p[(R) % 3, cc] / 2 + s * res_p[(R + 1) % 3, cc] / 2

            k += 1

        return res.I

    def CalcNiggliUB(self, a, b, c, alpha, beta, gamma, celltype, Center):
        if Center == "P":
            X = self.CalcConventionalUB(a, b, c, alpha, beta, gamma, celltype)
            return X

        Res = matrix([[0.0, 0.0, 0.0], [0.0, 0.0, 0.0], [0.0, 0.0, 0.0]])
        ConvUB = self.CalcConventionalUB(a, b, c, alpha, beta, gamma, celltype)
        if ConvUB is None:
            return None

        ResP = numpy.matrix.copy(ConvUB)
        ResP = ResP.I

        if celltype == "H" and Center == "I":
            Center = "R"

        if Center == "I":
            Res = Peak2ConvCell_Test._calc_result_center_I(Res, ResP)

        elif Center == "F":
            if celltype == "H" or celltype == "M":
                return None

            Res = Peak2ConvCell_Test._calc_result_center_F(Res, ResP)

        elif Center == "A" or Center == "B" or Center == "C":
            if celltype == "H":
                return None
            if celltype == "M" and Center == "B":
                return None

            r = 2
            if Center == "A":
                r = 0
                if b == c and celltype == "O":  # result would be orthorhombic primitive
                    return None

            elif Center == "B":
                r = 1
                if a == c and celltype == "O":
                    return None

            elif a == b and celltype == "O":
                return None

            Res = Peak2ConvCell_Test._calc_result_center_ABC(Res, ResP, r)

        elif Center == "R":
            if celltype != "H" or alpha > 120:  # alpha =120 planar, >120 no go or c under a-b plane.
                self.conventionalUB = None
                return None

                # Did not work with 0 error. FindUBUsingFFT failed
                # Alpha = alpha*math.pi/180

            Res[0, 0] = 0.5 * a
            Res[0, 1] = math.sqrt(3) * a / 2
            Res[0, 2] = 0.5 * b
            Res[1, 0] = -a
            Res[1, 1] = 0
            Res[1, 2] = 0.5 * b
            Res[2, 0] = 0.5 * a
            Res[2, 1] = -math.sqrt(3) * a / 2
            Res[2, 2] = 0.5 * b

            Rhomb2Hex = matrix([[1.0, -1.0, 0.0], [-1.0, 0.0, 1.0], [-1.0, -1.0, -1.0]])

            self.conventionalUB = Rhomb2Hex * Res
            Res = Res.I

            self.conventionalUB = self.Niggli(self.conventionalUB.I)

        Res = self.Niggli(Res)
        if numpy.linalg.det(Res) < 0:
            for cc in range(3):
                Res[cc, 0] *= -1

        return Res

    def Perturb(self, val, error):
        return val + random.random() * error - error / 2

    def Next(self, hkl1):
        # print "Next"
        hkl = matrix([[hkl1[0, 0]], [hkl1[1, 0]], [hkl1[2, 0]]])
        S = math.fabs(hkl[0, 0]) + math.fabs(hkl[1, 0]) + math.fabs(hkl[2, 0])
        # print ["S=",S]
        # The sum of abs hkl's = S until not possible. Increasing lexicographically
        if hkl[2, 0] < 0:
            # print "Nexta"
            hkl[2, 0] = -hkl[2, 0]
            # print hkl
            return hkl

        if math.fabs(hkl[0, 0]) + math.fabs(hkl[1, 0] + 1) <= S:
            # print "Nextb"
            hkl[1, 0] += 1
            hkl[2, 0] = -(S - math.fabs(hkl[0, 0]) - math.fabs(hkl[1, 0]))
        elif math.fabs(hkl[0, 0] + 1) <= S:
            # print "Nextc"
            hkl[0, 0] = hkl[0, 0] + 1.0
            hkl[1, 0] = -(S - math.fabs(hkl[0, 0]))
            hkl[2, 0] = 0
        else:
            # print "Nextd"
            hkl[1, 0] = 0
            hkl[2, 0] = 0
            hkl[0, 0] = -S - 1
            # print hkl
        return hkl

    def FixLatParams(self, List):
        npos = 0
        nneg = 0
        if len(List) < 6:
            return List
        has90 = False
        for i in range(3, 6):
            if math.fabs(List[i] - 90) < 0.05:
                nneg += 1
                has90 = True
            elif List[i] < 90:
                npos += 1
            else:
                nneg += 1
        over90 = False
        if nneg == 3 or has90 or nneg == 1:
            over90 = True

        for i in range(3, 6):
            if List[i] > 90 and not over90:
                List[i] = 180 - List[i]
            elif List[i] < 90 and over90:
                List[i] = 180 - List[i]

        bdotc = math.cos(List[3] / 180.0 * math.pi) * List[1] * List[2]
        adotc = math.cos(List[4] / 180.0 * math.pi) * List[0] * List[2]
        adotb = math.cos(List[5] / 180.0 * math.pi) * List[1] * List[0]
        if List[0] > List[1] or (List[0] == List[1] and math.fabs(bdotc) > math.fabs(adotc)):
            List = self.XchangeSides(List, 0, 1)
        bdotc = math.cos(List[3] / 180.0 * math.pi) * List[1] * List[2]
        adotc = math.cos(List[4] / 180.0 * math.pi) * List[0] * List[2]
        adotb = math.cos(List[5] / 180.0 * math.pi) * List[1] * List[0]
        if List[1] > List[2] or (List[1] == List[2] and math.fabs(adotc) > math.fabs(adotb)):
            List = self.XchangeSides(List, 1, 2)
        bdotc = math.cos(List[3] / 180.0 * math.pi) * List[1] * List[2]
        adotc = math.cos(List[4] / 180.0 * math.pi) * List[0] * List[2]
        adotb = math.cos(List[5] / 180.0 * math.pi) * List[1] * List[0]

        if List[0] > List[1] or (List[0] == List[1] and math.fabs(bdotc) > math.fabs(adotc)):
            List = self.XchangeSides(List, 0, 1)

        return List

    def FixUpPlusMinus(self, UB):  # TODO make increasing lengthed sides too
        M = matrix([[1.0, 0.0, 0.0], [0.0, 1.0, 0.0], [0.0, 0.0, 1.0]])
        G = UB.T * UB
        # G.I

        if G[0, 1] > 0:
            if G[0, 2] > 0:
                if G[1, 2] > 0:
                    return UB
                else:
                    M[1, 1] = M[2, 2] = -1
            elif G[1, 2] > 0:
                M[0, 0] = M[2, 2] = -1
            else:
                M[1, 1] = M[0, 0] = -1
        else:
            if G[0, 2] > 0:
                if G[1, 2] > 0:
                    M[1, 1] = M[0, 0] = -1
                else:
                    M[0, 0] = M[2, 2] = -1
            elif G[1, 2] > 0:
                M[2, 2] = M[1, 1] = -1
            else:
                return UB

        return UB * M
        # is closeness to 90 deg( cos of ), and equal sides

    def FixUB(self, UB, tolerance):
        done = 1
        print("A")
        while done == 1:
            done = 0
            X = UB.T * UB
            # X.I

            print("B1", X)
            if X[0, 0] > X[1, 1] or (
                math.fabs(X[0, 0] - X[1, 1]) < tolerance / 10 and math.fabs(X[1, 2]) > math.fabs(X[0, 2]) + tolerance / 10
            ):
                done = 1
                for i in range(0, 3):
                    sav = UB[i, 0]
                    UB[i, 0] = UB[i, 1]
                    UB[i, 1] = sav
                print("B")
                continue

            print("B2")
            if X[1, 1] > X[2, 2] or (math.fabs(X[1, 1] - X[2, 2]) < tolerance and math.fabs(X[1, 0]) < math.fabs(X[2, 0]) - tolerance / 10):
                done = 1
                for i in range(0, 3):
                    sav = UB[i, 1]
                    UB[i, 1] = UB[i, 2]
                    UB[i, 2] = sav

                print("C")
                continue

            print("B3")
            if numpy.linalg.det(UB) < 0:
                for i in range(0, 3):
                    UB[i, 0] = -1 * UB[i, 0]

                print("D")
                done = 1
                continue
            print("E")
            L = [X[0, 1], X[0, 2], X[1, 2]]

            nneg = 0
            is90 = False
            odd = -1
            for i in range(0, 3):
                if math.fabs(L[i]) < tolerance:
                    is90 = True
                    odd = i
                    nneg = nneg + 1
                elif L[i] < 0:
                    nneg = nneg + 1

            if nneg == 3 or nneg == 0:
                continue

            for i in range(0, 3):
                if is90:
                    if nneg == 1:
                        odd = i
                        break
                    if nneg == 2 and odd != i and L[i] > 0:
                        odd = i
                        break

                elif nneg == 1 and L[i] < 0:
                    odd = i
                elif nneg == 2 and L[i] > 0:
                    odd = i
            odd = 2 - odd
            i1 = (odd + 1) % 3
            i2 = (odd + 2) % 3
            print(["L=", L, odd, i1, i2, is90, tolerance])
            print(UB)
            for i in range(0, 3):
                UB[i, i1] = -1 * UB[i, i1]
                UB[i, i2] = -1 * UB[i, i2]
            print(UB)
            done = 1
        return UB

    def getPeaks(self, _Inst, UB, error, Npeaks):
        CreatePeaksWorkspace(InstrumentWorkspace="Sws", NumberOfPeaks=0, OutputWorkspace="Peaks")
        Peaks = mtd["Peaks"]

        MinAbsQ = 100000000
        UBi = matrix([[0.0, 0.0, 0.0], [0.0, 0.0, 0.0], [0.0, 0.0, 0.0]])

        for ii in range(3):
            for jj in range(ii, 3):
                UBi = UB[ii, jj]
                if math.fabs(UBi) < MinAbsQ and UBi != 0:
                    MinAbsQ = math.fabs(UBi)

        hkl = matrix([[0.0], [0.0], [0.0]])

        Error = error * MinAbsQ
        npeaks = 0

        done = False
        while not done:
            Qs = UB * hkl
            Qs *= 2 * math.pi

            for qs in range(3):
                Qs[qs, 0] = self.Perturb(Qs[qs, 0], Error)

            if Qs is not None and Qs[2, 0] > 0:
                # QQ= numpy.array([Qs[0,0],Qs[1,0],Qs[2,0]])
                QQ = mantid.kernel.V3D(Qs[0, 0], Qs[1, 0], Qs[2, 0])
                norm = QQ.norm()

                if 0.3 < norm < 30:
                    peak = Peaks.createPeak(QQ, 1.0)

                    peak.setQLabFrame(mantid.kernel.V3D(Qs[0, 0], Qs[1, 0], Qs[2, 0]), 1.0)

                    Peaks.addPeak(peak)
                    npeaks += 1

            hkl = self.Next(hkl)
            if npeaks >= Npeaks:
                done = True
            if math.fabs(hkl[0, 0]) > 15:
                done = True
            if math.fabs(hkl[1, 0]) > 15:
                done = True
            if math.fabs(hkl[2, 0]) > 15:
                done = True

        return Peaks

    def newSetting(self, side1, side2, Xtal, Center, ang, i1, i2a):
        C = Center
        if Center == "A" or Center == "B" or Center == "C":
            C = "C"
        if Xtal == "O":
            if ang > 20 or i1 > 0 or i2a > 1:
                return False
            elif (side1 == 0 and side2 != 0) and (C == "F" or C == "C"):  # No Tetragonal "F" or C Center
                return False
            elif (C == "F" or C == "C") and (side1 == side2 and side1 != 0):
                return False
            else:
                return True

        if Xtal == "H":
            if ang > 20 or i2a > 1 or not (C == "P" or C == "I"):
                return False
            elif side2 > side1:
                return False
            else:
                return True

        if Xtal != "M":
            return False
        return True

    def MonoClinicRearrange(self, Sides, _Xtal, _Center, i1, i2a):
        i1q = i1
        i2q = (i1 + i2a) % 3
        i3q = (i2q + 1) % 3
        if i1q == i3q:
            i3q = (i3q + 1) % 3
        a = Sides[i1q]
        b = Sides[i2q]
        c = Sides[i3q]

        return [a, b, c]

    def getMatrixAxis(self, v, Xtal):
        ident = matrix([[1.0, 0.0, 0.0], [0.0, 1.0, 0.0], [0.0, 0.0, 1.0]])
        if Xtal != "H" or v >= 2:
            return ident
        ident[v, v] = 0
        ident[2, 2] = 0
        v1 = 2
        ident[v, v1] = 1
        ident[v1, v] = 1
        return ident

    def getLat(self, UB):
        G = UB.T * UB
        G1 = G.I
        Res = [
            math.sqrt(G1[0, 0]),
            math.sqrt(G1[1, 1]),
            math.sqrt(G1[2, 2]),
            math.acos(G1[1, 2] / Res[1] / Res[2]) * 180.0 / math.pi,
            math.acos(G1[0, 2] / Res[0] / Res[2]) * 180.0 / math.pi,
            math.acos(G1[0, 1] / Res[0] / Res[1]) * 180.0 / math.pi,
        ]
        return Res

    def AppendForms(self, condition, Center, CenterTarg, FormNums, List2Append):
        L = List2Append
        if condition and Center != CenterTarg:
            for i in range(len(FormNums)):
                L.append(FormNums[i])
        elif Center == CenterTarg:
            for i in range(len(FormNums)):
                L.append(FormNums[i])
        return L

    def Xlate(self, Xtal, Center, sides, LatNiggle):  # sides are sides of conventional cell
        if Xtal == "O":
            C = Center
            if sides[0] == sides[1]:
                if sides[1] == sides[2]:
                    X = "Cubic"
                    Z1 = list(self.Cubic)
                else:
                    X = "Tetragonal"
                    Z1 = list(self.Tetr)
            elif sides[0] == sides[2]:
                X = "Tetragonal"
                Z1 = list(self.Tetr)
            elif sides[1] == sides[2]:
                X = "Tetragonal"
                Z1 = list(self.Tetr)
            else:
                X = "Orthorhombic"
                Z1 = list(self.Orth)

            if C == "A" or C == "B":
                C = "C"

        elif Xtal == "H":
            if Center == "I":
                C = "R"
                X = "Rhombohedral"
                Z1 = list(self.Hex)
            else:
                C = "P"
                X = "Hexagonal"
                Z1 = list(self.Hex)
        else:  # Monoclinic
            X = "Monoclinic"
            Z1 = list(self.Mon)
            C = Center
            LL = [
                math.cos(LatNiggle[5] / 180 * math.pi) * LatNiggle[0] * LatNiggle[1],
                math.cos(LatNiggle[4] / 180 * math.pi) * LatNiggle[0] * LatNiggle[2],
                math.cos(LatNiggle[3] / 180 * math.pi) * LatNiggle[2] * LatNiggle[1],
            ]

            if C == "A" or C == "B":
                C = "C"

            if C == "C" or C == "I":  # 'I':
                Z1 = self.AppendForms(LatNiggle[2] * LatNiggle[2] < 4 * math.fabs(LL[2]) + 0.001, "C", C, [10, 14, 39], Z1)
                Z1 = self.AppendForms(LatNiggle[0] * LatNiggle[0] < 4 * math.fabs(LL[1]) + 0.001, "C", C, [20, 25, 41], Z1)

                Z1 = self.AppendForms(LatNiggle[1] * LatNiggle[1] < 4 * math.fabs(LL[2] + 0.001), "C", C, [37], Z1)

                Z1 = self.AppendForms(
                    3 * LatNiggle[0] * LatNiggle[0] < LatNiggle[2] * LatNiggle[2] + 2 * math.fabs(LL[1]) + 0.001, "I", C, [17], Z1
                )
                Z1 = self.AppendForms(
                    3 * LatNiggle[1] * LatNiggle[1] < LatNiggle[2] * LatNiggle[2] + 2 * math.fabs(LL[2] + 0.001), "I", C, [27], Z1
                )

        if C == "P":
            Z2 = self.CentP
        elif C == "F":
            Z2 = self.CentF
        elif C == "I" or C == "R":
            Z2 = self.CentI
        elif C == "C":
            Z2 = self.CentC
        Z1 = sorted(Z1)
        return [X, C, Z1, Z2]

    def MatchXtlparams(self, List1a, List2, tolerance, message):
        List1 = List1a

        self.assertEqual(len(List1a), 6, "Not the correct number of Xtal parameters." + message)
        self.assertEqual(len(List2), 6, "Not the correct number of Xtal parameters." + message)
        Var = ["a", "b", "c", "alpha", "beta", "gamma"]
        self.assertDelta(List1[0], List2[0], tolerance, message + "for " + Var[0])
        self.assertDelta(List1[1], List2[1], tolerance, message + "for " + Var[1])
        self.assertDelta(List1[2], List2[2], tolerance, message + "for " + Var[2])
        angtolerance = tolerance * 180 / math.pi
        if List1[3] < 90 and List2[3] >= 90:
            List1[3] = 180 - List1[3]
            List1[4] = 180 - List1[4]
            List1[5] = 180 - List1[5]

        if List1[0] > List1[1] - tolerance:
            if List1[1] > List1[2] - tolerance:  # 3 equal sides
                match = False

                _i = 0
                for _i in range(0, 3):
                    match = (
                        math.fabs(List1[3] - List2[3]) < angtolerance
                        and math.fabs(List1[4] - List2[4]) < angtolerance
                        and math.fabs(List1[5] - List2[5]) < angtolerance
                    )

                    if match:
                        break
                    List1 = self.XchangeSides(List1, 1, 0)

                    match = (
                        math.fabs(List1[3] - List2[3]) < angtolerance
                        and math.fabs(List1[4] - List2[4]) < angtolerance
                        and math.fabs(List1[5] - List2[5]) < angtolerance
                    )
                    if match:
                        break

                    List1 = self.XchangeSides(List1, 1, 2)

                match = (
                    math.fabs(List1[3] - List2[3]) < angtolerance
                    and math.fabs(List1[4] - List2[4]) < angtolerance
                    and math.fabs(List1[5] - List2[5]) < angtolerance
                )
                self.assertTrue(match, "Angles do not match in any order")
            else:
                self.assertDelta(List1[5], List2[5], angtolerance, "Error in " + Var[5])
                if math.fabs(List1[3] - List2[3]) > angtolerance:
                    List1 = self.XchangeSides(List1, 0, 1)
                self.assertDelta(List1[3], List2[3], angtolerance, "Error in " + Var[3])
                self.assertDelta(List1[4], List2[4], angtolerance, "Error in " + Var[4])
        elif List1[1] > List1[2] - tolerance:
            self.assertDelta(List1[3], List2[3], angtolerance, "Error in " + Var[3])
            if math.fabs(List1[4] - List2[4]) > angtolerance:
                List1 = self.XchangeSides(List1, 1, 2)

            self.assertDelta(List1[4], List2[4], angtolerance, "Error in " + Var[5])

            self.assertDelta(List1[5], List2[5], angtolerance, "Error in " + Var[5])
        else:
            self.assertDelta(List1[3], List2[3], angtolerance, "Error in " + Var[3])

            self.assertDelta(List1[4], List2[4], angtolerance, "Error in " + Var[5])

            self.assertDelta(List1[5], List2[5], angtolerance, "Error in " + Var[5])

    def XchangeSides(self, Lat1, s1, s2):
        Lat = list(Lat1)
        if s1 < 0 or s2 < 0 or s1 >= 3 or s2 > 2 or s1 == s2:
            return Lat
        sav = Lat[s1]
        Lat[s1] = Lat[s2]
        Lat[s2] = sav
        sav = Lat[s1 + 3]
        Lat[s1 + 3] = Lat[s2 + 3]
        Lat[s2 + 3] = sav

        return Lat

    def GetConvCell(self, Peaks, XtalCenter1, wsName, nOrigIndexed, tolerance, matchLat):
        CopySample(Peaks, wsName, CopyMaterial="0", CopyEnvironment="0", CopyName="0", CopyShape="0", CopyLattice="1")
        OrLat = mtd[wsName].sample().getOrientedLattice()
        Lat1 = [OrLat.a(), OrLat.b(), OrLat.c(), OrLat.alpha(), OrLat.beta(), OrLat.gamma()]
        FormXtal = XtalCenter1[2]
        FormCenter = XtalCenter1[3]
        i1 = 0
        i2 = 0
        Lat0 = self.FixLatParams(matchLat)
        Lat1 = self.FixLatParams(Lat1)
        # print "--------------------- Getting the Conventional Cell for--------------------------------"
        # print Lat1
        # print Lat0
        # print [FormXtal,FormCenter]
        angTolerance = tolerance * 180 / math.pi
        while i1 < len(FormXtal) and i2 < len(FormCenter):
            if FormXtal[i1] < FormCenter[i2]:
                i1 = i1 + 1
            elif FormXtal[i1] > FormCenter[i2]:
                i2 = i2 + 1
            else:
                Res = SelectCellWithForm(Peaks, FormXtal[i1], True)

                if Res[0] > 0.85 * nOrigIndexed:
                    CopySample(Peaks, "Temp", CopyMaterial="0", CopyEnvironment="0", CopyName="0", CopyShape="0", CopyLattice="1")
                    OrLat = mtd["Temp"].sample().getOrientedLattice()
                    Lat1 = [OrLat.a(), OrLat.b(), OrLat.c(), OrLat.alpha(), OrLat.beta(), OrLat.gamma()]
                    Lat1 = self.FixLatParams(Lat1)
                    print(["Formnum,Lat1,Lat0", FormXtal[i1], Lat1, Lat0])
                    if (
                        math.fabs(Lat0[0] - Lat1[0]) < tolerance
                        and math.fabs(Lat0[1] - Lat1[1]) < tolerance
                        and math.fabs(Lat0[2] - Lat1[2]) < tolerance
                    ):
                        for dummy_i in range(3):
                            if (
                                math.fabs(Lat0[3] - Lat1[3]) < angTolerance
                                and math.fabs(Lat0[4] - Lat1[4]) < angTolerance
                                and math.fabs(Lat0[5] - Lat1[5]) < angTolerance
                            ):
                                break
                            if Lat1[0] > Lat1[1] - tolerance:
                                Lat1 = self.XchangeSides(Lat1, 0, 1)

                            if (
                                math.fabs(Lat0[3] - Lat1[3]) < angTolerance
                                and math.fabs(Lat0[4] - Lat1[4]) < angTolerance
                                and math.fabs(Lat0[5] - Lat1[5]) < angTolerance
                            ):
                                break
                            if Lat1[1] > Lat1[2] - tolerance:
                                Lat1 = self.XchangeSides(Lat1, 1, 2)

                            if (
                                math.fabs(Lat0[3] - Lat1[3]) < angTolerance
                                and math.fabs(Lat0[4] - Lat1[4]) < angTolerance
                                and math.fabs(Lat0[5] - Lat1[5]) < angTolerance
                            ):
                                break

                        if (
                            math.fabs(Lat0[3] - Lat1[3]) < angTolerance
                            and math.fabs(Lat0[4] - Lat1[4]) < angTolerance
                            and math.fabs(Lat0[5] - Lat1[5]) < angTolerance
                        ):
                            return Lat1
                    i1 = i1 + 1
                    i2 = i2 + 1
                    CopySample(wsName, Peaks, CopyMaterial="0", CopyEnvironment="0", CopyName="0", CopyShape="0", CopyLattice="1")
        return []

    def runTest(self):
        CreateSingleValuedWorkspace(OutputWorkspace="Sws", DataValue="3")

        CreateSingleValuedWorkspace(OutputWorkspace="Temp", DataValue="3")
        LoadInstrument(Workspace="Sws", InstrumentName="TOPAZ", RewriteSpectraMap=True)
        Inst = mtd["Sws"].getInstrument()
        startA = 2
        side1Ratios = [1.0, 1.2, 3.0, 8.0]
        alphas = [20, 50, 80, 110, 140]
        xtal = ["O", "M", "H"]  # ['O','M','H']
        centerings = ["P", "I", "F", "A", "B", "C"]
        # ['P','I','F','A', 'B',  'C']
        error = [0.0]  # [ 0, .05,  0.1, 0, 0.15]
        Npeaks = 150
        for Error in error:
            for side1 in range(0, 4):  # make (0,4)
                for side2 in range(side1, 4):  # make side1,4
                    for Xtal in xtal:
                        for Center in centerings:
                            for ang in alphas:
                                for i1 in range(3):
                                    for i2a in range(1, 3):
                                        if self.newSetting(side1, side2, Xtal, Center, ang, i1, i2a):
                                            print("=============================================================")
                                            Sides = [startA, startA * side1Ratios[side1], startA * side1Ratios[side2]]
                                            Sides = self.MonoClinicRearrange(Sides, Xtal, Center, i1, i2a)
                                            print([Sides, Error, Xtal, Center, ang, i1, i2a])

                                            UBconv = self.CalcConventionalUB(Sides[0], Sides[1], Sides[2], ang, ang, ang, Xtal)

                                            UBnig = self.CalcNiggliUB(Sides[0], Sides[1], Sides[2], ang, ang, ang, Xtal, Center)

                                            UBconv = self.conventionalUB
                                            V = self.getMatrixAxis(i1, Xtal)
                                            if UBconv is None:
                                                continue
                                            if UBnig is None:
                                                continue
                                            UBnig = V * UBnig
                                            UBconv = V * UBconv
                                            # UBnig1= self.FixUB(UBnig,.05)
                                            UBnig = self.FixUpPlusMinus(UBnig)
                                            UBconv = self.FixUpPlusMinus(UBconv)
                                            Lat0 = self.getLat(UBnig)

                                            Lat0 = self.FixLatParams(Lat0)
                                            print(["UBnig", UBnig, Lat0])

                                            Peaks = self.getPeaks(Inst, UBnig, Error, Npeaks + Error * 300)

                                            # -------Failed tests because of FindUBUsingFFT -------------------

                                            if (
                                                side1 == 1
                                                and side2 == 2
                                                and Error == 0.0
                                                and Xtal == "M"
                                                and Center == "C"
                                                and i1 == 0
                                                and i2a == 1
                                                and ang == 140
                                            ):
                                                continue

                                            if (
                                                side1 == 2
                                                and side2 == 2
                                                and Error == 0.0
                                                and Xtal == "M"
                                                and Center == "P"
                                                and i1 == 1
                                                and i2a == 1
                                                and ang == 110
                                            ):
                                                continue  # one side doubled

                                            if (
                                                side1 == 3
                                                and side2 == 3
                                                and Error == 0.0
                                                and Xtal == "M"
                                                and Center == "I"
                                                and i1 == 1
                                                and i2a == 2
                                            ):
                                                continue

                                            if (
                                                side1 == 3
                                                and side2 == 3
                                                and Error == 0.0
                                                and Xtal == "M"
                                                and Center == "I"
                                                and i1 == 2
                                                and i2a == 1
                                            ):
                                                continue

                                            if (
                                                side1 == 3
                                                and side2 == 3
                                                and Error == 0.0
                                                and Xtal == "H"
                                                and Center == "I"
                                                and i1 == 2
                                                and i2a == 1
                                                and ang == 20
                                            ):
                                                continue
                                                # ------------------------------ end Failed FindUB test------------
                                            FindUBUsingFFT(Peaks, Lat0[0] * 0.5, Lat0[2] * 2.0, 0.15)
                                            InPks = IndexPeaks(Peaks, 0.10)

                                            CopySample(
                                                Peaks,
                                                "Sws",
                                                CopyMaterial="0",
                                                CopyEnvironment="0",
                                                CopyName="0",
                                                CopyShape="0",
                                                CopyLattice="1",
                                            )
                                            OrLat = mtd["Sws"].sample().getOrientedLattice()

                                            Lat1 = [OrLat.a(), OrLat.b(), OrLat.c(), OrLat.alpha(), OrLat.beta(), OrLat.gamma()]

                                            Lat1 = self.FixLatParams(Lat1)

                                            MatchXtalTol = 0.03 * (1 + 4 * Error) * (side1Ratios[side2])
                                            print(Lat0)
                                            print(Lat1)
                                            self.MatchXtlparams(Lat1, Lat0, MatchXtalTol, "Niggli values do not match")

                                            # Now see if the conventional cell is in list
                                            XtalCenter1 = self.Xlate(Xtal, Center, Sides, Lat0)  # get proper strings for SelectCellOfType

                                            Lat0 = self.getLat(UBconv)
                                            Lat0 = self.FixLatParams(Lat0)
                                            Lat1 = self.GetConvCell(Peaks, XtalCenter1, "Sws", InPks[0], MatchXtalTol, Lat0)

                                            Lat1 = self.FixLatParams(Lat1)

                                            self.MatchXtlparams(Lat1, Lat0, MatchXtalTol, "Conventional lattice parameter do not match")
                                            self.assertTrue(len(Lat1) > 4, "Conventional values do not match")
                                            # "XYXYZS"

    def requiredFiles(self):
        return []
