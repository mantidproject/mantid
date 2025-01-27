# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.api import IFunction, AlgorithmManager, mtd
from mantid.kernel import logger
from mantid.simpleapi import (
    CalculateChiSquared,
    CreateEmptyTableWorkspace,
    EvaluateFunction,
    FunctionFactory,
    FunctionWrapper,
    plotSpectrum,
)
from .function import PeaksFunction, PhysicalProperties, ResolutionModel, Background, Function
from .energies import energies
from .normalisation import split2range, ionname2Nre
from .CrystalFieldMultiSite import CrystalFieldMultiSite
from scipy.constants import physical_constants
from scipy.optimize._numdiff import approx_derivative
import numpy as np
import re
import scipy.optimize as sp
from typing import Callable, Dict, List, Tuple
import warnings

# RegEx pattern matching a composite function parameter name, eg f2.Sigma.
FN_PATTERN = re.compile("f(\\d+)\\.(.+)")

# RegEx pattern matching a composite function parameter name, eg f2.Sigma. Multi-spectrum case.
FN_MS_PATTERN = re.compile("f(\\d+)\\.f(\\d+)\\.(.+)")

CONSTRAINTS_PATTERN = re.compile(r"constraints=\((.*?)\)")
FWHM_PATTERN = re.compile(r"FWHM[X|Y]\d+=\(\),")
PHYSICAL_PROPERTIES_PATTERN = re.compile(r"(name=.*?,)(.*?)(PhysicalProperties=\(.*?\),)")
TEMPERATURES_PATTERN = re.compile(r"(name=.*?,)(.*?)(Temperatures=\(.*?\),)")
TIES_PATTERN = re.compile(r",ties=\((.*?)\)")


def makeWorkspace(xArray, yArray):
    """Create a workspace that doesn't appear in the ADS"""
    alg = AlgorithmManager.createUnmanaged("CreateWorkspace")
    alg.initialize()
    alg.setChild(True)
    alg.setProperty("DataX", xArray)
    alg.setProperty("DataY", yArray)
    alg.setProperty("OutputWorkspace", "dummy")
    alg.execute()
    return alg.getProperty("OutputWorkspace").value


def islistlike(arg):
    return (not hasattr(arg, "strip")) and (hasattr(arg, "__getitem__") or hasattr(arg, "__iter__")) and hasattr(arg, "__len__")


def cfpstrmaker(x, pref="B"):
    return [pref + str(k) + str(x) for k in [2, 4, 6] if x <= k]


def getSymmAllowedParam(sym_str):
    if "T" in sym_str or "O" in sym_str:
        return ["B40", "B60"]
    if any([sym_str == val for val in ["C1", "Ci"]]):
        return sum([cfpstrmaker(i) for i in range(7)] + [cfpstrmaker(i, "IB") for i in range(1, 7)], [])
    retval = cfpstrmaker(0)
    if "6" in sym_str or "3" in sym_str:
        retval += cfpstrmaker(6)
        if any([sym_str == val for val in ["C6", "C3h", "C6h"]]):
            retval += cfpstrmaker(6, "IB")
    if ("3" in sym_str and "3h" not in sym_str) or "S6" in sym_str:
        retval += cfpstrmaker(3)
        if any([sym_str == val for val in ["C3", "S6"]]):
            retval += cfpstrmaker(3, "IB") + cfpstrmaker(6, "IB")
    if "4" in sym_str or "2" in sym_str:
        retval += cfpstrmaker(4)
        if any([sym_str == val for val in ["C4", "S4", "C4h"]]):
            retval += cfpstrmaker(4, "IB")
    if ("2" in sym_str and "2d" not in sym_str) or "Cs" in sym_str:
        retval += cfpstrmaker(2)
        if any([sym_str == val for val in ["C2", "Cs", "C2h"]]):
            retval += cfpstrmaker(2, "IB") + cfpstrmaker(4, "IB")
    return retval


