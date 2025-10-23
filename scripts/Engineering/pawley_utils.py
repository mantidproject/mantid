# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import annotations
from mantid.simpleapi import CreateSingleValuedWorkspace, LoadCIF, CreateWorkspace
import numpy as np
from mantid.fitfunctions import FunctionWrapper, CompositeFunctionWrapper
from mantid.api import FunctionFactory
from mantid.geometry import CrystalStructure, ReflectionGenerator, PointGroupFactory, PointGroup
from mantid.kernel import V3D, logger
from typing import Optional, Tuple, TYPE_CHECKING, Sequence
from itertools import chain
from scipy.optimize import least_squares
from plugins.algorithms.poldi_utils import simulate_2d_data, get_dspac_array_from_ws
from abc import ABC, abstractmethod


if TYPE_CHECKING:
    from mantid.dataobjects import Workspace2D
    from scipy.optimize import OptimizeResult


class PeakProfile(ABC):
    def __init__(self):
        self.func_name: str
        self.labels: Tuple[str]
        self.p: np.ndarray
        self.default_isfree: np.ndarray

    @abstractmethod
    def get_mantid_peak_params(self, dpk: float):
        pass


class PVProfile(PeakProfile):
    def __init__(self):
        self.func_name = "PseudoVoigt"
        self.labels = ("sig0", "sig1", "sig2", "gam0", "gam1", "gam2", "gsize", "dst2", "zeta", "fsz")
        self.p: np.ndarray = np.array([5.0e-4, 1.0e-3, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0])
        self.default_isfree: np.ndarray = np.array([1, 1, 0, 0, 0, 0, 0, 0, 0, 0], dtype=bool)

    def get_mantid_peak_params(self, dpk: float) -> dict:
        """
        TCH pseudo-voigt profile used in Fullprof (typically convoluted with back-to-back exponentials for TOF)
        [1] FullProf Manual (http://psi.ch/sites/default/files/import/sinq/dmc/ManualsEN/fullprof.pdf)
        [2] Using FullProf to analyze Time of Flight Neutron Powder Diffraction data (https://www.ill.eu/sites/fullprof/downloads/Docs/TOF_FullProf.pdf)
        """
        # calculate FWHM
        fwhm_g = np.sqrt(
            8
            * np.log(2)
            * (
                (self.p[0] ** 2)
                + ((self.p[1] ** 2) + self.p[-3] * ((1 - self.p[-2]) ** 2)) * dpk**2
                + ((self.p[2] ** 2) + self.p[-4]) * dpk**4
            )
        )  # Panel 6 in [2] Hg**2 / (8ln2)
        fwhm_l = self.p[3] + (self.p[4] + self.p[-2] * np.sqrt(8 * np.log(2) * self.p[-3])) * dpk + (self.p[5] + self.p[-1]) * dpk**2
        fwhm_pv = (
            fwhm_g**5
            + 2.69269 * (fwhm_g**4) * (fwhm_l)
            + 2.42843 * (fwhm_g**3) * (fwhm_l**2)
            + 4.47163 * (fwhm_g**2) * (fwhm_l**3)
            + 0.07842 * (fwhm_g) * (fwhm_l**4)
            + fwhm_l**5
        ) ** (1 / 5)  # Eq.3.16 in [1]
        # calculate mixing
        fwhm_ratio = fwhm_l / fwhm_pv
        mixing = 1.36603 * fwhm_ratio - 0.47719 * fwhm_ratio**2 + 0.11116 * fwhm_ratio**3  # Eq.3.17 in [1]
        return {"Mixing": mixing, "FWHM": fwhm_pv}


class GaussianProfile(PeakProfile):
    def __init__(self):
        self.func_name = "Gaussian"
        self.labels = ("sig0", "sig1", "sig2")
        self.p: np.ndarray = np.array([5.0e-4, 1.0e-3, 0.0])
        self.default_isfree: np.ndarray = np.array([1, 1, 0], dtype=bool)

    def get_mantid_peak_params(self, dpk: float) -> dict:
        """
        Gasusian profile from pseudo-voigt profile used in Fullprof with zeta=0
        Note dst2 and gsize perfectly corralated withsig1 and sig2 respectively
        [1] Using FullProf to analyze Time of Flight Neutron Powder Diffraction data (https://www.ill.eu/sites/fullprof/downloads/Docs/TOF_FullProf.pdf)
        """
        return {"Sigma": np.sqrt((self.p[0] ** 2) + (self.p[1] ** 2) * dpk**2 + (self.p[2] ** 2) * dpk**4)}  # Panel 6 in [1] with zeta=0


