# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import numpy as np

from mantid.api import AlgorithmManager
from mantid.fitfunctions import FunctionWrapper, CompositeFunctionWrapper
from mantid.simpleapi import FunctionFactory, plotSpectrum
import CrystalField
from collections import OrderedDict


def makeWorkspace(xArray, yArray, child=True, ws_name="dummy"):
    """
    Create a workspace.
    @param xArray: DataX values
    @param yArray: DataY values
    @param child: if true, the workspace won't appear in the ADS
    @param ws_name: name of the workspace
    """
    alg = AlgorithmManager.createUnmanaged("CreateWorkspace")
    alg.initialize()
    alg.setChild(child)
    alg.setProperty("DataX", xArray)
    alg.setProperty("DataY", yArray)
    alg.setProperty("OutputWorkspace", ws_name)
    alg.execute()
    return alg.getProperty("OutputWorkspace").value


def get_parameters_for_add(cf, new_ion_index):
    """get params from crystalField object to append"""
    ion_prefix = f"ion{new_ion_index}."
    return get_parameters(cf, ion_prefix, "")


def get_parameters_for_add_from_multisite(cfms, new_ion_index):
    """get params from crystalFieldMultiSite object to append"""
    params = {}
    for i in range(len(cfms.Ions)):
        ion_prefix = f"ion{new_ion_index + i}."
        existing_prefix = f"ion{i}." if cfms._isMultiSite() else ""
        params.update(get_parameters(cfms, ion_prefix, existing_prefix))
    return params


def get_parameters(crystal_field, ion_prefix, existing_prefix):
    params = {}
    for bparam in CrystalField.CrystalField.field_parameter_names:
        params[ion_prefix + bparam] = crystal_field[existing_prefix + bparam]
    return params


