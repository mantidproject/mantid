# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import annotations
from mantid.simpleapi import CreateSingleValuedWorkspace, LoadCIF, CreateWorkspace, EvaluateFunction, Fit
import numpy as np
from mantid.fitfunctions import FunctionWrapper, CompositeFunctionWrapper
from mantid.api import FunctionFactory
from mantid.geometry import CrystalStructure, ReflectionGenerator, PointGroupFactory, PointGroup
from mantid.kernel import V3D, logger, UnitConversion, DeltaEModeType
from typing import Optional, Tuple, TYPE_CHECKING, Sequence, Union
from scipy.optimize import least_squares
from plugins.algorithms.poldi_utils import simulate_2d_data, get_dspac_array_from_ws
from abc import ABC, abstractmethod


if TYPE_CHECKING:
    from mantid.dataobjects import Workspace2D
    from scipy.optimize import OptimizeResult


class InstrumentParams:
    def __init__(self):
        self.p: np.ndarray = np.array([1.0, 0.0])
        self.labels = ("scale", "shift")
        self.default_isfree: np.ndarray = np.zeros_like(self.p, dtype=bool)

    def get_peak_centre(self, dpk: float) -> float:
        return self.p[0] * dpk + self.p[1]


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


class BackToBackGauss(PeakProfile):
    def __init__(self):
        self.func_name = "BackToBackExponential"
        self.labels = ("sig0", "sig1", "sig2", "alpha_0", "alpha_1", "beta_0", "beta_1")
        self.p: np.ndarray = np.array([0, 9.06, 6.52, 0, 0.0968, 0.0216, 0.0123])  # for ENGIN-X North Bank
        self.default_isfree: np.ndarray = np.zeros_like(self.p, dtype=bool)

    def get_mantid_peak_params(self, dpk: float) -> dict:
        return {"S": self._calc_sigma(dpk), "A": self._calc_alpha(dpk), "B": self._calc_beta(dpk)}

    def _calc_sigma(self, dpk: float) -> float:
        return np.sqrt((self.p[0] ** 2) + (self.p[1] * dpk) ** 2 + (self.p[2] ** 2) * dpk**4)

    def _calc_alpha(self, dpk: float) -> float:
        return self.p[3] + self.p[4] / dpk

    def _calc_beta(self, dpk: float) -> float:
        return self.p[5] + self.p[6] / (dpk**4)


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

    def set_hkls(self, hkls: Sequence[np.ndarray], do_sort: bool = True):
        self.hkls = []
        for hkl in hkls:
            hkl_vec = V3D(*hkl)
            if self.spgr.isAllowedReflection(hkl_vec):
                self.hkls.append(hkl_vec)
            else:
                logger.warning(f"Reflection {hkl} not allowed by spacegroup")
        if do_sort:
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

    def merge_reflections(self, decimal_places=4):
        _, ihkls = np.unique((self.calc_dspacings() * (10**decimal_places)).astype(int), return_index=True)
        self.hkls = [self.hkls[ipk] for ipk in np.sort(ihkls)]


class MtdFuncMixin:
    """
    Methods used to interact with mantid composite function for unconstrained fits in PawleyPattern1D and
    PawleyPattern2DNoConstraints (although the getters may also be helpful for debugging PawleyPattern2D fits
    """

    def get_peak_centers(self):
        cen_par_name = self.comp_func[0].function.getCentreParameterName()
        return self.get_peak_params(cen_par_name)

    def get_peak_params(self, param_name: str):
        bg_func_names = FunctionFactory.Instance().getBackgroundFunctionNames()
        return np.array([f.function.getParameterValue(param_name) for f in self.comp_func if f.name not in bg_func_names])

    def get_peak_fwhm(self):
        bg_func_names = FunctionFactory.Instance().getBackgroundFunctionNames()
        return np.array([f.function.fwhm() for f in self.comp_func if f.name not in bg_func_names])

    def get_peak_intensities(self):
        bg_func_names = FunctionFactory.Instance().getBackgroundFunctionNames()
        return np.array([f.function.intensity() for f in self.comp_func if f.name not in bg_func_names])

    def set_mantid_peak_param_isfree(self, param_names: Union[str, Sequence[str]], isfree: bool = False):
        # relevant only for subsequent unconstrained fits - not used in Pawley fits
        bg_func_names = FunctionFactory.Instance().getBackgroundFunctionNames()
        if isinstance(param_names, str):
            param_names = [param_names]  #  force ot be list with single element
        for func in self.comp_func:
            if func.name not in bg_func_names:
                for param_name in param_names:
                    if isfree:
                        func.free(param_name)
                    else:
                        func.fix(param_name)