class Phase:
    def __init__(self, crystal_structure: CrystalStructure, hkls: Optional[np.ndarray] = None):
        self.unit_cell = crystal_structure.getUnitCell()
        # check initial unit cell compatible with spacegroup
        self.spgr = crystal_structure.getSpaceGroup()
        if not self.spgr.isAllowedUnitCell(self.unit_cell):
            raise ValueError("Unit cell not compatible with spacegroup")
        # add hkls - check if compatible wth spacegorup
        if hkls is not None:
            self.set_hkls(hkls)
        # set free parameters beased on lattice system
        self.param_names = np.array(["a", "b", "c", "alpha", "beta", "gamma"])
        self.alatt = self._get_alatt()
        ptgr = PointGroupFactory.createPointGroupFromSpaceGroup(self.spgr)
        match ptgr.getLatticeSystem():
            case PointGroup.LatticeSystem.Cubic:
                # a=b=c, alpha=beta=gamma=90
                self.ipars = [slice(0, 3)]  # a
            case PointGroup.LatticeSystem.Tetragonal | PointGroup.LatticeSystem.Hexagonal:
                # a=b!=c (tetrag has alpha=beta=gamma=90, hexag has alpha=beta=90, gamma= 120)
                self.ipars = [slice(0, 2), 2]  # a, c
            case PointGroup.LatticeSystem.Orthorhombic:
                # alpha=beta=gamma=90
                self.ipars = range(3)  # a, b, c
            case PointGroup.LatticeSystem.Rhombohedral:
                # a=b=c, alpha=beta=gamma
                self.ipars = [slice(0, 3), slice(3, 6)]  # a, alpha
            case PointGroup.LatticeSystem.Monoclinic:
                # a!=b!=c, alpha=gamma=90
                self.ipars = [0, 1, 2, 4]  # a, b, c, beta
            case _:  # Triclinic
                self.ipars = range(len(self.alatt))  # a, b, c, alpha, beta, gamma

    @classmethod
    def from_cif(cls, cif_file: str):
        ws = CreateSingleValuedWorkspace(StoreInADS=False, EnableLogging=False)
        LoadCIF(ws, cif_file, StoreInADS=False)
        return Phase(ws.sample().getCrystalStructure())

    @classmethod
    def from_alatt(cls, alatt: np.ndarray, spgr: str = "P 1", basis: str = ""):
        alatt_str = " ".join([str(par) for par in alatt])
        xtal = CrystalStructure(alatt_str, spgr, basis)
        return Phase(xtal)

    def _get_alatt(self) -> np.ndarray[float]:
        return np.array([getattr(self.unit_cell, method)() for method in self.param_names])

    def get_params(self) -> np.ndarray[float]:
        return np.array([self.alatt[ipar][0] if isinstance(ipar, slice) else self.alatt[ipar] for ipar in self.ipars])

    def get_param_names(self) -> np.ndarray[str]:
        return np.array([self.param_names[ipar][0] if isinstance(ipar, slice) else self.param_names[ipar] for ipar in self.ipars])

    def set_params(self, pars: np.ndarray[float]):
        for ipar, par in enumerate(pars):
            self.alatt[self.ipars[ipar]] = par
        self.unit_cell.set(*self.alatt)

    def set_hkls(self, hkls: Sequence[np.ndarray]):
        self.hkls = []
        for hkl in hkls:
            hkl_vec = V3D(*hkl)
            if self.spgr.isAllowedReflection(hkl_vec):
                self.hkls.append(hkl_vec)
            else:
                logger.warning(f"Reflection {hkl} not allowed by spacegroup")
        # sort by descending d-spacing
        self.hkls = [self.hkls[ipk] for ipk in np.argsort(self.calc_dspacings())[::-1]]

    def set_hkls_from_dspac_limits(self, dmin: float, dmax: float):
        xtal = CrystalStructure(" ".join([str(par) for par in self.alatt]), self.spgr.getHMSymbol(), "")
        generator = ReflectionGenerator(xtal)
        self.set_hkls(generator.getUniqueHKLs(dmin, dmax))

    def calc_dspacings(self) -> np.ndarray[float]:
        return np.array([self.unit_cell.d(hkl) for hkl in self.hkls])

    def nhkls(self) -> int:
        return len(self.hkls)

    def nparams(self) -> int:
        return len(self.ipars)