# pylint: disable=too-many-instance-attributes,too-many-public-methods
class CrystalField(object):
    """Calculates the crystal fields for one ion"""

    allowed_symmetries = [
        "C1",
        "Ci",
        "C2",
        "Cs",
        "C2h",
        "C2v",
        "D2",
        "D2h",
        "C4",
        "S4",
        "C4h",
        "D4",
        "C4v",
        "D2d",
        "D4h",
        "C3",
        "S6",
        "D3",
        "C3v",
        "D3d",
        "C6",
        "C3h",
        "C6h",
        "D6",
        "C6v",
        "D3h",
        "D6h",
        "T",
        "Td",
        "Th",
        "O",
        "Oh",
    ]

    lande_g = [
        6.0 / 7.0,
        4.0 / 5.0,
        8.0 / 11.0,
        3.0 / 5.0,
        2.0 / 7.0,
        0.0,
        2.0,
        3.0 / 2.0,
        4.0 / 3.0,
        5.0 / 4.0,
        6.0 / 5.0,
        7.0 / 6.0,
        8.0 / 7.0,
    ]

    default_peakShape = "Gaussian"
    default_background = "FlatBackground"
    default_spectrum_size = 200

    field_parameter_names = [
        "BmolX",
        "BmolY",
        "BmolZ",
        "BextX",
        "BextY",
        "BextZ",
        "B20",
        "B21",
        "B22",
        "B40",
        "B41",
        "B42",
        "B43",
        "B44",
        "B60",
        "B61",
        "B62",
        "B63",
        "B64",
        "B65",
        "B66",
        "IB21",
        "IB22",
        "IB41",
        "IB42",
        "IB43",
        "IB44",
        "IB61",
        "IB62",
        "IB63",
        "IB64",
        "IB65",
        "IB66",
    ]

    def __init__(self, Ion, Symmetry, **kwargs):
        """
        Constructor.

        @param Ion: A rare earth ion. Possible values:
                    Ce, Pr, Nd, Pm, Sm, Eu, Gd, Tb, Dy, Ho, Er, Tm, Yb

        @param Symmetry: Symmetry of the field. Possible values:
                         C1, Ci, C2, Cs, C2h, C2v, D2, D2h, C4, S4, C4h, D4, C4v, D2d, D4h, C3,
                         S6, D3, C3v, D3d, C6, C3h, C6h, D6, C6v, D3h, D6h, T, Td, Th, O, Oh

        @param kwargs: Other field parameters and attributes. Acceptable values include:
                        ToleranceEnergy:     energy tolerance,
                        ToleranceIntensity:  intensity tolerance,
                        ResolutionModel:     A resolution model.
                        FWHMVariation:       Absolute value of allowed variation of a peak width during a fit.
                        FixAllPeaks:         A boolean flag that fixes all parameters of the peaks.

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
                        PhysicalProperty: A list of PhysicalProperties objects denoting the required data type
                                          Note that physical properties datasets should follow inelastic spectra
                                          See the Crystal Field Python Interface help page for more details.
        """

        self._background = None

        if "Temperature" in kwargs:
            temperature = kwargs["Temperature"]
            del kwargs["Temperature"]
        else:
            temperature = -1

        # Create self.function attribute
        self._makeFunction(Ion, Symmetry, temperature)
        self.Temperature = temperature
        self.Ion = Ion
        self.Symmetry = Symmetry
        self._resolutionModel = None
        self._physprop = None

        free_parameters = {key: kwargs[key] for key in kwargs if key in CrystalField.field_parameter_names}

        if "ResolutionModel" in kwargs and "FWHM" in kwargs:
            msg = "Both ResolutionModel and FWHM specified but can only accept one width option."
            msg += " Preferring to use ResolutionModel, and ignoring FWHM."
            kwargs.pop("FWHM")
            warnings.warn(msg, SyntaxWarning)

        for key in kwargs:
            if key == "ToleranceEnergy":
                self.ToleranceEnergy = kwargs[key]
            elif key == "ToleranceIntensity":
                self.ToleranceIntensity = kwargs[key]
            elif key == "IntensityScaling":
                self.IntensityScaling = kwargs[key]
            elif key == "FWHM":
                self.FWHM = kwargs[key]
            elif key == "ResolutionModel":
                self.ResolutionModel = kwargs[key]
            elif key == "NPeaks":
                self.NPeaks = kwargs[key]
            elif key == "FWHMVariation":
                self.FWHMVariation = kwargs[key]
            elif key == "FixAllPeaks":
                self.FixAllPeaks = kwargs[key]
            elif key == "PhysicalProperty":
                self.PhysicalProperty = kwargs[key]
            elif key not in free_parameters:
                raise RuntimeError("Unknown attribute/parameters %s" % key)

        # Cubic is a special case where B44=5*B40, B64=-21*B60
        is_cubic = self.Symmetry.startswith("T") or self.Symmetry.startswith("O")
        symm_allowed_par = getSymmAllowedParam(self.Symmetry)

        for param in CrystalField.field_parameter_names:
            if param in free_parameters:
                self.function.setParameter(param, free_parameters[param])
            if is_cubic and (param == "B44" or param == "B64"):
                continue
            if param not in symm_allowed_par:
                self.function.fixParameter(param)
            else:
                self.function.freeParameter(param)

        self._setPeaks()

        # Required to build the target function for the first time (which includes applying all ties)
        self.crystalFieldFunction.initialize()

        # Eigensystem
        self._dirty_eigensystem = True
        self._eigenvalues = None
        self._eigenvectors = None
        self._hamiltonian = None

        # Peak lists
        self._dirty_peaks = True
        self._peakList = None

        # Spectra
        self._plot_window = {}

        # self._setDefaultTies()
        self.chi2 = None

    def _makeFunction(self, ion, symmetry, temperature):
        if temperature is not None and islistlike(temperature) and len(temperature) > 1:
            self.function = FunctionFactory.createFunction("CrystalFieldMultiSpectrum")
            self._isMultiSpectrum = True
            tempStr = "Temperatures"
        else:
            self.function = FunctionFactory.createFunction("CrystalFieldSpectrum")
            self._isMultiSpectrum = False
            tempStr = "Temperature"
        self.function.setAttributeValue("Ion", ion)
        self.function.setAttributeValue("Symmetry", symmetry)
        if temperature:
            temperature = [float(val) for val in temperature] if islistlike(temperature) else float(temperature)
            self.function.setAttributeValue(tempStr, temperature)

    def _remakeFunction(self, temperature):
        """Redefines the internal function, e.g. when `Temperature` (number of datasets) change"""
        fieldParams = self._getFieldParameters()
        self._makeFunction(self.Ion, self.Symmetry, temperature)
        for item in fieldParams.items():
            self.function.setParameter(item[0], item[1])
        for param in CrystalField.field_parameter_names:
            if param not in fieldParams.keys():
                self.function.fixParameter(param)

    def _setPeaks(self):
        if self._isMultiSpectrum:
            self._peaks = []
            # Even a single spectrum when used in conjunction with physical properties
            # is treated as a multi-spectra and the number of spectra includes the number
            # of physical properties. However, only spectra have the peaks.
            len_physical_properties = 0
            if self._physprop is not None:
                if isinstance(self._physprop, list):
                    len_physical_properties = len(self._physprop)
                else:
                    len_physical_properties = 1
            for i in range(self.NumberOfSpectra - len_physical_properties):
                self._peaks.append(PeaksFunction(self.crystalFieldFunction, "f%s." % i, 1))
        else:
            self._peaks = PeaksFunction(self.crystalFieldFunction, "", 0)

    @property
    def crystalFieldFunction(self):
        if not self._isMultiSpectrum and self.background is not None:
            return self.function[1]
        else:
            return self.function

    def makePeaksFunction(self, i):
        """Form a definition string for the CrystalFieldPeaks function
        @param i: Index of a spectrum.
        """
        temperature = self._getTemperature(i)
        out = "name=CrystalFieldPeaks,Ion=%s,Symmetry=%s,Temperature=%s" % (self.Ion, self.Symmetry, temperature)
        out += ",ToleranceEnergy=%s,ToleranceIntensity=%s" % (self.ToleranceEnergy, self.ToleranceIntensity)
        out += ",%s" % ",".join(["%s=%s" % item for item in self._getFieldParameters().items()])
        return out

    def makeSpectrumFunction(self, i=0):
        """Form a definition string for the CrystalFieldSpectrum function
        @param i: Index of a spectrum.
        """
        if not self._isMultiSpectrum:
            return str(self.function)
        else:
            funs = self.function.createEquivalentFunctions()
            return str(funs[i])

    def makePhysicalPropertiesFunction(self, i=0):
        """Form a definition string for one of the crystal field physical properties functions
        @param i: Index of the dataset (default=0), or a PhysicalProperties object.
        """
        if hasattr(i, "toString"):
            out = i.toString()
            ppobj = i
        else:
            if self._physprop is None:
                raise RuntimeError("Physical properties environment not defined.")
            ppobj = self._physprop[i] if islistlike(self._physprop) else self._physprop
            if hasattr(ppobj, "toString"):
                out = ppobj.toString()
            else:
                return ""
        out += ",Ion=%s,Symmetry=%s" % (self.Ion, self.Symmetry)
        fieldParams = self._getFieldParameters()
        if len(fieldParams) > 0:
            out += ",%s" % ",".join(["%s=%s" % item for item in fieldParams.items()])
        ties = self._getFieldTies()
        if ppobj.TypeID == PhysicalProperties.SUSCEPTIBILITY:
            ties += "," if ties else ""
            ties += "Lambda=0" if ppobj.Lambda == 0.0 else ""
            ties += ",Chi0=0" if ppobj.Chi0 == 0.0 else ""
        if len(ties) > 0:
            out += ",ties=(%s)" % ties
        constraints = self._getFieldConstraints()
        if len(constraints) > 0:
            out += ",constraints=(%s)" % constraints
        return out

    def makeMultiSpectrumFunction(self):
        fun = re.sub(FWHM_PATTERN, "", str(self.function))
        fun = re.sub(TEMPERATURES_PATTERN, r"\1\3\2", fun)
        fun = re.sub(PHYSICAL_PROPERTIES_PATTERN, r"\1\3\2", fun)
        return fun

    @property
    def Ion(self):
        """Get value of Ion attribute. For example:

        cf = CrystalField(...)
        ...
        ion = cf.Ion
        """
        return self.crystalFieldFunction.getAttributeValue("Ion")

    @Ion.setter
    def Ion(self, value):
        """Set new value of Ion attribute. For example:

        cf = CrystalField(...)
        ...
        cf.Ion = 'Pr'
        """
        self._nre = ionname2Nre(value)
        self.crystalFieldFunction.setAttributeValue("Ion", value)
        self._dirty_eigensystem = True
        self._dirty_peaks = True

    @property
    def Symmetry(self):
        """Get value of Symmetry attribute. For example:

        cf = CrystalField(...)
        ...
        symm = cf.Symmetry
        """
        return self.crystalFieldFunction.getAttributeValue("Symmetry")

    @Symmetry.setter
    def Symmetry(self, value):
        """Set new value of Symmetry attribute. For example:

        cf = CrystalField(...)
        ...
        cf.Symmetry = 'Td'
        """
        if value not in self.allowed_symmetries:
            msg = "Value %s is not allowed for attribute Symmetry.\nList of allowed values: %s" % (
                value,
                ", ".join(self.allowed_symmetries),
            )
            raise RuntimeError(msg)
        self.crystalFieldFunction.setAttributeValue("Symmetry", value)
        self._dirty_eigensystem = True
        self._dirty_peaks = True

        self.crystalFieldFunction.initialize()

    @property
    def ToleranceEnergy(self):
        """Get energy tolerance"""
        return self.crystalFieldFunction.getAttributeValue("ToleranceEnergy")

    @ToleranceEnergy.setter
    def ToleranceEnergy(self, value):
        """Set energy tolerance"""
        self.crystalFieldFunction.setAttributeValue("ToleranceEnergy", float(value))
        self._dirty_peaks = True

    @property
    def ToleranceIntensity(self):
        """Get intensity tolerance"""
        return self.crystalFieldFunction.getAttributeValue("ToleranceIntensity")

    @ToleranceIntensity.setter
    def ToleranceIntensity(self, value):
        """Set intensity tolerance"""
        self.crystalFieldFunction.setAttributeValue("ToleranceIntensity", float(value))
        self._dirty_peaks = True

    @property
    def IntensityScaling(self):
        if not self._isMultiSpectrum:
            return self.crystalFieldFunction.getParameterValue("IntensityScaling")
        iscaling = []
        for i in range(self.NumberOfSpectra):
            paramName = "IntensityScaling%s" % i
            iscaling.append(self.crystalFieldFunction.getParameterValue(paramName))
        return iscaling

    @IntensityScaling.setter
    def IntensityScaling(self, value):
        if not self._isMultiSpectrum:
            if islistlike(value):
                if len(value) == 1:
                    value = value[0]
                else:
                    raise ValueError("IntensityScaling is expected to be a single floating point value")
            self.crystalFieldFunction.setParameter("IntensityScaling", value)
        else:
            n = self.NumberOfSpectra
            if not islistlike(value) or len(value) != n:
                raise ValueError("IntensityScaling is expected to be a list of %s values" % n)
            for i in range(n):
                paramName = "IntensityScaling%s" % i
                self.crystalFieldFunction.setParameter(paramName, value[i])

        self._dirty_peaks = True

    @property
    def Temperature(self):
        attrName = "Temperatures" if self._isMultiSpectrum else "Temperature"
        return self.crystalFieldFunction.getAttributeValue(attrName)

    @Temperature.setter
    def Temperature(self, value):
        if islistlike(value) and len(value) == 1:
            value = value[0]
        if self._isMultiSpectrum:
            if not islistlike(value):
                # Try to keep current set of field parameters.
                self._remakeFunction(float(value))
                return
            self.crystalFieldFunction.setAttributeValue("Temperatures", value)
        else:
            if islistlike(value):
                self._remakeFunction(value)
                return
            self.crystalFieldFunction.setAttributeValue("Temperature", float(value))
        self._dirty_peaks = True

    @property
    def FWHM(self):
        attrName = "FWHMs" if self._isMultiSpectrum else "FWHM"
        fwhm = self.crystalFieldFunction.getAttributeValue(attrName)
        if self._isMultiSpectrum:
            nDatasets = len(self.Temperature)
            if len(fwhm) != nDatasets:
                return list(fwhm) * nDatasets
        return fwhm

    @FWHM.setter
    def FWHM(self, value):
        if islistlike(value) and len(value) == 1:
            value = value[0]
        if self._isMultiSpectrum:
            if not islistlike(value):
                value = [value] * self.NumberOfSpectra
            if len(value) != len(self.Temperature):
                if self.PhysicalProperty is not None and len(value) == len(self.Temperature) - len(self.PhysicalProperty):
                    value = value + [0] * len(self.PhysicalProperty)
                    # Cast all types to match the first elem so we don't have mixed lists of int/doubles
                    value = [float(v) for v in value]
                else:
                    raise RuntimeError(
                        "Vector of FWHMs must either have same size as Temperatures (%i) or have size 1." % (len(self.Temperature))
                    )
            self.crystalFieldFunction.setAttributeValue("FWHMs", value)
        else:
            if islistlike(value):
                raise ValueError("FWHM is expected to be a single floating point value")
            self.crystalFieldFunction.setAttributeValue("FWHM", float(value))
        # If both FWHM and ResolutionModel is set, may cause runtime errors
        self._resolutionModel = None
        if self._isMultiSpectrum:
            for i in range(self.NumberOfSpectra):
                if self.crystalFieldFunction.getAttributeValue("FWHMX%s" % i):
                    self.crystalFieldFunction.setAttributeValue("FWHMX%s" % i, [])
                if self.crystalFieldFunction.getAttributeValue("FWHMY%s" % i):
                    self.crystalFieldFunction.setAttributeValue("FWHMY%s" % i, [])
        else:
            if self.crystalFieldFunction.getAttributeValue("FWHMX"):
                self.crystalFieldFunction.setAttributeValue("FWHMX", [])
            if self.crystalFieldFunction.getAttributeValue("FWHMY"):
                self.crystalFieldFunction.setAttributeValue("FWHMY", [])

    @property
    def FWHMVariation(self):
        return self.crystalFieldFunction.getAttributeValue("FWHMVariation")

    @FWHMVariation.setter
    def FWHMVariation(self, value):
        self.crystalFieldFunction.setAttributeValue("FWHMVariation", float(value))

    def __getitem__(self, item):
        return self.crystalFieldFunction.getParameterValue(item)

    def __setitem__(self, key, value):
        self.crystalFieldFunction.setParameter(key, value)

    @property
    def ResolutionModel(self):
        return self._resolutionModel

    @ResolutionModel.setter
    def ResolutionModel(self, value):
        if hasattr(value, "model"):
            self._resolutionModel = value
        else:
            self._resolutionModel = ResolutionModel(value)
        if self._isMultiSpectrum:
            NumberOfPhysProp = len(self._physprop) if islistlike(self._physprop) else (0 if self._physprop is None else 1)
            NSpec = self.NumberOfSpectra - NumberOfPhysProp
            if not self._resolutionModel.multi or self._resolutionModel.NumberOfSpectra != NSpec:
                raise RuntimeError(
                    "Resolution model is expected to have %s functions, found %s" % (NSpec, self._resolutionModel.NumberOfSpectra)
                )
            for i in range(self._resolutionModel.NumberOfSpectra):
                model = self._resolutionModel.model[i]
                self.crystalFieldFunction.setAttributeValue("FWHMX%s" % i, model[0])
                self.crystalFieldFunction.setAttributeValue("FWHMY%s" % i, model[1])
        else:
            model = self._resolutionModel.model
            self.crystalFieldFunction.setAttributeValue("FWHMX", model[0])
            self.crystalFieldFunction.setAttributeValue("FWHMY", model[1])
        # If FWHM is set, it overrides resolution model, so unset it
        if self._isMultiSpectrum and any(self.crystalFieldFunction.getAttributeValue("FWHMs")):
            self.crystalFieldFunction.setAttributeValue("FWHMs", [0.0] * self.NumberOfSpectra)
        elif not self._isMultiSpectrum and self.crystalFieldFunction.getAttributeValue("FWHM"):
            self.crystalFieldFunction.setAttributeValue("FWHM", 0.0)

    @property
    def FixAllPeaks(self):
        return self.crystalFieldFunction.getAttributeValue("FixAllPeaks")

    @FixAllPeaks.setter
    def FixAllPeaks(self, value):
        self.crystalFieldFunction.setAttributeValue("FixAllPeaks", value)

    @property
    def PeakShape(self):
        return self.crystalFieldFunction.getAttributeValue("PeakShape")

    @PeakShape.setter
    def PeakShape(self, value):
        self.crystalFieldFunction.setAttributeValue("PeakShape", value)

    @property
    def NumberOfSpectra(self):
        return self.crystalFieldFunction.getNumberDomains()

    @property
    def NPeaks(self):
        return self.crystalFieldFunction.getAttributeValue("NPeaks")

    @NPeaks.setter
    def NPeaks(self, value):
        self.crystalFieldFunction.setAttributeValue("NPeaks", value)

    @property
    def peaks(self):
        return self._peaks

    @property
    def background(self):
        return self._background

    @background.setter
    def background(self, value):
        """
        Define the background function.
        Args:
            value: an instance of function.Background class or a list of instances
                in a multi-spectrum case
        """
        if self._background is not None:
            raise ValueError("Background has been set already")
        if not hasattr(value, "toString"):
            raise TypeError("Expected a Background object, found %s" % str(value))
        if not self._isMultiSpectrum:
            fun_str = value.toString() + ";" + str(self.function)
            self.function = FunctionFactory.createInitialized(fun_str)
            self._background = self._makeBackgroundObject(value)
            self._setPeaks()
        else:
            self.function.setAttributeValue("Background", value.toString())
            self._background = []
            for ispec in range(self.NumberOfSpectra):
                prefix = "f%s." % ispec
                self._background.append(self._makeBackgroundObject(value, prefix))

    def _makeBackgroundObject(self, value, prefix=""):
        if len(value.functions) > 1:
            prefix += "f0."

        n_functions = 0
        peak, background = None, None
        if value.peak is not None:
            peak = Function(self.function, prefix=prefix + f"f{n_functions}.")
            n_functions += 1
        if value.background is not None:
            background = Function(self.function, prefix=prefix + f"f{n_functions}.")
            n_functions += 1

        other_functions = []
        for function_index in range(n_functions, len(value.functions)):
            other_functions.append(Function(self.function, prefix=prefix + f"f{function_index}."))

        return Background(peak=peak, background=background, functions=other_functions)

    @property
    def PhysicalProperty(self):
        return self._physprop

    @PhysicalProperty.setter
    def PhysicalProperty(self, value):
        vlist = value if islistlike(value) else [value]
        if all([hasattr(pp, "TypeID") for pp in vlist]):
            nOldPP = len(self._physprop) if islistlike(self._physprop) else (0 if self._physprop is None else 1)
            self._physprop = value
        else:
            errmsg = "PhysicalProperty input must be a PhysicalProperties"
            errmsg += " instance or a list of such instances"
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
            self.FWHM = ww
            self._setPeaks()
            ppids = [pp.TypeID for pp in vlist]
            self.function.setAttributeValue("PhysicalProperties", [0] * len(tt) + ppids)
            for attribs in [pp.getAttributes(i + len(tt)) for i, pp in enumerate(vlist)]:
                for item in attribs.items():
                    if "Lambda" in item[0] or "Chi0" in item[0]:
                        self.function.setParameter(item[0], item[1])
                        if item[1] == 0.0:
                            self.function.tie(item[0], "0.")
                        else:
                            self.function.removeTie(item[0])
                    else:
                        self.function.setAttributeValue(item[0], item[1])

    @property
    def isPhysicalPropertyOnly(self):
        return not islistlike(self.Temperature) and self.Temperature < 0 and self.PhysicalProperty is not None

    def ties(self, **kwargs):
        """Set ties on the field parameters.

        @param kwargs: Ties as name=value pairs: name is a parameter name,
            the value is a tie string or a number. For example:
                tie(B20 = 0.1, IB23 = '2*B23')
        """
        for tie in kwargs:
            self.crystalFieldFunction.tie(tie, str(kwargs[tie]))

    def constraints(self, *args):
        """
        Set constraints for the field parameters.

        @param args: A list of constraints. For example:
                constraints('B00 > 0', '0.1 < B43 < 0.9')
        """
        self.crystalFieldFunction.addConstraints(",".join(args))

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
        peaks = np.array([self._peakList.column(0), self._peakList.column(1)])
        return peaks

    def getSpectrum(self, i=0, workspace=None, ws_index=0, x_range: Tuple[int, int] = None):
        """
        Get the i-th spectrum calculated with the current field and peak parameters.

        Alternatively can be called getSpectrum(workspace, ws_index). Spectrum index i is assumed zero.

        Examples:

            cf.getSpectrum() # Return the first spectrum calculated on a generated set of x-values.
            cf.getSpectrum(1, ws, 5) # Calculate the second spectrum using the x-values from the 6th spectrum
                                     # in workspace ws.
            cf.getSpectrum(ws) # Calculate the first spectrum using the x-values from the 1st spectrum
                               # in workspace ws.
            cf.getSpectrum(ws, 3) # Calculate the first spectrum using the x-values from the 4th spectrum
                                  # in workspace ws.
            cf.getSpectrum(x_range=(-2,4)) # Return the first spectrum calculated from -2 to 4.

        @param i: Index of a spectrum to get.
        @param workspace: A workspace to base on. If not given the x-values of the output spectrum will be
                          generated if not specified with x_range.
        @param ws_index:  An index of a spectrum from workspace to use.
        @param x_range: Return the first spectrum calculated on the specified set of x-values. This setting is
                        ignored when a workspace to base on was provided.
        @return: A tuple of (x, y) arrays
        """
        wksp = workspace
        # Allow to call getSpectrum with a workspace as the first argument.
        if not isinstance(i, int):
            if wksp is not None:
                if not isinstance(wksp, int):
                    raise RuntimeError("Spectrum index is expected to be int. Got %s" % i.__class__.__name__)
                ws_index = wksp
            wksp = i
            i = 0

        if (self.Temperature[i] if islistlike(self.Temperature) else self.Temperature) < 0:
            raise RuntimeError("You must first define a temperature for the spectrum")

        # Workspace is given, always calculate
        if wksp is None:
            xArray = None
        elif isinstance(wksp, list) or isinstance(wksp, np.ndarray):
            xArray = wksp
        else:
            return self._calcSpectrum(i, wksp, ws_index)

        if xArray is None:
            if x_range is None:
                x_min, x_max = self.calc_xmin_xmax(i)
            else:
                x_min, x_max = x_range
            xArray = np.linspace(x_min, x_max, self.default_spectrum_size)

        yArray = np.zeros_like(xArray)
        wksp = makeWorkspace(xArray, yArray)
        return self._calcSpectrum(i, wksp, 0)

    def getHeatCapacity(self, workspace=None, ws_index=0):
        """
        Get the heat cacpacity calculated with the current crystal field parameters

        Examples:

            cf.getHeatCapacity()    # Returns the heat capacity from 1 < T < 300 K in 1 K steps
            cf.getHeatCapacity(ws)  # Returns the heat capacity with temperatures given by ws.
            cf.getHeatCapacity(ws, ws_index)  # Use the spectrum indicated by ws_index for x-values

        @param workspace: Either a Mantid workspace whose x-values will be used as the temperatures
                          to calculate the heat capacity; or a list of numpy ndarray of temperatures.
                          Temperatures are in Kelvin.
        @param ws_index:  The index of a spectrum in workspace to use (default=0).
        """
        return self._getPhysProp(PhysicalProperties("Cv"), workspace, ws_index)

    def getSusceptibility(self, *args, **kwargs):
        """
        Get the magnetic susceptibility calculated with the current crystal field parameters.
        The susceptibility is calculated using Van Vleck's formula (2nd order perturbation theory)

        Examples:

            cf.getSusceptibility()      # Returns the susceptibility || [001] for 1<T<300 K in 1 K steps
            cf.getSusceptibility(T)     # Returns the susceptibility with temperatures given by T.
            cf.getSusceptibility(ws, 0) # Use x-axis of spectrum 0 of workspace ws as temperature
            cf.getSusceptibility(T, [1, 1, 1])  # Returns the susceptibility along [111].
            cf.getSusceptibility(T, 'powder')   # Returns the powder averaged susceptibility
            cf.getSusceptibility(T, 'cgs')      # Returns the susceptibility || [001] in cgs normalisation
            cf.getSusceptibility(..., Inverse=True)  # Calculates the inverse susceptibility instead
            cf.getSusceptibility(Temperature=ws, ws_index=0, Hdir=[1, 1, 0], Unit='SI', Inverse=True)

        @param Temperature: Either a Mantid workspace whose x-values will be used as the temperatures
                            to calculate the heat capacity; or a list or numpy ndarray of temperatures.
                            Temperatures are in Kelvin.
        @param ws_index: The index of a spectrum to use (default=0) if using a workspace for x.
        @param Hdir: The magnetic field direction to calculate the susceptibility along. Either a
                     Cartesian vector with z along the quantisation axis of the CF parameters, or the
                     string 'powder' (case insensitive) to get the powder averaged susceptibility
                     default: [0, 0, 1]
        @param Unit: Any one of the strings 'bohr', 'SI' or 'cgs' (case insensitive) to indicate whether
                     to output in atomic (bohr magneton/Tesla/ion), SI (m^3/mol) or cgs (cm^3/mol) units.
                     default: 'cgs'
        @param Inverse: Whether to calculate the susceptibility (Inverse=False, default) or inverse
                        susceptibility (Inverse=True).
        """
        # Sets defaults / parses keyword arguments
        workspace = kwargs["Temperature"] if "Temperature" in kwargs.keys() else None
        ws_index = kwargs["ws_index"] if "ws_index" in kwargs.keys() else 0

        # Parses argument list
        args = list(args)
        if len(args) > 0:
            workspace = args.pop(0)
        if "mantid" in str(type(workspace)) and len(args) > 0:
            ws_index = args.pop(0)

        # _calcSpectrum updates parameters and susceptibility has a 'Lambda' parameter which other
        # CF functions don't have. This causes problems if you want to calculate another quantity after
        x, y = self._getPhysProp(PhysicalProperties("chi", *args, **kwargs), workspace, ws_index)
        return x, y

    def getMagneticMoment(self, *args, **kwargs):
        """
        Get the magnetic moment calculated with the current crystal field parameters.
        The moment is calculated by adding a Zeeman term to the CF Hamiltonian and then diagonlising
        the result. This function calculates either M(H) [default] or M(T), but can only calculate
        a 1D function (e.g. not M(H,T) simultaneously).

        Examples:

            cf.getMagneticMoment()       # Returns M(H) for H||[001] from 0 to 30 T in 0.1 T steps
            cf.getMagneticMoment(H)      # Returns M(H) for H||[001] at specified values of H (in Tesla)
            cf.getMagneticMoment(ws, 0)  # Use x-axis of spectrum 0 of ws as applied field magnitude.
            cf.getMagneticMoment(H, [1, 1, 1])  # Returns the magnetic moment along [111].
            cf.getMagneticMoment(H, 'powder')   # Returns the powder averaged M(H)
            cf.getMagneticMoment(H, 'cgs')      # Returns the moment || [001] in cgs units (emu/mol)
            cf.getMagneticMoment(Temperature=T) # Returns M(T) for H=1T || [001] at specified T (in K)
            cf.getMagneticMoment(10, [1, 1, 0], Temperature=T) # Returns M(T) for H=10T || [110].
            cf.getMagneticMoment(..., Inverse=True)  # Calculates 1/M instead (keyword only)
            cf.getMagneticMoment(Hmag=ws, ws_index=0, Hdir=[1, 1, 0], Unit='SI', Temperature=T, Inverse=True)

        @param Hmag: The magnitude of the applied magnetic field in Tesla, specified either as a Mantid
                     workspace whose x-values will be used; or a list or numpy ndarray of field points.
                     If Temperature is specified as a list / array / workspace, Hmag must be scalar.
                     (default: 0-30T in 0.1T steps, or 1T if temperature vector specified)
        @param Temperature: The temperature in Kelvin at which to calculate the moment.
                            Temperature is a keyword argument only. Can be a list, ndarray or workspace.
                            If Hmag is a list / array / workspace, Temperature must be scalar.
                            (default=1K)
        @param ws_index: The index of a spectrum to use (default=0) if using a workspace for x.
        @param Hdir: The magnetic field direction to calculate the susceptibility along. Either a
                     Cartesian vector with z along the quantisation axis of the CF parameters, or the
                     string 'powder' (case insensitive) to get the powder averaged susceptibility
                     default: [0, 0, 1]
        @param Unit: Any one of the strings 'bohr', 'SI' or 'cgs' (case insensitive) to indicate whether
                     to output in atomic (bohr magneton/ion), SI (Am^2/mol) or cgs (emu/mol) units.
                     default: 'bohr'
        @param Inverse: Whether to calculate the susceptibility (Inverse=False, default) or inverse
                        susceptibility (Inverse=True). Inverse is a keyword argument only.
        """
        # Sets defaults / parses keyword arguments
        workspace = None
        ws_index = kwargs["ws_index"] if "ws_index" in kwargs.keys() else 0
        hmag = kwargs["Hmag"] if "Hmag" in kwargs.keys() else 1.0
        temperature = kwargs["Temperature"] if "Temperature" in kwargs.keys() else 1.0

        # Checks whether to calculate M(H) or M(T)
        hmag_isscalar = not islistlike(hmag) or len(hmag) == 1
        hmag_isvector = islistlike(hmag) and len(hmag) > 1
        t_isscalar = not islistlike(temperature) or len(temperature) == 1
        t_isvector = islistlike(temperature) and len(temperature) > 1
        if hmag_isscalar and (t_isvector or "mantid" in str(type(temperature))):
            typeid = 4
            workspace = temperature
            kwargs["Hmag"] = hmag[0] if islistlike(hmag) else hmag
        else:
            typeid = 3
            if t_isscalar and (hmag_isvector or "mantid" in str(type(hmag))):
                workspace = hmag
            kwargs["Temperature"] = temperature[0] if islistlike(temperature) else temperature

        # Parses argument list
        args = list(args)
        if len(args) > 0:
            if typeid == 4:
                kwargs["Hmag"] = args.pop(0)
            else:
                workspace = args.pop(0)
        if "mantid" in str(type(workspace)) and len(args) > 0:
            ws_index = args.pop(0)

        pptype = "M(T)" if (typeid == 4) else "M(H)"
        self._typeid = self._str2id(typeid) if isinstance(typeid, str) else int(typeid)

        return self._getPhysProp(PhysicalProperties(pptype, *args, **kwargs), workspace, ws_index)

    def _calc_gJuB(self):
        gj = 2.0 if (self._nre < 1) else self.lande_g[self._nre - 1]
        gJuB = gj * physical_constants["Bohr magneton in eV/T"][0] * 1000.0
        return gJuB

    def getDipoleMatrixComponent(self, nComponent, gJuB=None):
        self._calcEigensystem()  # will not recalculate if already called (unless _dirty_eigensystem)
        if gJuB is None:
            gJuB = self._calc_gJuB()

        if nComponent == "X" or nComponent == "x":
            _, _, h_n = energies(self._nre, BextX=1.0)
        elif nComponent == "Y" or nComponent == "y":
            _, _, h_n = energies(self._nre, BextY=1.0)
        elif nComponent == "Z" or nComponent == "z":
            _, _, h_n = energies(self._nre, BextZ=1.0)
        else:
            raise Exception("Invalid Argument, nComponent must be: X, Y or Z (case insensitive)")

        i_n = np.dot(np.conj(np.transpose(self._eigenvectors)), np.dot(h_n, self._eigenvectors))
        return np.multiply(i_n, np.conj(i_n)) / (gJuB**2)

    def getDipoleMatrix(self):
        """Returns the dipole transition matrix as a numpy array"""
        gJuB = self._calc_gJuB()
        trans = (
            self.getDipoleMatrixComponent("X", gJuB) + self.getDipoleMatrixComponent("Y", gJuB) + self.getDipoleMatrixComponent("Z", gJuB)
        )
        return trans

    def printWavefunction(self, index=None):
        # Pretty prints the eigenvector(s) of a crystal field Hamiltonian matrix
        # cf.printWavefunction() - prints all wavefucntions
        # cf.printWavefunction(index) - prints the wavefunctions for a (list) of indices.
        self._calcEigensystem()
        ev = self._eigenvectors
        J2 = ev.shape[0] - 1  # twice the total angular momentum quantum number J
        is_integral = J2 % 2 == 0
        if index is None:
            index = range(ev.shape[0])
        elif not hasattr(index, "__iter__"):
            if index >= ev.shape[0]:
                raise Exception("IndexError: Index %s out of range" % index)
            index = [index]
        for lv in index:
            print(("Ground" if lv == 0 else f"{lv}. excited") + " state wavefunctions are:")
            lvstr = ""
            for ii, jz2 in enumerate(range(-J2, J2 + 1, 2)):
                if np.abs(ev[ii, lv]) < 1e-5:
                    continue
                val = ev[ii, lv] if np.abs(ev[ii, lv].imag) > 1e-3 else ev[ii, lv].real
                jzstr = f"{int(jz2 / 2)}" if is_integral else f"{int(jz2)}/2"
                # The Hamiltonian is expressed in the basis of the azimuthal quantum number |Jz>
                # which goes from |Jz=-J> to |Jz=+J> in steps of 1. The values of the eigenvectors
                # are coefficients corresponding to each of these quantum numbers
                lvstr += ("+" if (ii > 0 and ev[ii, lv].real > 0) else "") + f"{val:.3f}|{jzstr}> "
            print(lvstr + "\n")

    def plot(self, i=0, workspace=None, ws_index=0, x_range: Tuple[int, int] = None, name=None):
        """Plot a spectrum. Parameters are the same as in getSpectrum(...)"""
        createWS = AlgorithmManager.createUnmanaged("CreateWorkspace")
        createWS.initialize()

        xArray, yArray = self.getSpectrum(i, workspace, ws_index, x_range)
        ws_name = name if name is not None else "CrystalField_%s" % self.Ion

        if isinstance(i, int):
            if workspace is None:
                if i > 0:
                    ws_name += "_%s" % i
                createWS.setProperty("DataX", xArray)
                createWS.setProperty("DataY", yArray)
                createWS.setProperty("OutputWorkspace", ws_name)
                createWS.execute()
                plot_window = self._plot_window[i] if i in self._plot_window else None
                self._plot_window[i] = plotSpectrum(ws_name, 0, window=plot_window, clearWindow=True)
            else:
                ws_name += "_%s" % workspace
                if i > 0:
                    ws_name += "_%s" % i
                createWS.setProperty("DataX", xArray)
                createWS.setProperty("DataY", yArray)
                createWS.setProperty("OutputWorkspace", ws_name)
                createWS.execute()
                plotSpectrum(ws_name, 0)
        else:
            ws_name += "_%s" % i
            createWS.setProperty("DataX", xArray)
            createWS.setProperty("DataY", yArray)
            createWS.setProperty("OutputWorkspace", ws_name)
            createWS.execute()
            plotSpectrum(ws_name, 0)

    def update(self, func, index=0):
        """
        Update values of the fitting parameters.
        @param func: A IFunction object containing new parameter values.
        @param index: The index of the function to update in the Background object.
        """
        self.function = func
        if self._background is not None:
            if isinstance(self._background, list):
                for background in self._background:
                    background.update(func, index)
            else:
                self._background.update(func, index)
        self._setPeaks()

    def calc_xmin_xmax(self, i):
        """Calculate the x-range containing interesting features of a spectrum (for plotting)
        @param i: If an integer is given then calculate the x-range for the i-th spectrum.
                  If None given get the range covering all the spectra.
        @return: Tuple (xmin, xmax)
        """
        peaks = self.getPeakList(i)
        x_min = np.min(peaks[0])
        x_max = np.max(peaks[0])
        # dx probably should depend on peak widths
        deltaX = np.abs(x_max - x_min) * 0.1
        if x_min < 0:
            x_min -= deltaX
        x_max += deltaX
        return x_min, x_max

    def __add__(self, other):
        if isinstance(other, CrystalFieldMultiSite):
            return (other).__radd__(self)
        elif isinstance(other, CrystalFieldSite):
            return (1.0 * self).__add__(other)
        if isinstance(other, CrystalField):
            return (1.0 * self).__add__(1.0 * other)

    def __mul__(self, factor):
        ffactor = float(factor)
        if ffactor == 0.0:
            msg = "Intensity scaling factor for %s(%s) is set to zero " % (self.Ion, self.Symmetry)
            warnings.warn(msg, SyntaxWarning)
        return CrystalFieldSite(self, ffactor)

    def __rmul__(self, factor):
        return self.__mul__(factor)

    def _getTemperature(self, i):
        """Get temperature value for i-th spectrum."""
        if not self._isMultiSpectrum:
            if i != 0:
                raise RuntimeError("Cannot evaluate spectrum %s. Only 1 temperature is given." % i)
            return float(self.Temperature)
        else:
            temperatures = self.Temperature
            nTemp = len(temperatures)
            if -nTemp <= i < nTemp:
                return float(temperatures[i])
            else:
                raise RuntimeError("Cannot evaluate spectrum %s. Only %s temperatures are given." % (i, nTemp))

    def _getFWHM(self, i):
        """Get default FWHM value for i-th spectrum."""
        if not self._isMultiSpectrum:
            # if i != 0 assume that value for all spectra
            return float(self.FWHM)
        else:
            fwhm = self.FWHM
            nFWHM = len(fwhm)
            if -nFWHM <= i < nFWHM:
                return float(fwhm[i])
            else:
                raise RuntimeError("Cannot get FWHM for spectrum %s. Only %s FWHM are given." % (i, nFWHM))

    def _getIntensityScaling(self, i):
        """Get default intensity scaling value for i-th spectrum."""
        if self._intensityScaling is None:
            raise RuntimeError("Default intensityScaling must be set.")
        if islistlike(self._intensityScaling):
            return self._intensityScaling[i] if len(self._intensityScaling) > 1 else self._intensityScaling[0]
        else:
            return self._intensityScaling

    def _getPeaksFunction(self, i):
        if isinstance(self.peaks, list):
            return self.peaks[i]
        return self.peaks

    def _getFieldParameters(self):
        """
        Get the values of non-zero field parameters.
        Returns:
            a dict with name: value pairs.
        """
        params = {}
        for name in self.field_parameter_names:
            value = self.crystalFieldFunction.getParameterValue(name)
            if value != 0.0:
                params[name] = value
        return params

    def _getFieldTies(self):
        ties = re.search(TIES_PATTERN, str(self.crystalFieldFunction))
        return re.sub(FN_PATTERN, "", ties.group(1)).rstrip(",") if ties else ""

    def _getFieldConstraints(self):
        constraints = re.search(CONSTRAINTS_PATTERN, str(self.crystalFieldFunction))
        return constraints.group(1) if constraints else ""

    def _getPhysProp(self, ppobj, workspace, ws_index):
        """
        Returns a physical properties calculation
        @param ppobj: a PhysicalProperties object indicating the physical property type and environment
        @param workspace: workspace or array/list of x-values.
        @param ws_index:  An index of a spectrum in workspace to use.
        """
        try:
            typeid = ppobj.TypeID
        except AttributeError:
            raise RuntimeError("Invalid PhysicalProperties object specified")

        defaultX = [np.linspace(1, 300, 300), np.linspace(1, 300, 300), np.linspace(0, 30, 300), np.linspace(0, 30, 300)]
        funstr = self.makePhysicalPropertiesFunction(ppobj)
        if workspace is None:
            xArray = defaultX[typeid - 1]
        elif isinstance(workspace, list) or isinstance(workspace, np.ndarray):
            xArray = workspace
        else:
            return self._calcSpectrum(funstr, workspace, ws_index)

        yArray = np.zeros_like(xArray)
        wksp = makeWorkspace(xArray, yArray)
        return self._calcSpectrum(funstr, wksp, ws_index)

    def _calcEigensystem(self):
        """Calculate the eigensystem: energies and wavefunctions.
        Also store them and the hamiltonian.
        Protected method. Shouldn't be called directly by user code.
        """
        if self._dirty_eigensystem:
            self._eigenvalues, self._eigenvectors, self._hamiltonian = energies(self._nre, **self._getFieldParameters())
            self._dirty_eigensystem = False

    def _calcPeaksList(self, i):
        """Calculate a peak list for spectrum i"""
        if self._dirty_peaks:
            alg = AlgorithmManager.createUnmanaged("EvaluateFunction")
            alg.initialize()
            alg.setChild(True)
            alg.setProperty("Function", self.makePeaksFunction(i))
            del alg["InputWorkspace"]
            alg.setProperty("OutputWorkspace", "dummy")
            alg.execute()
            self._peakList = alg.getProperty("OutputWorkspace").value

    def _calcSpectrum(self, i, workspace, ws_index, funstr=None):
        """Calculate i-th spectrum.

        @param i: Index of a spectrum or function string
        @param workspace: A workspace used to evaluate the spectrum function.
        @param ws_index:  An index of a spectrum in workspace to use.
        """
        alg = AlgorithmManager.createUnmanaged("EvaluateFunction")
        alg.initialize()
        alg.setChild(True)
        alg.setProperty("Function", i if isinstance(i, str) else self.makeSpectrumFunction(i))
        alg.setProperty("InputWorkspace", workspace)
        alg.setProperty("WorkspaceIndex", ws_index)
        alg.setProperty("OutputWorkspace", "dummy")
        alg.execute()
        out = alg.getProperty("OutputWorkspace").value
        # Create copies of the x and y because `out` goes out of scope when this method returns
        # and x and y get deallocated
        return np.array(out.readX(0)), np.array(out.readY(1))

    def isMultiSpectrum(self):
        return self._isMultiSpectrum


