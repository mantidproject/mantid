# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from mantid import config
from mantid.api import (DataProcessorAlgorithm, AlgorithmFactory, Progress,
                        FileFinder, MultipleFileProperty, ITableWorkspaceProperty)
from mantid.simpleapi import (CreateEmptyTableWorkspace, CreateSampleWorkspace, LoadLog, LoadNexusLogs)
from mantid.kernel import (Direction, FloatBoundedValidator, IntListValidator,
                           StringMandatoryValidator, V3D)
import numpy as np
from os import path
import warnings


class FindGoniometerFromUB(DataProcessorAlgorithm):

    def name(self):
        return "FindGoniometerFromUB"

    def category(self):
        return "Diffraction\\Reduction"

    def seeAlso(self):
        return ["SetGoniometer", "CalculateUMatrix"]

    def summary(self):
        return "Takes a series of exported UB (from SaveISawUB) and using the first UB supplied as a reference" \
               " saves UB files that share common indexing (i.e. no inversion or swapping of crystallographic axes " \
               "relative to the reference UB) in the default save directory and produces a table of the goniometer" \
               " angle (phi) and the unit vector of the goniometer axis for each run (excl. the reference)."

    def PyInit(self):
        # Input
        self.declareProperty(
            MultipleFileProperty('UBfiles', extensions=".mat"),
            doc='Files with exported UB (from SaveISawUB). The filename must have .mat extension and only contain '
                'the run number as would be supplied to Load (e.g. WISH00043350.mat). The first UB will be the '
                'reference (typically, but not necessarily, omega = phi = 0) and a consistent UB will be made '
                'for each of the subsequent runs.')
        self.declareProperty(
            name="Chi",
            defaultValue=45.0,
            direction=Direction.Input,
            validator=FloatBoundedValidator(
                lower=0.0, upper=90.0),
            doc="Angle of the goniometer axis from +ve Z (vertical up).")
        self.declareProperty(
            name="ChiTol",
            defaultValue=5.0,
            validator=FloatBoundedValidator(
                lower=0.0, upper=45.0),
            direction=Direction.Input,
            doc="Tolerance of chi angle to test for consistency.")
        self.declareProperty(
            name="PhiTol",
            defaultValue=15.0,
            validator=FloatBoundedValidator(
                lower=0.0, upper=45.0),
            direction=Direction.Input,
            doc="Tolerance of phi angle to test for consistency (due to uncertainty on the gonio axis at omega=0"
                "this tol might need to be quite large).")
        self.declareProperty(
            name="PhiHand",
            defaultValue=1,
            validator=IntListValidator([-1, 1]),
            direction=Direction.Input,
            doc="Handedness for the phi angles (rotation around goniometer axis) in the"
                " log file (1 for ccw/RH, -1 for cw/LH)")
        self.declareProperty(
            name="DOmega",
            defaultValue=0.0,
            direction=Direction.Input,
            validator=FloatBoundedValidator(
                lower=0.0, upper=180.0),
            doc="omega rotation between component of goniometer axis in XY-plane from +ve Y-axis"
                "(perpendicular to ki)")
        self.declareProperty(
            name="OmegaHand",
            defaultValue=1,
            validator=IntListValidator([-1, 1]),
            direction=Direction.Input,
            doc="Handedness for the omega angles (rotation around +ve Z) in the log file (1 for ccw/RH, -1 for cw/LH)")
        # output
        self.declareProperty(
            ITableWorkspaceProperty(
                name="OutputTable",
                defaultValue="",
                validator=StringMandatoryValidator(),
                direction=Direction.Output),
            doc="Output table to display goniometer axis and angles for runs with UBs that can share common indexing")

    def validateInputs(self):
        issues = dict()
        # UBpath must contain more than one run
        ubFiles = self.getProperty('UBfiles').value
        if len(ubFiles) < 2:
            issues['UBfiles'] = "At least two UB files are required"
        return issues

    def PyExec(self):

        # setup progress bar
        prog_reporter = Progress(self, start=0.0, end=1.0, nreports=3)
        # Get input
        ubFiles = self.getProperty('UBfiles').value
        chiRef = self.getProperty('Chi').value
        chiTol = self.getProperty('ChiTol').value
        phiTol = self.getProperty('PhiTol').value
        phiHand = self.getProperty('PhiHand').value
        dOmega = self.getProperty('DOmega').value
        omegaHand = self.getProperty('OmegaHand').value
        # rotation matrices work in right-handed coordinate system so the handedness

        # self.log().information(msg)
        # MultipleFileProperty -> UB matrix (first one for reference)

        # update progress
        prog_reporter.report(1, "Load Logs and UBs")

        matUB = []  # container to hold UB matrix arrays
        omega = np.zeros(len(ubFiles))  # rot around vertical axis (assumed to be correct)
        phiRef = np.zeros(len(ubFiles))  # rot around gonio axis (needs to be refined)
        for irun in range(0, len(ubFiles)):
            # get rotation angles from logs (handedness given in input)
            # these rotation matrices are defined in right-handed coordinate system (i.e. omegaHand = 1 etc.)
            tmpWS = CreateSampleWorkspace(StoreInADS=False)
            _, fname = path.split(ubFiles[irun])
            dataPath = FileFinder.findRuns(fname.split('.mat')[0])[0]
            if dataPath[-4:] == ".raw":
                # assume log is kept separately in a .log file with same path
                LoadLog(Workspace=tmpWS, Filename="".join(dataPath[:-4] + '.log'))
            elif dataPath[-4:] == '.nxs':
                # logs are kept with data in nexus file
                LoadNexusLogs(Workspace=tmpWS, Filename=dataPath)
            # read omega and phi (in RH coords)
            omega[irun] = omegaHand * tmpWS.getRun().getLogData('ccr_pos').value[0]
            phiRef[irun] = phiHand * tmpWS.getRun().getLogData('ewald_pos').value[0]
            # load UB
            matUB += [self.readUB(ubFiles[irun])]

        # update progress
        prog_reporter.report(2, "Calculate Rotations")

        # create table to store the found goniometer axes and angles
        gonioTable = CreateEmptyTableWorkspace(StoreInADS=False)
        # Add some columns, Recognized types are: int,float,double,bool,str,V3D,long64
        gonioTable.addColumn(type="str", name="Run")
        gonioTable.addColumn(type="float", name="Chi")
        gonioTable.addColumn(type="float", name="Phi")
        gonioTable.addColumn(type="V3D", name="GonioAxis")

        # get save directory
        saveDir = config['defaultsave.directory']

        # make the UB for omega = 0 from reference
        zeroUB = self.getR(omega[0], [0, 0, 1]).T @ matUB[0]
        # get relative change in phi in range [0,360)
        dPhiRef = phiRef - phiRef[0]
        dPhiRef = dPhiRef + np.ceil(-dPhiRef / 360) * 360

        # calculate the rotation matrix that maps the UB of the first run onto subsequent UBs
        for irun in range(1, len(ubFiles)):
            chi, phi, u = self.getGonioAngles(matUB[irun], zeroUB, omega[irun])
            # phi relative to first run in RH/LH convention of user
            # check if phi and chi are not within tolerance of expected
            if abs(chi - chiRef) > chiTol and abs(phi - dPhiRef[irun]) > phiTol:
                # generate predicted UB to find axes permutation
                self.log().information("The following UB\n{}\nis not consistent with the reference, attempting to "
                                       "find an axes swap/inversion that make it consistent.")
                # nominal goniometer axis
                gonio = self.getR(omegaHand * dOmega, [0, 0, 1]) @ self.getR(-chiRef, [1, 0, 0]) @ [0, 0, 1]
                predictedUB = self.getR(omega[irun], [0, 0, 1]) @ self.getR(dPhiRef[irun], gonio) @ zeroUB
                # try a permutation of the UB axes (as in TransformHKL)
                # UB' = UB M^-1
                # HKL' = M HKL
                minv = np.linalg.inv(matUB[irun]) @ predictedUB
                minv = self.getSignMaxAbsValInCol(minv)
                # redo angle calculation on permuted UB
                matUB[irun] = matUB[irun] @ minv
                chi, phi, u = self.getGonioAngles(matUB[irun], zeroUB, omega[irun])

            if abs(chi - chiRef) <= chiTol and abs(phi - dPhiRef[irun]) <= phiTol:
                # save the consistent UB to the ubPath
                _, nameUB = path.split(ubFiles[irun])
                newUBPath = path.join(saveDir, nameUB[:-4] + '_consistent' + nameUB[-4:])
                self.writeConsistentUB(ubFiles[irun], newUBPath, matUB[irun])
                # populate row of table
                phi2print = phiHand * (phi + phiRef[0])
                phi2print = phi2print + np.ceil(-phi2print / 360) * 360
                nextRow = {'Run': nameUB[:-4],
                           'Chi': chi,
                           'Phi': phi2print,
                           'GonioAxis': V3D(u[0], u[1], u[2])}
                gonioTable.addRow(nextRow)
            else:
                warnings.warn("WARNING: The UB {0} cannot be made consistent with the reference UB. "
                              "Check the goniometer angles and handedness supplied "
                              "and the accuracy of reference UB.".format(ubFiles[irun]))

        # assign output
        self.setProperty("OutputTable", gonioTable)
        # complete progress
        prog_reporter.report(3, "Done")

    def readUB(self, fpath):
        """
        :param fpath: path to UB files exported with SaveIsawUB
        :return: 3x3 UB matrix
        """
        ub = np.loadtxt(fpath, skiprows=0, max_rows=3).T
        return ub

    def getSignMaxAbsValInCol(self, mat):
        """
        Used to find most likely permutation of axes to provide consistency with reference UB.
        :param mat: a 2D array
        :return out: sign of largest element in each column of abs(matrix)
        """
        return np.sign(mat) * (abs(mat) == abs(mat).max(axis=0))

    def getSingleAxis(self, r):
        """
        Single-axis rotation for a given rotation matrix using the Rodrigues formula
        :param r: Rotation matrix from which to find the single-axis rotation
        :return: theta: rotation (ccw) around goniometer axis
        :return: u: goniometer axis unit vector
        """
        phi = np.arccos((np.trace(r) - 1) / 2) * (180 / np.pi)
        u = (1 / (2 * np.sin(phi * np.pi / 180))) * np.array(
            [r[2, 1] - r[1, 2], r[0, 2] - r[2, 0], r[1, 0] - r[0, 1]])
        return phi, u

    def getR(self, theta, u):
        """
        Make a rotation axis for any angle around a supplied axis
        :param theta: angle of rotation (ccw) around the axis provided
        :param u: unit vector for axis of rotation
        :return: R: rotation matrix
        """
        w = np.array([[0, -u[2], u[1]], [u[2], 0, -u[0]], [-u[1], u[0], 0]])
        return np.eye(3, 3) + np.sin(theta * np.pi / 180) * w + (1 - np.cos(theta * np.pi / 180)) * (
                w @ w)

    def getGonioAngles(self, aUB, zeroUB, omega):
        """
        Do the inverse of the rotation around the vertical axis between aUB and the zeroUB
        then use Rodrigues' rotation formula to find the single-axis rotation
        (i.e. the goniometer axis in the reference run).
        :param aUB: UB matrix at finite goniometer angles (omega and phi)
        :param zeroUB: UB matrix for omega = 0
        :param omega: omega rotation
        :return: chi: angle of gonio from +ve Z axis
        :return: phi: angle of rotation (ccw) around gonio axis to get between reference and aUB run
        :return: u: goniometer axis unit vector
        """

        # undo omega rotation between  (recall for a rotation matrix R^-1 = R.T)
        # rOmega rPhi U = U'
        rPhi = self.getR(omega, [0, 0, 1]).T @ aUB @ np.linalg.inv(zeroUB)
        phi, u = self.getSingleAxis(rPhi)
        # force z component to be positive (i.e. vertically upwards)
        if np.sign(u[-1]) < 0:
            u = u * np.sign(u[-1])
            phi = -phi
        # calculate chi [0,90]
        chi = np.arccos(np.dot(u, [0, 0, 1])) * (180 / np.pi)
        # force phi to be in range [0,360)
        phi = phi + np.ceil(-phi / 360) * 360
        return chi, phi, u

    def writeConsistentUB(self, oldUBPath, newUBPath, newUB):
        """
        :param oldUBPath: path of old UB that was loaded (incl. filename)
        :param newUBPath: path to write the consistent UB (incl. filename)
        :param newUB: the new consistent UB to write
        """
        # read the content of the old UB (very small file)
        with open(oldUBPath, 'r') as fid:
            lines = fid.read().splitlines()
        # replace the first three lines with the three cols of UB matrix
        # (.mat file contains the transpose of the UB matrix)
        for irow in range(0, newUB.shape[1]):
            lines[irow] = " ".join(["{0:>11.8f}".format(uu) for uu in newUB[:, irow]]) + " "
        # write lines to new file (with a newline at end)
        with open(newUBPath, 'w+') as fid:
            fid.writelines('\n'.join(lines + ['']))


# register algorithm with mantid
AlgorithmFactory.subscribe(FindGoniometerFromUB)
