# pylint: disable=line-too-long, invalid-name, too-many-locals, too-many-branches, unused-variable
# pylint: disable=attribute-defined-outside-init, old-style-class, too-many-instance-attributes

"""
Contains the ISISDisk class which calculates resolution and flux for ISIS Disk chopper
spectrometer (LET) - using the functions in MulpyRep and additional tables of instrument parameters
"""

import numpy as np
from . import MulpyRep
from .ISISFermi import ISISFermi
from matplotlib import pyplot

class ISISDisk:
    """
    Calculates the resolution and flux for the ISIS disk chopper spectrometer LET
    """

    def __init__(self, instname=None, variant=None, freq=None):
        if instname:
            self.setInstrument(instname, variant)
            self.freq = 0
            if freq is not None:
                self.setFrequency(freq)
        else:
            self.instname = None
            self.variant = None
        self.Ei = None

    def setInstrument(self, instname, variant=None):
        """
        Sets instrument parameters from name
        """
        instname = instname.upper()
        if 'LET' in instname:
            self.instname = 'LET'
            # Sets parameters which are the same for all configurations
            self.dist = [7.83, 8.4, 11.75, 15.66, 23.5]        # distance to each chopper in m (same for all conf)
            self.nslot = [6, 1, 2, 6, 2]                       # number of slots in each chopper. Assumed equally spaced
            self.guide_width = [40, 40, 40, 40, 20]            # width of the guide in mm
            self.radius = [290, 545, 290, 290, 290]            # radius in mm of each disk at centre of window
            self.numDisk = [2, 1, 1, 1, 2]                     # whether double or single disks
            # possible instname: ['LET', 'LETHIFLUX', 'LETINTERMED', 'LETHIRES'] - corresponds to different configurations
            if 'FLUX' in instname or (variant is not None and 'FLUX' in variant.upper()):
                self.slot_width = [40, 890, 56, 52, 31]        # width of chopper slots in mm
                self.variant = 'High flux'
            elif 'RES' in instname or (variant is not None and 'RES' in variant.upper()):
                self.slot_width = [40, 890, 56, 52, 20]        # width of chopper slots in mm
                self.variant = 'High resolution'
            else:
                self.slot_width = [40, 890, 56, 52, 20]        # width of chopper slots in mm
                self.variant = 'Intermediate'
            self.ph_ind = 1        # index of chopper with user-determined phase
            self.samp_det = 3.5    # sample to detector distance in m
            self.chop_samp = 1.5   # final chopper to sample distance
            self.source_rep = 10   # rep rate of source
            self.tmod = 3500       # maximimum emmision window from moderator in us
            self.frac_ei = 0.90    # fraction of Ei to plot energy loss lines
            self.Chop2Phase = 5    # Phase delay time in usec for chopper 2 (T0/frame overlap chopper)
        elif 'MERLIN' in instname:
            self.dist = [9.3, 10.1]
            self.nslot = [1, 2]
            self.slot_width = [950, 10]
            self.guide_width = [64, 10]
            self.radius = [250, 290]
            self.numDisk = [1, 1]
            self.samp_det = 2.5
            self.chop_samp = 1.82
            self.source_rep = 50
            self.tmod = 50
            self.frac_ei = 0.90
            self.instname = 'MERLIN'
            self.variant = 'G'
            self.ph_ind = 0
            self.Chop2Phase = 1500 # Phased to not let neutrons with Ei>200meV through
        else:
            raise ValueError('Instrument %s not recognised.' % (instname))

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

    def setFrequency(self, frequency, **kwargs):
        """
        Sets the chopper frequencies, in Hz.
        If scalar, sets the resolution chopper freq to this and the pulse remover to freq/2
        """
        if 'LET' in self.instname:
            if 'FLUX' in self.variant.upper():
                if hasattr(frequency, "__len__"):
                    if len(frequency) == 1:
                        self.freq = [frequency[0]/4., 10., frequency[0]/2., frequency[0]/2., frequency[0]]
                    elif len(frequency) == 2:
                        self.freq = [frequency[1]/2., 10., frequency[1], frequency[0]/2., frequency[0]]
                    elif len(frequency) == 5:
                        self.freq = frequency
                    else:
                        raise ValueError('Frequency must be a 1-, 2- or 5-element list/array')
                else:
                    self.freq = [frequency/4., 10., frequency/4., frequency/2., frequency]
            elif 'RES' in self.variant.upper():
                if hasattr(frequency, "__len__"):
                    if len(frequency) == 1:
                        self.freq = [frequency[0]/2., 10., frequency[0]/2., frequency[0]/2., frequency[0]]
                    elif len(frequency) == 2:
                        self.freq = [frequency[0]/2., 10., frequency[1], frequency[0]/2., frequency[0]]
                    elif len(frequency) == 5:
                        self.freq = frequency
                    else:
                        raise ValueError('Frequency must be a 1-, 2- or 5-element list/array')
                else:
                    self.freq = [frequency/2., 10., frequency/2., frequency/2., frequency]
            else:
                if hasattr(frequency, "__len__"):
                    if len(frequency) == 1:
                        self.freq = [frequency[0]/4., 10., frequency[0]/2., frequency[0]/2., frequency[0]]
                    elif len(frequency) == 2:
                        self.freq = [frequency[1]/2., 10., frequency[1], frequency[0]/2., frequency[0]]
                    elif len(frequency) == 5:
                        self.freq = frequency
                    else:
                        raise ValueError('Frequency must be a 1-, 2- or 5-element list/array')
                else:
                    self.freq = [frequency/4., 10., frequency/2., frequency/2., frequency]
            if 'Chopper2Phase' in kwargs.keys():
                self.Chop2Phase = kwargs['Chopper2Phase']
        elif 'MERLIN' in self.instname:
            if hasattr(frequency, "__len__"):
                self.freq = [50., frequency[0]]
            else:
                self.freq = [50., frequency]
            if 'Chopper2Phase' in kwargs.keys():
                self.Chop2Phase = kwargs['Chopper2Phase']
        else:
            raise RuntimeError('Instrument name has not been set')

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
            raise ValueError('Incident energy must be greater than zero')

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
        instpars = [self.dist, self.nslot, self.slot_width, self.guide_width, self.radius, self.numDisk,
                    self.samp_det, self.chop_samp, self.source_rep, self.tmod, self.frac_ei, self.ph_ind]
        Eis, _, chop_times, lastChopDist, _ = MulpyRep.calcChopTimes(Ei, self.freq, instpars, self.Chop2Phase)
        res_el, percent, chop_width, mod_width = MulpyRep.calcRes(Eis, chop_times, lastChopDist, self.chop_samp, self.samp_det)
        if single_mode:
            #ie_list = [ii for ii,ee in enumerate(Eis) if np.abs((ee-Ei)/Ei)<0.05]
            ie_list = np.where(np.abs(np.array(Eis)-Ei) == np.min(np.abs(np.array(Eis)-Ei)))[0]
            Et = np.linspace(0.05, 0.95*Ei, 19, endpoint=True) if (Etrans is None) else Etrans
        else:
            ie_list = range(len(Eis))
            # This is the relative energy transfer
            Et = np.linspace(0.05, 0.95, 19, endpoint=True) if (Etrans is None) else Etrans
        res_list = []
        if not np.shape(Et):
            Et = [Et]
        for ie in ie_list:
            t_mod_chop = 252.82 * lastChopDist * np.sqrt(81.81/Eis[ie])
            res = []
            for en in Et:
                Ef = Eis[ie] - en
                fac = (Ef/Eis[ie])**1.5
                chopRes = (2*chop_width[ie]/t_mod_chop) * ((self.samp_det+self.chop_samp+lastChopDist)/self.samp_det)*fac
                modRes = (2*mod_width[ie]/t_mod_chop) * (1+(self.chop_samp/self.samp_det)*fac)
                res.append(np.sqrt(chopRes**2 + modRes**2)*Eis[ie])
            if len(ie_list) == 1:
                res_list = res
            else:
                res_list.append(res)
        return Eis, res_list, res_el, percent, ie_list

    def getElasticResolution(self, Ei_in=None, frequency=None):
        """
        Returns the elastic resolution as a tuple [FWHM(meV), FWFM(percent)] for a given Ei
        """
        Ei = self.Ei if Ei_in is None else Ei_in
        if Ei is None:
            raise ValueError('Incident energy has not been specified')
        if frequency:
            oldfreq = self.freq
            self.setFrequency(frequency)
        if 'LET' in self.instname:
            _, _, res_el, percent, _ = self.__LETgetResolution(True, 0., Ei_in)
        elif 'MERLIN' in self.instname:
            merlin = ISISFermi('Merlin', self.variant, self.freq[-1])
            res_el = merlin.getResolution(0., Ei)
            percent = res_el / Ei
        if frequency:
            self.setFrequency(oldfreq)
        # now calculate the resolution and flux.
        return res_el, percent

    def getResolution(self, Etrans=None, Ei_in=None, frequency=None):
        """
        Returns the energy resolution at a given Ei and Etrans
        """
        Ei = self.Ei if Ei_in is None else Ei_in
        if Ei is None:
            raise ValueError('Incident energy has not been specified')
        if frequency:
            oldfreq = self.freq
            self.setFrequency(frequency)
        if 'LET' in self.instname:
            _, res, _, _, _ = self.__LETgetResolution(True, Etrans, Ei_in)
        elif 'MERLIN' in self.instname:
            if Etrans is None:
                Etrans = np.linspace(0.05*Ei, 0.95*Ei, 19, endpoint=True)
            merlin = ISISFermi('Merlin', self.variant, self.freq[-1])
            res = merlin.getResolution(Etrans, Ei)
        else:
            raise RuntimeError('Instrument name has not been set')
        if frequency:
            self.setFrequency(oldfreq)
        return res

    def getFlux(self, Ei_in=None, frequency=None):
        """
        Returns the instrument flux at a given Ei by interpolating from a table of measured flux.
        """
        Ei = self.Ei if Ei_in is None else Ei_in
        if Ei is None:
            raise ValueError('Incident energy has not been specified')
        if frequency:
            oldfreq = self.freq
            self.setFrequency(frequency)
        if 'LET' in self.instname:
            _, _, _, percent, ie_list = self.__LETgetResolution(True, 0., Ei)
            flux = MulpyRep.calcFlux(Ei, self.freq[-1], [percent[ie_list[0]]], self.slot_width[-1])[0]
        elif 'MERLIN' in self.instname:
            merlin = ISISFermi('Merlin', self.variant, self.freq[-1])
            flux = merlin.getFlux(Ei)
        else:
            raise RuntimeError('Instrument name has not been set')
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
            raise ValueError('Focused incident energy has not been specified')
        instpars = [self.dist, self.nslot, self.slot_width, self.guide_width, self.radius, self.numDisk,
                    self.samp_det, self.chop_samp, self.source_rep, self.tmod, self.frac_ei, self.ph_ind]
        Eis, _, _, _, _ = MulpyRep.calcChopTimes(Ei, self.freq, instpars, self.Chop2Phase)
        return Eis

    def getMultiRepResolution(self, Etrans=None, Ei_in=None, frequency=None):
        """
        Returns a list of resolutions for all allowed Ei's in multirep mode.
        The input energy transfer is interpreted as fractions of Ei. e.g. linspace(0,0.9,100)
        """
        Ei = self.Ei if Ei_in is None else Ei_in
        if Ei is None:
            raise ValueError('Incident energy has not been specified')
        if frequency:
            oldfreq = self.freq
            self.setFrequency(frequency)
        if 'LET' in self.instname:
            _, res, _, _, _ = self.__LETgetResolution(False, Etrans, Ei)
        elif 'MERLIN' in self.instname:
            merlin = ISISFermi('Merlin', self.variant, self.freq[-1])
            Eis = self.getAllowedEi()
            res = []
            for ee in Eis:
                res.append(merlin.getResolution(Etrans, ee))
        else:
            raise RuntimeError('Instrument name has not been set')
        if frequency:
            self.setFrequency(oldfreq)
        return res

    def getMultiRepFlux(self, Ei_in=None, frequency=None):
        """
        Returns a list of the flux of the instrument for all allowed Ei's in multirep mode.
        """
        Ei = self.Ei if Ei_in is None else Ei_in
        if Ei is None:
            raise ValueError('Incident energy has not been specified')
        if frequency:
            oldfreq = self.freq
            self.setFrequency(frequency)
        if 'LET' in self.instname:
            Eis, _, _, percent, _ = self.__LETgetResolution(False, 0., Ei)
            flux = MulpyRep.calcFlux(Eis, self.freq[-1], percent, self.slot_width[-1])
        elif 'MERLIN' in self.instname:
            Eis, _, _, percent, _ = self.__LETgetResolution(False, 0., Ei)
            merlin = ISISFermi('Merlin', self.variant, self.freq[-1])
            flux = []
            for ee in Eis:
                flux.append(merlin.getFlux(ee))
        else:
            raise RuntimeError('Instrument name has not been set')
        if frequency:
            self.setFrequency(oldfreq)
        return flux

    def plotFrame(self, h_plt=None, Ei_in=None, frequency=None):
        """
        Plots the time-distance diagram into a given Matplotlib axes, i
        for a give focused incident energy (in meV) and chopper frequencies (in Hz).
        """
        Ei = self.Ei if Ei_in is None else Ei_in
        if Ei is None:
            raise ValueError('Incident energy has not been specified')
        if frequency:
            oldfreq = self.freq
            self.setFrequency(frequency)
        instpars = [self.dist, self.nslot, self.slot_width, self.guide_width, self.radius, self.numDisk,
                    self.samp_det, self.chop_samp, self.source_rep, self.tmod, self.frac_ei, self.ph_ind]
        Eis, chop_times, _, lastChopDist, lines = MulpyRep.calcChopTimes(Ei, self.freq, instpars, self.Chop2Phase)
        if frequency:
            self.setFrequency(oldfreq)
        plt = pyplot if h_plt is None else h_plt
        dist, samDist, DetDist, fracEi = tuple([self.dist, self.chop_samp, self.samp_det, self.frac_ei])
        modSamDist = dist[-1] + samDist
        totDist = modSamDist + DetDist
        for i in range(len(dist)):
            plt.plot([-20000, 120000], [dist[i], dist[i]], c='k', linewidth=1.)
            for j in range(len(chop_times[i][:])):
                plt.plot(chop_times[i][j], [dist[i], dist[i]], c='white', linewidth=1.)
        plt.plot([-20000, 120000], [totDist, totDist], c='k', linewidth=2.)
        for i in range(len(lines)):
            x0 = -lines[i][0][1] / lines[i][0][0]
            x1 = (modSamDist-lines[i][0][1]) / lines[i][0][0]
            plt.plot([x0, x1], [0, modSamDist], c='b')
            x2 = (totDist-lines[i][0][1]) / lines[i][0][0]
            plt.plot([x1, x2], [modSamDist, totDist], c='b')
            newline = [lines[i][0][0]*np.sqrt(1+fracEi), modSamDist-lines[i][0][0]*np.sqrt(1+fracEi)*x1]
            x3 = (totDist-newline[1]) / (newline[0])
            plt.plot([x1, x3], [modSamDist, totDist], c='r')
            newline = [lines[i][0][0]*np.sqrt(1-fracEi), modSamDist-lines[i][0][0]*np.sqrt(1-fracEi)*x1]
            x4 = (totDist-newline[1]) / (newline[0])
            plt.plot([x1, x4], [modSamDist, totDist], c='r')
            plt.text(x2, totDist+0.2, "{:3.1f}".format(Eis[i]))
        xmax = 100000 if 'LET' in self.instname else 20000
        if h_plt is None:
            plt.xlim(0, xmax)
            plt.xlabel(r'TOF ($\mu$sec)')
            plt.ylabel('Distance (m)')
            plt.show()
        else:
            plt.set_xlim(0, xmax)
            plt.set_xlabel(r'TOF ($\mu$sec)')
            plt.set_ylabel(r'Distance (m)')