class CrystalFieldSite(object):
    """
    A helper class for the multi-site algebra. It is a result of the '*' operation between a CrystalField
    and a number. Multiplication sets the abundance for the site and which the returned object holds.
    """

    def __init__(self, crystalField, abundance):
        self.crystalField = crystalField
        self.abundance = abundance

    def __add__(self, other):
        if isinstance(other, CrystalField):
            return self.__add__(1.0 * other)
        elif isinstance(other, CrystalFieldSite):
            abundances = [self.abundance, other.abundance]
            other = other.crystalField
        elif isinstance(other, CrystalFieldMultiSite):
            return other.__radd__(self)
        else:
            raise TypeError("Unsupported operand type(s) for +: CrystalFieldSite and %s" % other.__class__.__name__)
        ions = [self.crystalField.Ion, other.Ion]
        symmetries = [self.crystalField.Symmetry, other.Symmetry]
        temperatures = [self.crystalField._getTemperature(x) for x in range(self.crystalField.NumberOfSpectra)]
        params = {}
        for bparam in CrystalField.field_parameter_names:
            params["ion0." + bparam] = self.crystalField[bparam]
            params["ion1." + bparam] = other[bparam]
        params["ion0.IntensityScaling"] = abundances[0]
        params["ion1.IntensityScaling"] = abundances[1]
        # Check IntensityScaling settings in original objects
        # If only one has IntensityScaling settings use these for CrystalFieldMultiSite object
        # Warn if both objects have IntensityScaling settings and these are not equal
        if self.crystalField.NumberOfSpectra > 1:
            differentIntensities = False
            for x in range(self.crystalField.NumberOfSpectra):
                if not (
                    np.isclose(self.crystalField.IntensityScaling[x], other.IntensityScaling[x])
                    or np.isclose(other.IntensityScaling[x], 1.0)
                ):
                    if np.isclose(self.crystalField.IntensityScaling[x], 1.0):
                        params["sp" + str(x) + ".IntensityScaling"] = other.IntensityScaling[x]
                    else:
                        differentIntensities = True
                else:
                    params["sp" + str(x) + ".IntensityScaling"] = self.crystalField.IntensityScaling[x]
            if differentIntensities:
                warnings.warn("Mismatch between IntensityScaling values of CrystalField objects", RuntimeWarning)
        # Preserve ties and fixes
        ties = {}
        fixes = []
        ties, fixes = self.getTiesAndFixes(other)
        if self.crystalField.ResolutionModel is None:
            FWHM = [self.crystalField._getFWHM(x) for x in range(self.crystalField.NumberOfSpectra)]
            return CrystalFieldMultiSite(
                Ions=ions,
                Symmetries=symmetries,
                Temperatures=temperatures,
                FWHM=FWHM,
                abundances=abundances,
                parameters=params,
                ties=ties,
                fixedParameters=fixes,
            )
        else:
            return CrystalFieldMultiSite(
                Ions=ions,
                Symmetries=symmetries,
                Temperatures=temperatures,
                ResolutionModel=self.crystalField.ResolutionModel,
                abundances=abundances,
                parameters=params,
                ties=ties,
                fixedParameters=fixes,
            )

    def getTiesAndFixes(self, other):
        ties = {}
        fixes = []
        for prefix, obj in {"ion0.": self.crystalField, "ion1.": other}.items():
            tiestr = obj.function.getTies()
            if tiestr:
                for tiepair in [tie.split("=") for tie in tiestr.split(",")]:
                    ties[prefix + tiepair[0]] = tiepair[1]
            for par_id in [id for id in range(obj.function.nParams()) if obj.function.isFixed(id)]:
                parName = obj.function.getParamName(par_id)
                if obj.background is not None:
                    parName = parName.split(".")[-1]
                if parName not in self.crystalField.field_parameter_names:
                    continue
                fixes.append(prefix + parName)
        return ties, fixes


