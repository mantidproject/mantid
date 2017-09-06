

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
        parameter_dict = None
        attribute_dict = None

        free_parameters = []
        if 'parameters' in kwargs:
            parameter_dict = kwargs['parameters']
            del kwargs['parameters']
        if 'attributes' in kwargs:
            attribute_dict = kwargs['attributes']
            del kwargs['attributes']
        if 'Temperatures' in kwargs:
            self.Temperatures = kwargs['Temperatures']
            del kwargs['Temperatures']
            if 'FWHMs' in kwargs:
                self.FWHMs = kwargs['FWHMs']
                del kwargs['FWHMs']
            elif 'ResolutionModel' in kwargs:
                self.ResolutionModel = kwargs['ResolutionModel']
                del kwargs['ResolutionModel']
            else:
                raise RuntimeError("If temperatures are set, must also set FWHMs or ResolutionModel")
        for key in kwargs:
            if key == 'ToleranceEnergy':
                self.ToleranceEnergy = kwargs[key]
            elif key == 'ToleranceIntensity':
                self.ToleranceIntensity = kwargs[key]
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
            else:
                # Crystal field parameters
                self.function.setParameter(key, kwargs[key])
                free_parameters.append(key)
        if not self.isMultiSite():
            for param in CrystalFieldMultiSite.field_parameter_names:
                if param not in free_parameters:
                    self.function.fixParameter(param)
        if attribute_dict is not None:
            for name, value in attribute_dict.items():
                self.function.setAttributeValue(name, value)
        if parameter_dict is not None:
            for name, value in parameter_dict.items():
                self.function.setParameter(name, value)





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

    def getParameter(self, param):
        print self.function.numParams()
        self.function.getParameterValue(param)

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
            value = [value] * len(self.Temperatures)
        self.function.setAttributeValue('FWHMs', value)

    @property
    def FWHMVariation(self):
        return self.function.getAttributeValue('FWHMVariation')

    @FWHMVariation.setter
    def FWHMVariation(self, value):
        self.function.setAttributeValue('FWHMVariation', float(value))

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
        return self.function.getNumberDomains()

    @property
    def NPeaks(self):
        return self.function.getAttributeValue('NPeaks')

    @NPeaks.setter
    def NPeaks(self, value):
        self.function.setAttributeValue('NPeaks', value)

