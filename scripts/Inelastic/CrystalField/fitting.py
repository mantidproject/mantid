import numpy as np


class CrystalField(object):
    """Calculates the crystal fields for one ion"""

    ion_nre_map = {'Ce': 1, 'Pr': 2, 'Nd': 3, 'Pm': 4, 'Sm': 5, 'Eu': 6, 'Gd': 7,
                   'Tb': 8, 'Dy': 9, 'Ho': 10, 'Er': 11, 'Tm': 12, 'Yb': 13}

    allowed_symmetries = ['C1', 'Ci', 'C2', 'Cs', 'C2h', 'C2v', 'D2', 'D2h', 'C4', 'S4', 'C4h',
                          'D4', 'C4v', 'D2d', 'D4h', 'C3', 'S6', 'D3', 'C3v', 'D3d', 'C6', 'C3h',
                          'C6h', 'D6', 'C6v', 'D3h', 'D6h', 'T', 'Td', 'Th', 'O', 'Oh']

    default_peakShape = 'Gaussian'

    def __init__(self, Ion, Symmetry, **kwargs):
        """
        Constructor.

        :param Ion: A rare earth ion. Possible values:
                    Ce, Pr, Nd, Pm, Sm, Eu, Gd, Tb, Dy, Ho, Er, Tm, Yb

        :param Symmetry: Symmetry of the field. Possible values:
                         C1, Ci, C2, Cs, C2h, C2v, D2, D2h, C4, S4, C4h, D4, C4v, D2d, D4h, C3,
                         S6, D3, C3v, D3d, C6, C3h, C6h, D6, C6v, D3h, D6h, T, Td, Th, O, Oh

        :param kwargs: Other field parameters and attributes. Acceptable values include:
                        ToleranceEnergy:     energy tolerance,
                        ToleranceIntensity:  intensity tolerance,
                        ResolutionModel: A resolution model.

                        Field parameters:

                        BmolX: The x-component of the molecular field,
                        BmolY: The y-component of the molecular field,
                        BmolZ: The z-component of the molecular field,
                        BextX: The x-component of the external field,
                        BextY: The y-component of the external field,
                        BextZ: The z-component of the external field,
                        B20: Real part of the B20 field parameter,
                        B21: Real part of the B21 field parameter,
                        B22: Real part of the B22 field parameter,
                        B40: Real part of the B40 field parameter,
                        B41: Real part of the B41 field parameter,
                        B42: Real part of the B42 field parameter,
                        B43: Real part of the B43 field parameter,
                        B44: Real part of the B44 field parameter,
                        B60: Real part of the B60 field parameter,
                        B61: Real part of the B61 field parameter,
                        B62: Real part of the B62 field parameter,
                        B63: Real part of the B63 field parameter,
                        B64: Real part of the B64 field parameter,
                        B65: Real part of the B65 field parameter,
                        B66: Real part of the B66 field parameter,
                        IB21: Imaginary part of the B21 field parameter,
                        IB22: Imaginary part of the B22 field parameter,
                        IB41: Imaginary part of the B41 field parameter,
                        IB42: Imaginary part of the B42 field parameter,
                        IB43: Imaginary part of the B43 field parameter,
                        IB44: Imaginary part of the B44 field parameter,
                        IB61: Imaginary part of the B61 field parameter,
                        IB62: Imaginary part of the B62 field parameter,
                        IB63: Imaginary part of the B63 field parameter,
                        IB64: Imaginary part of the B64 field parameter,
                        IB65: Imaginary part of the B65 field parameter,
                        IB66: Imaginary part of the B66 field parameter,


                        Each of the following parameters can be either a single float or an array of floats.
                        They are either all floats or all arrays of the same size.

                        IntensityScaling: A scaling factor for the intensity of each spectrum.
                        FWHM: A default value for the full width at half maximum of the peaks.
                        Temperature: A temperature "of the spectrum" in Kelvin
        """
        self._ion = Ion
        self._symmetry = Symmetry
        self._toleranceEnergy = 1e-10
        self._toleranceIntensity = 1e-3
        self._temperature = None
        self._intensityScaling = 1.0
        self._FWHM = 0.0
        self._resolutionModel = None
        self._fieldParameters = {}

        self.peaks = None
        self.background = None

        for key in kwargs:
            if key == 'ToleranceEnergy':
                self._toleranceEnergy = kwargs[key]
            elif key == 'ToleranceIntensity':
                self._toleranceIntensity = kwargs[key]
            elif key == 'IntensityScaling':
                self._intensityScaling = kwargs[key]
            elif key == 'FWHM':
                self._FWHM = kwargs[key]
            elif key == 'ResolutionModel':
                self._resolutionModel = kwargs[key]
            elif key == 'Temperature':
                self._temperature = kwargs[key]
            else:
                # Crystal field parameters
                self._fieldParameters[key] = kwargs[key]

        # Eigensystem
        self._dirty_eigensystem = True
        self._eigenvalues = None
        self._eigenvectors = None
        self._hamiltonian = None

        # Peak lists
        self._dirty_peaks = True
        self._peakList = None

        # Spectra
        self._dirty_spectra = True

    def _getTemperature(self, i):
        """Get temperature value for i-th spectrum."""
        if self._temperature is None:
            raise RuntimeError('Temperature must be set.')
        if isinstance(self._temperature, float) or isinstance(self._temperature, int):
            if i != 0:
                raise RuntimeError('Cannot evaluate spectrum %s. Only 1 temperature is given.' % i)
            return float(self._temperature)
        else:
            n = len(self._temperature)
            if i >= -n and i < n:
                return float(self._temperature[i])
            else:
                raise RuntimeError('Cannot evaluate spectrum %s. Only %s temperatures are given.' % (i, n))

    def _getPeaksFunction(self, i):
        if self.peaks is None:
            raise RuntimeError('Peaks function(s) must be set.')
        if isinstance(self.peaks, list):
            return self.peaks[i]
        return self.peaks

    def _calcEigensystem(self):
        """Calculate the eigensystem: energies and wavefunctions.
        Also store them and the hamiltonian.
        Protected method. Shouldn't be called directly by user code.
        """
        if self._dirty_eigensystem:
            from energies import energies
            nre = self.ion_nre_map[self._ion]
            self._eigenvalues, self._eigenvectors, self._hamiltonian = energies(nre, **self._fieldParameters)
            self._dirty_eigensystem = False

    def _makePeaksFunction(self, i):
        """Form a definition string for the CrystalFieldPeaks function
        :param i: Index of a spectrum.
        """
        temperature = self._getTemperature(i)
        s = 'name=CrystalFieldPeaks,Ion=%s,Symmetry=%s,Temperature=%s' % (self._ion, self._symmetry, temperature)
        s += ',ToleranceEnergy=%s,ToleranceIntensity=%s' % (self._toleranceEnergy, self._toleranceIntensity)
        s += ',%s' % ','.join(['%s=%s' % item for item in self._fieldParameters.iteritems()])
        return s

    def _calcPeaksList(self, i):
        """Calculate a peak list for spectrum i"""
        if self._dirty_peaks:
            from mantid.api import AlgorithmManager
            alg = AlgorithmManager.createUnmanaged('EvaluateFunction')
            alg.initialize()
            alg.setChild(True)
            alg.setProperty('Function',  self._makePeaksFunction(i))
            del alg['InputWorkspace']
            alg.setProperty('OutputWorkspace', 'dummy')
            alg.execute()
            self._peakList = alg.getProperty('OutputWorkspace').value

    def _makeCrystalFieldSpectrumFunction(self, i):
        """Form a definition string for CrystalFieldSpectrumFunction function

        :param i: Index of a spectrum
        """
        temperature = self._getTemperature(i)

    @property
    def Ion(self):
        """Get value of Ion attribute. For example:

        cf = CrystalField(...)
        ...
        ion = cf.Ion
        """
        return self._ion

    @Ion.setter
    def Ion(self, value):
        """Set new value of Ion attribute. For example:

        cf = CrystalField(...)
        ...
        cf.Ion = 'Pr'
        """
        if value not in self.ion_nre_map.keys():
            msg = 'Value %s is not allowed for attribute Ion.\nList of allowed values: %s' %\
                  (value, ', '.join(self.ion_nre_map.keys()))
            raise RuntimeError(msg)
        self._ion = value
        self._dirty_eigensystem = True
        self._dirty_peaks = True
        self._dirty_spectra = True

    @property
    def Symmetry(self):
        """Get value of Symmetry attribute. For example:

        cf = CrystalField(...)
        ...
        symm = cf.Symmetry
        """
        return self._symmetry

    @Symmetry.setter
    def Symmetry(self, value):
        """Set new value of Symmetry attribute. For example:

        cf = CrystalField(...)
        ...
        cf.Symmetry = 'Td'
        """
        if value not in self.allowed_symmetries:
            msg = 'Value %s is not allowed for attribute Symmetry.\nList of allowed values: %s' % \
                  (value, ', '.join(self.allowed_symmetries))
            raise RuntimeError(msg)
        self._symmetry = value
        self._dirty_eigensystem = True
        self._dirty_peaks = True
        self._dirty_spectra = True

    @property
    def ToleranceEnergy(self):
        """Get energy tolerance"""
        return self._toleranceEnergy

    @ToleranceEnergy.setter
    def ToleranceEnergy(self, value):
        """Set energy tolerance"""
        self._toleranceEnergy = value
        self._dirty_peaks = True
        self._dirty_spectra = True

    @property
    def ToleranceIntensity(self):
        """Get intensity tolerance"""
        return self._toleranceIntensity

    @ToleranceIntensity.setter
    def ToleranceIntensity(self, value):
        """Set intensity tolerance"""
        self._toleranceIntensity = value
        self._dirty_peaks = True
        self._dirty_spectra = True

    @property
    def IntensityScaling(self):
        return self._intensityScaling

    @IntensityScaling.setter
    def IntensityScaling(self, value):
        self._intensityScaling = value
        self._dirty_peaks = True
        self._dirty_spectra = True

    @property
    def Temperature(self):
        return self._temperature

    @Temperature.setter
    def Temperature(self, value):
        self._temperature= value
        self._dirty_peaks = True
        self._dirty_spectra = True

    @property
    def FWHM(self):
        return self._FWHM

    @FWHM.setter
    def FWHM(self, value):
        self._FWHM = value
        self._dirty_spectra = True

    def getEigenvalues(self):
        self._calcEigensystem()
        return self._eigenvalues

    def getEigenvectors(self):
        self._calcEigensystem()
        return self._eigenvectors

    def getHamiltonian(self):
        self._calcEigensystem()
        return self._hamiltonian

    def getPeakList(self, i=0):
        """Get the peak list for spectrum i as a numpy array"""
        self._calcPeaksList(i)
        peaks = np.array([self._peakList.column(0), self._peakList.column(10)])
        return peaks

    def getSpectrum(self, i=0):
        pass