# pylint: disable=too-few-public-methods
class CrystalFieldFit(object):
    """
    Object that controls fitting.
    """

    def __init__(self, Model=None, Temperature=None, FWHM=None, InputWorkspace=None, ResolutionModel=None, **kwargs):
        self.model = Model
        if Temperature is not None:
            self.model.Temperature = Temperature
        if FWHM is not None:
            self.model.FWHM = FWHM
        if ResolutionModel is not None:
            self.model.ResolutionModel = ResolutionModel
        self._input_workspace = InputWorkspace
        self._output_workspace_base_name = "fit"
        self._fit_properties = kwargs
        self._function = None
        self._estimated_parameters = None
        self._free_cef_parameters = []
        if self.model.NumberOfSpectra == 1:
            logger.notice("Fit single spectrum")
        else:
            logger.notice("Fit multiple spectra")

    def fit(self):
        """
        Run Fit algorithm. Update function parameters.
        """
        self.check_consistency()
        if isinstance(self._input_workspace, list):
            return self._fit_multi()
        else:
            return self._fit_single()

    def monte_carlo(self, **kwargs):
        fix_all_peaks = self.model.FixAllPeaks
        self.model.FixAllPeaks = True
        if isinstance(self._input_workspace, list):
            self._monte_carlo_multi(**kwargs)
        else:
            self._monte_carlo_single(**kwargs)
        self.model.FixAllPeaks = fix_all_peaks

    def gofit(self, algorithm_callable: Callable, **kwargs) -> None:
        """
        Performs a fit using an algorithm from the GOFit python package.
        @param algorithm_callable: The algorithm callable from the GOFit python package.
        @param kwargs: Keyword arguments. The following keywords are understood:

            - jacobian: A boolean to specify whether to use a Jacobian (regularisation and multistart only).
            - parameter_bounds: A dictionary of tuples containing the upper and lower bounds for each parameter
                                (multistart and alternating only).

        the remaining kwargs are passed to the GOFit algorithm callable.
        """
        # Get the name of the algorithm as the name of the callable python function.
        algorithm_name = algorithm_callable.__name__

        # Find the B parameters and Shape parameters we want to optimize across, and their initial values
        b_parameters, shape_parameters, p0 = self._find_b_and_shape_parameters_to_optimize()
        all_parameters = b_parameters + shape_parameters

        # Read the x, y and error data from the input workspace.
        x, y, e = self._input_workspace.readX(0), self._input_workspace.readY(0), self._input_workspace.readE(0)

        # Find the number of data points, and parameters.
        m = self._input_workspace.getNumberBins(0)
        n = len(all_parameters)

        # Create a wrapper around a callable mantid fitting function
        callable_func = FunctionWrapper(self.model.function, all_parameters)

        def wrapped_func(*params):
            return callable_func(x, *params)

        # Calculates the residual using a non-linear least squares cost function
        def residual(params):
            return np.ravel((y - wrapped_func(*np.array(params))) / e)

        # Pop the 'parameter_bounds' and 'jacobian' arguments
        parameter_bounds = kwargs.pop("parameter_bounds", dict())
        jacobian = kwargs.pop("jacobian", False)

        # Get the algorithm args to use for the specific algorithm we are using
        algorithm_args = [m, n] + self._get_algorithm_args(
            algorithm_name, all_parameters, b_parameters, p0, residual, parameter_bounds, jacobian
        )

        # Attempt to do a fit using one of the GOFit algorithms. A TypeError can occur when provided an invalid kwarg
        try:
            params, status = algorithm_callable(*algorithm_args, **kwargs)
        except TypeError as ex:
            logger.error(str(ex))
        else:
            print(f"GOFit exited with status code {status}.")
            self._process_gofit_output(all_parameters, params, "_" + algorithm_name)

    def _get_algorithm_args(
        self,
        algorithm_name: str,
        all_parameters: List[str],
        b_parameters: List[str],
        p0: List[float],
        residual: Callable,
        parameter_bounds: Dict[str, Tuple[float, float]],
        jacobian: bool,
    ) -> List:
        """Gets the algorithm arguments to be used for a specific GOFit algorithm."""
        algorithm_args = []
        if algorithm_name == "regularisation":
            algorithm_args.append(np.array(p0))
        elif algorithm_name == "alternating":
            algorithm_args.extend([len(b_parameters), np.array(p0)])

        if algorithm_name == "multistart" or algorithm_name == "alternating":
            xl, xu = self._parse_lower_and_upper_bounds(all_parameters, parameter_bounds)
            algorithm_args.extend([np.array(xl), np.array(xu)])

        algorithm_args.append(residual)

        if jacobian and (algorithm_name == "regularisation" or algorithm_name == "multistart"):
            algorithm_args.append(lambda p: approx_derivative(residual, p, method="2-point"))
        return algorithm_args

    def _find_b_and_shape_parameters_to_optimize(self) -> Tuple[List[str], List[str], List[float]]:
        """Finds the B parameters and Shape parameters we want to optimize across."""
        b_params, shape_params, initial_values = [], [], []
        for i in range(self.model.function.nParams()):
            name = self.model.function.parameterName(i)
            value = self.model.function.getParameterValue(i)
            if (name.startswith("B") or name.startswith("IB")) and value != 0.0:
                b_params.append(name)
                initial_values.append(value)
            elif "FWHM" in name or name == "IntensityScaling":
                shape_params.append(name)
                initial_values.append(value)
        return b_params, shape_params, initial_values

    @staticmethod
    def _parse_lower_and_upper_bounds(
        parameters_names: List[str], parameter_bounds: Dict[str, Tuple[float, float]]
    ) -> Tuple[List[float], List[float]]:
        """Parses the lower and upper bounds into two separate lists."""
        xl, xu = [], []
        for name in parameters_names:
            bounds = parameter_bounds.get(name, [0, 1])
            xl.append(bounds[0])
            xu.append(bounds[1])
        return xl, xu

    def _process_gofit_output(self, parameter_names: List[str], parameter_values: List[float], suffix: str = "") -> None:
        """Create an output workspace with the fitted data, and a parameter table."""
        for name, value in zip(parameter_names, parameter_values):
            self.model.function.setParameter(name, value)

        output_name = self._fit_properties["Output"] if "Output" in self._fit_properties else self._output_workspace_base_name
        EvaluateFunction(Function=str(self.model.function), InputWorkspace=self._input_workspace, OutputWorkspace=output_name + suffix)
        self._create_parameter_table(output_name, suffix)

    def _create_parameter_table(self, output_name: str, suffix: str) -> None:
        """Creates a table workspace to display the output parameters from a GOFit."""
        parameter_table = CreateEmptyTableWorkspace(OutputWorkspace=output_name + suffix + "_parameters")
        parameter_table.setTitle("Fit Parameters")
        parameter_table.addColumn("str", "Parameter")
        parameter_table.addColumn("float", "Value")
        for i in range(self.model.function.nParams()):
            parameter_table.addRow([self.model.function.parameterName(i), self.model.function.getParameterValue(i)])

    def two_step_fit(self, OverwriteMaxIterations: list = None, OverwriteMinimizers: list = None, Iterations: int = 20) -> None:
        logger.warning("Please note that this is a first experimental version of the two_step_fit algorithm.")
        fix_all_peaks = self.model.FixAllPeaks
        fit_properties = self._fit_properties
        self._overwrite_maxiterations = OverwriteMaxIterations
        self._overwrite_minimizer = OverwriteMinimizers
        self.check_fit_properties()
        self._iterations = Iterations
        self.check_consistency()
        self.find_free_cef_parameters()
        self._two_step_fit()
        self.model.FixAllPeaks = fix_all_peaks
        self._fit_properties = fit_properties

    def _two_step_fit(self) -> None:
        for iter in range(self._iterations):
            # Fit CEF parameters only
            self.model.FixAllPeaks = True
            self.overwrite_fit_properties(0)
            self.fit()
            self._function = self.model.function
            # Fit peaks only
            for parameter in self._free_cef_parameters:
                self._function.fixParameter(parameter)
            self.model.FixAllPeaks = False
            self.overwrite_fit_properties(1)
            self.fit()
            self._function = self.model.function
            for parameter in self._free_cef_parameters:
                self._function.removeTie(parameter)
            iter += 1

    def two_step_fit_sc(self, OverwriteMaxIterations: list = None, OverwriteMinimizers: list = None, Iterations: int = 20) -> None:
        if isinstance(self.model, CrystalFieldMultiSite):
            logger.notice("The two_step_fit_sc algorithm is only available for single-site calculations at the moment")
            return
        logger.warning("Please note that this is a first experimental version of the two_step_fit_sc algorithm.")
        fix_all_peaks = self.model.FixAllPeaks
        fit_properties = self._fit_properties
        self._overwrite_maxiterations = OverwriteMaxIterations
        self._overwrite_minimizer = OverwriteMinimizers
        self.check_fit_properties()
        self._iterations = Iterations
        if self._overwrite_maxiterations is not None:
            Options = {"disp": False, "maxiter": self._overwrite_maxiterations[0]}
        else:
            Options = {"disp": False}
        self.check_consistency()
        self.find_free_cef_parameters()
        self._two_step_fit_sc(Options)
        self.model.FixAllPeaks = fix_all_peaks
        self._fit_properties = fit_properties

    def _two_step_fit_sc(self, opt: dict = None) -> None:
        for iter in range(self._iterations):
            # Fit CEF parameters only
            if self._overwrite_minimizer is not None:
                self.fit_sp(self._overwrite_minimizer[0], opt)
            else:
                self.fit_sp("L-BFGS-B", opt)
            self._function = self.model.function
            # Fit peaks only
            for parameter in self._free_cef_parameters:
                self._function.fixParameter(parameter)
            self.model.FixAllPeaks = False
            self.overwrite_fit_properties(1)
            self.fit()
            self._function = self.model.function
            for parameter in self._free_cef_parameters:
                self._function.removeTie(parameter)
            iter += 1

    def check_fit_properties(self) -> None:
        if self._overwrite_maxiterations is not None and len(self._overwrite_maxiterations) != 2:
            raise RuntimeError("You must provide two values for overwriting MaxIterations")

        if self._overwrite_minimizer is not None and len(self._overwrite_minimizer) != 2:
            raise RuntimeError("You must provide two values for overwriting Minimizers")

    def overwrite_fit_properties(self, index: int) -> None:
        if index > 1:
            return
        if self._overwrite_maxiterations is not None:
            self._fit_properties["MaxIterations"] = self._overwrite_maxiterations[index]
        if self._overwrite_minimizer is not None:
            self._fit_properties["Minimizer"] = self._overwrite_minimizer[index]

    def find_free_cef_parameters(self) -> None:
        """store free CEF parameters"""
        if isinstance(self.model, CrystalFieldMultiSite):
            fun = self.model.function
        else:
            if self._function is None:
                self._function = self.model.crystalFieldFunction
            fun = self._function
        for par_id in [id for id in range(fun.nParams()) if not fun.isFixed(id)]:
            parName = fun.getParamName(par_id)
            if (parName in CrystalField.field_parameter_names or "IntensityScaling" in parName) and parName not in fun.getTies():
                self._free_cef_parameters.append(par_id)

    def estimate_parameters(self, EnergySplitting, Parameters, **kwargs):
        self.check_consistency()
        if isinstance(self.model, CrystalFieldMultiSite):
            constraints = []
            for ni in range(len(self.model.Ions)):
                pars = Parameters[ni] if islistlike(Parameters[ni]) else Parameters
                ion = self.model.Ions[ni]
                ranges = split2range(Ion=ion, EnergySplitting=EnergySplitting, Parameters=pars)
                constraints += [("%s<ion%d.%s<%s" % (-bound, ni, parName, bound)) for parName, bound in ranges.items()]
        else:
            ranges = split2range(Ion=self.model.Ion, EnergySplitting=EnergySplitting, Parameters=Parameters)
            constraints = [("%s<%s<%s" % (-bound, parName, bound)) for parName, bound in ranges.items()]
        self.model.constraints(*constraints)
        if "Type" not in kwargs or kwargs["Type"] == "Monte Carlo":
            if "OutputWorkspace" in kwargs and kwargs["OutputWorkspace"].strip() != "":
                output_workspace = kwargs["OutputWorkspace"]
            else:
                output_workspace = "estimated_parameters"
                kwargs["OutputWorkspace"] = output_workspace
        else:
            output_workspace = None
        self.monte_carlo(**kwargs)
        if output_workspace is not None:
            self._estimated_parameters = mtd[output_workspace]

    def get_number_estimates(self):
        """
        Get a number of parameter sets estimated with self.estimate_parameters().
        """
        if self._estimated_parameters is None:
            return 0
        else:
            return self._estimated_parameters.columnCount() - 1

    def select_estimated_parameters(self, index):
        ne = self.get_number_estimates()
        if ne == 0:
            raise RuntimeError("There are no estimated parameters.")
        if index > ne:
            raise RuntimeError("There are only %s sets of estimated parameters, requested set #%s" % (ne, index))
        for row in range(self._estimated_parameters.rowCount()):
            name = self._estimated_parameters.cell(row, 0)
            value = self._estimated_parameters.cell(row, index)
            model_pname = name if isinstance(self.model, CrystalFieldMultiSite) else name.split(".")[-1]
            self.model[model_pname] = value
            if self._function is not None:
                self._function.setParameter(name, value)

    def _monte_carlo_single(self, **kwargs):
        """
        Call EstimateFitParameters algorithm in a single spectrum case.
        Args:
            **kwargs: Properties of the algorithm.
        """
        fun = self.model.makeSpectrumFunction()
        if "CrystalFieldMultiSpectrum" in fun:
            # Hack to ensure that 'PhysicalProperties' attribute is first
            # otherwise it won't set up other attributes properly
            fun = re.sub(PHYSICAL_PROPERTIES_PATTERN, r"\1\3\2", fun)
        alg = AlgorithmManager.createUnmanaged("EstimateFitParameters")
        alg.initialize()
        alg.setProperty("Function", fun)
        alg.setProperty("InputWorkspace", self._input_workspace)
        for param in kwargs:
            alg.setProperty(param, kwargs[param])
        alg.execute()
        function = alg.getProperty("Function").value
        self.model.update(function)
        self._function = function

    def _monte_carlo_multi(self, **kwargs):
        """
        Call EstimateFitParameters algorithm in a multi-spectrum case.
        Args:
            **kwargs: Properties of the algorithm.
        """
        fun = self.model.makeMultiSpectrumFunction()
        alg = AlgorithmManager.createUnmanaged("EstimateFitParameters")
        alg.initialize()
        alg.setProperty("Function", fun)
        alg.setProperty("InputWorkspace", self._input_workspace[0])
        i = 1
        for workspace in self._input_workspace[1:]:
            alg.setProperty("InputWorkspace_%s" % i, workspace)
            i += 1
        for param in kwargs:
            alg.setProperty(param, kwargs[param])
        alg.execute()
        function = alg.getProperty("Function").value
        self.model.update(function)
        self._function = function

    def _fit_single(self):
        """
        Fit when the model has a single spectrum.
        """
        if isinstance(self.model, CrystalFieldMultiSite):
            fun = str(self.model.function)
        else:
            if self._function is None:
                if self.model.isPhysicalPropertyOnly:
                    fun = self.model.makePhysicalPropertiesFunction()
                else:
                    fun = self.model.makeSpectrumFunction()
            else:
                fun = str(self._function)
        if "CrystalFieldMultiSpectrum" in fun:
            fun = re.sub(PHYSICAL_PROPERTIES_PATTERN, r"\1\3\2", fun)
        alg = AlgorithmManager.createUnmanaged("Fit")
        alg.initialize()
        alg.setProperty("Function", fun)
        alg.setProperty("InputWorkspace", self._input_workspace)
        alg.setProperty("Output", self._output_workspace_base_name)
        self._set_fit_properties(alg)
        alg.execute()
        function = alg.getProperty("Function").value
        self.model.update(function)
        self.model.chi2 = alg.getProperty("OutputChi2overDoF").value

    def _fit_multi(self):
        """
        Fit when the model has multiple spectra.
        """
        if isinstance(self.model, CrystalFieldMultiSite):
            fun = str(self.model.function)
        else:
            fun = self.model.makeMultiSpectrumFunction()
        alg = AlgorithmManager.createUnmanaged("Fit")
        alg.initialize()
        alg.setProperty("Function", fun)
        alg.setProperty("InputWorkspace", self._input_workspace[0])
        i = 1
        for workspace in self._input_workspace[1:]:
            alg.setProperty("InputWorkspace_%s" % i, workspace)
            i += 1
        alg.setProperty("Output", self._output_workspace_base_name)
        self._set_fit_properties(alg)
        alg.execute()
        function = alg.getProperty("Function").value
        self.model.update(function)
        self.model.chi2 = alg.getProperty("OutputChi2overDoF").value

    def fit_sp(self, Solver: str, Options: dict = None) -> None:
        """
        Run scipy.optimize.minimize algorithm for CEF parameters only. Update function parameters.
        """
        self.check_consistency()
        if isinstance(self.model, CrystalFieldMultiSite):
            fun = self.model.function
        else:
            fun = self._function
        cef_fixed = []
        for par_id in [id for id in range(fun.nParams()) if fun.isFixed(id)]:
            parName = fun.getParamName(par_id)
            if parName in CrystalField.field_parameter_names or "IntensityScaling" in parName:
                cef_fixed.append(par_id)
        x0 = []
        fun.setAttributeValue("FixAllPeaks", True)
        for par_id in self._free_cef_parameters:
            x0.append(fun.getParameterValue(par_id))
            fun.fixParameter(par_id)
        opt = {"disp": False}
        if Options is not None:
            opt = Options
        for x, pos in zip(x0, self._free_cef_parameters):
            fun.removeTie(pos)
            res = sp.minimize(self._evaluate_cf, [x], args=(fun, pos), method=Solver, options=opt)
            fun.fixParameter(pos)
            if res.success:
                if self._function is not None:
                    if isinstance(res.x, list):
                        fun.setParameter(pos, float(res.x[0]))
                    else:
                        fun.setParameter(pos, float(res.x))
        self.model.update(fun)

    def _evaluate_cf(self, x0: float, fun: IFunction, cef_pos: int) -> float:
        fun.setParameter(cef_pos, x0[0])
        if isinstance(self._input_workspace, list):
            ws_kwargs = {}
            ws_kwargs["InputWorkspace"] = self._input_workspace[0]
            i = 1
            for workspace in self._input_workspace[1:]:
                ws_kwargs[f"InputWorkspace_{i}"] = workspace
                i += 1
            # clean up multispectrum function to prevent problems during evaluation
            # e.g. remove FWHMX0/FWHMY0 and FWHMX1/FWHMY1
            fun_str = re.sub(FWHM_PATTERN, "", str(fun))
            # move Temperature and PhysicalProperties settings to front
            fun_str = re.sub(TEMPERATURES_PATTERN, r"\1\3\2", fun_str)
            fun_str = re.sub(PHYSICAL_PROPERTIES_PATTERN, r"\1\3\2", fun_str)
            # remove peaks above MaxPeakCount
            fun_str = re.sub(r"f[0-9]+\.f([" + str(fun.getAttributeValue("MaxPeakCount")) + r"-9]|[1-9][0-9])\.\w+=.*?,", "", fun_str)
            return CalculateChiSquared(fun_str, **ws_kwargs)[1]
        else:
            return CalculateChiSquared(str(fun), self._input_workspace)[1]

    def _set_fit_properties(self, alg):
        for prop in self._fit_properties.items():
            alg.setProperty(*prop)

    def check_consistency(self):
        """Checks that list input variables are consistent"""
        num_ws = self.model.NumberOfSpectra
        errmsg = "Number of input workspaces not consistent with model"
        if islistlike(self._input_workspace):
            if num_ws != len(self._input_workspace):
                raise ValueError(errmsg)
            # If single element list, force use of _fit_single()
            if len(self._input_workspace) == 1:
                self._input_workspace = self._input_workspace[0]
        elif num_ws != 1:
            raise ValueError(errmsg)
        if not isinstance(self.model, CrystalFieldMultiSite) and not self.model.isPhysicalPropertyOnly:
            tt = self.model.Temperature
            if any([val < 0 for val in (tt if islistlike(tt) else [tt])]):
                raise RuntimeError("You must first define a temperature for the spectrum")