# make this abstract base class?
class PawleyPattern1D:
    def __init__(self, ws: Workspace2D, phases: Phase, profile: PeakProfile, bg_func: Optional[FunctionWrapper] = None):
        self.ws = ws
        self.phases = phases
        self.alatt_params = [phase.get_params() for phase in self.phases]
        self.alatt_isfree = [np.ones_like(alatt, dtype=bool) for alatt in self.alatt_params]
        self.profile = profile
        self.profile_params = [self.profile.p.copy() for _ in self.phases]
        self.profile_isfree = [self.profile.default_isfree.copy() for _ in self.phases]
        self.peak_func = FunctionFactory.Instance().createPeakFunction(self.profile.func_name)
        self.intens = [np.ones(len(phase.hkls), dtype=float) for phase in self.phases]
        self.intens_isfree = [np.ones_like(pars, dtype=bool) for pars in self.intens]
        self.bg_params = []
        if bg_func is not None:
            self.bg_params = np.array([bg_func.function.getParamValue(ipar) for ipar in range(bg_func.nParams())])
        self.bg_isfree = np.ones_like(self.bg_params, dtype=bool)
        self.make_profile_function(bg_func)
        self.initial_params = None

    def make_profile_function(self, bg_func: Optional[FunctionWrapper] = None):
        self.comp_func = CompositeFunctionWrapper()
        for iphase, phase in enumerate(self.phases):
            for ipk in range(phase.nhkls()):
                self.comp_func += FunctionWrapper(FunctionFactory.Instance().createPeakFunction(self.profile.func_name))
        if bg_func is not None:
            self.comp_func += bg_func
        self.comp_func.function.setAttributeValue("NumDeriv", True)
        self.update_profile_function()

    def update_profile_function(self):
        self.profile.p = self.profile_params
        istart = 0
        for iphase, phase in enumerate(self.phases):
            self.profile.p = self.profile_params[iphase]
            # set alatt for phase
            phase.set_params(self.alatt_params[iphase])
            dpks = phase.calc_dspacings()
            for ipk, dpk in enumerate(dpks):
                self.comp_func[istart + ipk].function.setCentre(dpk)
                for par_name, val in self.profile.get_mantid_peak_params(dpk).items():
                    self.comp_func[istart + ipk][par_name] = val
                self.comp_func[istart + ipk].function.setIntensity(self.intens[iphase][ipk])
            istart += phase.nhkls()
        if len(self.bg_params) > 0:
            [self.comp_func[len(self.comp_func) - 1].function.setParameter(ipar, par) for ipar, par in enumerate(self.bg_params)]

    def get_params(self) -> np.ndarray[float]:
        return np.array(list(chain(*self.alatt_params, *self.intens, *self.profile_params, self.bg_params)))

    def get_free_params(self) -> np.ndarray[float]:
        return self.get_params()[self.get_isfree()]

    def get_isfree(self) -> np.ndarray[bool]:
        return np.array(list(chain(*self.alatt_isfree, *self.intens_isfree, *self.profile_isfree, self.bg_isfree)))

    def set_params(self, params: np.ndarray[float]):
        # set alatt
        istart = 0
        for iphase, phase in enumerate(self.phases):
            npars = phase.nparams()
            self.alatt_params[iphase] = params[istart : istart + npars]
            istart = istart + npars
        # set peak parameters
        for iphase, phase in enumerate(self.phases):
            npars = self.intens[iphase].size
            self.intens[iphase] = params[istart : istart + npars]
            istart = istart + npars
        # set profile parameters
        for iphase, phase in enumerate(self.phases):
            npars = self.profile_params[iphase].size
            self.profile_params[iphase] = params[istart : istart + npars]
            istart = istart + npars
        # set global profile and bg_func params
        if len(self.bg_params) > 0:
            self.bg_params = params[istart:]

    def set_free_params(self, free_params: np.ndarray[float]):
        params = self.get_params()
        params[self.get_isfree()] = free_params
        self.set_params(params)

    def eval_profile(self, params: np.ndarray[float]) -> np.ndarray[float]:
        self.set_free_params(params)
        self.update_profile_function()
        return self.comp_func(self.ws.readX(0))

    def eval_resids(self, params: np.ndarray[float]) -> np.ndarray[float]:
        return self.ws.readY(0) - self.eval_profile(params)

    def estimate_initial_params(self):
        y = self.ws.readY(0)
        bg = 0
        if len(self.bg_params) > 0:
            bg = np.median(y)
            ipar = self.comp_func[len(self.comp_func) - 1].function.getParameterIndex("A0")
            self.bg_params[:] = 0
            self.bg_params[ipar] = bg
        # estimate average intensity (should get correct order of magnitude)
        ycalc_sum = np.sum(self.eval_profile(self.get_free_params()) - bg)
        scale = np.sum(y - bg) / ycalc_sum
        for iphase in range(len(self.phases)):
            self.intens[iphase] *= scale

    def fit(self, **kwargs) -> OptimizeResult:
        default_kwargs = {"xtol": 1e-5, "diff_step": 1e-3, "x_scale": "jac", "verbose": 2}
        kwargs = {**default_kwargs, **kwargs}
        self.initial_params = self.get_free_params()
        return least_squares(lambda p: self.eval_resids(p), self.initial_params, **kwargs)

    def undo_fit(self):
        if self.initial_params is not None:
            self.set_free_params(self.initial_params)


