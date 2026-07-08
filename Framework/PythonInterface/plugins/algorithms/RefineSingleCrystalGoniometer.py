# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from mantid.api import AlgorithmFactory, PythonAlgorithm
from mantid.dataobjects import PeaksWorkspaceProperty
from mantid.kernel import (
    Direction,
    IntBoundedValidator,
    FloatBoundedValidator,
    StringListValidator,
    EnabledWhenProperty,
    PropertyCriterion,
)

from mantid.simpleapi import (
    CreateEmptyTableWorkspace,
    FilterPeaks,
    DeleteWorkspace,
    RenameWorkspace,
    CloneWorkspace,
    CombinePeaksWorkspaces,
    FindUBUsingFFT,
    IndexPeaks,
    mtd,
)

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

        self.declareProperty("Tolerance", 0.12, validator=FloatBoundedValidator(lower=0.0), doc="The tolerance used in IndexPeaks.")

        self.declareProperty(
            "Cell",
            "Triclinic",
            validator=StringListValidator(
                ["Fixed", "Cubic", "Rhombohedral", "Tetragonal", "Hexagonal", "Orthorhombic", "Monoclinic", "Triclinic"]
            ),
            doc="The cell type to optimize. Must be one of: {Fixed, Cubic, Rhombohedral, Tetragonal,"
            + "Hexagonal, Orthorhombic, Monoclinic, Triclinic}.",
        )

        self.declareProperty("NumIterations", 1, validator=IntBoundedValidator(lower=1), doc="The number of IndexPeaks iterations.")

        self.declareProperty(
            "LargeOffset",
            False,
            doc="If True, index each run independently with FindUBUsingFFT (robust to large/unreliable "
            "goniometer offsets) instead of a single IndexPeaks pass across all runs using the existing UB. "
            "Runs whose independently-indexed lattice constants are inconsistent with the other runs are "
            "logged and excluded before refinement.",
        )

        self.declareProperty(
            "MinD",
            1.0,
            validator=FloatBoundedValidator(lower=0.0),
            doc="Minimum d-spacing passed to FindUBUsingFFT. Only used when LargeOffset=True.",
        )

        self.declareProperty(
            "MaxD",
            15.0,
            validator=FloatBoundedValidator(lower=0.0),
            doc="Maximum d-spacing passed to FindUBUsingFFT. Only used when LargeOffset=True.",
        )

        self.declareProperty(
            "LatticeOutlierTolerance",
            5.0,
            validator=FloatBoundedValidator(lower=0.0),
            doc="Number of median-absolute-deviations a run's independently-indexed lattice constants "
            "(a, b, c, alpha, beta, gamma) may differ from the across-run median before that run is "
            "excluded. Only used when LargeOffset=True.",
        )

        large_offset_enabled = EnabledWhenProperty("LargeOffset", PropertyCriterion.IsNotDefault)
        self.setPropertySettings("MinD", large_offset_enabled)
        self.setPropertySettings("MaxD", large_offset_enabled)
        self.setPropertySettings("LatticeOutlierTolerance", large_offset_enabled)

    def validateInputs(self):
        issues = dict()

        peaks = self.getProperty("Peaks").value
        large_offset = self.getProperty("LargeOffset").value

        if not large_offset and not peaks.sample().hasOrientedLattice():
            issues["Peaks"] = "Peaks workspace must have an orientation matrix unless LargeOffset=True."

        return issues

    def PyExec(self):
        # Save the workspace to file in ascii format

        large_offset = self.getProperty("LargeOffset").value

        peaks = self.getProperty("Peaks").value

        for n in range(self.getProperty("NumIterations").value):
            self.table = peaks.name() + "_#{}".format(n)

            CreateEmptyTableWorkspace(OutputWorkspace=self.table)

            mtd[self.table].addColumn("float", "Requested Omega")
            mtd[self.table].addColumn("float", "Refined Omega")

            mtd[self.table].addColumn("float", "Requested Chi")
            mtd[self.table].addColumn("float", "Refined Chi")

            mtd[self.table].addColumn("float", "Requested Phi")
            mtd[self.table].addColumn("float", "Refined Phi")

            runs = np.unique(peaks.column("RunNumber")).tolist()

            if large_offset and n == 0:
                # _index_runs_using_fft sets self.U/a/b/c/alpha/beta/gamma from
                # the first retained run's independent indexing, since a single
                # UB shared across all runs is not yet available at this point.
                peaks, runs = self._index_runs_using_fft(peaks, runs)
            else:
                IndexPeaks(PeaksWorkspace=peaks, Tolerance=self.getProperty("Tolerance").value, CommonUBForAll=False)

                ol = peaks.sample().getOrientedLattice()

                self.U = ol.getU().copy()

                self.a = ol.a()
                self.b = ol.b()
                self.c = ol.c()
                self.alpha = ol.alpha()
                self.beta = ol.beta()
                self.gamma = ol.gamma()

            self.peak_dict = {}

            for i, run in enumerate(runs):
                FilterPeaks(InputWorkspace=peaks, FilterVariable="RunNumber", FilterValue=run, Operator="=", OutputWorkspace="_tmp")

                Q = np.array(mtd["_tmp"].column("QLab"))
                hkl = np.array(mtd["_tmp"].column("IntHKL"))

                mask = hkl.any(axis=1)

                R = mtd["_tmp"].getPeak(0).getGoniometerMatrix().copy()

                omega, chi, phi = Rotation.from_matrix(R).as_euler("YZY", degrees=True).tolist()

                self.peak_dict[run] = (omega, chi, phi), Q[mask], hkl[mask]

                DeleteWorkspace(Workspace="_tmp")

            self._optimize_lattice(self.getProperty("Cell").value, peaks)

    def _index_runs_using_fft(self, peaks, runs):
        """
        Index each run independently via FindUBUsingFFT and combine the consistent runs.

        Used for large/unreliable goniometer offsets, where a single UB shared
        across all runs (the ``LargeOffset=False`` path) cannot index most
        runs. Each run is indexed on its own using FindUBUsingFFT (which does
        not depend on the existing orientation), then the resulting lattice
        constants are compared across runs: any run whose lattice constants
        deviate from the across-run median by more than
        ``LatticeOutlierTolerance`` median-absolute-deviations (in any of
        a, b, c, alpha, beta, gamma) is logged and excluded, since an
        inconsistent cell likely indicates a bad per-run indexing rather
        than a real difference in the sample.

        Sets self.U and self.a/b/c/alpha/beta/gamma from the first retained
        run's independent indexing, since a single UB shared across all runs
        (which the non-large-offset path reads these from) is not available
        yet. These become the starting model for the joint refinement.

        Parameters
        ----------
        peaks : PeaksWorkspace
            Input peaks workspace containing multiple runs.
        runs : list of int
            Run numbers present in `peaks`.

        Returns
        -------
        combined : PeaksWorkspace
            Peaks workspace combining only the runs with consistent lattice
            constants, with a common UB set from the first retained run.
        good_runs : list of int
            The subset of `runs` retained in `combined`, in their original
            order.
        """

        tolerance = self.getProperty("Tolerance").value
        min_d = self.getProperty("MinD").value
        max_d = self.getProperty("MaxD").value
        n_mad = self.getProperty("LatticeOutlierTolerance").value

        # Captured once, before any renaming: on iterations after the first,
        # `peaks` is a name-tracking ADS proxy (returned by the previous call's
        # `mtd[peaks_name]`) rather than the property-bound handle used on the
        # first iteration, and it self-invalidates the moment its ADS slot is
        # overwritten below -- so peaks.name() must not be called again after that.
        peaks_name = peaks.name()

        lattice_params = {}
        indexed = {}
        success = False

        try:
            for run in runs:
                ws_name = "_fft_tmp_{}".format(run)

                FilterPeaks(InputWorkspace=peaks, FilterVariable="RunNumber", FilterValue=run, Operator="=", OutputWorkspace=ws_name)
                indexed[run] = ws_name

                FindUBUsingFFT(PeaksWorkspace=ws_name, MinD=min_d, MaxD=max_d)

                IndexPeaks(PeaksWorkspace=ws_name, Tolerance=tolerance, CommonUBForAll=True, UpdateUB=True)

                ol = mtd[ws_name].sample().getOrientedLattice()
                lattice_params[run] = np.array([ol.a(), ol.b(), ol.c(), ol.alpha(), ol.beta(), ol.gamma()])

            values = np.array([lattice_params[run] for run in runs])
            med = np.median(values, axis=0)
            mad = np.median(np.abs(values - med), axis=0)
            safe_mad = np.where(mad > 0, mad, 1.0)

            good_runs = []
            for run in runs:
                deviation = np.abs(lattice_params[run] - med) / safe_mad
                if np.any(deviation > n_mad):
                    self.log().warning(
                        "Run {} has lattice constants a={:.4f}, b={:.4f}, c={:.4f}, alpha={:.4f}, "
                        "beta={:.4f}, gamma={:.4f} inconsistent with the across-run median "
                        "a={:.4f}, b={:.4f}, c={:.4f}, alpha={:.4f}, beta={:.4f}, gamma={:.4f}; "
                        "excluding this run from refinement.".format(run, *lattice_params[run], *med)
                    )
                    DeleteWorkspace(Workspace=indexed[run])
                    del indexed[run]
                else:
                    good_runs.append(run)

            if not good_runs:
                raise RuntimeError(
                    "LargeOffset indexing failed: none of the runs produced lattice constants "
                    "consistent with the across-run median. Check MinD/MaxD and LatticeOutlierTolerance."
                )

            first_run = good_runs[0]

            self.U = mtd[indexed[first_run]].sample().getOrientedLattice().getU().copy()
            self.a, self.b, self.c, self.alpha, self.beta, self.gamma = lattice_params[first_run]

            combine = "_{}_large_offset".format(peaks_name)

            for i, run in enumerate(good_runs):
                ws_name = indexed[run]

                if i == 0:
                    CloneWorkspace(InputWorkspace=ws_name, OutputWorkspace=combine)
                else:
                    CombinePeaksWorkspaces(LHSWorkspace=combine, RHSWorkspace=ws_name, OutputWorkspace=combine)

                DeleteWorkspace(Workspace=ws_name)
                del indexed[run]

            RenameWorkspace(InputWorkspace=combine, OutputWorkspace=peaks_name)
            success = True
        finally:
            if not success:
                for ws_name in indexed.values():
                    if ws_name in mtd:
                        DeleteWorkspace(Workspace=ws_name)

        return mtd[peaks_name], good_runs

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

    def _optimize_lattice(self, cell, peaks):
        """
        Refine the orientation and lattice parameters under constraints.

        Parameters
        ----------
        cell : str
            Lattice centering to constrain paramters.
        peaks : PeaksWorkspace
            Workspace to update with the refined goniometer matrices and UB.
            Passed explicitly rather than re-read from the "Peaks" property,
            since LargeOffset indexing replaces that workspace with a newly
            combined one and the property still references the original.

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

        for peak in peaks:
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

        ol = peaks.sample().getOrientedLattice()
        ol.setUB(UB)
        ol.setModUB(UB @ ol.getModHKL())
        ol.setError(sig_a, sig_b, sig_c, sig_alpha, sig_beta, sig_gamma)


# Register algorithm with Mantid
AlgorithmFactory.subscribe(RefineSingleCrystalGoniometer)
