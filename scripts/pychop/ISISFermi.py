# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=line-too-long, bad-continuation, invalid-name, unused-variable
# pylint: disable=attribute-defined-outside-init, old-style-class, no-value-for-parameter
# pylint: disable=too-many-locals, too-many-arguments

"""
Contains the ISISFermi class which calculates the resolution and flux for ISIS Fermi chopper
spectrometers (MAPS, MARI, MERLIN) - using the functions in Chop.py and addition lookup tables.
"""

import warnings
import numpy as np
from . import Chop
from scipy.interpolate import interp1d
from scipy.special import erf


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


class ISISFermi:
    """
    Class to calculate the resolution and flux of ISIS Fermi chopper spectrometers (MAPS, MARI, MERLIN).
    """

    __Instruments = {
        #         x0       xa      x1      x2      wa       ha       gamma  idet  dd      tbin
        "MAPS": [10.1000, 8.2700, 1.9000, 6.0000, 0.09400, 0.09400, 0.0000, 2, 0.0250, 0.000e-6],
        "MARI": [10.0500, 7.1900, 1.6890, 4.0220, 0.06667, 0.06667, 0.0000, 2, 0.0250, 0.000e-6],
        "MERLIN": [10.0000, 7.1900, 1.8200, 2.5000, 0.06667, 0.06667, 0.0000, 2, 0.0250, 0.000e-6],
    }
    __samplesDimensions = {
        #         sx    sy     sz     isam
        "MAPS": [2.00, 48.00, 48.00, 0],
        "MARI": [20.00, 19.00, 50.00, 2],
        "MERLIN": [2.00, 40.00, 40.00, 0],
    }
    __chopperParameters = {
        #                    pslit  pslat radius rho       tjit
        "MARI": {
            "A": {"par": [0.760, 0.550, 49.0, 1300.0, 0.0, 0.0], "fluxcorr": 3.0, "title": "MARI A (500meV)"},
            "B": {"par": [1.140, 0.550, 49.0, 820.0, 0.0, 0.0], "fluxcorr": 3.0, "title": "MARI B (200meV)"},
            "C": {"par": [1.520, 0.550, 49.0, 580.0, 0.0, 0.0], "fluxcorr": 3.0, "title": "MARI B (100meV)"},
            "G": {"par": [0.380, 0.020, 10.0, 800.0, 0.0, 0.0], "fluxcorr": 3.2, "title": "MARI G (Gadolinium)"},
            "R": {"par": [1.143, 0.550, 49.0, 1300.0, 0.0, 0.0], "fluxcorr": 3.0, "title": "MARI R (500meV)"},
            "S": {"par": [2.280, 0.550, 49.0, 1300.0, 0.0, 0.0], "fluxcorr": 2.7, "title": "MARI S (Sloppy)"},
        },
        "MAPS": {
            "A": {"par": [1.087, 0.534, 49.0, 1300.0, 0.0, 0.0], "fluxcorr": 3.0, "title": "MAPS A (500meV)"},
            "B": {"par": [1.812, 0.534, 49.0, 920.0, 0.0, 0.0], "fluxcorr": 3.0, "title": "MAPS B (200meV)"},
            "S": {"par": [2.899, 0.534, 49.0, 1300.0, 0.0, 0.0], "fluxcorr": 2.5, "title": "MAPS S (Sloppy)"},
        },
        "MERLIN": {
            "S": {"par": [2.28, 0.55, 49.0, 1300.0, 0.0, 0.0], "fluxcorr": 3.0, "title": "HET S (Sloppy)"},
            "G": {"par": [0.2, 0.02, 5.0, 1.0e6, 0.0, 0.0], "fluxcorr": 3.0, "title": "MERLIN G (Gadolinium)"},
        },
    }
    # The following old HET choppers are no longer used, but retained here for completeness.
    #'A':{'par':[0.76, 0.55, 49., 1300., 0., 0.], 'fluxcorr':3., 'title':'HET A (500meV)'},
    #'B':{'par':[1.29, 0.55, 49., 920., 0., 0.], 'fluxcorr':3., 'title':'HET B (200meV)'},
    #'C':{'par':[1.71, 0.55, 49., 580., 0., 0.], 'fluxcorr':3., 'title':'HET C (100meV)'},
    #'D':{'par':[1.52, 0.55, 49., 410., 0., 0.], 'fluxcorr':3., 'title':'HET D (50meV)'},
    __Moderators = {
        "MAPS": {
            "imod": 2,
            "ch_mod": "AP",
            "mod_pars": [38.6, 0.5226],
            "mod_scale_fn": soft_hat,
            "mod_scale_par": [1.0, 0.0, 0.0, 150.0, 0.01, 70.0],
            "theta": 32.0,
        },
        "MARI": {"imod": 2, "ch_mod": "CH4", "mod_pars": [38.6, 0.5226], "mod_scale_fn": None, "theta": 13.0},
        "MERLIN": {"imod": 2, "ch_mod": "AP", "mod_pars": [80.0, 0.5226], "mod_scale_fn": None, "theta": 26.7},
    }

    # For Merlin, set the disk chopper phase to block neutrons with energy greater than 200meV
    # For MAPS and MARI, corresponds to the default slit to let the desired rep through
    __DiskChopperMode = {"MERLIN": 1500, "MAPS": 0, "MARI": 2}

    def __init__(self, instname=None, choppername="", freq=0):
        warnings.warn(
            "The ISISFermi class is deprecated and will be removed in the next Mantid version. "
            "Please use the Instrument class or the official PyChop CLI interface.",
            DeprecationWarning,
        )
        if instname:
            self.setInstrument(instname, choppername, freq)
            self.diskchopper_phase = self.__DiskChopperMode[instname]
        else:
            self.instname = None
            self.diskchopper_phase = None
        self.Ei = None

    def setInstrument(self, instname, choppername="", freq=0):
        """
        Resets the instrument name of this chopper object

        mychop = ISISFermi
        mychop.setInstrument('Merlin', 'G', 600)

        Chopper type and frequency must also be given
        """
        instname = instname.upper()
        if instname not in self.__Instruments.keys():
            raise ValueError("Instrument %s not recognised" % (instname))
        self.instname = instname
        if choppername:
            self.setChopper(choppername, freq)

    def setChopper(self, choppername, freq=50, diskchopper_phase=None, diskchopper_freq=None):
        """
        Resets the chopper type of this chopper object

        mychop = ISISFermi('MAPS', 'S', 400)
        mychop.setChopper('A', 500)

        Chopper frequency must also be given
        """
        choppername = choppername.upper()
        if choppername not in self.__chopperParameters[self.instname].keys():
            raise ValueError("Chopper %s of %s not recognised" % (choppername, self.instname))
        self.choppername = choppername
        self.freq = freq if np.shape(freq) else [freq]
        if self.freq[0] % 50 != 0:
            raise ValueError("Chopper frequency must be a multiple of 50Hz")
        if self.freq[0] <= 0:
            raise ValueError("Chopper frequency must be greater than 0Hz")
        if diskchopper_phase is not None:
            self.diskchopper_phase = diskchopper_phase

    def getChopper(self):
        """
        Returns the chopper package name/type
        """
        return self.choppername

    def setFrequency(self, frequency, **kwargs):
        """
        Resets the chopper frequency name of this chopper object

        mychop = ISISFermi('Mari', 'G', 300)
        mychop.setFrequency(250)
        """
        if "Chopper2Phase" in kwargs.keys():
            diskchopper_phase = kwargs["Chopper2Phase"]
        elif "diskchopper_phase" in kwargs.keys():
            diskchopper_phase = kwargs["diskchopper_phase"]
        else:
            diskchopper_phase = None
        self.setChopper(self.choppername, frequency, diskchopper_phase)

    def getFrequency(self):
        """
        Returns the Fermi chopper frequency in Hz.
        """
        return self.freq

    def setEi(self, Ei):
        """
        (Re)sets the desired incident energy.

        mychop = ISISFermi('Mari', 'G', 300)
        mychop.setEi(25.)
        """
        self.Ei = Ei

    def getEi(self):
        """
        Returns the currently set incident energy in meV
        """
        return self.Ei

    def getChopWidth(self, Ei_in=None):
        """
        Calculates the chopper time width for a given neutron energy (Ei)

        mari_S = ISISFermi('Mari', 'S')
        tsqch = mari_S.getChopWidth(Ei)

        Inputs:
            ei - incident energy in meV [optional, if omitted use preset Ei in current object]

        Output:
            tsqch - the time-of-flight width due to the chopper opening in microseconds
        """
        Ei = self.Ei if Ei_in is None else Ei_in
        if not Ei:
            raise ValueError("Incident energy has not be specified")
        chop_par = self.__chopperParameters[self.instname][self.choppername]["par"]
        pslit = chop_par[0] / 1000.00
        radius = chop_par[2] / 1000.00
        rho = chop_par[3] / 1000.00
        return Chop.tchop(self.freq[0], Ei, pslit, radius, rho)

    def getModWidth(self, Ei_in=None):
        """
        Calculates the moderator time width for a given neutron energy (Ei)

        marmod = ISISFermi('Mari')
        tsqmod = marmod.getModWidth(Ei)

        Inputs:
            ei - incident energy in meV [optional, if omitted use preset Ei in current object]

        Output:
            tsqmod - the time-of-flight width due to the pulse/moderator in microseconds
        """
        Ei = self.Ei if Ei_in is None else Ei_in
        if not Ei:
            raise ValueError("Incident energy has not been specified")
        if self.__Moderators[self.instname]["imod"] == 0:
            tsqmod = Chop.tchi(self.__Moderators[self.instname]["mod_pars"] / 1000.0, Ei)
        elif self.__Moderators[self.instname]["imod"] == 1:
            tsqmod = Chop.tikeda(tuple(self.__Moderators[self.instname]["mod_pars"]), Ei)
        elif self.__Moderators[self.instname]["imod"] == 2:
            if self.__Moderators[self.instname]["mod_scale_fn"]:
                d0 = self.__Moderators[self.instname]["mod_scale_fn"](Ei, self.__Moderators[self.instname]["mod_scale_par"])
                d0 *= self.__Moderators[self.instname]["mod_pars"][0]
            else:
                d0 = self.__Moderators[self.instname]["mod_pars"][0]
            tsqmod = Chop.tchi_2(d0 / 1000.0, self.__Moderators[self.instname]["mod_pars"][1] / 1000.0, Ei)
        else:
            raise RuntimeError("PyChop: Bug in instrument-moderator database. Please file a bug report")
        return tsqmod

    def getVanVar(self, Ei_in=None, frequency=None, Etrans=0):
        """
        Calculates vanadium time-of-flight widths

        v_van, tsqmod, tsqch = getVanVar()
        v_van, tsqmod, tsqch = getVanVar(ei)
        v_van, tsqmod, tsqch = getVanVar(ei, omega, etrans)

        Inputs:
            ei - incident energy in meV [default: preset incident energy]
            omega - chopper frequency in Hz  [default: preset frequency]
            etrans - list of numpy array of energy transfers to calculate for (meV) [default: 0]

        Output:
            v_van - the squared incoherent (Vanadium) time-of-flight width at etrans in s**2
            tmod - the time-of-flight width due to the pulse/moderator in s
            tchp - the time-of-flight width due to the chopper opening in s
        """
        Ei = self.Ei if Ei_in is None else Ei_in
        if not Ei:
            raise ValueError("Incident energy has not been specified")
        if frequency:
            oldfreq = self.freq
            self.setFrequency(frequency)
        tsqmod = self.getModWidth(Ei)
        tsqchp = self.getChopWidth(Ei)
        omega = self.freq[0] * 2 * np.pi
        tsqjit = (self.__chopperParameters[self.instname][self.choppername]["par"][5] * 1.0e-6) ** 2
        # Sample parameters
        sam_dims = np.array(self.__samplesDimensions[self.instname])
        sam_dims[:2] /= 1000
        v_x, v_y, v_z = Chop.sam0(*list(sam_dims))
        v_xy = 0.00
        # Detector parameters
        phi = 0.0
        v_van = []
        if not np.shape(Etrans):
            Etrans = [Etrans]
        for en in Etrans:
            omega_f = 0.69468875 * np.sqrt(Ei - en)
            deld, sigd, sigdz, sigdd, effic = Chop.detect2(1.00, 1.00, omega_f, *self.__Instruments[self.instname][7:9])
            v_dd = sigdd**2
            v_van.append(self.__van_calc(tsqmod, tsqchp, tsqjit, v_x, v_y, v_xy, v_dd, Ei, en, phi, omega))
        if frequency:
            self.setFrequency(oldfreq)
        # Put in pre-factors for tsqmod and tsqchop to allow comparison to CHOP, by giving time width at the detectors
        # Only consider elastic case, (though inelastic is calculable by setting r = (vi/vf)^3 = (Ei/Ef)^1.5
        r = 1
        x0 = self.__Instruments[self.instname][0]
        x1 = self.__Instruments[self.instname][2]
        x2 = self.__Instruments[self.instname][3]
        tmod = np.sqrt((((x1 + (r * x2)) / x0) ** 2) * tsqmod)
        tchp = np.sqrt((1 + (x1 + (r * x2)) / x0) ** 2 * tsqchp)
        return v_van, tmod, tchp

    def getWidths(self, Ei_in=None, frequency=None):
        """
        Calculates the time widths contributing to the calculated energy resolution
        """
        Ei = self.Ei if Ei_in is None else Ei_in
        if not Ei:
            raise ValueError("Incident energy has not been specified")
        v_van, tmod, tchp = self.getVanVar(Ei, frequency, 0.0)
        x2 = self.__Instruments[self.instname][3]
        tbin = self.__Instruments[self.instname][9]
        res_el = np.real(2.35482 * 8.747832e-4 * np.sqrt(Ei**3) * (np.sqrt(v_van[0] + (tbin**2 / 12.00)) * 1.0e6) / x2)
        return {"Moderator": tmod * 2.35482e6, "Chopper": tchp * 2.35482e6, "Energy": res_el}

    def __van_calc(self, v_mod, v_ch, v_jit, v_x, v_y, v_xy, v_dd, Ei, eps, phi, omega):
        """
        Calculates the 'vanadium' time widths due to the moderator and Fermi chopper
        """
        x0, xa, x1, x2, wa, ha, gam_deg = tuple(self.__Instruments[self.instname][:7])
        thetam = self.__Moderators[self.instname]["theta"] * (np.pi / 180.00)
        gam = gam_deg * (np.pi / 180.00)
        veli = 437.39160 * np.sqrt(Ei)
        velf = 437.39160 * np.sqrt(Ei - np.array(eps))
        rat = (veli / velf) ** 3
        tanthm = np.tan(thetam)
        am = -(x1 + rat * x2) / x0
        ach = 1.00 + (x1 + rat * x2) / x0
        # g1 = (1.00 - (omega*(x0+x1)*tanthm/veli)) # wrong (in original CHOP!) - should be (xa+x1), not (x0+x1)
        g1 = 1.00 - (omega * (xa + x1) * tanthm / veli)
        g2 = 1.00 - (omega * (x0 - xa) * tanthm / veli)
        f1 = 1.00 + ((x1 / x0) * g1)
        f2 = 1.00 + ((x1 / x0) * g2)
        gg1 = g1 / (omega * (xa + x1))
        gg2 = g2 / (omega * (xa + x1))
        ff1 = f1 / (omega * (xa + x1))
        ff2 = f2 / (omega * (xa + x1))
        bb = ((-np.sin(gam) / veli) + (np.sin(gam - phi) / velf)) - (ff2 * np.cos(gam))
        aya = ff1 + ((rat * x2 / x0) * gg1)
        ay = bb - ((rat * x2 / x0) * gg2 * np.cos(gam))
        a_dd = 1.00 / velf
        v_van_m = am**2 * v_mod
        v_van_ch = ach**2 * v_ch
        v_van_jit = ach**2 * v_jit
        v_van_ya = aya**2 * (wa**2 / 12.00)
        v_van_y = ay**2 * v_y
        v_van_dd = a_dd**2 * v_dd
        # Old version:
        # v_van = (v_van_m + v_van_ch + v_van_jit + v_van_ya)
        # New (RAE) version:
        v_van = v_van_m + v_van_ch + v_van_jit + v_van_ya + v_van_y + v_van_dd
        return v_van

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
            van - the incoherent (Vanadium) energy width at etrans in meV
        """
        Ei = self.Ei if Ei_in is None else Ei_in
        if not Ei:
            raise ValueError("Incident energy has not been specified")
        if frequency:
            oldfreq = self.freq
            self.setFrequency(frequency)
        # If not set, sets energy transfers to values to compare exactly to RAE's original implementation.
        if Etrans is None:
            Etrans = Ei - np.linspace(0.05 * Ei, 0.95 * Ei + 0.05 * 0.05 * Ei, 19, endpoint=True)
        convert = 2.3548200
        x2 = self.__Instruments[self.instname][3]
        tbin = self.__Instruments[self.instname][9]
        v_van, tsqmod, tsqchop = self.getVanVar(Ei, frequency, Etrans)
        if not np.shape(Etrans):
            Etrans = [Etrans]
        van = []
        for ie, en in enumerate(Etrans):
            van.append(np.real(convert * 8.747832e-4 * np.sqrt((Ei - en) ** 3) * (np.sqrt(v_van[ie] + (tbin**2 / 12.00)) * 1.0e6) / x2))
        if frequency:
            self.setFrequency(oldfreq)
        return np.array(van)

    def getFlux(self, Ei_in=None, frequency=None):
        """
        Calculates the flux at the sample position in  n / cm**2 . uA.s

        flux = getFlux()
        flux = getFlux(ei)
        flux = getFlux(ei, omega)

        Inputs:
            ei - incident energy in meV [default: preset energy]
            omega - chopper frequency in Hz  [default: preset frequency]

        Output:
            flux - the flux in n / cm**2 . uA.s
        """
        Ei = self.Ei if Ei_in is None else Ei_in
        if Ei is None:
            raise ValueError("Incident energy has not been specified")
        if frequency:
            oldfreq = self.freq
            self.setFrequency(frequency)
        moderator_flux = self.getMeasuredFlux(Ei)
        chop_par = self.__chopperParameters[self.instname][self.choppername]["par"]
        pslit = chop_par[0] / 1000.00
        dslat = (chop_par[0] + chop_par[1]) / 1000.00
        radius = chop_par[2] / 1000.00
        rho = chop_par[3] / 1000.00
        chopper_transmission = Chop.achop(Ei, self.freq[0], dslat, pslit, radius, rho)
        x0 = self.__Instruments[self.instname][0]
        x1 = self.__Instruments[self.instname][2]
        flux = 84403.060 * moderator_flux * (chopper_transmission / dslat) / (x0 * (x1 + x0))
        flux /= self.__chopperParameters[self.instname][self.choppername]["fluxcorr"]
        if frequency:
            self.setFrequency(oldfreq)
        return flux

    def getResFlux(self, Etrans=None, Ei_in=None, frequency=None):
        """
        Returns the resolution and flux as a tuple.
        """
        return self.getResolution(Etrans, Ei_in, frequency), self.getFlux(Ei_in, frequency)

    def getResFluxRAE(self, Etrans=None, Ei_in=None, frequency=None):
        """
        Returns the resolution and flux in the same format as RAE's PyChop.
        """
        # Outputs in the same format as RAE's code
        van_el = self.getResolution(0.0, Ei_in, frequency)
        van = self.getResolution(Etrans, Ei_in, frequency)
        flux = self.getFlux(Ei_in, frequency)
        van_vars = self.getVanVar(Ei_in)
        return van_el, van, flux, van_vars[1], van_vars[2], van_vars[0][0]

    def getAnalyticFlux(self, Ei):
        """
        Returns the calculated flux using TGP's model of the moderators.
        """
        ch_mod = self.__Moderators[self.instname]["ch_mod"]
        theta_m = self.__Moderators[self.instname]["theta"] * np.pi / 180.0
        return Chop.flux_calc(Ei, ch_mod, theta_m)

    # ------------------------------------------------------------------------------------------------- #
    # Flux lookup tables.
    # ------------------------------------------------------------------------------------------------- #
    def getMeasuredFlux(self, Ei_in=None):
        """
        # Looks up the flux at the sample position for ISIS Fermi chopper spectrometers from tables of
        # absolute flux measurements
        """
        Ei = self.Ei if Ei_in is None else Ei_in
        if Ei is None:
            raise ValueError("Incident energy has not be specified")
        # Store the info inside this file, for better portability
        lam, flux_meas = self.flux_store()
        # Interpolate for wavelength
        f = interp1d(lam, flux_meas, kind="cubic")
        lamb = np.sqrt(81.81 / Ei)
        return f(lamb)

    def flux_store(self):
        """
        # Return stored flux from calibrated flux measurement. Avoids needing to keep in file with PyChop distribution
        """
        lam = [
            1.5000e-01,
            2.5000e-01,
            3.5000e-01,
            4.5000e-01,
            5.5000e-01,
            6.5000e-01,
            7.5000e-01,
            8.5000e-01,
            9.5000e-01,
            1.0500e00,
            1.1500e00,
            1.2500e00,
            1.3500e00,
            1.4500e00,
            1.5500e00,
            1.6500e00,
            1.7500e00,
            1.8500e00,
            1.9500e00,
            2.0500e00,
            2.1500e00,
            2.2500e00,
            2.3500e00,
            2.4500e00,
            2.5500e00,
            2.6500e00,
            2.7500e00,
            2.8500e00,
            2.9500e00,
            3.0500e00,
            3.1500e00,
            3.2500e00,
            3.3500e00,
            3.4500e00,
            3.5500e00,
            3.6500e00,
            3.7500e00,
            3.8500e00,
            3.9500e00,
            4.0500e00,
            4.1500e00,
            4.2500e00,
            4.3500e00,
            4.4500e00,
            4.5500e00,
            4.6500e00,
            4.7500e00,
            4.8500e00,
            4.9500e00,
            5.0500e00,
            5.1500e00,
            5.2500e00,
            5.3500e00,
            5.4500e00,
            5.5500e00,
            5.6500e00,
            5.7500e00,
            5.8500e00,
            5.9500e00,
            6.0500e00,
            6.1500e00,
            6.2500e00,
            6.3500e00,
            6.4500e00,
            6.5500e00,
            6.6500e00,
            6.7500e00,
            6.8500e00,
            6.9500e00,
            7.0500e00,
            7.1500e00,
            7.2500e00,
            7.3500e00,
            7.4500e00,
            7.5500e00,
            7.6500e00,
            7.7500e00,
            7.8500e00,
            7.9500e00,
            8.0500e00,
            8.1500e00,
            8.2500e00,
            8.3500e00,
            8.4500e00,
            8.5500e00,
            8.6500e00,
            8.7500e00,
            8.8500e00,
            8.9500e00,
            9.0500e00,
            9.1500e00,
            9.2500e00,
            9.3500e00,
            9.4500e00,
            9.5500e00,
            9.6500e00,
            9.7500e00,
            9.8500e00,
            9.9500e00,
            1.0050e01,
        ]
        lamb = {
            "MAPS": [
                0.0181,
                0.0511,
                0.0841,
                0.1170,
                0.1500,
                0.1829,
                0.2159,
                0.2489,
                0.2818,
                0.3148,
                0.3477,
                0.3807,
                0.4137,
                0.4466,
                0.4796,
                0.5126,
                0.5455,
                0.5785,
                0.6114,
                0.6444,
                0.6774,
                0.7103,
                0.7433,
                0.7762,
                0.8092,
                0.8422,
                0.8751,
                0.9081,
                0.9411,
                0.9740,
                1.0070,
                1.0399,
                1.0729,
                1.1059,
                1.1388,
                1.1718,
                1.2047,
                1.2377,
                1.2707,
                1.3036,
                1.3366,
                1.3696,
                1.4025,
                1.4355,
                1.4684,
                1.5014,
                1.5344,
                1.5673,
                1.6003,
                1.6332,
                1.6662,
                1.6992,
                1.7321,
                1.7651,
                1.7980,
                1.8310,
                1.8640,
                1.8969,
                1.9299,
                1.9629,
                1.9958,
                2.0288,
                2.0617,
                2.0947,
                2.1277,
                2.1606,
                2.1936,
                2.2266,
                2.2595,
                2.2925,
                2.3254,
                2.3584,
                2.3914,
                2.4243,
                2.4573,
                2.4902,
                2.5232,
                2.5562,
                2.5891,
                2.6221,
                2.6551,
                2.6880,
                2.7210,
                2.7539,
                2.7869,
                2.8199,
                2.8528,
                2.8858,
                2.9187,
                2.9517,
                2.9847,
                3.0176,
                3.0506,
                3.0835,
                3.1165,
                3.1495,
                3.1824,
                3.2154,
                3.2484,
                3.2813,
                3.3143,
                3.3472,
                3.3802,
                3.4132,
                3.4461,
                3.4791,
                3.5120,
                3.5450,
                3.5780,
                3.6109,
                3.6439,
                3.6769,
                3.7098,
                3.7428,
                3.7757,
                3.8087,
                3.8417,
                3.8746,
                3.9076,
                3.9406,
                3.9735,
                4.0065,
                4.0394,
                4.0724,
                4.1054,
                4.1383,
                4.1713,
                4.2042,
                4.2372,
                4.2702,
                4.3031,
                4.3361,
                4.3690,
                4.4020,
                4.4350,
                4.4679,
                4.5009,
                4.5339,
                4.5668,
                4.5998,
                4.6327,
                4.6657,
                4.6987,
                4.7316,
                4.7646,
                4.7976,
                4.8305,
                4.8635,
                4.8964,
                4.9294,
                4.9624,
                4.9953,
                5.0283,
                5.0612,
                5.0942,
                5.1272,
                5.1601,
                5.1931,
                5.2260,
                5.2590,
                5.2920,
                5.3249,
                5.3579,
                5.3909,
                5.4238,
                5.4568,
                5.4897,
                5.5227,
                5.5557,
                5.5886,
                5.6216,
                5.6546,
                5.6875,
                5.7205,
                5.7534,
                5.7864,
                5.8194,
                5.8523,
                5.8853,
                5.9182,
                5.9512,
                5.9842,
                6.0171,
                6.0501,
                6.0831,
                6.1160,
                6.1490,
                6.1819,
                6.2149,
                6.2479,
                6.2808,
                6.3138,
                6.3467,
                6.3797,
                6.4127,
                6.4456,
                6.4786,
                6.5115,
                6.5445,
                6.5750,
            ],
            "MARI": lam,
            "MERLIN": lam,
        }
        flux = {
            "MAPS": [
                2.60775e08,
                8.08616e07,
                4.59455e07,
                3.16183e07,
                2.37830e07,
                1.91458e07,
                1.58204e07,
                1.36088e07,
                1.19225e07,
                1.06105e07,
                9.55533e06,
                8.77411e06,
                8.21901e06,
                7.67656e06,
                7.15055e06,
                6.70757e06,
                6.39276e06,
                6.22826e06,
                6.19798e06,
                6.27731e06,
                6.47994e06,
                6.91604e06,
                7.50573e06,
                8.09035e06,
                8.76875e06,
                9.49975e06,
                1.01658e07,
                1.07548e07,
                1.13597e07,
                1.19941e07,
                1.25374e07,
                1.28821e07,
                1.31248e07,
                1.32727e07,
                1.32305e07,
                1.30834e07,
                1.27595e07,
                1.24351e07,
                1.20115e07,
                1.15789e07,
                1.11218e07,
                1.07191e07,
                1.03272e07,
                9.93856e06,
                9.59376e06,
                9.19836e06,
                8.81571e06,
                8.50457e06,
                8.29274e06,
                8.05058e06,
                7.88437e06,
                7.63907e06,
                7.32047e06,
                6.99681e06,
                6.68968e06,
                6.45270e06,
                6.24161e06,
                6.01323e06,
                5.74713e06,
                5.48816e06,
                5.27153e06,
                5.01780e06,
                4.76127e06,
                4.48172e06,
                4.21345e06,
                3.97093e06,
                3.74819e06,
                3.53683e06,
                3.32935e06,
                3.12404e06,
                2.92801e06,
                2.74479e06,
                2.61634e06,
                2.48606e06,
                2.38826e06,
                2.29410e06,
                2.17636e06,
                2.07461e06,
                1.97063e06,
                1.87220e06,
                1.77780e06,
                1.70202e06,
                1.62584e06,
                1.55763e06,
                1.48989e06,
                1.42924e06,
                1.37959e06,
                1.34056e06,
                1.31926e06,
                1.28573e06,
                1.25559e06,
                1.22426e06,
                1.18988e06,
                1.15714e06,
                1.13032e06,
                1.09423e06,
                1.06161e06,
                1.02650e06,
                9.95519e05,
                9.56437e05,
                9.24815e05,
                8.90446e05,
                8.56656e05,
                8.28196e05,
                8.01094e05,
                7.79358e05,
                7.56306e05,
                7.35949e05,
                7.24375e05,
                7.02174e05,
                6.86458e05,
                6.65894e05,
                6.43176e05,
                6.24539e05,
                6.01304e05,
                5.82505e05,
                5.61653e05,
                5.41996e05,
                5.25903e05,
                5.10613e05,
                4.96677e05,
                4.82118e05,
                4.64661e05,
                4.65809e05,
                4.68617e05,
                4.56137e05,
                4.42141e05,
                4.27460e05,
                4.10041e05,
                3.98628e05,
                3.84161e05,
                3.71166e05,
                3.57501e05,
                3.45980e05,
                3.35925e05,
                3.23733e05,
                3.13815e05,
                3.03413e05,
                2.91757e05,
                2.82348e05,
                2.72917e05,
                2.57271e05,
                2.41863e05,
                2.58619e05,
                2.53316e05,
                2.43464e05,
                2.35779e05,
                2.29787e05,
                2.22481e05,
                2.14144e05,
                2.08181e05,
                2.01866e05,
                1.95864e05,
                1.89808e05,
                1.84740e05,
                1.78016e05,
                1.73397e05,
                1.68777e05,
                1.65549e05,
                1.61071e05,
                1.56594e05,
                1.52364e05,
                1.49546e05,
                1.46162e05,
                1.43155e05,
                1.40167e05,
                1.37583e05,
                1.35294e05,
                1.32192e05,
                1.30639e05,
                1.27633e05,
                1.25179e05,
                1.23187e05,
                1.21203e05,
                1.18074e05,
                1.15095e05,
                1.12187e05,
                1.10561e05,
                1.08411e05,
                1.05109e05,
                1.03695e05,
                1.01165e05,
                9.87797e04,
                9.77841e04,
                9.40768e04,
                9.27353e04,
                9.14937e04,
                8.88289e04,
                8.74353e04,
                8.53251e04,
                8.30339e04,
                8.22249e04,
                7.94099e04,
                7.79037e04,
                7.62865e04,
                7.47047e04,
                7.38535e04,
                7.17228e04,
                6.98927e04,
                6.89509e04,
            ],
            "MARI": [
                4.2938e06,
                5.7061e06,
                5.7984e06,
                4.9964e06,
                3.8796e06,
                3.1474e06,
                2.4375e06,
                1.9361e06,
                1.6104e06,
                1.4590e06,
                1.3767e06,
                1.3706e06,
                1.4005e06,
                1.4263e06,
                1.4117e06,
                1.3879e06,
                1.3323e06,
                1.2699e06,
                1.1757e06,
                1.0680e06,
                9.5587e05,
                8.5549e05,
                7.4173e05,
                6.9001e05,
                6.2889e05,
                5.4829e05,
                4.8420e05,
                4.2834e05,
                3.8604e05,
                3.4006e05,
                2.9744e05,
                2.6364e05,
                2.2980e05,
                2.0105e05,
                1.7640e05,
                1.5583e05,
                1.3732e05,
                1.2105e05,
                1.0599e05,
                9.6490e04,
                8.8906e04,
                7.9308e04,
                7.1857e04,
                6.5562e04,
                5.9023e04,
                5.0767e04,
                5.0665e04,
                4.5641e04,
                4.1721e04,
                3.6969e04,
                3.3218e04,
                3.0231e04,
                2.7621e04,
                2.5398e04,
                2.1588e04,
                1.2268e04,
                3.5855e03,
                2.0985e02,
                3.1064e02,
                4.0448e02,
                4.8934e02,
                4.8558e02,
                1.1776e03,
                2.9689e01,
                0.0000e00,
                0.0000e00,
                0.0000e00,
                0.0000e00,
                0.0000e00,
                0.0000e00,
                0.0000e00,
                0.0000e00,
                0.0000e00,
                0.0000e00,
                0.0000e00,
                0.0000e00,
                0.0000e00,
                0.0000e00,
                0.0000e00,
                0.0000e00,
                0.0000e00,
                0.0000e00,
                0.0000e00,
                0.0000e00,
                0.0000e00,
                0.0000e00,
                0.0000e00,
                0.0000e00,
                0.0000e00,
                0.0000e00,
                0.0000e00,
                0.0000e00,
                0.0000e00,
                0.0000e00,
                0.0000e00,
                0.0000e00,
                0.0000e00,
                0.0000e00,
                0.0000e00,
                0.0000e00,
            ],
            "MERLIN": [
                3.4827e07,
                1.9455e07,
                1.3618e07,
                1.0922e07,
                9.1074e06,
                8.9949e06,
                1.0717e07,
                1.4113e07,
                1.7890e07,
                2.0977e07,
                2.2164e07,
                2.1097e07,
                1.9449e07,
                1.7522e07,
                1.5395e07,
                1.3518e07,
                1.1794e07,
                1.0664e07,
                9.4134e06,
                7.8688e06,
                6.7881e06,
                5.9428e06,
                4.9238e06,
                4.2219e06,
                3.8432e06,
                3.3606e06,
                2.9299e06,
                2.6110e06,
                2.3335e06,
                2.0563e06,
                1.8005e06,
                1.6002e06,
                1.4490e06,
                1.3368e06,
                1.2599e06,
                1.1239e06,
                1.0524e06,
                9.7266e05,
                8.6218e05,
                7.6681e05,
                7.0049e05,
                6.7822e05,
                6.2993e05,
                6.2571e05,
                5.8608e05,
                5.3686e05,
                5.5309e05,
                5.2245e05,
                4.8365e05,
                4.4384e05,
                3.9445e05,
                3.7199e05,
                3.2812e05,
                3.1162e05,
                2.9301e05,
                2.6704e05,
                2.6396e05,
                2.4635e05,
                2.1940e05,
                2.1146e05,
                2.0127e05,
                1.9412e05,
                1.7908e05,
                1.7902e05,
                1.6332e05,
                1.1746e05,
                0.0000e00,
                0.0000e00,
                0.0000e00,
                0.0000e00,
                0.0000e00,
                0.0000e00,
                0.0000e00,
                0.0000e00,
                0.0000e00,
                0.0000e00,
                0.0000e00,
                0.0000e00,
                0.0000e00,
                0.0000e00,
                0.0000e00,
                0.0000e00,
                0.0000e00,
                0.0000e00,
                0.0000e00,
                0.0000e00,
                0.0000e00,
                0.0000e00,
                0.0000e00,
                0.0000e00,
                0.0000e00,
                0.0000e00,
                0.0000e00,
                0.0000e00,
                0.0000e00,
                0.0000e00,
                0.0000e00,
                0.0000e00,
                0.0000e00,
                0.0000e00,
            ],
        }
        return lamb[self.instname], flux[self.instname]