class PawleyPatternBase(MtdFuncMixin, ABC):
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
        self.inst = InstrumentParams()
        self.inst_params = self.inst.p.copy()
        self.inst_isfree = self.inst.default_isfree.copy()
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

    def update_profile_function(self):
        self.profile.p = self.profile_params
        self.inst.p = self.inst_params
        istart = 0
        for iphase, phase in enumerate(self.phases):
            self.profile.p = self.profile_params[iphase]
            # set alatt for phase
            phase.set_params(self.alatt_params[iphase])
            dpks = self.inst.get_peak_centre(phase.calc_dspacings())  # apply scale and shift to calculated d
            pk_cens = self._get_peak_cens(dpks)  # could involve unit conversion etc.
            for ipk, dpk in enumerate(dpks):
                self.comp_func[istart + ipk].function.setCentre(pk_cens[ipk])
                for par_name, val in self.profile.get_mantid_peak_params(dpk).items():
                    self.comp_func[istart + ipk][par_name] = val
                self.comp_func[istart + ipk].function.setIntensity(self.intens[iphase][ipk])
            istart += phase.nhkls()
        if len(self.bg_params) > 0:
            [self.comp_func[len(self.comp_func) - 1].function.setParameter(ipar, par) for ipar, par in enumerate(self.bg_params)]

    def _get_peak_cens(self, dpks: np.ndarray[float]) -> np.ndarray[float]:
        return dpks

    def get_params(self) -> np.ndarray[float]:
        return np.concatenate((*self.alatt_params, *self.intens, *self.profile_params, self.inst_params, self.bg_params))

    def get_free_params(self) -> np.ndarray[float]:
        return self.get_params()[self.get_isfree()]

    def get_isfree(self) -> np.ndarray[bool]:
        return np.concatenate((*self.alatt_isfree, *self.intens_isfree, *self.profile_isfree, self.inst_isfree, self.bg_isfree)).astype(
            bool
        )

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
        # set instrument params
        iend = istart + len(self.inst_params)
        self.inst_params = params[istart:iend]
        # set global profile and bg_func params
        if len(self.bg_params) > 0:
            self.bg_params = params[iend:]

    def set_free_params(self, free_params: np.ndarray[float]):
        params = self.get_params()
        params[self.get_isfree()] = free_params
        self.set_params(params)

    def fit(self, **kwargs) -> OptimizeResult:
        default_kwargs = {"xtol": 1e-5, "diff_step": 1e-3, "x_scale": "jac", "verbose": 2}
        kwargs = {**default_kwargs, **kwargs}
        self.initial_params = self.get_free_params()
        res = least_squares(lambda p: self.eval_resids(p), self.initial_params, **kwargs)
        # update parameters
        self.set_free_params(res.x)
        self.update_profile_function()
        return res

    def undo_fit(self):
        if self.initial_params is not None:
            self.set_free_params(self.initial_params)

    @abstractmethod
    def eval_profile(self, params: np.ndarray[float]) -> np.ndarray[float]:
        raise NotImplementedError()

    @abstractmethod
    def eval_resids(self, params: np.ndarray[float]) -> np.ndarray[float]:
        raise NotImplementedError()