class PawleyPattern2D(PawleyPattern1D):
    def __init__(self, *args, global_scale: bool = True, lambda_max: float = 5.0, **kwargs):
        super().__init__(*args, **kwargs)
        self.lambda_max = lambda_max
        self.scales = None
        self.bgs = None
        self.set_global_scale(global_scale)
        # create workspace in d-spacing to contain diffraction pattern
        dspacs = get_dspac_array_from_ws(self.ws)
        self.ws_1d = CreateWorkspace(
            DataX=dspacs,
            DataY=np.zeros_like(dspacs),
            UnitX="dSpacing",
            YUnitLabel="Intensity (a.u.)",
            OutputWorkspace=f"{self.ws.name()}_pattern",
            EnableLogging=False,
        )

    def set_global_scale(self, global_scale: bool):
        self.global_scale = global_scale
        if not self.global_scale:
            # check if a peak intensity is fixed in any phase
            if all([all(phase_intens_isfree) for phase_intens_isfree in self.intens_isfree]):
                # fix intensity of most intense peak in first phase to avoid perfectly correlated scales
                self.intens_isfree[0][np.argmax(self.intens[0])] = False
            # zero global background and fix
            if len(self.bg_params) > 0:
                self.bg_params[:] = 0
                self.bg_isfree[:] = False

    def set_params_from_pawley1d(self, pawley1d: PawleyPattern1D):
        if len(pawley1d.phases) != len(self.phases):
            logger.error("PawleyPattern1D object has a different number of phases.")
            return
        if pawley1d.profile.func_name == self.profile.func_name:
            self.profile_params = pawley1d.profile_params.copy()
        intens_changed = False
        for iphase, phase in enumerate(self.phases):
            if phase.nhkls() == pawley1d.phases[iphase].nhkls():
                self.intens[iphase] = pawley1d.intens[iphase].copy()
                intens_changed = True
        if intens_changed:
            self._estimate_intensities()

    def estimate_initial_params(self):
        self._estimate_intensities()

    def eval_profile(self, params: np.ndarray[float]) -> np.ndarray[float]:
        self.set_free_params(params)
        self.update_profile_function()
        return self.comp_func(self.ws_1d.readX(0))

    def eval_2d(self, params: np.ndarray[float]) -> Workspace2D:
        self.ws_1d.setY(0, self.eval_profile(params))
        ws_sim = simulate_2d_data(self.ws, self.ws_1d, output_workspace=f"{self.ws.name()}_sim", lambda_max=self.lambda_max)
        if not self.global_scale:
            self.scales = np.zeros(self.ws.getNumberHistograms())
            self.bgs = np.zeros_like(self.scales)
            for ispec in range(self.ws.getNumberHistograms()):
                yobs = self.ws.readY(ispec)
                ycalc = ws_sim.readY(ispec)
                ppval = np.polyfit(ycalc, yobs, 1)
                self.scales[ispec], self.bgs[ispec] = ppval
                ws_sim.setY(ispec, np.polyval(ppval, ycalc))
        return ws_sim

    def eval_resids(self, params: np.ndarray[float]) -> np.ndarray[float]:
        ws_sim = self.eval_2d(params)
        return (self.ws.extractY() - ws_sim.extractY()).flat

    def _estimate_intensities(self):
        # scale intensities
        ws_sim = self.eval_2d(self.get_free_params())
        ppval = np.polyfit(ws_sim.extractY().flat, self.ws.extractY().flat, 1)
        for iphase in range(len(self.phases)):
            self.intens[iphase] *= ppval[0]