class SimpleFunction(object):
    """A helper object that simplifies getting and setting parameters of a simple named function."""

    def __init__(self, name):
        """
        Constructor
        :param name: A valid name registered with the FunctionFactory.
        """
        self._name = name
        # Function attributes.
        self._attrib = {}
        # Function parameters.
        self._params = {}

    @property
    def name(self):
        """Read only name of this function"""
        return self._name

    @property
    def attr(self):
        return self._attrib

    @property
    def param(self):
        return self._params

    def toString(self):
        """Create function initialisation string"""
        attrib = ['%s=%s' % item for item in self._attrib.iteritems()] + \
                 ['%s=%s' % item for item in self._params.iteritems()]
        if len(attrib) > 0:
            return 'name=%s,%s' % (self._name, ','.join(attrib))
        return 'name=%s' % self._name


class CompositeProperties(object):

    def __init__(self):
        self._properties = {}

    def __getitem__(self, item):
        if item not in self._properties:
            self._properties[item] = {}
        return self._properties[item]

    def getSize(self):
        s = self._properties.keys()
        if len(s) > 0:
            return max(s) + 1
        return 0

    def toStringList(self):
        prop_list = []
        for i in range(self.getSize()):
            if i in self._properties:
                props = self._properties[i]
                prop_list.append(','.join(['%s=%s' % item for item in props.iteritems()]))
            else:
                prop_list.append('')
        return prop_list


