# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""
This module is a wrapper around a set of instrument parameters (to be read from a YAML file)
and methods which then call either Chop.py or MulpyRep.py to do the resolution calculations.
"""

import numpy as np
import yaml
import warnings
import copy
from . import Chop, MulpyRep
from scipy.interpolate import interp1d
from scipy.special import erf
from scipy import constants
from scipy.optimize import curve_fit

# Some global constants
SIGMA2FWHM = 2 * np.sqrt(2 * np.log(2))
SIGMA2FWHMSQ = SIGMA2FWHM**2
E2V = np.sqrt((constants.e / 1000) * 2 / constants.neutron_mass)  # v = E2V * sqrt(E)    veloc in m/s, E in meV
E2L = 1.0e23 * constants.h**2 / (2 * constants.m_n * constants.e)  # lam = sqrt(E2L / E)  lam in Angst, E in meV
E2K = constants.e * 2 * constants.m_n / constants.hbar**2 / 1e23  # k = sqrt(E2K * E)    k in 1/Angst, E in meV


def wrap_attributes(obj, inval, allowed_var_names):
    for key in allowed_var_names:
        if inval:
            if hasattr(inval, key):
                setattr(obj, key, getattr(inval, key))
            elif hasattr(inval, "__getitem__") and key in inval:
                setattr(obj, key, inval[key])


def argparser(args, kwargs, argnames, defaults=None):
    argdict = {key: val for key, val in zip(argnames, defaults if defaults else [None] * len(argnames))}
    for key in kwargs:
        argdict[key] = kwargs[key]
    for idx in range(len(args)):
        argdict[argnames[idx]] = args[idx]
    return argdict


def _check_input(self, *args):
    vtype = ["Incident energy", "Frequency"]
    defval = [self.ei, self.frequency]
    retval = [defval[i] if args[i] is None else args[i] for i in range(min([len(defval), len(args)]))]
    if [v for v in retval if v is None]:
        raise ValueError("%s has not been specified." % (vtype[[i for i in range(len(retval)) if retval[i] is None][0]]))
    return tuple(retval) if len(retval) > 1 else retval[0]


def soft_hat(x, p):
    """
    ! Soft hat function, from Herbert subroutine library.
    ! For rescaling t-mod at low energy to account for broader moderator term
    """
    x = np.array(x)
    sig2fwhh = np.sqrt(8 * np.log(2))
    height, grad, x1, x2 = tuple(p[:4])
    sig1, sig2 = tuple(np.abs(p[4:6] / sig2fwhh))
    # linearly interpolate sig for x1<x<x2
    sig = ((x2 - x) * sig1 - (x1 - x) * sig2) / (x2 - x1)
    if np.shape(sig):
        sig[x < x1] = sig1
        sig[x > x2] = sig2
    # calculate blurred hat function with gradient
    e1 = (x1 - x) / (np.sqrt(2) * sig)
    e2 = (x2 - x) / (np.sqrt(2) * sig)
    y = (erf(e2) - erf(e1)) * ((height + grad * (x - (x2 + x1) / 2)) / 2)
    y = y + 1
    return y


class FermiChopper(object):
    """
    Class which represents a Fermi chopper package
    """

    __allowed_var_names = ["name", "pslit", "pslat", "radius", "rho", "tjit", "fluxcorr", "isPi", "ei_limits"]

    def __init__(self, inval=None):
        wrap_attributes(self, inval, self.__allowed_var_names)

    def __repr__(self):
        return self.name if self.name else "Undefined Fermi chopper package"

    def getWidthSquared(self, Ei, freq):
        return Chop.tchop(freq, Ei, self.pslit / 1000.0, self.radius / 1000.0, self.rho / 1000.0)

    def getWidth(self, *args):
        """Calculates the chopper time width in seconds for a given neutron energy (Ei)"""
        return np.sqrt(self.getWidthSquared(*args))

    def getTransmission(self, Ei, freq):
        """Calculates the chopper transmission"""
        dslat = (self.pslit + self.pslat) / 1000
        return Chop.achop(Ei, freq, dslat, self.pslit / 1000.0, self.radius / 1000.0, self.rho / 1000.0) / dslat

    @property
    def emin(self):
        return self.ei_limits[0] if hasattr(self, "ei_limits") else 0.1

    @property
    def emax(self):
        return self.ei_limits[1] if hasattr(self, "ei_limits") else 1000.0


class ChopperSystem(object):
    """
    Class which represents a set (list) of choppers in a line
    """

    __allowed_var_names = [
        "name",
        "chop_sam",
        "sam_det",
        "aperture_width",
        "aperture_height",
        "choppers",
        "variants",
        "frequency_matrix",
        "constant_frequencies",
        "max_frequencies",
        "default_frequencies",
        "source_rep",
        "n_frame",
        "emission_time",
        "overlap_ei_frac",
        "ei_limits",
        "flux_ref_slot",
        "flux_ref_freq",
        "frequency_names",
        "phaseOffset",
    ]

    def __init__(self, inval=None):
        # Default values
        self.source_rep = 50
        self.emission_time = 0
        self.overlap_ei_frac = 0.9
        self.n_frame = 1
        self._ei = None
        # Parse input values (if any)
        wrap_attributes(self, inval, self.__allowed_var_names)
        self._parse_choppers()
        self._parse_variants()
        self.phase = self.defaultPhase
        self.frequency = self.default_frequencies

    def __repr__(self):
        return self.name if self.name else "Undefined disk chopper system"

    def _parse_choppers(self):
        """Parses the choppers list to determine how to handle resolution and flux calculations"""
        if not self.choppers:
            return
        self.distance = []
        self.nslot = []
        self.slot_ang_pos = []
        self.slot_width = []
        self.guide_width = []
        self.radius = []
        self.numDisk = []
        self.isFermi = False
        self.isPhaseIndependent = []
        self.defaultPhase = []
        self.phaseOffset = None
        self.phaseNames = []
        for idx, chopper in enumerate(self.choppers):
            self.distance.append(chopper["distance"])
            if "packages" in chopper:
                self.isFermi = True
                self.idxFermi = idx
                self.packages = {key: FermiChopper(val) for key, val in list(chopper["packages"].items())}
                self.nslot.append(1)  # Assume Fermi chopper is curved, will not transmit PI pulse.
                self.slot_ang_pos.append(None)
                self.slot_width.append(10.0)
                self.guide_width.append(10.0)
                self.radius.append(290.0)
                self.numDisk.append(1)
            else:
                self.nslot.append(chopper["nslot"] if ("nslot" in chopper and chopper["nslot"]) else len(chopper["slot_ang_pos"]))
                self.slot_ang_pos.append(chopper["slot_ang_pos"] if ("slot_ang_pos" in chopper) else None)
                self.slot_width.append(chopper["slot_width"])
                self.guide_width.append(chopper["guide_width"])
                self.radius.append(chopper["radius"])
                self.numDisk.append(2 if ("isDouble" in chopper and chopper["isDouble"]) else 1)
                self.isPhaseIndependent.append(True if ("isPhaseIndependent" in chopper and chopper["isPhaseIndependent"]) else False)
                self.defaultPhase.append(chopper["defaultPhase"] if "defaultPhase" in chopper else 0)
                if "phaseOffset" in chopper:
                    self.phaseOffset = chopper["phaseOffset"]
                self.phaseNames.append(chopper["phaseName"] if "phaseName" in chopper else "Chopper %d phase delay time" % (idx))
        if not any(self.slot_ang_pos):
            self.slot_ang_pos = None
        self.isPhaseIndependent = [ii for ii in range(len(self.isPhaseIndependent)) if self.isPhaseIndependent[ii]]
        self.defaultPhase = [self.defaultPhase[ii] for ii in self.isPhaseIndependent]
        self.phaseNames = [self.phaseNames[ii] for ii in self.isPhaseIndependent]
        source_rep = self.source_rep if (not hasattr(self, "n_frame") or self.n_frame == 1) else [self.source_rep, self.n_frame]
        self._instpar = [
            self.distance,
            self.nslot,
            self.slot_ang_pos,
            self.slot_width,
            self.guide_width,
            self.radius,
            self.numDisk,
            self.sam_det,
            self.chop_sam,
            source_rep,
            self.emission_time,
            self.overlap_ei_frac,
            self.isPhaseIndependent,
        ]
        if self.isFermi:
            self.package = list(self.packages.keys())[0]

    def _parse_variants(self):
        if "variants" not in self.__dict__:
            return
        variant_keys = []
        for var in self.variants:
            if ("default" in self.variants[var].keys() and self.variants[var]["default"]) or var is None:
                self._default_variant = self._variant = var
            if var:
                [variant_keys.append(key) for key in self.variants[var].keys() if "default" not in key]
        self._variant_defaults = {}
        for key in set(variant_keys):
            self._variant_defaults[key] = copy.deepcopy(getattr(self, key))
        if "_variant" not in self.__dict__:
            self._default_variant = list(self.variants.keys())[0]
            warnings.warn("No default variants defined. Using " "%s" " as default" % (self._default_variant), SyntaxWarning)
            self.variant = self._default_variant

    def setChopper(self, *args, **kwargs):
        """
        Set the chopper package type (Fermi instruments) or variant (LET).

        maps = Instrument('MAPS')
        maps.setChopper('A', 400)                     # Sets package A at 400 Hz.
        maps.setChopper(package='A', freq=400)        # Explicit keywords
        let = Instrument('LET')
        let.setChopper('High resolution', [280, 140]) # Change to the high resolution variant at 280 Hz
        let.setChopper(variant='High resolution')
        """
        argdict = argparser(args, kwargs, ["package" if self.isFermi else "variant", "freq"])
        if self.isFermi:
            self.package = argdict["package"]
            if hasattr(self, "variants") and argdict["package"] in self.variants:
                self.variant = argdict["package"]
        elif argdict["variant"]:
            self.variant = argdict["variant"]
        if argdict["freq"]:
            self.frequency = argdict["freq"]

    def getChopper(self):
        return self.package if self.isFermi else self.variant

    def getChopperNames(self):
        choppers = list(self.packages.keys()) if self.isFermi else []
        return sorted(set(choppers + (list(self.variants.keys()) if hasattr(self, "variants") else [])))

    def setFrequency(self, *args, **kwargs):
        """
        Set the chopper frequency(ies) and (optionally) phase(s).

        maps = Instrument('MAPS', 'A')
        maps.setFrequency(400)                        # Sets frequency to 400 Hz.
        maps.setFrequency([400, 100], 1)              # Sets Fermi to 400Hz, disk to 100Hz, and multi-rep mode
        maps.setFrequency(freq=[400, 100], phase=1)
        let = Instrument('LET')
        let.setFrequency([240, 120])                  # Sets resolution chopper to 240Hz, pulse removal to 120Hz
        let.setFrequency([240, 120], phase=-20000)    # Additionally sets the frame overlap phase to -20000us
        """
        argdict = argparser(args, kwargs, ["freq", "phase"])
        if argdict["freq"]:
            self.frequency = argdict["freq"]
        if argdict["phase"]:
            self.phase = argdict["phase"]

    def getFrequency(self):
        return self.frequency

    def setEi(self, Ei):
        """Sets the (focussed) incident energy"""
        emin = max(self.emin, self.packages[self.package].emin if self.isFermi else 0)
        emax = min(self.emax, self.packages[self.package].emax if self.isFermi else np.inf)
        if Ei < emin or Ei > emax:
            raise ValueError(f"Ei={Ei} is outside limits [{emin}, {emax}]")
        self.ei = Ei

    def getEi(self):
        return self.ei

    def getAllowedEi(self, Ei_in=None):
        return set(np.round(self._MulpyRepDriver(Ei_in, calc_res=False)[0], decimals=4))

    def plotMultiRepFrame(self, h_plt=None, Ei_in=None, frequency=None, first_rep=False):
        """
        Plots the time-distance diagram into a given Matplotlib axes, i
        for a give focused incident energy (in meV) and chopper frequencies (in Hz).
        """
        if h_plt is None:
            try:
                from matplotlib import pyplot
            except ImportError:
                raise RuntimeError("plotMultiRepFrame: Cannot import matplotlib")
            plt = pyplot
        else:
            plt = h_plt
        _check_input(self, Ei_in)
        if frequency:
            oldfreq = self.freq
            self.setFrequency(frequency)
        Eis, _, _, lines, chop_times = tuple(self._MulpyRepDriver(Ei_in, calc_res=False))
        if frequency:
            self.setFrequency(oldfreq)
        modSamDist = self.distance[-1] + self.chop_sam
        totDist = modSamDist + self.sam_det
        xmax = 1.0e6 / self.source_rep
        if hasattr(self, "n_frame") and self.n_frame > 1:
            xmax *= self.n_frame
            for i in range(1, self.n_frame):
                plt.plot([i * 1.0e6 / self.source_rep] * 2, [0, totDist], c="k")
        limits = [-1.0e6 / self.source_rep, xmax]
        for i in range(len(self.distance)):
            plt.plot(limits, [self.distance[i], self.distance[i]], c="k", linewidth=1.0)
            for j in range(len(chop_times[i][:])):
                plt.plot(chop_times[i][j], [self.distance[i], self.distance[i]], c="white", linewidth=1.0)
        plt.plot(limits, [totDist, totDist], c="k", linewidth=2.0)
        for i in range(len(lines)):
            x0 = (-lines[i][0][1] / lines[i][0][0] - lines[i][1][1] / lines[i][1][0]) / 2.0
            x1 = ((modSamDist - lines[i][0][1]) / lines[i][0][0] + (modSamDist - lines[i][1][1]) / lines[i][1][0]) / 2.0
            if np.abs(x0) > 500:
                if first_rep:
                    continue
                maincolor = "m"
            else:
                maincolor = "b"
            plt.plot([x0, x1], [0, modSamDist], c=maincolor)
            x2 = ((totDist - lines[i][0][1]) / lines[i][0][0] + (totDist - lines[i][1][1]) / lines[i][1][0]) / 2.0
            lineM = totDist / (x2 - x0)
            plt.plot([x1, x2], [modSamDist, totDist], c=maincolor)
            newline = [lineM * np.sqrt(1 + self.overlap_ei_frac), modSamDist - lineM * np.sqrt(1 + self.overlap_ei_frac) * x1]
            x3 = (totDist - newline[1]) / (newline[0])
            plt.plot([x1, x3], [modSamDist, totDist], c="r")
            newline = [lineM * np.sqrt(1 - self.overlap_ei_frac), modSamDist - lineM * np.sqrt(1 - self.overlap_ei_frac) * x1]
            x4 = (totDist - newline[1]) / (newline[0])
            plt.plot([x1, x4], [modSamDist, totDist], c="r")
            plt.text(x2, totDist + 0.2, "{:3.1f}".format(Eis[i]))
        if h_plt is None:
            plt.xlim(0, xmax)
            plt.xlabel(r"TOF ($\mu$sec)")
            plt.ylabel("Distance (m)")
            plt.show()
        else:
            plt.set_xlim(0, xmax)
            plt.set_xlabel(r"TOF ($\mu$sec)")
            plt.set_ylabel(r"Distance (m)")

    def getWidthSquared(self, Ei_in=None):
        return self.getWidth(Ei_in, squared=True)

    def getWidth(self, Ei_in=None, squared=False):
        """Returns the chopper time width (FWHM) at the (final) chopper in microseconds"""
        if self.isFermi:
            return self._ChopDriver(Ei_in, squared), None
        else:
            chop_times = self._MulpyRepDriver(Ei_in, calc_res=False)[1]
            # Output of MulpyRep is FWHM in us - want it in seconds for later calculations
            wd = ((chop_times[1][1] - chop_times[1][0]) / 2.0 / 1.0e6, (chop_times[0][1] - chop_times[0][0]) / 2.0 / 1.0e6)
            return (wd[0] ** 2, wd[1] ** 2) if squared else wd

    def getDistances(self):
        """Returns the (mod->final_chop, aperture->final, chop->sam, sam->det, mod-first_chop) distances for instrument"""
        mod_chop = self.choppers[-1]["distance"]
        ap_chop = self.choppers[-1]["aperture_distance"] if ("aperture_distance" in self.choppers[-1]) else mod_chop
        return (mod_chop, ap_chop, self.chop_sam, self.sam_det, self.choppers[0]["distance"])

    def getTransmission(self, Ei_in=None, frequency=None, hires=False):
        """Calculates the flux transmission fraction through the chopper system at specified Ei and frequency"""
        Ei = _check_input(self, Ei_in)
        freq = frequency if frequency is not None else self._long_frequency[-1]
        if self.isFermi:
            x0, x1 = (self.choppers[-1]["distance"], self.chop_sam)
            magic = 84403.06 / x0 / (x1 + x0)  # Some magical conversion factor (??)
            fudge = self.packages[self.package].fluxcorr  # A chopper package dependent fudge factor
            return self.packages[self.package].getTransmission(Ei, freq) * magic / fudge
        else:
            # For disk choppers, transmission goes quadratic with freq at high resolution, linear at low
            freqdep = (self.flux_ref_freq / freq) ** 2 if hires else (self.flux_ref_freq / freq)
            return (self.slot_width[-1] / self.flux_ref_slot) * freqdep

    def setNFrame(self, value):
        self.n_frame = value
        self._instpar[9] = [self.source_rep, value]

    def _get_state(self, Ei_in=None):
        return hash((self.variant, self.package, tuple(self.frequency), tuple(self.phase), Ei_in if Ei_in else self.ei, self.n_frame))

    def _removeLowIntensityReps(self, Eis, lines, Ei=None):
        # Removes reps with Ei where there are no neutrons
        if not hasattr(self, "ei_limits") or not self.ei_limits:
            return Eis, lines
        Eis = np.array(Eis)
        idx = (Eis >= self.ei_limits[0]) * (Eis <= self.ei_limits[1])
        # Always keeps desired rep even if outside of range
        if Ei:
            idx += (np.abs(Eis - Ei) / np.abs(Eis)) < 0.1
        Eis = Eis[idx]
        lines = np.array(lines)[idx]
        return Eis, lines

    def _MulpyRepDriver(self, Ei_in=None, calc_res=True):
        """Private method to calculate resolution for given Ei from chopper opening times"""
        Ei = _check_input(self, Ei_in)
        if "_saved_state" not in self.__dict__ or (self._saved_state[0] != self._get_state(Ei)):
            Eis, all_times, chop_times, lastChopDist, lines = MulpyRep.calcChopTimes(
                Ei, self._long_frequency, self._instpar, self.phase, self.phaseOffset
            )
            Eis, lines = self._removeLowIntensityReps(Eis, lines, Ei)
            self._saved_state = [self._get_state(Ei), Eis, chop_times, lastChopDist, lines, all_times]
        else:
            Eis, chop_times, lastChopDist, lines, all_times = tuple(self._saved_state[1:])
        if calc_res:
            res_el, percent, chop_width, mod_width = MulpyRep.calcRes(
                Eis, chop_times, lastChopDist, self.chop_sam, self.sam_det, self.guide_width[-1], self.slot_width[-1]
            )
            return res_el, percent, chop_width, mod_width
        else:
            return [Eis, chop_times, lastChopDist, lines, all_times]

    def _ChopDriver(self, Ei_in=None, squared=False):
        """Private method to calculate resolution for given Ei from Fermi chopper"""
        Ei = _check_input(self, Ei_in)
        if squared:
            return self.packages[self.package].getWidthSquared(Ei, self._long_frequency[-1]) * SIGMA2FWHMSQ
        else:
            return self.packages[self.package].getWidth(Ei, self._long_frequency[-1]) * SIGMA2FWHM

    @property
    def frequency(self):
        return self._frequency

    @frequency.setter
    def frequency(self, value):
        freq = self.default_frequencies
        if not hasattr(value, "__len__"):
            value = [value]
        freq = [value[i] if i < len(value) else freq[i] for i in range(len(freq))]
        if self.max_frequencies and not (freq <= self.max_frequencies):
            raise ValueError("Value of frequencies outside maximum allowed")
        self._frequency = freq
        if hasattr(self, "constant_frequencies") and self.constant_frequencies:
            f0 = self.constant_frequencies
        else:
            f0 = [0] * np.shape(self.frequency_matrix)[0]
        self._long_frequency = np.dot(self.frequency_matrix, freq) + f0

    @property
    def phase(self):
        return self._phase

    @phase.setter
    def phase(self, value):
        phase = self.defaultPhase
        if not hasattr(value, "__len__"):
            value = [value]
        self._phase = [value[i] if i < len(value) else phase[i] for i in range(len(phase))]

    @property
    def ei(self):
        return self._ei

    @ei.setter
    def ei(self, value):
        if value < 0:
            raise ValueError("Incident neutron energy cannot be less than zero")
        self._ei = value

    @property
    def package(self):
        return self._package if self.isFermi else None

    @package.setter
    def package(self, value):
        if not self.isFermi:
            raise AttributeError("Cannot set Fermi chopper package on this instrument")
        if value not in self.packages.keys():
            ky = [k for k in self.packages.keys() if str(value).upper() == k.upper()]
            if not ky:
                raise ValueError("Fermi package " "%s" " not recognised. Allowed values are: %s" % (value, ", ".join(self.packages.keys())))
            else:
                value = ky[0]
        self._package = value
        # Sets whether to allow pi pulse or not
        idx = [i for i in range(len(self.choppers)) if "packages" in self.choppers[i]][0]
        self._instpar[1][idx] = 2 if self.packages[value].isPi else 1

    @property
    def variant(self):
        return self._variant if hasattr(self, "variants") and self.variants else None

    def getAllowedChopper(self):
        return self.packages.keys() if self.isFermi else (self.variants.keys() if self.variants else None)

    @variant.setter
    def variant(self, value):
        if "variants" not in self.__dict__:
            raise AttributeError("This instrument has no variants to set")
        for prop in self._variant_defaults:
            setattr(self, prop, copy.deepcopy(self._variant_defaults[prop]))
        self._variant = value
        if value not in self.variants.keys():
            ky = [k for k in self.variants.keys() if str(value).upper() == k.upper()]
            if not ky:
                raise ValueError("Variant " "%s" " not recognised. Allowed values are: %s" % (value, ", ".join(self.variants.keys())))
            else:
                value = ky[0]
        for prop in self.variants[value]:
            if prop == "choppers":
                for idx, chopper in enumerate(self.variants[value][prop]):
                    if chopper:
                        for ky in chopper:
                            self.choppers[idx][ky] = chopper[ky]
            elif prop in self.__allowed_var_names:
                setattr(self, prop, self.variants[value][prop])
        self._parse_choppers()

    @property
    def tjit(self):
        return self.packages[self.package].tjit if self.isFermi else 0.0

    @tjit.setter
    def tjit(self, value):
        self.packages[self.package].tjit = value

    @property
    def emin(self):
        return self.ei_limits[0] if hasattr(self, "ei_limits") and self.ei_limits else 0.1

    @property
    def emax(self):
        return self.ei_limits[1] if hasattr(self, "ei_limits") and self.ei_limits else 1000


class Moderator(object):
    """
    Class which represents a neutron moderator
    """

    __allowed_var_names = [
        "name",
        "imod",
        "ch_mod",
        "mod_pars",
        "mod_scale_fn",
        "mod_scale_par",
        "theta",
        "source_rep",
        "n_frame",
        "emission_time",
        "measured_flux",
        "measured_width",
    ]

    def __init__(self, inval=None):
        wrap_attributes(self, inval, self.__allowed_var_names)
        self.flux_units = "n/cm^2/s"
        if hasattr(self, "measured_flux") and self.measured_flux:
            if "scale_factor" in self.measured_flux:
                self.measured_flux["flux"] = np.array(self.measured_flux["flux"]) * float(self.measured_flux["scale_factor"])
            if "units" in self.measured_flux:
                self.flux_units = self.measured_flux["units"]
            idx = np.argsort(self.measured_flux["wavelength"])
            wavelength = np.array(self.measured_flux["wavelength"])[idx]
            flux = np.array(self.measured_flux["flux"])[idx]
            self.flux_interp = interp1d(wavelength, flux, kind="cubic")
            self.fmn, self.fmx = (min(wavelength), max(wavelength))
        if hasattr(self, "measured_width") and self.measured_width:
            idx = np.argsort(self.measured_width["wavelength"])
            wavelength = np.array(self.measured_width["wavelength"])[idx]
            widths = np.array(self.measured_width["width"])[idx]
            self.width_interp = interp1d(wavelength, widths, kind="slinear")
            self.wmn, self.wmx = (min(wavelength), max(wavelength))
            if "isSigma" not in self.measured_width.keys():
                self.measured_width["isSigma"] = False

    def __repr__(self):
        return self.name if self.name else "Undefined neutron moderator"

    def getAnalyticWidthsSquared(self, Ei):
        if self.imod == 0:
            # CHOP outputs the Gaussian sigma^2 in s^2, we want FWHM^2 in s^2
            tsqmod = Chop.tchi(self.mod_pars / 1000.0, Ei) * SIGMA2FWHMSQ
        elif self.imod == 1:
            tsqmod = Chop.tikeda(*tuple(self.mod_pars + [Ei])) * SIGMA2FWHMSQ
        elif self.imod == 2:
            d0 = self.mod_pars[0]
            if hasattr(self, "mod_scale_fn") and self.mod_scale_fn:
                try:
                    d0 *= globals()[self.mod_scale_fn](Ei, self.mod_scale_par)
                except KeyError:
                    pass
            tsqmod = Chop.tchi_2(d0 / 1000.0, self.mod_pars[1] / 1000.0, Ei) * SIGMA2FWHMSQ
        elif self.imod == 3:
            # Mode for LET - output of polynomial is FWHM in us
            tsqmod = np.polyval(self.mod_pars, np.sqrt(E2L / Ei)) ** 2 / 1e12
        else:
            raise RuntimeError("PyChop: Undefined moderator time profile type %d" % (self.imod))
        return tsqmod

    def getWidthSquared(self, Ei):
        """Returns the squared time gaussian FWHM width due to the sample in s^2"""
        if hasattr(self, "width_interp"):
            wavelength = np.sqrt(E2L / (Ei if not hasattr(Ei, "__len__") else Ei[0]))
            if wavelength >= self.wmn:
                # Data is obtained from measuring widths of powder Bragg peaks in backscattering
                # At low wavelengths / high energies, the peaks are too close together to discern
                # so there is no measurements, but the analytical expressions should still be good.
                width = self.width_interp(min([wavelength, self.wmx])) ** 2 / 1e12
                return (width * SIGMA2FWHMSQ) if self.measured_width["isSigma"] else width
        return self.getAnalyticWidthsSquared(Ei)

    def getWidth(self, Ei):
        """Calculates the moderator time width in seconds for a given neutron energy (Ei)"""
        if hasattr(self, "width_interp"):
            wavelength = np.sqrt(E2L / (Ei if not hasattr(Ei, "__len__") else Ei[0]))
            if wavelength >= self.wmn:
                width = self.width_interp(min([wavelength, self.wmx])) / 1e6  # Table has widths in microseconds
                return width * SIGMA2FWHM if self.measured_width["isSigma"] else width
        if self.imod == 3:
            # Mode for LET - output of polynomial is FWHM in us
            return np.polyval(self.mod_pars, np.sqrt(E2L / Ei)) / 1e6
        else:
            return np.sqrt(self.getAnalyticWidthSquared(Ei))

    def getFlux(self, Ei):
        """Returns the white beam flux estimate from either measured data (preferred) or analytical model (backup)"""
        return self.getMeasuredFlux(Ei) if hasattr(self, "flux_interp") else self.getAnalyticFlux(Ei)

    def getAnalyticFlux(self, Ei):
        """Estimate white beam flux from TGP's model of the moderators (ISIS TS1 only)"""
        if all([self.name != modtype for modtype in ["AP", "CH4", "H2"]]):
            raise AttributeError("No analytical model for moderator %s" % (self.name))
        return Chop.flux_calc(np.array(Ei), self.name, self.theta_m * np.pi / 180.0)

    def getMeasuredFlux(self, Ei):
        """Interpolates flux from a table of measured flux"""
        if not hasattr(self, "flux_interp"):
            raise AttributeError("This instrument does not have a table of measured flux")
        wavelengths = [
            min(max(wavelength, self.fmn), self.fmx) for wavelength in np.sqrt(E2L / np.array(Ei if hasattr(Ei, "__len__") else [Ei]))
        ]
        return self.flux_interp(wavelengths[0])

    @property
    def theta_m(self):
        return self.theta if (hasattr(self, "theta") and self.theta) else 0.0


