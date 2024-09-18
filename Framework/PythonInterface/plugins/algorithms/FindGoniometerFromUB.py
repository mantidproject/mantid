# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from mantid import config
from mantid.api import DataProcessorAlgorithm, AlgorithmFactory, Progress, FileFinder, MultipleFileProperty, ITableWorkspaceProperty
from mantid.kernel import Direction, FloatBoundedValidator, IntListValidator, StringMandatoryValidator, V3D
import numpy as np
from os import path
import warnings


def getSignMaxAbsValInCol(mat):
    """
    Used to find most likely permutation of axes to provide consistency with reference UB.
    :param mat: a 2D array
    :return out: sign of largest element in each column of abs(matrix)
    """
    return np.sign(mat) * (abs(mat) == abs(mat).max(axis=0))


def getR(theta, u):
    """
    Make a rotation axis for any angle around a supplied axis
    :param theta: angle of rotation (ccw) around the axis provided
    :param u: unit vector for axis of rotation
    :return: R: rotation matrix
    """
    w = np.array([[0, -u[2], u[1]], [u[2], 0, -u[0]], [-u[1], u[0], 0]])
    return np.eye(3, 3) + np.sin(theta * np.pi / 180) * w + (1 - np.cos(theta * np.pi / 180)) * (w @ w)


