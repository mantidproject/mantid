# pylint: disable=line-too-long, invalid-name, old-style-class, multiple-statements, too-many-branches

"""
This module contains the PyChop2 class which allows calculation of the resolution and flux of
direct geometry time-of-flight inelastic neutron spectrometers.
"""

from __future__ import (absolute_import, division, print_function)
from .ISISFermi import ISISFermi
from .ISISDisk import ISISDisk


class PyChop2:
    """
    PyChop2 is a class to calculate the energy resolution of direct geometry time-of-flight spectrometers
    based on calculations of the time spread of neutron bunches. It currently has separate implementations
    for instruments using Fermi choppers and disk choppers for monochromation.
    """

    __Classes = {
        'LET': ISISDisk,         # LET default
        'LETHIFLUX': ISISDisk,   # LET, high flux configuration
        'LETINTERMED': ISISDisk, # LET, medium flux configuration
        'LETHIRES': ISISDisk,    # LET, low flux configuration
        'MAPS': ISISFermi,
        'MARI': ISISFermi,
        'MERLIN': ISISFermi}
    __MultiRepClasses = {
        'LET': ISISDisk,         # LET default
        'LETHIFLUX': ISISDisk,   # LET, high flux configuration
        'LETINTERMED': ISISDisk, # LET, medium flux configuration
        'LETHIRES': ISISDisk,    # LET, low flux configuration
        'MERLIN': ISISDisk,
        'MAPS': ISISDisk,
        'MARI': ISISDisk}

    def __init__(self, instname, *args):
        instname = instname.upper()
        if instname not in self.__Classes.keys():
            raise ValueError('Instrument %s not recognised' % (instname))
        self.object = self.__Classes[instname](instname, *args)
        self.instname = instname

    def allowedInstruments(self):
        """
        ! Returns a list of currently implemented instruments
        """
        return self.__Classes.keys()

    def setInstrument(self, *args):
        """
        ! Sets the instrument to calculate for
        """
        if self.__Classes[args[0]] != self.__Classes[self.instname]:
            self.object = self.__Classes[args[0]](*args)
        else:
            self.object.setInstrument(*args)
        self.instname = args[0]

    def setChopper(self, *args):
        """
        ! Sets the chopper rotor (Fermi instruments) or instrument configuration (disks instruments)
        """
        self.object.setChopper(*args)

    def getChopper(self):
        """
        ! Returns the currently set chopper rotor or instrument configuration
        """
        return self.object.getChopper()

    def setFrequency(self, *args, **kwargs):
        """
        ! Sets the chopper frequency(ies)
        """
        self.object.setFrequency(*args, **kwargs)

    def getFrequency(self):
        """
        ! Returns (a list of) the current chopper frequency(ies)
        """
        return self.object.getFrequency()

    def setEi(self, *args):
        """
        ! Sets the desired or focused incident energy
        """
        self.object.setEi(*args)

    def getEi(self, *args):
        """
        ! Returns the currently set desired or focused incident energy
        """
        return self.object.getEi(*args)

    def getObject(self):
        """
        ! Returns the object instance which actually handles the calculation.
        ! This object's type is a subclass specific to Fermi or Disk instruments and will have
        ! additional methods specific to the class.
        """
        return self.object

    def getResolution(self, *args):
        """
        ! Returns the energy resolution as a function of energy transfer
        !
        ! .getResolution()           - if Ei is set, calculates for [0.05Ei,0.95Ei] in 20 steps
        ! .getResolution(Etrans)     - if Ei is set, calculates for Etrans energy transfer
        ! .getResolution(Etrans, Ei) - calculates for an Ei different from that set previously
        """
        return self.object.getResolution(*args)

    def getFlux(self, *args):
        """
        ! Returns (an estimate of) the neutron flux at the sample at the set Ei in n/cm^2/s
        """
        return self.object.getFlux(*args)

    def getResFlux(self, *args):
        """
        ! Returns a tuple of the (resolution, flux)
        """
        return self.object.getResFlux(*args)

    def getWidths(self, *args):
        """
        ! Returns the individual time widths that go into the calculated energy widths as a dict
        """
        return self.object.getWidths(*args)

    def __getMultiRepObject(self):
        """
        Private method to obtain multi-rep information
        """
        if self.instname not in self.__MultiRepClasses.keys():
            raise ValueError('Instrument %s does not support multirep mode')
        if self.__MultiRepClasses[self.instname] == self.__Classes[self.instname]:
            obj = self.object
        else:
            obj = self.__MultiRepClasses[self.instname](self.instname)
            obj.setChopper(self.object.getChopper())
            obj.setFrequency(self.object.getFrequency(), Chopper2Phase=self.object.diskchopper_phase)
            obj.setEi(self.object.getEi())
        return obj

    def getAllowedEi(self, *args):
        """
        ! For instruments which support multi-rep mode, returns a list of allowed incident energies
        """
        return self.__getMultiRepObject().getAllowedEi(*args)

    def getMultiRepResolution(self, *args):
        """
        ! For instruments which support multi-rep mode, returns the resolution for each rep
        """
        return self.__getMultiRepObject().getMultiRepResolution(*args)

    def getMultiRepFlux(self, *args):
        """
        ! For instruments which support multi-rep mode, returns the flux for each rep
        """
        return self.__getMultiRepObject().getMultiRepFlux(*args)

    def getMultiWidths(self, *args):
        """
        ! Returns the individual time widths that go into the calculated energy widths as a dict
        """
        return self.__getMultiRepObject().getMultiWidths(*args)

    def plotMultiRepFrame(self, *args):
        """
        ! For instruments which support multi-rep mode, plots the time-distance diagram
        """
        return self.__getMultiRepObject().plotFrame(*args)

    @classmethod
    def calculate(cls, *args, **kwargs):
        """
        ! Calculates the resolution and flux directly (without setting up a PyChop2 object)
        !
        ! PyChop2.calculate('mari', 's', 250., 55.)      # Instname, Chopper Type, Freq, Ei in order
        ! PyChop2.calculate('let', 180, 2.2)             # For LET, chopper type is not needed.
        ! PyChop2.calculate('let', [160., 80.], 1.)      # For LET, specify resolution and pulse remover freq
        ! PyChop2.calculate('let', 'High flux', 80, 2.2) # LET default is medium flux configuration
        ! PyChop2.calculate(inst='mari', chtyp='s', freq=250., ei=55.) # With keyword arguments
        ! PyChop2.calculate(inst='let', variant='High resolution', freq=[160., 80.], ei=2.2)
        !
        ! For LET, the allowed variant names are:
        !   'With Chopper 3'
        !   'Without Chopper 3'
        ! You have to use these strings exactly.
        !
        ! By default this function returns the elastic resolution and flux only.
        ! If you want the inelastic resolution, specify the inelastic energy transfer
        ! as either the last positional argument, or as a keyword argument, e.g.:
        !
        ! PyChop2.calculate('merlin', 'g', 450., 60., range(55))
        ! PyChop2.calculate('maps', 'a', 450., 600., etrans=np.linspace(0,550,55))
        !
        ! The results are returned as tuple: (resolution, flux)
        """
        if len(args) > 0:
            if not isinstance(args[0], str):
                raise ValueError('The first argument must be the instrument name')
            instname = args[0].upper()
        elif 'inst' in kwargs.keys():
            instname = kwargs['inst'].upper()
        else:
            raise RuntimeError('You must specify the instrument name')
        obj = cls(instname)
        argdict = {}
        if 'LET' not in instname:
            argname = ['inst', 'chtyp', 'freq', 'ei', 'etrans']
            lna = (len(argname) if len(args) > len(argname) else len(args))
            for ind in range(1, lna):
                argdict[argname[ind]] = args[ind]
            for ind in kwargs.keys():
                if ind in argname:
                    argdict[ind] = kwargs[ind]
            for ind in range(1, 4):
                if argname[ind] not in argdict:
                    raise RuntimeError('Parameter ''%s'' must be specified' % (argname[ind]))
            obj.setChopper(argdict['chtyp'], argdict['freq'])
            obj.setEi(argdict['ei'])
        else:
            if 'variant' in kwargs.keys():
                argdict['variant'] = kwargs['variant']
            if len(args) > 1 and isinstance(args[1], str):
                argname = ['inst', 'variant', 'freq', 'ei', 'etrans']
            else:
                argname = ['inst', 'freq', 'ei', 'etrans']
            lna = (len(argname) if len(args) > len(argname) else len(args))
            for ind in range(1, lna):
                argdict[argname[ind]] = args[ind]
            for ind in kwargs.keys():
                if ind in argname:
                    argdict[ind] = kwargs[ind]
            if 'variant' in argdict.keys():
                obj.setChopper(argdict['variant'])
            if 'freq' not in argdict.keys() or 'ei' not in argdict.keys():
                raise RuntimeError('The chopper frequency and focused incident energy must be specified')
            obj.setFrequency(argdict['freq'])
            obj.setEi(argdict['ei'])
        etrans = argdict['etrans'] if 'etrans' in argdict.keys() else 0.
        return obj.getResolution(etrans), obj.getFlux()