class Sample(object):
    """
    Class which represents a sample shape
    """

    __allowed_var_names = ["name", "sx", "sy", "sz", "isam", "gamma"]

    def __init__(self, inval=None):
        wrap_attributes(self, inval, self.__allowed_var_names)

    def __repr__(self):
        return self.name if self.name else "Undefined sample"

    def getWidthSquared(self):
        """Returns the squared time FWHM due to the sample in s^2"""
        # At the moment this routine only returns a non-zero y (beam-axis) width
        return Chop.sam0(self.sx / 1000.0, self.sy / 1000.0, self.sz / 1000.0, self.isam)[1] * SIGMA2FWHMSQ

    def getWidth(self):
        return np.sqrt(self.getWidthSquared)

    @property
    def gamma_deg(self):
        return self.gamma if (hasattr(self, "gamma") and self.gamma) else 0.0


class Detector(object):
    """
    Class which represents a neutron detector
    """

    __allowed_var_names = ["name", "idet", "dd", "tbin", "phi", "tthlims"]

    def __init__(self, inval=None):
        wrap_attributes(self, inval, self.__allowed_var_names)

    def __repr__(self):
        return self.name if self.name else "Undefined detector"

    def getWidthSquared(self, Ei, en=0):
        """Returns the squared time FWHM due to the detector in s^2"""
        return self.getWidth(Ei, en) ** 2

    def getWidth(self, Ei, en=0):
        return Chop.detect2(1.0, 1.0, np.sqrt(E2K * (Ei - en)), self.idet, self.dd)[3] * SIGMA2FWHM

    @property
    def phi_deg(self):
        return self.phi if (hasattr(self, "phi") and self.phi) else 0.0