class FindGoniometerFromUB(DataProcessorAlgorithm):
    def name(self):
        return "FindGoniometerFromUB"

    def category(self):
        return "Diffraction\\Reduction"

    def seeAlso(self):
        return ["SetGoniometer", "CalculateUMatrix"]

    def summary(self):
        return (
            "Takes a series of exported UB (from SaveISawUB) and using the first UB supplied as a reference"
            " saves UB files that share common indexing (i.e. no inversion or swapping of crystallographic axes "
            "relative to the reference UB) in the default save directory and produces a table of the goniometer"
            " angle (phi) and the unit vector of the goniometer axis for each run (excl. the reference)."
        )

    def PyInit(self):
        # Input
        self.declareProperty(
            MultipleFileProperty("UBfiles", extensions=".mat"),
            doc="Files with exported UB (from SaveISawUB). The filename must have .mat extension and only contain "
            "the run number as would be supplied to Load (e.g. WISH00043350.mat). The first UB will be the "
            "reference (typically, but not necessarily, omega = phi = 0) and a consistent UB will be made "
            "for each of the subsequent runs.",
        )
        self.declareProperty(
            name="Chi",
            defaultValue=45.0,
            direction=Direction.Input,
            validator=FloatBoundedValidator(lower=0.0, upper=90.0),
            doc="Angle of the goniometer axis from +ve Z (vertical up).",
        )
        self.declareProperty(
            name="ChiTol",
            defaultValue=5.0,
            validator=FloatBoundedValidator(lower=0.0, upper=45.0),
            direction=Direction.Input,
            doc="Tolerance of chi angle to test for consistency.",
        )
        self.declareProperty(
            name="PhiTol",
            defaultValue=15.0,
            validator=FloatBoundedValidator(lower=0.0, upper=45.0),
            direction=Direction.Input,
            doc="Tolerance of phi angle to test for consistency (due to uncertainty on the gonio axis at omega=0"
            "this tol might need to be quite large).",
        )
        self.declareProperty(
            name="PhiHand",
            defaultValue=1,
            validator=IntListValidator([-1, 1]),
            direction=Direction.Input,
            doc="Handedness for the phi angles (rotation around goniometer axis) in the" " log file (1 for ccw/RH, -1 for cw/LH)",
        )
        self.declareProperty(
            name="PhiLogName",
            defaultValue="ewald_pos",
            direction=Direction.Input,
            validator=StringMandatoryValidator(),
            doc="Name of log entry that records angle of rotation about goniometer axis",
        )
        self.declareProperty(
            name="DOmega",
            defaultValue=0.0,
            direction=Direction.Input,
            validator=FloatBoundedValidator(lower=0.0, upper=180.0),
            doc="omega rotation between component of goniometer axis in XY-plane from +ve Y-axis" "(perpendicular to ki)",
        )
        self.declareProperty(
            name="OmegaHand",
            defaultValue=1,
            validator=IntListValidator([-1, 1]),
            direction=Direction.Input,
            doc="Handedness for the omega angles (rotation around +ve Z) in the log file (1 for ccw/RH, -1 for cw/LH)",
        )
        self.declareProperty(
            name="OmegaLogName",
            defaultValue="ccr_pos",
            direction=Direction.Input,
            validator=StringMandatoryValidator(),
            doc="Name of log entry that records angle of rotation about vertical axis",
        )
        # output
        self.declareProperty(
            ITableWorkspaceProperty(name="OutputTable", defaultValue="", validator=StringMandatoryValidator(), direction=Direction.Output),
            doc="Output table to display goniometer axis and angles for runs with UBs that can share common indexing",
        )

    def validateInputs(self):
        issues = dict()
        # UBpath must contain more than one run
        ubFiles = self.getProperty("UBfiles").value
        if len(ubFiles) < 2:
            issues["UBfiles"] = "At least two UB files are required"
        return issues

    def PyExec(self):
        # setup progress bar
        prog_reporter = Progress(self, start=0.0, end=1.0, nreports=3)
        # Get input
        ubFiles = self.getProperty("UBfiles").value
        chiRef = self.getProperty("Chi").value
        chiTol = self.getProperty("ChiTol").value
        phiTol = self.getProperty("PhiTol").value
        phiHand = self.getProperty("PhiHand").value
        phiLogName = self.getProperty("PhiLogName").value
        dOmega = self.getProperty("DOmega").value
        omegaHand = self.getProperty("OmegaHand").value
        omegaLogName = self.getProperty("OmegaLogName").value

        # update progress
        prog_reporter.report(1, "Load Logs and UBs")

        matUB, omega, phiRef = self.loadUBFiles(ubFiles, omegaHand, phiHand, omegaLogName, phiLogName)

        # update progress
        prog_reporter.report(2, "Calculate Rotations")

        # create table to store the found goniometer axes and angles
        gonioTable = self.createGoniometerTable()

        # make the UB for omega = 0 from reference
        zeroUB = getR(omega[0], [0, 0, 1]).T @ matUB[0]
        # get relative change in phi in range [0,360)
        dPhiRef = phiRef - phiRef[0]
        dPhiRef = dPhiRef + np.ceil(-dPhiRef / 360) * 360

        self.findConsistentUB(
            ubFiles, zeroUB, matUB, omega, dOmega, omegaHand, phiRef, dPhiRef, phiTol, phiHand, chiRef, chiTol, gonioTable
        )

        # assign output
        self.setProperty("OutputTable", gonioTable)
        # complete progress
        prog_reporter.report(3, "Done")

    def loadUBFiles(self, ubFiles, omegaHand, phiHand, omegaLogName, phiLogName):
        """
        load the ub files and update the
        :param ubFiles: list of paths to saved UB files
        :param omegaHand: handedness of omega rotation (ccw/cw)
        :param phiHand: handedness of phi rotation (ccw/cw)
        :param omegaLogName: name of log entry for omega angle
        :param phiLogName: name of log entry for phi angle
        :return: matUB: list containing the UB for each run
        :return: omega: array of omega values from log of each run
        :return: phiRef: array of nominal phi values from log of each run
        """
        matUB = []  # container to hold UB matrix arrays
        omega = np.zeros(len(ubFiles))  # rot around vertical axis (assumed to be correct)
        phiRef = np.zeros(len(ubFiles))  # rot around gonio axis (needs to be refined)
        for irun in range(0, len(ubFiles)):
            # get rotation angles from logs (handedness given in input)
            # these rotation matrices are defined in right-handed coordinate system (i.e. omegaHand = 1 etc.)
            _, fname = path.split(ubFiles[irun])
            dataPath = FileFinder.findRuns(fname.split(".mat")[0])[0]
            tmpWS = self.exec_child_alg("CreateSampleWorkspace")
            if dataPath[-4:] == ".raw":
                # assume log is kept separately in a .log file with same path
                self.exec_child_alg("CreateSampleWorkspace", Workspace=tmpWS, Filename="".join(dataPath[:-4] + ".log"))
            elif dataPath[-4:] == ".nxs":
                # logs are kept with data in nexus file
                self.exec_child_alg("LoadNexusLogs", Workspace=tmpWS, Filename=dataPath)
            # read omega and phi (in RH coords)
            omega[irun] = omegaHand * tmpWS.getRun().getLogData(omegaLogName).value[0]
            phiRef[irun] = phiHand * tmpWS.getRun().getLogData(phiLogName).value[0]
            # load UB
            self.exec_child_alg("LoadIsawUB", InputWorkspace=tmpWS, Filename=ubFiles[irun], CheckUMatrix=True)
            tmpUB = tmpWS.sample().getOrientedLattice().getUB()
            # permute axes to use IPNS convention (as in saved .mat file)
            matUB += [tmpUB[[2, 0, 1], :]]
        return matUB, omega, phiRef

    def findConsistentUB(
        self, ubFiles, zeroUB, matUB, omega, dOmega, omegaHand, phiRef, dPhiRef, phiTol, phiHand, chiRef, chiTol, gonioTable
    ):
        # calculate the rotation matrix that maps the UB of the first run onto subsequent UBs
        # get save directory
        saveDir = config["defaultsave.directory"]
        tmpWS = self.exec_child_alg("CreateSampleWorkspace")
        for irun in range(1, len(omega)):
            chi, phi, u = self.getGonioAngles(matUB[irun], zeroUB, omega[irun])
            # phi relative to first run in RH/LH convention of user
            # check if phi and chi are not within tolerance of expected
            if abs(chi - chiRef) > chiTol and abs(phi - dPhiRef[irun]) > phiTol:
                # generate predicted UB to find axes permutation
                self.log().information(
                    "The following UB\n{}\nis not consistent with the reference, attempting to "
                    "find an axes swap/inversion that make it consistent."
                )
                # nominal goniometer axis
                gonio = getR(omegaHand * dOmega, [0, 0, 1]) @ getR(-chiRef, [1, 0, 0]) @ [0, 0, 1]
                predictedUB = getR(omega[irun], [0, 0, 1]) @ getR(dPhiRef[irun], gonio) @ zeroUB
                # try a permutation of the UB axes (as in TransformHKL)
                # UB' = UB M^-1
                # HKL' = M HKL
                minv = np.linalg.inv(matUB[irun]) @ predictedUB
                minv = getSignMaxAbsValInCol(minv)
                # redo angle calculation on permuted UB
                matUB[irun] = matUB[irun] @ minv
                chi, phi, u = self.getGonioAngles(matUB[irun], zeroUB, omega[irun])

            if abs(chi - chiRef) <= chiTol and abs(phi - dPhiRef[irun]) <= phiTol:
                # save the consistent UB to the default save directory
                _, nameUB = path.split(ubFiles[irun])
                newUBPath = path.join(saveDir, nameUB[:-4] + "_consistent" + nameUB[-4:])
                # set as UB (converting back to non-IPNS convention)
                self.exec_child_alg("SetUB", Workspace=tmpWS, UB=matUB[irun][[1, 2, 0], :])
                self.exec_child_alg("SaveIsawUB", InputWorkspace=tmpWS, Filename=newUBPath)
                # populate row of table
                phi2print = phiHand * (phi + phiRef[0])
                phi2print = phi2print + np.ceil(-phi2print / 360) * 360
                nextRow = {"Run": nameUB[:-4], "Chi": chi, "Phi": phi2print, "GonioAxis": V3D(u[0], u[1], u[2])}
                gonioTable.addRow(nextRow)
            else:
                warnings.warn(
                    "WARNING: The UB {0} cannot be made consistent with the reference UB. "
                    "Check the goniometer angles and handedness supplied "
                    "and the accuracy of reference UB.".format(ubFiles[irun])
                )

    def createGoniometerTable(self):
        """
        :return: Empty table workspace with columns Run, Chi, Phi and GonioAxis (unit vector)
        """
        gonioTable = self.exec_child_alg(" CreateEmptyTableWorkspace")
        # Add some columns, Recognized types are: int,float,double,bool,str,V3D,long64
        gonioTable.addColumn(type="str", name="Run")
        gonioTable.addColumn(type="float", name="Chi")
        gonioTable.addColumn(type="float", name="Phi")
        gonioTable.addColumn(type="V3D", name="GonioAxis")
        return gonioTable

    def getSingleAxis(self, r):
        """
        Single-axis rotation for a given rotation matrix using the Rodrigues formula
        (see https://en.wikipedia.org/wiki/Rodrigues%27_rotation_formula)
        :param r: Rotation matrix from which to find the single-axis rotation
        :return: theta: rotation (ccw) around goniometer axis
        :return: u: goniometer axis unit vector
        """
        phi = np.arccos((np.trace(r) - 1) / 2) * (180 / np.pi)
        u = (1 / (2 * np.sin(phi * np.pi / 180))) * np.array([r[2, 1] - r[1, 2], r[0, 2] - r[2, 0], r[1, 0] - r[0, 1]])
        return phi, u

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
        rPhi = getR(omega, [0, 0, 1]).T @ aUB @ np.linalg.inv(zeroUB)
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

    def exec_child_alg(self, alg_name: str, **kwargs):
        alg = self.createChildAlgorithm(alg_name, enableLogging=False)
        alg.setAlwaysStoreInADS(False)
        alg.initialize()
        alg.setProperties(kwargs)
        alg.execute()
        out_props = tuple(alg.getProperty(prop).value for prop in alg.outputProperties())
        return out_props[0] if len(out_props) == 1 else out_props


# register algorithm with mantid
AlgorithmFactory.subscribe(FindGoniometerFromUB)
