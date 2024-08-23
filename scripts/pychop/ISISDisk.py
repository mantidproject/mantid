# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=line-too-long, invalid-name, too-many-locals, too-many-branches, unused-variable
# pylint: disable=attribute-defined-outside-init, old-style-class, too-many-instance-attributes

"""
Contains the ISISDisk class which calculates resolution and flux for ISIS Disk chopper
spectrometer (LET) - using the functions in MulpyRep and additional tables of instrument parameters
"""

import warnings
import numpy as np
from . import MulpyRep
from .ISISFermi import ISISFermi


class ISISDisk:
    """
    Calculates the resolution and flux for the ISIS disk chopper spectrometer LET
    """

    def __init__(self, instname=None, variant=None, freq=None):
        warnings.warn(
            "The ISISDisk class is deprecated and will be removed in the next Mantid version. "
            "Please use the Instrument class or the official PyChop CLI interface.",
            DeprecationWarning,
        )
        if instname:
            self.setInstrument(instname, variant)
            self.freq = 0
            if freq is not None:
                self.setFrequency(freq)
        else:
            self.instname = None
            self.variant = None
        self.Ei = None
        self.slot_ang_pos = []

    def setInstrument(self, instname, variant=None):
        """
        Sets instrument parameters from name
        """
        instname = instname.upper()
        if "LET" in instname:
            self.instname = "LET"
            # Sets parameters which are the same for all configurations
            self.dist = [7.83, 8.4, 11.75, 15.66, 23.5]  # distance to each chopper in m (same for all conf)
            self.nslot = [6, 1, 2, 6, 2]  # number of slots in each chopper. Assumed equally spaced
            self.guide_width = [40, 40, 40, 40, 20]  # width of the guide in mm
            self.radius = [290, 545, 290, 290, 290]  # radius in mm of each disk at centre of window
            self.numDisk = [2, 1, 1, 1, 2]  # whether double or single disks
            # possible instname: ['LET', 'LETHIFLUX', 'LETINTERMED', 'LETHIRES'] - corresponds to different configurations
            if "FLUX" in instname or (variant is not None and "FLUX" in variant.upper()):
                self.slot_width = [40, 890, 56, 52, 31]  # width of chopper slots in mm
                self.variant = "High flux"
            elif "RES" in instname or (variant is not None and "RES" in variant.upper()):
                self.slot_width = [40, 890, 56, 52, 15]  # width of chopper slots in mm
                self.variant = "High resolution"
            else:
                self.slot_width = [40, 890, 56, 52, 20]  # width of chopper slots in mm
                self.variant = "Intermediate"
            self.ph_ind = 1  # index of chopper with user-determined phase
            self.samp_det = 3.5  # sample to detector distance in m
            self.chop_samp = 1.5  # final chopper to sample distance
            self.source_rep = 10  # rep rate of source
            self.tmod = 3500  # maximum emission window from moderator in us
            self.frac_ei = 0.90  # fraction of Ei to plot energy loss lines
            self.Chop2Phase = 5  # Phase delay time in usec for chopper 2 (T0/frame overlap chopper)
        elif "MERLIN" in instname:
            self.dist = [9.3, 10.1]
            self.nslot = [10, 2]
            self.slot_width = [68, 10]
            self.guide_width = [64, 10]
            self.slot_ang_pos = [[-45.5, -32.5, -19.5, -6.5, 6.5, 19.5, 32.5, 45.5, 50, 150], [0, 180]]
            self.radius = [300, 290]
            self.numDisk = [1, 1]
            self.samp_det = 2.5
            self.chop_samp = 1.82
            self.source_rep = 50
            self.tmod = 50
            self.frac_ei = 0.90
            self.instname = "MERLIN"
            self.variant = "G"
            self.ph_ind = 0
            self.Chop2Phase = 1500  # Phased to not let neutrons with Ei>200meV through
        elif "MARI" in instname:
            self.dist = [7.85, 10.1]
            self.nslot = [4, 2]
            self.slot_ang_pos = [[0, 36.38, 72.76, 145.52], [0, 180]]
            self.slot_width = [65, 10]
            self.guide_width = [60, 10]
            self.radius = [367, 290]
            self.numDisk = [2, 1]
            self.samp_det = 4.0
            self.chop_samp = 1.694
            self.source_rep = 50
            self.tmod = 50
            self.frac_ei = 0.90
            self.instname = "MARI"
            self.variant = "G"
            self.ph_ind = "0"
            self.Chop2Phase = 2  # Mode (0,1,2,4 which slot for first rep)
        elif "MAPS" in instname:
            self.dist = [8.831, 10.143]
            self.nslot = [4, 1]
            self.slot_ang_pos = [[-180, -39.1, 0.0, 39.1], [0]]
            self.slot_width = [68, 10]
            self.guide_width = [50, 10]
            self.radius = [375, 290]
            self.numDisk = [1, 1]
            self.samp_det = 6.0
            self.chop_samp = 1.612
            self.source_rep = 50
            self.tmod = 50
            self.frac_ei = 0.90
            self.instname = "MAPS"
            self.variant = "S"
            self.ph_ind = "0"
            self.Chop2Phase = 1  # Mode (0==non reprate, 1,2,3==which slot for first rep)
        else:
            raise ValueError("Instrument %s not recognised." % (instname))

    def setChopper(self, variant):
        """
        Sets instrument variant from name
        """
        self.setInstrument(self.instname, variant)

    def getChopper(self):
        """
        Returns the instrument variant name
        """
        return self.variant

    def _LETfreq(self, frequency):
        if "FLUX" in self.variant.upper():
            if hasattr(frequency, "__len__"):
                if len(frequency) == 1:
                    self.freq = [frequency[0] / 4.0, 10.0, frequency[0] / 2.0, frequency[0] / 2.0, frequency[0]]
                elif len(frequency) == 2:
                    self.freq = [frequency[1] / 2.0, 10.0, frequency[1], frequency[0] / 2.0, frequency[0]]
                elif len(frequency) == 5:
                    self.freq = frequency
                else:
                    raise ValueError("Frequency must be a 1-, 2- or 5-element list/array")
            else:
                self.freq = [frequency / 4.0, 10.0, frequency / 4.0, frequency / 2.0, frequency]
        elif "RES" in self.variant.upper():
            if hasattr(frequency, "__len__"):
                if len(frequency) == 1:
                    self.freq = [frequency[0] / 2.0, 10.0, frequency[0] / 2.0, frequency[0] / 2.0, frequency[0]]
                elif len(frequency) == 2:
                    self.freq = [frequency[0] / 2.0, 10.0, frequency[1], frequency[1], frequency[0]]
                elif len(frequency) == 5:
                    self.freq = frequency
                else:
                    raise ValueError("Frequency must be a 1-, 2- or 5-element list/array")
            else:
                self.freq = [frequency / 2.0, 10.0, frequency / 2.0, frequency / 2.0, frequency]
        else:
            if hasattr(frequency, "__len__"):
                if len(frequency) == 1:
                    self.freq = [frequency[0] / 4.0, 10.0, frequency[0] / 2.0, frequency[0] / 2.0, frequency[0]]
                elif len(frequency) == 2:
                    self.freq = [frequency[1] / 2.0, 10.0, frequency[1], frequency[0] / 2.0, frequency[0]]
                elif len(frequency) == 5:
                    self.freq = frequency
                else:
                    raise ValueError("Frequency must be a 1-, 2- or 5-element list/array")
            else:
                self.freq = [frequency / 4.0, 10.0, frequency / 2.0, frequency / 2.0, frequency]

    def setFrequency(self, frequency, **kwargs):
        """
        Sets the chopper frequencies, in Hz.
        If scalar, sets the resolution chopper freq to this and the pulse remover to freq/2
        """
        if "LET" in self.instname:
            self._LET_freq(frequency)
            if "Chopper2Phase" in kwargs.keys():
                self.Chop2Phase = kwargs["Chopper2Phase"]
        elif "MERLIN" in self.instname:
            if hasattr(frequency, "__len__"):
                self.freq = [50.0, frequency[0]]
            else:
                self.freq = [50.0, frequency]
            if "Chopper2Phase" in kwargs.keys():
                self.Chop2Phase = kwargs["Chopper2Phase"]
        elif "MARI" in self.instname or "MAPS" in self.instname:
            if hasattr(frequency, "__len__"):
                self.freq = [50.0, frequency[0]]
            else:
                self.freq = [50.0, frequency]
            if "Chopper2Phase" in kwargs.keys():
                self.Chop2Phase = kwargs["Chopper2Phase"]
            if "MAPS" in self.instname and hasattr(frequency, "__len__") and len(frequency) > 1:
                self.freq = [frequency[1], frequency[0]]
        else:
            raise RuntimeError("Instrument name has not been set")

    def getFrequency(self):
        """
        Returns a list of the chopper frequencies in Hz.
        """
        return self.freq

    def setEi(self, Ei):
        """
        Sets the focused incident energy in meV
        """
        if Ei > 0:
            self.Ei = Ei
        else:
            raise ValueError("Incident energy must be greater than zero")

    def getEi(self):
        """
        Returns the focussed incident energy in meV
        """
        return self.Ei

    def __LETgetResolution(self, single_mode, Etrans=None, Ei_in=None):
        """
        Private method to calculate the resolution at given Ei, Etrans from chopper opening times.
        """
        Ei = self.Ei if Ei_in is None else Ei_in
        instpars = [
            self.dist,
            self.nslot,
            self.slot_ang_pos,
            self.slot_width,
            self.guide_width,
            self.radius,
            self.numDisk,
            self.samp_det,
            self.chop_samp,
            self.source_rep,
            self.tmod,
            self.frac_ei,
            self.ph_ind,
        ]
        Eis, _, chop_times, lastChopDist, lines = MulpyRep.calcChopTimes(Ei, self.freq, instpars, self.Chop2Phase)
        Eis, _ = self._removeLowIntensityReps(Eis, lines, Ei)
        res_el, percent, chop_width, mod_width = MulpyRep.calcRes(Eis, chop_times, lastChopDist, self.chop_samp, self.samp_det)
        if single_mode:
            # ie_list = [ii for ii,ee in enumerate(Eis) if np.abs((ee-Ei)/Ei)<0.05]
            ie_list = np.where(np.abs(np.array(Eis) - Ei) == np.min(np.abs(np.array(Eis) - Ei)))[0]
            Et = np.linspace(0.05, 0.95 * Ei, 19, endpoint=True) if (Etrans is None) else Etrans
        else:
            ie_list = range(len(Eis))
            # This is the relative energy transfer
            Et = np.linspace(0.05, 0.95, 19, endpoint=True) if (Etrans is None) else Etrans
        res_list = []
        if not np.shape(Et):
            Et = [Et]
        for ie in ie_list:
            t_mod_chop = 252.82 * lastChopDist * np.sqrt(81.81 / Eis[ie])
            res = []
            energy_transfer = Et if single_mode else np.array(Et) * Eis[ie]
            for en in energy_transfer:
                Ef = Eis[ie] - en
                if Ef > 0:
                    fac = (Ef / Eis[ie]) ** 1.5
                    chopRes = (2 * chop_width[ie] / t_mod_chop) * (1 + ((self.chop_samp + lastChopDist) / self.samp_det) * fac)
                    modRes = (2 * mod_width[ie] / t_mod_chop) * (1 + (self.chop_samp / self.samp_det) * fac)
                    res.append(np.sqrt(chopRes**2 + modRes**2) * Eis[ie])
                else:
                    res.append(np.nan)
            if len(ie_list) == 1:
                res_list = res
            else:
                res_list.append(res)
        # Multiply by distances to get widths at detector position
        chop_width = np.array(chop_width) * (self.samp_det + self.chop_samp + lastChopDist) / lastChopDist
        mod_width = np.array(mod_width) * (self.chop_samp + self.samp_det) / lastChopDist
        if len(ie_list) == 1:
            chop_width = chop_width[ie_list[0]]
            mod_width = mod_width[ie_list[0]]
            res_el = res_el[ie_list[0]]
        return Eis, res_list, res_el, percent, ie_list, chop_width, mod_width

    def getElasticResolution(self, Ei_in=None, frequency=None):
        """
        Returns the elastic resolution as a tuple [FWHM(meV), FWFM(percent)] for a given Ei
        """
        Ei = self.Ei if Ei_in is None else Ei_in
        if Ei is None:
            raise ValueError("Incident energy has not been specified")
        if frequency:
            oldfreq = self.freq
            self.setFrequency(frequency)
        if "LET" in self.instname:
            _, _, res_el, percent, _, chop_width, mod_width = self.__LETgetResolution(True, 0.0, Ei_in)
        elif "MERLIN" in self.instname:
            merlin = ISISFermi("Merlin", self.variant, self.freq[-1])
            res_el = merlin.getResolution(0.0, Ei)
            percent = res_el / Ei
            v_van, tmod, tchp = merlin.getVanVar(Ei)
            chop_width = tchp * 1.0e6
            mod_width = tmod * 1.0e6
        if frequency:
            self.setFrequency(oldfreq)
        # now calculate the resolution and flux.
        return res_el, percent, chop_width, mod_width

    def getWidths(self, Ei_in=None, frequency=None):
        """
        Returns the time widths contributing to the calculated energy width
        """
        res_el, percent, tchp, tmod = self.getElasticResolution(Ei_in, frequency)
        return {"Moderator": tmod, "Chopper": tchp, "Energy": res_el}

    def getMultiWidths(self, Ei_in=None, frequency=None):
        """
        Returns the time widths contributing to the calculated energy width for all reps
        """
        Ei = self.Ei if Ei_in is None else Ei_in
        if not Ei:
            raise ValueError("Incident energy has not been specified")
        if frequency:
            oldfreq = self.freq
            self.setFrequency(frequency)
        Eis, _, res_el, percent, _, chop_width, mod_width = self.__LETgetResolution(False, 0.0, Ei)
        if any([iname in self.instname for iname in ["MERLIN", "MAPS", "MARI"]]):
            chopper_inst = ISISFermi(self.instname, self.variant, self.freq[-1])
            tchp = []
            tmod = []
            for ee in Eis:
                res_el.append(chopper_inst.getResolution(0.0, Ei))
                v_van, mod_width, chop_width = chopper_inst.getVanVar(ee)
                tchp.append(chop_width * 1.0e6)
                tmod.append(mod_width * 1.0e6)
        else:
            tchp = chop_width
            tmod = mod_width
        if frequency:
            self.setFrequency(oldfreq)
        return {"Eis": Eis, "Moderator": tmod, "Chopper": tchp, "Energy": res_el}

    def getResolution(self, Etrans=None, Ei_in=None, frequency=None):
        """
        Returns the energy resolution at a given Ei and Etrans
        """
        Ei = self.Ei if Ei_in is None else Ei_in
        if Ei is None:
            raise ValueError("Incident energy has not been specified")
        if frequency:
            oldfreq = self.freq
            self.setFrequency(frequency)
        if "LET" in self.instname:
            _, res, _, _, _, _, _ = self.__LETgetResolution(True, Etrans, Ei)
        elif any([iname in self.instname for iname in ["MERLIN", "MAPS", "MARI"]]):
            if Etrans is None:
                Etrans = np.linspace(0.05 * Ei, 0.95 * Ei, 19, endpoint=True)
            chopper_inst = ISISFermi(self.instname, self.variant, self.freq[-1])
            res = chopper_inst.getResolution(Etrans, Ei, None, True)
        else:
            raise RuntimeError("Instrument name has not been set")
        if frequency:
            self.setFrequency(oldfreq)
        return res

    def getFlux(self, Ei_in=None, frequency=None):
        """
        Returns the instrument flux at a given Ei by interpolating from a table of measured flux.
        """
        Ei = self.Ei if Ei_in is None else Ei_in
        if Ei is None:
            raise ValueError("Incident energy has not been specified")
        if frequency:
            oldfreq = self.freq
            self.setFrequency(frequency)
        if "LET" in self.instname:
            _, _, _, percent, ie_list, _, _ = self.__LETgetResolution(True, 0.0, Ei)
            flux = MulpyRep.calcFlux(Ei, self.freq[-1], [percent[ie_list[0]]], self.slot_width[-1])[0]
        elif any([iname in self.instname for iname in ["MERLIN", "MAPS", "MARI"]]):
            chopper_inst = ISISFermi(self.instname, self.variant, self.freq[-1])
            flux = chopper_inst.getFlux(Ei)
        else:
            raise RuntimeError("Instrument name has not been set")
        if frequency:
            self.setFrequency(oldfreq)
        return flux

    def getResFlux(self, Etrans=None, Ei_in=None, frequency=None):
        """
        Returns the resolution and flux at a given Ei as a tuple
        """
        return self.getResolution(Etrans, Ei_in, frequency), self.getFlux(Ei_in, frequency)

    def getAllowedEi(self, Ei_in=None):
        """
        Returns a list of the allowed Ei's in multi-rep mode for the current chopper
        configurations and a given focused incident energy (in meV).
        """
        Ei = self.Ei if Ei_in is None else Ei_in
        if Ei is None:
            raise ValueError("Focused incident energy has not been specified")
        instpars = [
            self.dist,
            self.nslot,
            self.slot_ang_pos,
            self.slot_width,
            self.guide_width,
            self.radius,
            self.numDisk,
            self.samp_det,
            self.chop_samp,
            self.source_rep,
            self.tmod,
            self.frac_ei,
            self.ph_ind,
        ]
        Eis, _, _, _, lines = MulpyRep.calcChopTimes(Ei, self.freq, instpars, self.Chop2Phase)
        Eis, _ = self._removeLowIntensityReps(Eis, lines, Ei)
        return Eis

    def getMultiRepResolution(self, Etrans=None, Ei_in=None, frequency=None):
        """
        Returns a list of resolutions for all allowed Ei's in multirep mode.
        The input energy transfer is interpreted as fractions of Ei. e.g. linspace(0,0.9,100)
        """
        Ei = self.Ei if Ei_in is None else Ei_in
        if Ei is None:
            raise ValueError("Incident energy has not been specified")
        if frequency:
            oldfreq = self.freq
            self.setFrequency(frequency)
        if "LET" in self.instname:
            _, res, _, _, _, _, _ = self.__LETgetResolution(False, Etrans, Ei)
        elif any([iname in self.instname for iname in ["MERLIN", "MAPS", "MARI"]]):
            chopper_inst = ISISFermi(self.instname, self.variant, self.freq[-1])
            Eis = self.getAllowedEi()
            res = []
            for ee in Eis:
                try:
                    res.append(chopper_inst.getResolution(np.array(Etrans) * ee, ee))
                except ValueError:
                    res.append([])
        else:
            raise RuntimeError("Instrument name has not been set")
        if frequency:
            self.setFrequency(oldfreq)
        return res

    def getMultiRepFlux(self, Ei_in=None, frequency=None):
        """
        Returns a list of the flux of the instrument for all allowed Ei's in multirep mode.
        """
        Ei = self.Ei if Ei_in is None else Ei_in
        if Ei is None:
            raise ValueError("Incident energy has not been specified")
        if frequency:
            oldfreq = self.freq
            self.setFrequency(frequency)
        if "LET" in self.instname:
            Eis, _, _, percent, _, _, _ = self.__LETgetResolution(False, 0.0, Ei)
            flux = MulpyRep.calcFlux(Eis, self.freq[-1], percent, self.slot_width[-1])
        elif any([iname in self.instname for iname in ["MERLIN", "MAPS", "MARI"]]):
            chopper_inst = ISISFermi(self.instname, self.variant, self.freq[-1])
            Eis = self.getAllowedEi()
            flux = []
            for ee in Eis:
                try:
                    flux.append(chopper_inst.getFlux(ee))
                except ValueError:
                    flux.append([])
        else:
            raise RuntimeError("Instrument name has not been set")
        if frequency:
            self.setFrequency(oldfreq)
        return flux

    def plotFrame(self, h_plt=None, Ei_in=None, frequency=None):
        """
        Plots the time-distance diagram into a given Matplotlib axes, i
        for a give focused incident energy (in meV) and chopper frequencies (in Hz).
        """
        if h_plt is None:
            try:
                from matplotlib import pyplot
            except ImportError:
                raise RuntimeError("ISISDisk.plotFrame: Cannot import matplotlib")
            plt = pyplot
        else:
            plt = h_plt
        Ei = self.Ei if Ei_in is None else Ei_in
        if Ei is None:
            raise ValueError("Incident energy has not been specified")
        if frequency:
            oldfreq = self.freq
            self.setFrequency(frequency)
        instpars = [
            self.dist,
            self.nslot,
            self.slot_ang_pos,
            self.slot_width,
            self.guide_width,
            self.radius,
            self.numDisk,
            self.samp_det,
            self.chop_samp,
            self.source_rep,
            self.tmod,
            self.frac_ei,
            self.ph_ind,
        ]
        Eis, chop_times, _, lastChopDist, lines = MulpyRep.calcChopTimes(Ei, self.freq, instpars, self.Chop2Phase)
        Eis, lines = self._removeLowIntensityReps(Eis, lines, Ei)
        if frequency:
            self.setFrequency(oldfreq)
        dist, samDist, DetDist, fracEi = tuple([self.dist, self.chop_samp, self.samp_det, self.frac_ei])
        modSamDist = dist[-1] + samDist
        totDist = modSamDist + DetDist
        for i in range(len(dist)):
            plt.plot([-20000, 120000], [dist[i], dist[i]], c="k", linewidth=1.0)
            for j in range(len(chop_times[i][:])):
                plt.plot(chop_times[i][j], [dist[i], dist[i]], c="white", linewidth=1.0)
        plt.plot([-20000, 120000], [totDist, totDist], c="k", linewidth=2.0)
        for i in range(len(lines)):
            x0 = (-lines[i][0][1] / lines[i][0][0] - lines[i][1][1] / lines[i][1][0]) / 2.0
            x1 = ((modSamDist - lines[i][0][1]) / lines[i][0][0] + (modSamDist - lines[i][1][1]) / lines[i][1][0]) / 2.0
            plt.plot([x0, x1], [0, modSamDist], c="b")
            x2 = ((totDist - lines[i][0][1]) / lines[i][0][0] + (totDist - lines[i][1][1]) / lines[i][1][0]) / 2.0
            lineM = totDist / x2
            plt.plot([x1, x2], [modSamDist, totDist], c="b")
            newline = [lineM * np.sqrt(1 + fracEi), modSamDist - lineM * np.sqrt(1 + fracEi) * x1]
            x3 = (totDist - newline[1]) / (newline[0])
            plt.plot([x1, x3], [modSamDist, totDist], c="r")
            newline = [lineM * np.sqrt(1 - fracEi), modSamDist - lineM * np.sqrt(1 - fracEi) * x1]
            x4 = (totDist - newline[1]) / (newline[0])
            plt.plot([x1, x4], [modSamDist, totDist], c="r")
            plt.text(x2, totDist + 0.2, "{:3.1f}".format(Eis[i]))
        xmax = 100000 if "LET" in self.instname else 20000
        if h_plt is None:
            plt.xlim(0, xmax)
            plt.xlabel(r"TOF ($\mu$sec)")
            plt.ylabel("Distance (m)")
            plt.show()
        else:
            plt.set_xlim(0, xmax)
            plt.set_xlabel(r"TOF ($\mu$sec)")
            plt.set_ylabel(r"Distance (m)")

    def _removeLowIntensityReps(self, Eis, lines, Ei=None):
        # Removes reps with Ei where there are no neutrons (E<7 meV for Merlin, E>40 meV for LET)
        Eis = np.array(Eis)
        if "MERLIN" in self.instname:
            idx = Eis > 7  # Keep reps above 7meV
        elif "LET" in self.instname:
            idx = Eis < 30  # Keep reps below 30meV
        elif "MARI" in self.instname or "MAPS" in self.instname:
            idx = Eis < 2000
        else:
            idx = np.array(len(Eis) * [True])  # Keep all reps
        # Always keeps desired rep even if outside of range
        if Ei:
            idx1 = (np.abs(Eis - Ei) / np.abs(Eis)) < 0.1
            idx += idx1
        Eis = Eis[idx]
        lines = np.array(lines)[idx]
        return Eis, lines