class Instrument(object):
    """
    Class which represents a direct geometry neutron spectrometer
    """

    __allowed_var_names = ["name", "sample", "chopper_system", "moderator", "detector"]

    __child_methods = [
        "setChopper",
        "getChopper",
        "setFrequency",
        "getFrequency",
        "setEi",
        "getEi",
        "getAllowedEi",
        "plotMultiRepFrame",
        "getChopperNames",
        "isFermi",
    ]

    __child_properties = ["package", "variant", "frequency", "phase", "ei", "tjit", "emin", "emax"]

    __known_instruments = ["let", "maps", "mari", "merlin", "arcs", "cncs", "hyspec", "sequoia"]

    def __init__(self, instrument, chopper=None, freq=None):
        if isinstance(instrument, str):
            # check if it is a file or instrument name we want
            if instrument.lower() in self.__known_instruments:
                import os.path
                import sys

                folder = os.path.dirname(sys.modules[self.__module__].__file__)
                instrument = os.path.join(folder, instrument.lower() + ".yaml")
            try:
                with open(instrument) as f:
                    instrument = yaml.safe_load(f)
            except (OSError, IOError) as e:
                raise RuntimeError("Cannot open file %s . Error is %s" % (instrument, e))
        if (hasattr(instrument, "moderator") or hasattr(instrument, "chopper_system")) or (
            "moderator" in instrument or "chopper_system" in instrument
        ):
            wrap_attributes(self, instrument, self.__allowed_var_names)
            if isinstance(self.moderator, dict) and isinstance(self.chopper_system, dict):
                for key in ["source_rep", "n_frame", "emission_time"]:
                    if key in self.moderator:
                        self.chopper_system[key] = self.moderator[key]
        else:
            raise RuntimeError("Input to Instrument must be an Instrument object, a dictionary or a filename string")
        # If we have just loaded a YAML file or constructed from a dictionary, need to convert to correct class
        for elem_nm, classref in zip(["sample", "chopper_system", "moderator", "detector"], [Sample, ChopperSystem, Moderator, Detector]):
            try:
                element = getattr(self, elem_nm)
                if isinstance(element, dict):
                    setattr(self, elem_nm, classref(element))
                setattr(self, "has_" + elem_nm, True)
            except AttributeError:
                setattr(self, "has_" + elem_nm, False)
        if not self.has_chopper_system or not self.has_moderator:
            raise AttributeError("No chopper system or moderator found in input.")
        for method in self.__child_methods:
            setattr(self, method, getattr(self.chopper_system, method))
        for prop in self.__child_properties:
            setattr(
                type(self),
                prop,
                property(
                    lambda obj, prop=prop, self=self: ChopperSystem.__dict__[prop].__get__(self.chopper_system, ChopperSystem),
                    lambda obj, val, prop=prop, self=self: ChopperSystem.__dict__[prop].__set__(self.chopper_system, val),
                ),
            )
        # Now reset default chopper/variant and frequency
        if chopper or freq:
            self.setChopper(chopper if chopper else self.getChopper(), freq if freq else self.frequency)

    def setInstrument(self, instrument):
        self.__dict__.clear()
        self.__init__(instrument)

    def getFlux(self, Ei_in=None, frequency=None):
        """Returns the monochromatic flux estimate in n/cm^2/s"""
        Ei = _check_input(self.chopper_system, Ei_in)
        isHires = False if (self.isFermi or (self.getResolution(0.0, Ei) / Ei) > 0.02) else True
        return self.moderator.getFlux(Ei) * self.chopper_system.getTransmission(Ei, frequency, hires=isHires)

    def getMultiRepFlux(self, Ei_in=None, frequency=None):
        Ei, _ = _check_input(self.chopper_system, Ei_in, frequency)
        if frequency:
            oldfreq = self.frequency
            self.frequency = frequency
        fluxes = [self.getFlux(ei, frequency) for ei in self.getAllowedEi(Ei)]
        if frequency:
            self.frequency = oldfreq
        return fluxes

    def getResFlux(self, Etrans=None, Ei_in=None, frequency=None):
        """Returns the resolution and flux as a tuple."""
        return self.getResolution(Etrans, Ei_in, frequency), self.getFlux(Ei_in, frequency)

    def getWidths(self, Ei_in=None, frequency=None):
        """Returns the time FWHM of different components for one rep (Ei) in microseconds"""
        Ei = _check_input(self.chopper_system, Ei_in)
        try:
            widths = self.getVanVar(Ei, frequency)
        except ValueError:
            return None
        widths[1]["Energy"] = (2 * E2V * np.sqrt(Ei**3 * widths[0])) / self.chopper_system.sam_det
        return {k: v if "Energy" in k else np.sqrt(v) * 1e6 for k, v in list(widths[1].items())}

    def getMultiWidths(self, Ei_in=None, frequency=None):
        """Returns the time FWHM of different components for each possible rep (Ei) in seconds"""
        Ei = _check_input(self.chopper_system, Ei_in)
        Eis = self.getAllowedEi(Ei)
        outdic = {"Eis": Eis}
        widths = [self.getWidths(ei, frequency) for ei in Eis]
        for ky in widths[0]:
            outdic[ky] = np.hstack(np.array([w[ky] for w in widths if w]))
        return outdic

    def getResolution(self, Etrans=None, Ei_in=None, frequency=None):
        """
        Calculates resolution (energy) widths

        van = getResolution()
        van = getResolution(etrans)
        van = getResolution(etrans, ei, omega)

        Inputs:
            etrans - list of numpy array of energy transfers to calculate for (meV) [default: linspace(0.05Ei, 0.95Ei, 19)]
            ei - incident energy in meV [default: preset energy]
            omega - chopper frequency in Hz  [default: preset frequency]

        Output:
            van - the incoherent (Vanadium) energy FWHM at etrans in meV
        """
        Ei = _check_input(self.chopper_system, Ei_in)
        # If not set, sets energy transfers to values to compare exactly to RAE's original implementation.
        if Etrans is None:
            Etrans = np.linspace(0.05 * Ei, 0.95 * Ei + 0.05 * 0.05 * Ei, 19, endpoint=True)
        Etrans = np.array(Etrans if np.shape(Etrans) else [Etrans], dtype=float)
        if len(np.where(Etrans > Ei)[0]) > 0:
            warnings.warn("Cannot calculate for energy transfer greater than Ei (physically negative neutron energies!)")
        Etrans[np.where(Etrans >= Ei)] = np.nan
        v_van, _, _ = self.getVanVar(Ei, frequency, Etrans)
        x2 = self.chopper_system.sam_det
        Ef = Ei - np.array(Etrans)
        van = (2 * E2V * np.sqrt(Ef**3 * v_van)) / x2
        return van

    def getMultiRepResolution(self, Etrans=None, Ei_in=None, frequency=None):
        """Returns a list of FWHM in meV for all allowed Ei's in multirep mode (in same order as getAllowedEi)
        The input energy transfer is interpreted as fractions of Ei. e.g. linspace(0,0.9,100)"""
        Ei = _check_input(self.chopper_system, Ei_in)
        if Etrans is None:
            Etrans = np.linspace(0.05, 0.95, 19, endpoint=True)
        return [self.getResolution(Etrans * ei, ei, frequency) for ei in self.getAllowedEi(Ei)]

    def getVanVar(self, Ei_in=None, frequency=None, Etrans=0):
        """Calculates the time squared FWHM in s^2 at the sample (Vanadium widths) for different components"""
        Ei, _ = _check_input(self.chopper_system, Ei_in, frequency)
        Etrans = np.array(Etrans if np.shape(Etrans) else [Etrans])
        if frequency:
            oldfreq = self.frequency
            self.frequency = frequency
        tsqmod = self.moderator.getWidthSquared(Ei)
        tsqchp = self.chopper_system.getWidthSquared(Ei)
        tsqjit = self.tjit**2
        # Gets distances: x0=mod-final chopper, xa=aperture-final, x1=final-sample, x2=sample-det, xm=mod-first chopper
        x0, xa, x1, x2, xm = self.chopper_system.getDistances()
        # For Disk chopper spectrometers, the opening times of the first chopper can be the effective moderator time
        if tsqchp[1] is not None:
            frac_dist = 1 - (xm / x0)
            tsmeff = tsqmod * frac_dist**2  # Effective moderator time at first chopper
            x0 -= xm  # Propagate from first chopper, not from moderator (after rescaling tmod)
            tsqmod = tsmeff if (tsqchp[1] > tsmeff) else tsqchp[1]
        tsqchp = tsqchp[0]
        tsqmodchop = np.array([tsqmod, tsqchp, x0])
        # Propagate the time widths to the sample position
        omega = self.frequency[0] * 2 * np.pi
        vi = E2V * np.sqrt(Ei)
        vf = E2V * np.sqrt(Ei - Etrans)
        vratio = (vi / vf) ** 3
        tanthm = np.tan(self.moderator.theta_m * np.pi / 180.0)
        g1, g2 = tuple(1.0 - ((omega * tanthm / vi) * np.array([xa + x1, x0 - xa])))
        f1, f2 = tuple(1.0 + (x1 / x0) * np.array([g1, g2]))
        g1, g2, f1, f2 = tuple(np.array([g1, g2, f1, f2]) / (omega * (xa + x1)))
        modfac = (x1 + vratio * x2) / x0
        chpfac = 1.0 + modfac
        apefac = f1 + ((vratio * x2 / x0) * g1)
        tsqmod *= modfac**2
        tsqchp *= chpfac**2
        tsqjit *= chpfac**2
        tsqape = apefac**2 * (self.aperture_width**2 / 12.0) * SIGMA2FWHMSQ
        vsqvan = tsqmod + tsqchp + tsqjit + tsqape
        outdic = {"moderator": tsqmod, "chopper": tsqchp, "jitter": tsqjit, "aperture": tsqape}
        if self.has_detector and hasattr(self.detector, "idet"):
            phi = self.detector.phi_deg * np.pi / 180.0
            tsqdet = (1.0 / vf) ** 2 * np.array([self.detector.getWidthSquared(Ei, en) for en in Etrans])
            vsqvan += tsqdet
            outdic["detector"] = tsqdet
        else:
            phi = 0.0
        if self.has_sample:
            gam = self.sample.gamma_deg * np.pi / 180.0
            bb = (-np.sin(gam) / vi) + (np.sin(gam - phi) / vf) - (f2 * np.cos(gam))
            samfac = bb - ((vratio * x2 / x0) * g2 * np.cos(gam))
            tsqsam = samfac**2 * self.sample.getWidthSquared()
            vsqvan += tsqsam
            outdic["sample"] = tsqsam
        if frequency:
            self.frequency = oldfreq
        return vsqvan, outdic, tsqmodchop

    @property
    def aperture_width(self):
        if hasattr(self.chopper_system, "aperture_width") and self.chopper_system.aperture_width:
            return self.chopper_system.aperture_width
        return 0.0

    @property
    def aperture_height(self):
        if hasattr(self.chopper_system, "aperture_height") and self.chopper_system.aperture_height:
            return self.chopper_system.aperture_height
        return 0.0

    @property
    def instname(self):
        return self.name

    @property
    def n_frame(self):
        return self.chopper_system.n_frame

    @n_frame.setter
    def n_frame(self, value):
        self.moderator.n_frame = value
        self.chopper_system.setNFrame(value)

    @classmethod
    def calculate(cls, *args, **kwargs):
        """
        ! Calculates the resolution and flux directly
        !
        ! from pychop.Instruments import Instrument
        !
        ! Instrument.calculate('mari', 's', 250., 55.)                # Instname, Chopper Type, Freq, Ei in order
        ! Instrument.calculate('let', 'High flux', [160., 80.], 2.2)  # For LET, specify resolution and pulse remover freq
        ! Instrument.calculate(inst='mari', package='s', freq=250., ei=55.) # With keyword arguments
        ! Instrument.calculate(inst='let', variant='High resolution', freq=[160., 80.], ei=2.2)
        !
        ! For LET, the allowed variant names are:
        !   'High resolution'
        !   'High flux'
        !   'Intermediate'
        ! You have to use these strings exactly.
        !
        ! By default this function returns the elastic resolution and flux only.
        ! If you want the inelastic resolution, specify the inelastic energy transfer
        ! as either the last positional argument, or as a keyword argument, e.g.:
        !
        ! Instrument.calculate('merlin', 'g', 450., 60., range(55))
        ! Instrument.calculate('maps', 'a', 450., 600., etrans=np.linspace(0,550,55))
        !
        ! For fast calculations, one can return a polynomial approximation (cubic) of the
        ! resolution function. By passing etrans='polynomial', the calculator estimates the
        ! resolution for etrans=np.arange(-Ei, Ei, Ei*0.01) then fits it to a cubic polynomial.
        ! The resolution is then an array with coefficients, from the lowest power.
        !
        ! res, flux = Instrument.calculate(inst='cncs', variant='High flux', freq=240, ei=1.5, etrans='polynomial')
        !
        ! The results are returned as tuple: (resolution, flux)
        """
        argdict = argparser(args, kwargs, ["inst", "package", "frequency", "ei", "etrans", "variant"])
        if argdict["inst"] is None:
            raise RuntimeError("You must specify the instrument name")
        obj = cls(argdict["inst"])
        obj.setChopper(argdict["package"], argdict["frequency"])
        obj.ei = argdict["ei"]
        if argdict["variant"]:
            obj.variant = argdict["variant"]
        etrans = argdict["etrans"]
        return_polynomial = False
        if etrans is None:
            etrans = 0.0
        else:
            if etrans == "polynomial":
                return_polynomial = True
                etrans = np.arange(-obj.ei, obj.ei, obj.ei * 0.01)
            try:
                etrans = float(etrans)
            except TypeError:
                etrans = np.asarray(etrans, dtype=float)
        res = obj.getResolution(etrans)
        if return_polynomial:

            def cubic(x, x_0, x_1, x_2, x_3):
                return x_0 + x_1 * x + x_2 * x**2 + x_3 * x**3

            popt, pcov = curve_fit(cubic, etrans, res)
            res = popt
        flux = obj.getFlux()
        return res, flux

    def __repr__(self):
        return self.name if self.name else "Undefined instrument"