class PawleyPattern1D(PawleyPatternBase):
    def __init__(self, *args, ispec: int = 0, **kwargs):
        super().__init__(*args, **kwargs)
        self.xunit = self.ws.getAxis(0).getUnit().unitID()
        if self.xunit == "TOF":
            si = self.ws.spectrumInfo()
            if not si.hasDetectors(ispec):
                raise RuntimeError("Workspace has no detectors - cannot convert between TOF and d-spacing.")
        self.ispec = ispec
        self.update_profile_function()

    def _get_peak_cens(self, dpks: np.ndarray[float]) -> np.ndarray[float]:
        if self.xunit == "TOF":
            return self._convert_dspac_to_tof(dpks)
        return dpks

    def _convert_dspac_to_tof(self, dpks: np.ndarray[float]) -> np.ndarray[float]:
        diff_consts = self.ws.spectrumInfo().diffractometerConstants(self.ispec)
        return np.asarray([UnitConversion.run("dSpacing", "TOF", dpk, 0, DeltaEModeType.Elastic, diff_consts) for dpk in dpks])

    def eval_profile(self, params: np.ndarray[float]) -> np.ndarray[float]:
        self.set_free_params(params)
        self.update_profile_function()
        return self.comp_func(self.ws.readX(self.ispec))

    def eval_resids(self, params: np.ndarray[float]) -> np.ndarray[float]:
        return self.ws.readY(self.ispec) - self.eval_profile(params)

    def get_eval_workspace(self, **kwargs):
        if "OutputWorkspace" not in kwargs:
            kwargs["OutputWorkspace"] = f"{self.ws.name()}_eval"
        return EvaluateFunction(Function=self.comp_func, InputWorkspace=self.ws, WorkspaceIndex=self.ispec, **kwargs)

    def fit_no_constraints(self, **kwargs):
        # select cost function
        default_kwargs = {
            "Minimizer": "Levenberg-Marquardt",
            "MaxIterations": 500,
            "StepSizeMethod": "Sqrt epsilon",
            "IgnoreInvalidData": True,
            "CreateOutput": True,
            "OutputCompositeMembers": False,
            "CostFunction": "Unweighted least squares",
        }
        kwargs = {**default_kwargs, **kwargs}
        res = Fit(Function=self.comp_func, InputWorkspace=self.ws, WorkspaceIndex=self.ispec, **kwargs)
        # upate parameters in comp_func (can't replace comp_func as result doesn't return individual IPEakFunctions)
        [self.comp_func.setParameter(ipar, res.Function.getParamValue(ipar)) for ipar in range(self.comp_func.nParams())]
        return res

    def estimate_initial_params(self):
        ws_eval = self.get_eval_workspace(StoreInADS=False)
        ppval = np.polyfit(ws_eval.readY(1), ws_eval.readY(0), 1)
        scale, bg = ppval
        for iphase in range(len(self.phases)):
            self.intens[iphase] = self.intens[iphase] * scale
            self.intens[iphase][self.intens[iphase] < 1e-8] = 1e-8  # arbitrary small number
        if len(self.bg_params) > 0:
            if np.allclose(self.bg_params, 0) and self.comp_func[len(self.comp_func) - 1].function.hasParameter("A0"):
                # try and set background (assume hasn't been previously fitted)
                ipar = self.comp_func[len(self.comp_func) - 1].function.getParameterIndex("A0")
                self.bg_params[ipar] = bg
        self.update_profile_function()


class Poldi2DEvalMixin:
    """
    Methods relating to 2D POLDI workspace evaluation/simulation used in PawleyPattern2D and
    PawleyPattern2DNoConstraints classes
    """

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

    def eval_profile(self, params: np.ndarray[float]) -> np.ndarray[float]:
        self.set_free_params(params)
        return self.comp_func(self.ws_1d.readX(0))