class PeaksFunction(object):
    """A helper object that simplifies getting and setting parameters of a composite function
    containing multiple peaks of the same type.

    The object of this class has no access to the C++ fit function it represents. It means that
    it doesn't know what attributes or parameters the function defines and relies on the user
    to provide correct information.
    """
    def __init__(self, name):
        """
        Constructor.

        :param name: The name of the function of each peak.  E.g. Gaussian
        """
        # Name of the peaks
        self._name = name
        # Collection of all attributes
        self._attrib = CompositeProperties()
        # Collection of all parameters
        self._params = CompositeProperties()

    @property
    def name(self):
        """Read only name of the peak function"""
        return self._name

    @property
    def attr(self):
        """Get a dict of all currently set attributes.
        Use this property to set or get an attribute.
        You can only get an attribute that has been previously set via this property.
        """
        return self._attrib

    @property
    def param(self):
        """Get a dict of all currently set parameters
        Use this property to set or get a parameter.
        You can only get an parameter that has been previously set via this property.
        Example:

            fun = PeaksFunction('Gaussian')
            # Set Sigma parameter of the second peak
            peaks.param[1]['Sigma'] = 0.1
            ...
            # Get the value of a previously set parameter
            sigma = peaks.param[1]['Sigma']
            ...
            # Trying to get a value that wasn't set results in an error
            height = peaks[1]['Height'] # error
        """
        return self._params

    def toString(self):
        """Create function initialisation string"""
        n = max(self._attrib.getSize(), self._params.getSize())
        if n == 0:
            raise RuntimeError('PeaksFunction cannot be empty')
        attribs = self._attrib.toStringList()
        params = self._params.toStringList()
        if len(attribs) < n:
            attribs += [''] * (n - len(attribs))
        if len(params) < n:
            params += [''] * (n - len(params))
        peaks = []
        for i in range(n):
            a = attribs[i]
            p = params[i]
            if len(a) != 0 or len(p) != 0:
                if len(a) == 0:
                    peaks.append('name=%s,%s' % (self._name, p))
                elif len(p) == 0:
                    peaks.append('name=%s,%s' % (self._name, a))
                else:
                    peaks.append('name=%s,%s,%s' % (self._name, a,p))
            else:
                peaks.append('name=%s' % self._name)
        return ';'.join(peaks)


class Background(object):
    """Object representing spectrum background: a sum of a central peak and a
    background.
    """

    def __init__(self, peak, background):
        self.peak = peak
        self.background = background

    def toString(self):
        return '%s;%s' % (self.peak, self.background)

