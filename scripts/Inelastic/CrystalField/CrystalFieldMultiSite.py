

class CrystalFieldMultiSite(object):

    field_parameter_names = ['BmolX', 'BmolY', 'BmolZ', 'BextX', 'BextY', 'BextZ',
                             'B20', 'B21', 'B22', 'B40', 'B41', 'B42', 'B43', 'B44', 'B60', 'B61', 'B62', 'B63', 'B64',
                             'B65', 'B66',
                             'IB21', 'IB22', 'IB41', 'IB42', 'IB43', 'IB44', 'IB61', 'IB62', 'IB63', 'IB64', 'IB65',
                             'IB66']

    def __init__(self, Ions, Symmetries, **kwargs):

        self._makeFunction(Ions, Symmetries)
        self.Ions = Ions
        self.Symmetries = Symmetries

        free_parameters = []
        for key in kwargs:
            if key == 'Temperatures':
                self.Temperatures = kwargs[key]
            elif key == 'ToleranceEnergy':
                self.ToleranceEnergy = kwargs[key]
            elif key == 'ToleranceIntensity':
                self.ToleranceIntensity = kwargs[key]
            elif key == 'FWHMs':
                self.FWHMs = kwargs[key]
            elif key == 'ResolutionModel':
                self.ResolutionModel = kwargs[key]
            elif key == 'NPeaks':
                self.NPeaks = kwargs[key]
            elif key == 'FWHMVariation':
                self.FWHMVariation = kwargs[key]
            elif key == 'FixAllPeaks':
                self.FixAllPeaks = kwargs[key]
            elif key == 'PeakShape':
                self.PeakShape = kwargs[key]
            elif key == 'PhysicalProperty':
                self.PhysicalProperty = kwargs[key]
            elif key == 'parameters':
                for name, value in kwargs[key].items():
                    self.function.setParameter(name, value)
            elif key == 'attributes':
                for name, value in kwargs[key].items():
                    self.function.setAttributeValue(name, value)
            else:
                # Crystal field parameters
                self.function.setParameter(key, kwargs[key])
                free_parameters.append(key)
        if not self.isMultiSite():
            for param in CrystalFieldMultiSite.field_parameter_names:
                if param not in free_parameters:
                    self.function.fixParameter(param)
        #  else:  return to this?


    def isMultiSite(self):
        return len(self.Ions) > 1

    def _makeFunction(self, ion, symmetry):
        from mantid.simpleapi import FunctionFactory
        self.function = FunctionFactory.createFunction('CrystalFieldFunction')

    @staticmethod
    def iterable_to_string(iterable):
        values_as_string = ""
        for element in iterable:
            values_as_string += ","
            values_as_string += element
        values_as_string = values_as_string[1:]
        return values_as_string

    @property
    def Ions(self):
        string_ions = self.function.getAttributeValue('Ions')
        string_ions = string_ions[1:-1]
        return string_ions.split(",")

    @Ions.setter
    def Ions(self, value):
        if isinstance(value, basestring):
            self.function.setAttributeValue('Ions', value)
        else:
            self.function.setAttributeValue('Ions', self.iterable_to_string(value))

    @property
    def Symmetries(self):
        string_symmetries = self.function.getAttributeValue('Symmetries')
        string_symmetries = string_symmetries[1:-1]
        return string_symmetries.split(",")

    @Symmetries.setter
    def Symmetries(self, value):
        if isinstance(value, basestring):
            self.function.setAttributeValue('Symmetries', value)
        else:
            self.function.setAttributeValue('Symmetries', self.iterable_to_string(value))

    @property
    def ToleranceEnergy(self):
        """Get energy tolerance"""
        return self.function.getAttributeValue('ToleranceEnergy')

    @ToleranceEnergy.setter
    def ToleranceEnergy(self, value):
        """Set energy tolerance"""
        self.function.setAttributeValue('ToleranceEnergy', float(value))

    @property
    def ToleranceIntensity(self):
        """Get intensity tolerance"""
        return self.function.getAttributeValue('ToleranceIntensity')

    @ToleranceIntensity.setter
    def ToleranceIntensity(self, value):
        """Set intensity tolerance"""
        self.function.setAttributeValue('ToleranceIntensity', float(value))

    @property
    def Temperatures(self):
        return list(self.function.getAttributeValue("Temperatures"))

    @Temperatures.setter
    def Temperatures(self, value):
        self.function.setAttributeValue('Temperatures', value)

    @property
    def FWHMs(self):
        fwhm = self.function.getAttributeValue('FWHMs')
        nDatasets = len(self.Temperatures)
        if len(fwhm) != nDatasets:
            return list(fwhm) * nDatasets
        return list(fwhm)

    @FWHMs.setter
    def FWHMs(self, value):
        if len(value) == 1:
            value = value[0]
            value = [value] * self.NumberOfSpectra
        self.function.setAttributeValue('FWHMs', value)

    @property
    def FWHMVariation(self):
        return self.function.getAttributeValue('FWHMVariation')

    @FWHMVariation.setter
    def FWHMVariation(self, value):
        self.function.setAttributeValue('FWHMVariation', float(value))

    @property
    def ResolutionModel(self):
        return self._resolutionModel

    @ResolutionModel.setter
    def ResolutionModel(self, value):
        from .function import ResolutionModel
        if isinstance(value, ResolutionModel):
            self._resolutionModel = value
        else:
            self._resolutionModel = ResolutionModel(value)
        if not self._resolutionModel.multi or self._resolutionModel.NumberOfSpectra != self.NumberOfSpectra:
            raise RuntimeError('Resolution model is expected to have %s functions, found %s' %
                                (self.NumberOfSpectra, self._resolutionModel.NumberOfSpectra))
        for i in range(self.NumberOfSpectra):
            model = self._resolutionModel.model[i]
            for i in range(self.NumberOfSpectra):
                model = self._resolutionModel.model[i]
                # self.function.setAttributeValue('FWHMX%s' % i, model[0])
                # self.function.setAttributeValue('FWHMY%s' % i, model[1])

    @property
    def PhysicalProperty(self):
        return self._physprop

    @PhysicalProperty.setter
    def PhysicalProperty(self, value):
        from .function import PhysicalProperties
        vlist = value if islistlike(value) else [value]
        if all([isinstance(pp, PhysicalProperties) for pp in vlist]):
            nOldPP = len(self._physprop) if islistlike(self._physprop) else (0 if self._physprop is None else 1)
            self._physprop = value
        else:
            errmsg = 'PhysicalProperty input must be a PhysicalProperties'
            errmsg += ' instance or a list of such instances'
            raise ValueError(errmsg)
        # If a spectrum (temperature) is already defined, or multiple physical properties
        # given, redefine the CrystalFieldMultiSpectrum function.
        if not self.isPhysicalPropertyOnly or islistlike(self.PhysicalProperty):
            tt = self.Temperature if islistlike(self.Temperature) else [self.Temperature]
            ww = list(self.FWHM) if islistlike(self.FWHM) else [self.FWHM]
            # Last n-set of temperatures correspond to PhysicalProperties
            if nOldPP > 0:
                tt = tt[:-nOldPP]
            # Removes 'negative' temperature, which is a flag for no INS dataset
            tt = [val for val in tt if val > 0]
            pptt = [0 if val.Temperature is None else val.Temperature for val in vlist]
            self._remakeFunction(list(tt) + pptt)
            if len(tt) > 0 and len(pptt) > 0:
                ww += [0] * len(pptt)
            self.FWHM = ww
            ppids = [pp.TypeID for pp in vlist]
            self.function.setAttributeValue('PhysicalProperties', [0] * len(tt) + ppids)
            for attribs in [pp.getAttributes(i + len(tt)) for i, pp in enumerate(vlist)]:
                for item in attribs.items():
                    self.function.setAttributeValue(item[0], item[1])

    @property
    def FixAllPeaks(self):
        return self.function.getAttributeValue('FixAllPeaks')

    @FixAllPeaks.setter
    def FixAllPeaks(self, value):
        self.function.setAttributeValue('FixAllPeaks', value)

    @property
    def PeakShape(self):
        return self.function.getAttributeValue('PeakShape')

    @PeakShape.setter
    def PeakShape(self, value):
        self.function.setAttributeValue('PeakShape', value)

    @property
    def NumberOfSpectra(self):
        return len(self.Temperatures)

    @property
    def NPeaks(self):
        return self.function.getAttributeValue('NPeaks')

    @NPeaks.setter
    def NPeaks(self, value):
        self.function.setAttributeValue('NPeaks', value)
