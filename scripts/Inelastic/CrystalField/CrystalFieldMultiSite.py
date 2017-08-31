

class CrystalFieldMultiSite(object):

    field_parameter_names = ['BmolX', 'BmolY', 'BmolZ', 'BextX', 'BextY', 'BextZ',
                             'B20', 'B21', 'B22', 'B40', 'B41', 'B42', 'B43', 'B44', 'B60', 'B61', 'B62', 'B63', 'B64',
                             'B65', 'B66',
                             'IB21', 'IB22', 'IB41', 'IB42', 'IB43', 'IB44', 'IB61', 'IB62', 'IB63', 'IB64', 'IB65',
                             'IB66']

    def __init__(self, Ions, Symmetries, kwargs):

        self._makeFunction(Ions, Symmetries)
        self.Ions = Ions  # lists of these?
        self.Symmetries = Symmetries

        free_parameters = []
        for key in kwargs:
            if key == 'Temperature':
                self.Temperature = kwargs[key]
            elif key == 'ToleranceEnergy':
                self.ToleranceEnergy = kwargs[key]
            elif key == 'ToleranceIntensity':
                self.ToleranceIntensity = kwargs[key]
            elif key == 'IntensityScaling':
                self.IntensityScaling = kwargs[key]
            elif key == 'FWHM':
                self.FWHM = kwargs[key]
            elif key == 'ResolutionModel':
                self.ResolutionModel = kwargs[key]
            elif key == 'NPeaks':
                self.NPeaks = kwargs[key]
            elif key == 'FWHMVariation':
                self.FWHMVariation = kwargs[key]
            elif key == 'FixAllPeaks':
                self.FixAllPeaks = kwargs[key]
            elif key == 'PhysicalProperty':
                self.PhysicalProperty = kwargs[key]
            else:
                # Crystal field parameters
                self.function.setParameter(key, kwargs[key])
                free_parameters.append(key)

        for param in CrystalFieldMultiSite.field_parameter_names:
            if param not in free_parameters:
                self.function.fixParameter(param)

    def _makeFunction(self, ion, symmetry):
        from mantid.simpleapi import FunctionFactory
        self.function = FunctionFactory.createFunction('CrystalFieldFunction')
        self._isMultiSpectrum = True

    @property
    def Ions(self):
        return self.function.getAttributeValue('Ion')

    @Ions.setter
    def Ions(self, value):
        self.function.setAttributeValue('Ion', value)

    @property
    def Symmetries(self):
        return self.function.getAttributeValue('Symmetry')

    @Symmetries.setter
    def Symmetries(self, value):
        self.function.setAttributeValue('Symmetry', value)

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
    def IntensityScaling(self):
        iscaling = []
        for i in range(self.NumberOfSpectra):
            paramName = 'IntensityScaling%s' % i
            iscaling.append(self.function.getParameterValue(paramName))
        return iscaling

    @IntensityScaling.setter
    def IntensityScaling(self, value):
        n = self.NumberOfSpectra
        if len(value) != n:
            raise ValueError('IntensityScaling is expected to be a list of %s values' % n)
        for i in range(n):
            paramName = 'IntensityScaling%s' % i
            self.function.setParameter(paramName, value[i])

        self._dirty_peaks = True
        self._dirty_spectra = True

    @property
    def Temperature(self):
        attrName = 'Temperatures' if self._isMultiSpectrum else 'Temperature'
        return self.function.getAttributeValue(attrName)

    @Temperature.setter
    def Temperature(self, value):
        self.function.setAttributeValue('Temperatures', value)

    @property
    def FWHM(self):
        attrName = 'FWHMs'
        fwhm = self.function.getAttributeValue(attrName)
        nDatasets = len(self.Temperature)
        if len(fwhm) != nDatasets:
            return list(fwhm) * nDatasets
        return fwhm

    @FWHM.setter
    def FWHM(self, value):
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
            self.function.setAttributeValue('FWHMX%s' % i, model[0])
            self.function.setAttributeValue('FWHMY%s' % i, model[1])

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