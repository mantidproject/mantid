# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from mantid.api import AlgorithmFactory, PythonAlgorithm
from mantid.dataobjects import PeaksWorkspaceProperty
from mantid.kernel import Direction, IntBoundedValidator, FloatBoundedValidator, StringListValidator

from mantid.simpleapi import CreateEmptyTableWorkspace, FilterPeaks, DeleteWorkspace, IndexPeaks, mtd

import numpy as np

from scipy.spatial.transform import Rotation
import scipy.optimize
import scipy.linalg


class RefineSingleCrystalGoniometer(PythonAlgorithm):
    def name(self):
        return "RefineSingleCrystalGoniometer"

    def category(self):
        # defines the category the algorithm will be put in the algorithm browser
        return "Crystal\\Fitting"

    def summary(self):
        return (
            "Refines the UB-matrix and goniometer offsets simultaneously."
            "This improves the indexing of the peaks for those cases when there is sample misorientation."
        )

    def PyInit(self):
        # Declare properties

        self.declareProperty(
            PeaksWorkspaceProperty(name="Peaks", defaultValue="", direction=Direction.Input),
            doc="The PeaksWorkspace to be refined.",
        )

        self.declareProperty("tol", 0.12, validator=FloatBoundedValidator(lower=0.0), doc="The tolerance used in IndexPeaks.")

        self.declareProperty(
            "cell",
            "Monoclinic",
            validator=StringListValidator(
                ["Fixed", "Cubic", "Rhombohedral", "Tetragonal", "Hexagonal", "Orthorhombic", "Monoclinic", "Triclinic"]
            ),
            doc="The cell type to optimize. Must be one of: {Fixed, Cubic, Rhombohedral, Tetragonal,"
            + "Hexagonal, Orthorhombic, Monoclinic, Triclinic}.",
        )

        self.declareProperty("n_iter", 1, validator=IntBoundedValidator(lower=1), doc="The number of IndexPeaks iterations.")

    def PyExec(self):
        # Save the workspace to file in ascii format

        for n in range(self.getProperty("n_iter").value):
            peaks = self.getProperty("Peaks").value

            self.table = peaks.name() + "_#{}".format(n)

            CreateEmptyTableWorkspace(OutputWorkspace=self.table)

            mtd[self.table].addColumn("float", "Requested Omega")
            mtd[self.table].addColumn("float", "Refined Omega")

            mtd[self.table].addColumn("float", "Requested Chi")
            mtd[self.table].addColumn("float", "Refined Chi")

            mtd[self.table].addColumn("float", "Requested Phi")
            mtd[self.table].addColumn("float", "Refined Phi")

            ol = peaks.sample().getOrientedLattice()

            self.U = ol.getU().copy()

            self.a = ol.a()
            self.b = ol.b()
            self.c = ol.c()
            self.alpha = ol.alpha()
            self.beta = ol.beta()
            self.gamma = ol.gamma()

            self.peak_dict = {}

            runs = np.unique(peaks.column("RunNumber")).tolist()

            IndexPeaks(PeaksWorkspace=peaks, Tolerance=self.getProperty("tol").value, CommonUBForAll=False)

            for i, run in enumerate(runs):
                FilterPeaks(InputWorkspace=peaks, FilterVariable="RunNumber", FilterValue=run, Operator="=", OutputWorkspace="_tmp")

                Q = np.array(mtd["_tmp"].column("QLab"))
                hkl = np.array(mtd["_tmp"].column("IntHKL"))

                mask = hkl.any(axis=1)

                R = mtd["_tmp"].getPeak(0).getGoniometerMatrix().copy()

                omega, chi, phi = Rotation.from_matrix(R).as_euler("YZY", degrees=True).tolist()

                self.peak_dict[run] = (omega, chi, phi), Q[mask], hkl[mask]

                DeleteWorkspace(Workspace="_tmp")

            self._optimize_lattice(self.getProperty("cell").value)

    def _calculate_goniometer(self, omega, chi, phi):
        return Rotation.from_euler("YZY", [omega, chi, phi], degrees=True).as_matrix()

    def _get_orientation_angles(self):
        """
        Current orientation angles.

        Returns
        -------
        phi : float
            Rotation axis azimuthal angle in radians.
        theta : float
            Rotation axis polar angle in radians.
        omega : float
            Rotation angle in radians.

        """

        omega = np.arccos((np.trace(self.U) - 1) / 2)

        val, vec = np.linalg.eig(self.U)

        ux, uy, uz = vec[:, np.argwhere(np.isclose(val, 1))[0][0]].real

        theta = np.arccos(uz)
        phi = np.arctan2(uy, ux)

        return phi, theta, omega

    def _get_lattice_parameters(self):
        """
        Current lattice parameters.

        Returns
        -------
        a, b, c : float
            Lattice constants in angstroms.
        alpha, beta, gamma : float
            Lattice angles in degrees.

        """

        a, b, c = self.a, self.b, self.c
        alpha, beta, gamma = self.alpha, self.beta, self.gamma

        return a, b, c, alpha, beta, gamma

    def _U_matrix(self, phi, theta, omega):
        u0 = np.cos(phi) * np.sin(theta)
        u1 = np.sin(phi) * np.sin(theta)
        u2 = np.cos(theta)

        w = omega * np.array([u0, u1, u2])

        U = scipy.spatial.transform.Rotation.from_rotvec(w).as_matrix()

        return U

    def _B_matrix(self, a, b, c, alpha, beta, gamma):
        alpha, beta, gamma = np.deg2rad([alpha, beta, gamma])

        G = np.array(
            [
                [a**2, a * b * np.cos(gamma), a * c * np.cos(beta)],
                [b * a * np.cos(gamma), b**2, b * c * np.cos(alpha)],
                [c * a * np.cos(beta), c * b * np.cos(alpha), c**2],
            ]
        )

        B = scipy.linalg.cholesky(np.linalg.inv(G), lower=False)

        return B

    def _fixed(self, x):
        a, b, c = self.a, self.b, self.c
        alpha, beta, gamma = self.alpha, self.beta, self.gamma
        return (a, b, c, alpha, beta, gamma, *x)

    def _cubic(self, x):
        a, *params = x

        return (a, a, a, 90, 90, 90, *params)

    def _rhombohedral(self, x):
        a, alpha, *params = x

        return (a, a, a, alpha, alpha, alpha, *params)

    def _tetragonal(self, x):
        a, c, *params = x

        return (a, a, c, 90, 90, 90, *params)

    def _hexagonal(self, x):
        a, c, *params = x

        return (a, a, c, 90, 90, 120, *params)

    def _orthorhombic(self, x):
        a, b, c, *params = x

        return (a, b, c, 90, 90, 90, *params)

    def _monoclinic(self, x):
        a, b, c, beta, *params = x

        return (a, b, c, 90, beta, 90, *params)

    def _triclinic(self, x):
        a, b, c, alpha, beta, gamma, *params = x

        return (a, b, c, alpha, beta, gamma, *params)

    def _residual(self, x, peak_dict, func):
        """
        Optimization residual function.

        Parameters
        ----------
        x : list
            Parameters.
        peak_dict : dictionary
            Goniometer angles, Q-lab vectors, Miller indices.            .
        func : function
            Lattice constraint function.

        Returns
        -------
        residual : list
            Least squares residuals.

        """

        a, b, c, alpha, beta, gamma, phi, theta, omega, *params = func(x)

        B = self._B_matrix(a, b, c, alpha, beta, gamma)
        U = self._U_matrix(phi, theta, omega)

        UB = np.dot(U, B)

        params = np.array(params).reshape(-1, 3)

        diff = []

        for i, run in enumerate(peak_dict.keys()):
            (omega, chi, phi), Q, hkl = peak_dict[run]
            omega_off, chi_off, phi_off = params[i]
            R = self._calculate_goniometer(omega + omega_off, chi + chi_off, phi + phi_off)
            # hkl = np.einsum("ij,lj->li", ub_inv @ R.T, Q)
            # int_hkl = np.round(hkl)
            # diff += (hkl - int_hkl).flatten().tolist()
            diff += (np.einsum("ij,lj->li", R @ UB, hkl) * 2 * np.pi - Q).flatten().tolist()

        return diff + params.flatten().tolist()

    def _optimize_lattice(self, cell):
        """
        Refine the orientation and lattice parameters under constraints.

        Parameters
        ----------
        cell : str
            Lattice centering to constrain paramters.

        """

        a, b, c, alpha, beta, gamma = self._get_lattice_parameters()

        phi, theta, omega = self._get_orientation_angles()

        fun_dict = {
            "Fixed": self._fixed,
            "Cubic": self._cubic,
            "Rhombohedral": self._rhombohedral,
            "Tetragonal": self._tetragonal,
            "Hexagonal": self._hexagonal,
            "Orthorhombic": self._orthorhombic,
            "Monoclinic": self._monoclinic,
            "Triclinic": self._triclinic,
        }

        x0_dict = {
            "Fixed": (),
            "Cubic": (a,),
            "Rhombohedral": (a, alpha),
            "Tetragonal": (a, c),
            "Hexagonal": (a, c),
            "Orthorhombic": (a, b, c),
            "Monoclinic": (a, b, c, beta),
            "Triclinic": (a, b, c, alpha, beta, gamma),
        }

        fun = fun_dict[cell]
        x0 = x0_dict[cell]

        n = 3 * len(self.peak_dict.keys())

        x0 += (phi, theta, omega) + (0,) * n
        args = (self.peak_dict, fun)

        sol = scipy.optimize.least_squares(self._residual, x0=x0, args=args)

        a, b, c, alpha, beta, gamma, phi, theta, omega, *params = fun(sol.x)

        B = self._B_matrix(a, b, c, alpha, beta, gamma)
        U = self._U_matrix(phi, theta, omega)

        params = np.array(params).reshape(-1, 3)

        peak_dict = {}
        for i, run in enumerate(self.peak_dict.keys()):
            (omega, chi, phi), Q, hkl = self.peak_dict[run]
            omega_off, chi_off, phi_off = params[i]
            omega_prime, chi_prime, phi_prime = omega + omega_off, chi + chi_off, phi + phi_off
            mtd[self.table].addRow([omega, omega_prime, chi, chi_prime, phi, phi_prime])
            R = self._calculate_goniometer(omega_prime, chi_prime, phi_prime)
            peak_dict[run] = R

        for peak in self.getProperty("Peaks").value:
            run = peak.getRunNumber()
            peak.setGoniometerMatrix(peak_dict[run])

        UB = np.dot(U, B)

        J = sol.jac
        cov = np.linalg.inv(J.T.dot(J))

        chi2dof = np.sum(sol.fun**2) / (sol.fun.size - sol.x.size)
        cov *= chi2dof

        sig = np.sqrt(np.diagonal(cov))

        sig_a, sig_b, sig_c, sig_alpha, sig_beta, sig_gamma, *_ = fun(sig)

        if np.isclose(a, sig_a):
            sig_a = 0
        if np.isclose(b, sig_b):
            sig_b = 0
        if np.isclose(c, sig_c):
            sig_c = 0

        if np.isclose(alpha, sig_alpha):
            sig_alpha = 0
        if np.isclose(beta, sig_beta):
            sig_beta = 0
        if np.isclose(gamma, sig_gamma):
            sig_gamma = 0

        ol = self.getProperty("Peaks").value.sample().getOrientedLattice()
        ol.setUB(UB)
        ol.setModUB(UB @ ol.getModHKL())
        ol.setError(sig_a, sig_b, sig_c, sig_alpha, sig_beta, sig_gamma)


# Register algorithm with Mantid
AlgorithmFactory.subscribe(RefineSingleCrystalGoniometer)