class PawleyPattern2D(Poldi2DEvalMixin, PawleyPatternBase):
    def __init__(self, *args, global_scale: bool = True, lambda_max: float = 5.0, **kwargs):
        super().__init__(*args, **kwargs)
        self.lambda_max = lambda_max
        self.scales = None
        self.bgs = None
        self.set_global_scale(global_scale)
        # create workspace in d-spacing to contain diffraction pattern
        self.ws_1d = self.make_1d_ws(self.ws)
        self.update_profile_function()

    @staticmethod
    def make_1d_ws(ws):
        dspacs = get_dspac_array_from_ws(ws)
        return CreateWorkspace(
            DataX=dspacs,
            DataY=np.zeros_like(dspacs),
            UnitX="dSpacing",
            YUnitLabel="Intensity (a.u.)",
            OutputWorkspace=f"{ws.name()}_pattern",
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

    def _estimate_intensities(self):
        # scale intensities
        ws_sim = self.eval_2d(self.get_free_params())
        ppval = np.polyfit(ws_sim.extractY().flat, self.ws.extractY().flat, 1)
        for iphase in range(len(self.phases)):
            self.intens[iphase] *= ppval[0]

    def create_no_constriants_fit(self):
        return PawleyPattern2DNoConstraints(self.ws, self.comp_func, self.global_scale, self.lambda_max)


class PawleyPattern2DNoConstraints(MtdFuncMixin, Poldi2DEvalMixin):
    def __init__(self, ws: Workspace2D, func: FunctionWrapper, global_scale: bool = True, lambda_max: float = 5.0):
        self.ws = ws
        self.ws_1d = PawleyPattern2D.make_1d_ws(self.ws)
        self.comp_func = func
        self.intens_par_name = self.get_peak_intensity_param_name()
        self.has_bg = self.comp_func[len(self.comp_func) - 1].name in FunctionFactory.Instance().getBackgroundFunctionNames()
        self.lambda_max = lambda_max
        self.scales = None
        self.bgs = None
        self.set_global_scale(global_scale)
        self.initial_params = None

    def get_peak_intensity_param_name(self):
        pk_func = FunctionFactory.Instance().createPeakFunction(self.comp_func[0].name)
        pk_func.setIntensity(1.0)
        return next(pk_func.getParamName(ipar) for ipar in range(pk_func.nParams()) if pk_func.isExplicitlySet(ipar))

    def get_npks(self):
        return len(self.comp_func) - 1 if self.has_bg else len(self.comp_func)

    def set_global_scale(self, global_scale: bool):
        self.global_scale = global_scale
        if not self.global_scale:
            npks = self.get_npks()
            ipar_intens = self.comp_func[0].getParameterIndex(self.intens_par_name)
            ipk_max_intens = 0
            max_intens = -np.inf
            any_fixed = False
            for ipk in range(npks):
                if self.comp_func[ipk].isFixed(ipar_intens):
                    any_fixed = True
                    break
                intens = self.comp_func[ipk].function.intensity()
                if intens > max_intens:
                    max_intens = intens
                    ipk_max_intens = ipk
            if not any_fixed:
                # fix intensity of most intense peak to avoid perfectly correlated scales
                self.comp_func[ipk_max_intens].fix(self.intens_par_name)
            # zero global background and fix
            if self.has_bg:
                bg_func = self.comp_func[npks]
                [bg_func.function.setParameter(ipar, 0) for ipar in range(bg_func.nParams())]
                bg_func.function.fixAll()

    def get_params(self):
        return np.asarray([self.comp_func.getParameterValue(ipar) for ipar in range(self.comp_func.nParams())])

    def get_free_params(self) -> np.ndarray[float]:
        params = self.get_params()
        return params[self.get_isfree()]

    def get_isfree(self) -> np.ndarray[bool]:
        return np.asarray([not self.comp_func.isFixed(ipar) for ipar in range(self.comp_func.nParams())])

    def set_params(self, params: np.ndarray[float]):
        [self.comp_func.setParameter(ipar, val) for ipar, val in enumerate(params)]

    def set_free_params(self, free_params: np.ndarray[float]):
        params = self.get_params()
        params[self.get_isfree()] = free_params
        self.set_params(params)

    def fit(self, **kwargs) -> OptimizeResult:
        default_kwargs = {"xtol": 1e-5, "diff_step": 1e-3, "x_scale": "jac", "verbose": 2}
        kwargs = {**default_kwargs, **kwargs}
        self.initial_params = self.get_free_params()
        res = least_squares(lambda p: self.eval_resids(p), self.initial_params, **kwargs)
        # update parameters
        self.set_free_params(res.x)
        return res