class CrystalFieldMultiSite(object):
    def __init__(self, Ions, Symmetries, **kwargs):
        self._makeFunction()

        bg_params = {}
        backgroundPeak = kwargs.pop("BackgroundPeak", None)
        if backgroundPeak is not None:
            bg_params["peak"] = backgroundPeak
        background = kwargs.pop("Background", None)
        if background is not None:
            bg_params["background"] = background
        if len(bg_params) > 0:
            self._setBackground(**bg_params)

        self.Ions = Ions
        self.Symmetries = Symmetries
        self._plot_window = {}
        self.chi2 = None
        self._resolutionModel = None

        parameter_dict = kwargs.pop("parameters", None)
        attribute_dict = kwargs.pop("attributes", None)
        ties_dict = kwargs.pop("ties", None)
        constraints_list = kwargs.pop("constraints", None)
        fix_list = kwargs.pop("fixedParameters", None)

        kwargs = self._setMandatoryArguments(kwargs)

        self._abundances = OrderedDict()
        abundances = kwargs.pop("abundances", None)
        self._makeAbundances(abundances)

        self._setRemainingArguments(kwargs)

        self.default_spectrum_size = 200

        if attribute_dict is not None:
            for name, value in attribute_dict.items():
                self.function.setAttributeValue(name, value)
        if parameter_dict is not None:
            for name, value in parameter_dict.items():
                self.function.setParameter(name, value)
        if ties_dict:
            for name, value in parameter_dict.items():
                self.function.tie(name, value)
        if constraints_list:
            self.function.addConstraints(",".join(constraints_list))
        if fix_list:
            for param in fix_list:
                self.function.fixParameter(param)

    def _setMandatoryArguments(self, kwargs):
        if "Temperatures" in kwargs or "Temperature" in kwargs:
            self.Temperatures = kwargs.pop("Temperatures") if "Temperatures" in kwargs else kwargs.pop("Temperature")
            if "FWHM" in kwargs or "FWHMs" in kwargs:
                self.FWHM = kwargs.pop("FWHMs") if "FWHMs" in kwargs else kwargs.pop("FWHM")
            elif "ResolutionModel" in kwargs:
                self.ResolutionModel = kwargs.pop("ResolutionModel")
            else:
                raise RuntimeError("If temperatures are set, must also set FWHM or ResolutionModel")
        return kwargs

    def _setRemainingArguments(self, kwargs):
        possible_args = [
            "ToleranceEnergy",
            "ToleranceIntensity",
            "NPeaks",
            "FWHMVariation",
            "FixAllPeaks",
            "PeakShape",
            "PhysicalProperty",
        ]
        for attribute in possible_args:
            value = kwargs.pop(attribute, None)
            if value is not None:
                setattr(self, attribute, value)

        for key in kwargs:  # Crystal field parameters remain - must be set last
            self.function.setParameter(key, kwargs[key])

    def _isMultiSite(self):
        return len(self.Ions) > 1

    def _makeFunction(self):
        self.function = FunctionFactory.createFunction("CrystalFieldFunction")

    def getParameter(self, param):
        self.function.getParameterValue(param)

    def _getSpectrumTwoArgs(self, arg1, arg2):
        if isinstance(arg1, int):
            i = arg1
            ws = arg2
            ws_index = 0
            if self.Temperatures[i] < 0:
                raise RuntimeError(f"You must first define a valid temperature for spectrum {i}")
        elif isinstance(arg2, int):
            i = 0
            ws = arg1
            ws_index = arg2
        else:
            raise TypeError(f"expected int for one argument in GetSpectrum, got {arg1.__class__.__name__} and {arg2.__class__.__name__}")

        if isinstance(ws, list) or isinstance(ws, np.ndarray):
            ws = self._convertToWS(ws)

        return self._calcSpectrum(i, ws, ws_index)

    def calc_xmin_xmax(self, i=0):
        peaks = np.array([])
        for idx in range(len(self.Ions)):
            blm = {}
            for bparam in CrystalField.CrystalField.field_parameter_names:
                blm[bparam] = self.function.getParameterValue(f"ion{idx}." + bparam)
            _cft = CrystalField.CrystalField(self.Ions[idx], "C1", Temperature=self.Temperatures[i], **blm)
            peaks = np.append(peaks, _cft.getPeakList()[0])
        return np.min(peaks), np.max(peaks)

    def getSpectrum(self, *args, **kwargs):
        """
        Get a specified spectrum calculated with the current field and peak parameters.

        Alternatively can be called getSpectrum(workspace, ws_index). Spectrum index is assumed zero.

        Examples:
            cf.getSpectrum()         # Calculate the first spectrum using automatically generated x-values
            cf.getSpectrum(1)        # Calculate the second spectrum using automatically generated x-values
            cf.getSpectrum(1, ws, 5) # Calculate the second spectrum using the x-values from the 6th spectrum
                                     # in workspace ws.
            cf.getSpectrum(ws)       # Calculate the first spectrum using the x-values from the 1st spectrum
                                     # in workspace ws.
            cf.getSpectrum(ws, 3)    # Calculate the first spectrum using the x-values from the 4th spectrum
                                     # in workspace ws.
            cf.getSpectrum(2, ws)    # Calculate the third spectrum using the x-values from the 1st spectrum
                                     # in workspace ws.
            cf.getSpectrum(x_range=(-2,4)) # Return the first spectrum calculated from -2 to 4.

        @return: A tuple of (x, y) arrays
        """
        i = 0
        xrange = kwargs.get("x_range")
        if xrange is not None:
            x_min, x_max = xrange
        else:
            if len(args) == 3:
                if self.Temperatures[args[0]] < 0:
                    raise RuntimeError("You must first define a temperature for the spectrum")
                return self._calcSpectrum(args[0], args[1], args[2])
            elif len(args) == 1:
                if isinstance(args[0], int):
                    x_min, x_max = self.calc_xmin_xmax(args[0])
                    i = args[0]
                else:
                    return self._calcSpectrum(0, args[0], 0)
            elif len(args) == 2:
                return self._getSpectrumTwoArgs(*args)
            else:
                x_min, x_max = self.calc_xmin_xmax()

        xArray = np.linspace(x_min, x_max, self.default_spectrum_size)
        return self._calcSpectrum(i, xArray, 0)

    def _convertToWS(self, wksp_list):
        """
        converts a list or numpy array to workspace
        @param wksp_list: A list or ndarray used to make the workspace
        """
        xArray = wksp_list
        yArray = np.zeros_like(xArray)
        return makeWorkspace(xArray, yArray)

    def _calcSpectrum(self, i, workspace, ws_index):
        """Calculate i-th spectrum.

        @param i: Index of a spectrum or function string
        @param workspace: A workspace / list / ndarray used to evaluate the spectrum function
        @param ws_index:  An index of a spectrum in workspace to use.
        """
        if isinstance(workspace, list) or isinstance(workspace, np.ndarray):
            workspace = self._convertToWS(workspace)
        alg = AlgorithmManager.createUnmanaged("EvaluateFunction")
        alg.initialize()
        alg.setChild(True)
        alg.setProperty("Function", self.makeSpectrumFunction(i))
        alg.setProperty("InputWorkspace", workspace)
        alg.setProperty("WorkspaceIndex", ws_index)
        alg.setProperty("OutputWorkspace", "dummy")
        alg.execute()
        out = alg.getProperty("OutputWorkspace").value
        # Create copies of the x and y because `out` goes out of scope when this method returns
        # and x and y get deallocated
        return np.array(out.readX(0)), np.array(out.readY(1))

    def makeSpectrumFunction(self, i=0):
        """Form a definition string for the CrystalFieldSpectrum function
        @param i: Index of a spectrum.
        """
        if self.NumberOfSpectra == 1:
            return str(self.function)
        else:
            funs = self.function.createEquivalentFunctions()
            return str(funs[i])

    def _makeAbundances(self, abundances):
        """Create dict for ion intensity scalings"""
        if abundances is not None:
            for ion_index in range(len(self.Ions)):
                self._abundances[f"ion{ion_index}"] = abundances[ion_index]
            max_ion = max(self._abundances, key=lambda key: self._abundances[key])
            ties = {}
            for ion in self._abundances.keys():
                if ion is not max_ion:
                    factor = self._abundances[ion] / self._abundances[max_ion]
                    tie_key = ion + ".IntensityScaling"
                    tie_value = str(factor) + "*" + max_ion + ".IntensityScaling"
                    ties[tie_key] = tie_value
            self.ties(ties)
        else:
            for ion_index in range(len(self.Ions)):
                self._abundances[f"ion{ion_index}"] = 1.0

    def update(self, func):
        """
        Update values of the fitting parameters.
        @param func: A IFunction object containing new parameter values.
        """
        self.function = func

    def fix(self, *args):
        for a in args:
            self.function.fixParameter(a)

    def ties(self, *args, **kwargs):
        """
        Set ties on the field parameters.

        @param kwargs: Ties as name=value pairs: name is a parameter name,
            the value is a tie string or a number. For example:
                tie(B20 = 0.1, IB23 = '2*B23')
        """
        for arg in args:
            if isinstance(arg, dict):
                kwargs.update(arg)
            else:
                raise TypeError("")
        for tie in kwargs:
            self.function.tie(tie, str(kwargs[tie]))

    def constraints(self, *args):
        """
        Set constraints for the field parameters.

        @param args: A list of constraints. For example:
                constraints('B00 > 0', '0.1 < B43 < 0.9')
        """
        self.function.addConstraints(",".join(args))

    def plot(self, *args):
        """Plot a spectrum. Parameters are the same as in getSpectrum(...) with additional name argument"""
        ws_name = args[3] if len(args) == 4 else f"CrystalFieldMultiSite_{self.Ions}"
        xArray, yArray = self.getSpectrum(*args)
        if len(args) > 0:
            ws_name += f"_{args[0]}"
            if isinstance(args[0], int):
                ws_name += f"_{args[1]}"
        makeWorkspace(xArray, yArray, child=False, ws_name=ws_name)
        plotSpectrum(ws_name, 0)

    def _setBackground(self, **kwargs):
        """
        Set background function(s).

        Can provide one argument or both. Each argument can be a string or FunctionWrapper object.
        Can also pass two functions as one argument by passing a single string or CompositeFunctionWrapper.

        Examples:
        setBackground(Gaussian())
        setBackground(background=LinearBackground())
        setBackground(peak='name=Gaussian,Height=1', background='name=LinearBackground')
        setBackground(Gaussian(), 'name=LinearBackground')
        setBackground(Gaussian() + LinearBackground())
        setBackground('name=Gaussian,Height=0,PeakCentre=1,Sigma=0;name=LinearBackground,A0=0,A1=0')

        @param peak: A function passed as the peak. Can be a string or FunctionWrapper e.g.
                'name=Gaussian,Height=0,PeakCentre=1,Sigma=0' or Gaussian(PeakCentre=1)
        @param background: A function passed as the background. Can be a string or FunctionWrapper e.g.
                'name=LinearBackground,A0=1' or LinearBackground(A0=1)
        """
        self._background = CrystalField.Function(self.function, prefix="bg.")
        if len(kwargs) == 2:
            self._setCompositeBackground(kwargs["peak"], kwargs["background"])
        elif len(kwargs) == 1:
            if "peak" in kwargs.keys():
                self._setSingleBackground(kwargs["peak"], "peak")
            elif "background" in kwargs.keys():
                self._setSingleBackground(kwargs["background"], "background")
            else:
                raise RuntimeError("_setBackground expects peak or background arguments only")
        else:
            raise RuntimeError(f"_setBackground takes 1 or 2 arguments, got {len(kwargs)}")

    def _setSingleBackground(self, background, property_name):
        if isinstance(background, str):
            self._setBackgroundUsingString(background, property_name)
        elif isinstance(background, CompositeFunctionWrapper):
            if len(background) == 2:
                peak, background = str(background).split(";")
                self._setCompositeBackground(peak, background)
            else:
                raise ValueError(f"composite function passed to background must have exactly 2 functions, got {len(background)}")
        elif isinstance(background, FunctionWrapper):
            setattr(self._background, property_name, CrystalField.Function(self.function, prefix="bg."))
            self.function.setAttributeValue("Background", str(background))
        else:
            raise TypeError("background argument(s) must be string or function object(s)")

    def _setCompositeBackground(self, peak, background):
        self._background.peak = CrystalField.Function(self.function, prefix="bg.f0.")
        self._background.background = CrystalField.Function(self.function, prefix="bg.f1.")
        self.function.setAttributeValue("Background", f"{peak};{background}")

    def _setBackgroundUsingString(self, background, property_name):
        number_of_functions = background.count(";") + 1
        if number_of_functions == 2:
            peak, background = background.split(";")
            self._setCompositeBackground(peak, background)
        elif number_of_functions == 1:
            setattr(self._background, property_name, CrystalField.Function(self.function, prefix="bg."))
            self.function.setAttributeValue("Background", background)
        else:
            raise ValueError(f"string passed to background must have exactly 1 or 2 functions, got {number_of_functions}")

    def _combine_multisite(self, other):
        """Used to add two CrystalFieldMultiSite"""
        ions = self.Ions + other.Ions
        symmetries = self.Symmetries + other.Symmetries
        abundances = list(self._abundances.values()) + list(other._abundances.values())
        params = get_parameters_for_add_from_multisite(self, 0)
        params.update(get_parameters_for_add_from_multisite(other, len(self.Ions)))
        new_cf = CrystalFieldMultiSite(
            Ions=ions, Symmetries=symmetries, Temperatures=self.Temperatures, FWHM=self.FWHM, parameters=params, abundances=abundances
        )
        return new_cf

    def __getitem__(self, item):
        if self.function.hasAttribute(item):
            return self.function.getAttributeValue(item)
        else:
            return self.function.getParameterValue(item)

    def __setitem__(self, key, value):
        self.function.setParameter(key, value)

    def __add__(self, other):
        scale_factor = 1.0
        if hasattr(other, "abundance"):  # is CrystalFieldSite
            scale_factor = other.abundance
            other = other.crystalField
        elif isinstance(other, CrystalFieldMultiSite):
            return self._combine_multisite(other)
        if not isinstance(other, CrystalField.CrystalField):
            raise TypeError(f"Unsupported operand type(s) for +: CrystalFieldMultiSite and {other.__class__.__name__}")
        ions = self.Ions + [other.Ion]
        symmetries = self.Symmetries + [other.Symmetry]
        abundances = list(self._abundances.values()) + [scale_factor]
        params = get_parameters_for_add_from_multisite(self, 0)
        params.update(get_parameters_for_add(other, len(self.Ions)))
        new_cf = CrystalFieldMultiSite(
            Ions=ions, Symmetries=symmetries, Temperatures=self.Temperatures, FWHM=self.FWHM, parameters=params, abundances=abundances
        )
        return new_cf

    def __radd__(self, other):
        scale_factor = 1.0
        if hasattr(other, "abundance"):  # is CrystalFieldSite
            scale_factor = other.abundance
            other = other.crystalField
        if not isinstance(other, CrystalField.CrystalField):
            raise TypeError(f"Unsupported operand type(s) for +: CrystalFieldMultiSite and {other.__class__.__name__}")
        ions = [other.Ion] + self.Ions
        symmetries = [other.Symmetry] + self.Symmetries
        abundances = [scale_factor] + list(self._abundances.values())
        params = get_parameters_for_add(other, 0)
        params.update(get_parameters_for_add_from_multisite(self, 1))
        new_cf = CrystalFieldMultiSite(
            Ions=ions, Symmetries=symmetries, Temperatures=self.Temperatures, FWHM=self.FWHM, parameters=params, abundances=abundances
        )
        return new_cf

    @property
    def background(self):
        return self._background

    @background.setter
    def background(self, value):
        if hasattr(value, "peak") and hasattr(value, "background"):
            # Input is a CrystalField.Background object
            if value.peak and value.background:
                self._setBackground(peak=str(value.peak.function), background=str(value.background.function))
            elif value.peak:
                self._setBackground(peak=str(value.peak.function))
            else:
                self._setBackground(background=str(value.background.function))
        elif hasattr(value, "function"):
            self._setBackground(background=str(value.function))
        else:
            self._setBackground(background=value)
        # Need this for a weird python bug: "IndexError: Function index (2) out of range (2)"
        # if user calls print(self.function) after setting background
        _ = self.function.getTies()

    @property
    def Ions(self):
        string_ions = self.function.getAttributeValue("Ions")
        string_ions = string_ions[1:-1]
        return string_ions.split(",")

    @Ions.setter
    def Ions(self, value):
        if isinstance(value, str):
            self.function.setAttributeValue("Ions", value)
        else:
            self.function.setAttributeValue("Ions", ",".join(value))

    @property
    def Symmetries(self):
        string_symmetries = self.function.getAttributeValue("Symmetries")
        string_symmetries = string_symmetries[1:-1]
        return string_symmetries.split(",")

    @Symmetries.setter
    def Symmetries(self, value):
        if isinstance(value, str):
            self.function.setAttributeValue("Symmetries", value)
        else:
            self.function.setAttributeValue("Symmetries", ",".join(value))

    @property
    def ToleranceEnergy(self):
        """Get energy tolerance"""
        return self.function.getAttributeValue("ToleranceEnergy")

    @ToleranceEnergy.setter
    def ToleranceEnergy(self, value):
        """Set energy tolerance"""
        self.function.setAttributeValue("ToleranceEnergy", float(value))

    @property
    def ToleranceIntensity(self):
        """Get intensity tolerance"""
        return self.function.getAttributeValue("ToleranceIntensity")

    @ToleranceIntensity.setter
    def ToleranceIntensity(self, value):
        """Set intensity tolerance"""
        self.function.setAttributeValue("ToleranceIntensity", float(value))

    @property
    def Temperatures(self):
        return list(self.function.getAttributeValue("Temperatures"))

    @Temperatures.setter
    def Temperatures(self, value):
        self.function.setAttributeValue("Temperatures", value)

    @property
    def Temperature(self):
        return list(self.function.getAttributeValue("Temperatures"))

    @Temperature.setter
    def Temperatures(self, value):
        self.function.setAttributeValue("Temperatures", value)

    @property
    def FWHMs(self):
        fwhm = self.function.getAttributeValue("FWHMs")
        nDatasets = len(self.Temperatures)
        if len(fwhm) != nDatasets:
            return list(fwhm) * nDatasets
        return list(fwhm)

    @FWHMs.setter
    def FWHMs(self, value):
        if CrystalField.fitting.islistlike(value):
            if len(value) != len(self.Temperatures):
                value = [value[0]] * len(self.Temperatures)
        else:
            value = [value] * len(self.Temperatures)
        self.function.setAttributeValue("FWHMs", value)
        self._resolutionModel = None

    @property
    def FWHM(self):
        return self.FWHMs

    @FWHM.setter
    def FWHM(self, value):
        self.FWHMs = value

    @property
    def ResolutionModel(self):
        return self._resolutionModel

    @ResolutionModel.setter
    def ResolutionModel(self, value):
        if hasattr(value, "model"):
            self._resolutionModel = value
        else:
            self._resolutionModel = CrystalField.ResolutionModel(value)
        nSpec = len(self.Temperatures)
        if nSpec > 1:
            if not self._resolutionModel.multi or self._resolutionModel.NumberOfSpectra != nSpec:
                raise RuntimeError(
                    "Resolution model is expected to have %s functions, found %s" % (nSpec, self._resolutionModel.NumberOfSpectra)
                )
            for i in range(nSpec):
                model = self._resolutionModel.model[i]
                self.function.setAttributeValue("sp%i.FWHMX" % i, model[0])
                self.function.setAttributeValue("sp%i.FWHMY" % i, model[1])
        else:
            model = self._resolutionModel.model
            self.function.setAttributeValue("FWHMX", model[0])
            self.function.setAttributeValue("FWHMY", model[1])

    @property
    def FWHMVariation(self):
        return self.function.getAttributeValue("FWHMVariation")

    @FWHMVariation.setter
    def FWHMVariation(self, value):
        self.function.setAttributeValue("FWHMVariation", float(value))

    @property
    def FixAllPeaks(self):
        return self.function.getAttributeValue("FixAllPeaks")

    @FixAllPeaks.setter
    def FixAllPeaks(self, value):
        self.function.setAttributeValue("FixAllPeaks", value)

    @property
    def PeakShape(self):
        return self.function.getAttributeValue("PeakShape")

    @PeakShape.setter
    def PeakShape(self, value):
        self.function.setAttributeValue("PeakShape", value)

    @property
    def NumberOfSpectra(self):
        return self.function.getNumberDomains()

    @property
    def NPeaks(self):
        return self.function.getAttributeValue("NPeaks")

    @NPeaks.setter
    def NPeaks(self, value):
        self.function.setAttributeValue("NPeaks", value)
