# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import re
from mantid import logger

PARAMETER_NAME_PATTERN = re.compile(r"([a-zA-Z][\w.]+)")


class FunctionParameters(object):
    """
    A helper class that simplifies access to parameters of nested composite fitting functions.
    """

    def __init__(self, function, prefix=""):
        self.function = function
        self.prefix = prefix

    def __getitem__(self, name):
        return self.function.getParameterValue(self.prefix + name)

    def __setitem__(self, name, value):
        self.function.setParameter(self.prefix + name, value)

    def update(self, function):
        self.function = function


class FunctionAttributes(object):
    """
    A helper class that simplifies access to attributes of nested composite fitting functions.
    """

    def __init__(self, function, prefix=""):
        self.function = function
        self.prefix = prefix

    def __getitem__(self, name):
        return self.function.getAttributeValue(self.prefix + name)

    def __setitem__(self, name, value):
        self.function.setAttributeValue(self.prefix + name, value)

    def update(self, function):
        self.function = function


class Function(object):
    """A helper object that simplifies getting and setting parameters of a simple named function."""

    def __init__(self, name_or_function, **kwargs):
        """
        Initialise new instance.
        @param name: A valid name registered with the FunctionFactory.
        @param kwargs: Parameters (but not attributes) of this function. To set attributes use `attr` property.
                Example:
                    f = Function('TabulatedFunction', Scaling=2.0)
                    f.attr['Workspace'] = 'workspace_with_data'
        """
        from mantid.simpleapi import FunctionFactory

        if isinstance(name_or_function, str):
            self.function = FunctionFactory.createFunction(name_or_function)
        else:
            self.function = name_or_function
        if "prefix" in kwargs:
            self.prefix = kwargs["prefix"]
            del kwargs["prefix"]
        else:
            self.prefix = ""
        # Function attributes.
        self._attrib = FunctionAttributes(self.function, self.prefix)
        # Function parameters.
        self._params = FunctionParameters(self.function, self.prefix)
        # The rest of kw arguments are treated as function parameters
        for param in kwargs:
            self._params[param] = kwargs[param]

    @property
    def attr(self):
        return self._attrib

    @property
    def param(self):
        return self._params

    def fix(self, *args):
        """
        Set fixes for the parameters in the function.

        @param args: A list of parameters to fix. Specifying 'all' will fix all of the parameters in a function.
        """
        params = self._validate_parameter_args(*args)

        if "all" in [param.lower() for param in params]:
            self.function.fixAll()
        else:
            for param in params:
                if self.function.hasParameter(self.prefix + param):
                    self.function.fixParameter(self.prefix + param)
                else:
                    logger.warning(f"Cannot fix parameter '{param}' as it does not exist in the Function.")

    @staticmethod
    def _validate_parameter_args(*args):
        """
        Validates the parameter arguments passed to the 'fix' function.
        @param args: The arguments to be validated.
        @return: A list of validate parameter names.
        """
        params = []
        for param in args:
            if not isinstance(param, str):
                logger.warning(f"Ignoring {repr(param)} because a string was expected.")
            else:
                params.append(param)

        if not args:
            logger.warning("No parameters were passed to fix. If you intended to fix all use: fix('all')")

        return params

    def ties(self, **kwargs):
        """Set ties on the parameters.

        @param kwargs: Ties as name=value pairs: name is a parameter name,
            the value is a tie string or a number. For example:
                tie(A0 = 0.1, A1 = '2*A0')
        """
        for param in kwargs:
            if self.function.hasParameter(self.prefix + param):
                self.function.tie(self.prefix + param, str(kwargs[param]))
            else:
                logger.warning(f"Cannot tie parameter '{param}' as it does not exist in the Function.")

    def constraints(self, *args):
        """
        Set constraints for the parameters.

        @param args: A list of constraints. For example:
                constraints('A0 > 0', '0.1 < A1 < 0.9')
        """
        for arg in args:
            constraint = re.sub(PARAMETER_NAME_PATTERN, "%s\\1" % self.prefix, arg)
            self.function.addConstraints(constraint)

    def toString(self):
        """Create function initialisation string"""
        if self.prefix != "":
            raise RuntimeError("Cannot convert to string a part of function")
        return str(self.function)

    def update(self, function):
        """
        Update values of the fitting parameters.
        @param func: A IFunction object containing new parameter values.
        """
        self._attrib.update(function)
        self._params.update(function)


class CompositeProperties(object):
    """
    A helper class that simplifies access of attributes and parameters of a composite function.
    """

    def __init__(self, function, prefix, kind, first_index):
        """
        Constructor.
        Args:
            function: a function that this object provides access to
            prefix: a prefix that is prepended to properties names. This makes it easier to access parameters
                    of a nested composite function.
            kind: a kind of properties accessed: 'attributes' or 'parameters'
            firstIndex: shifts the index of a member function
        """
        self.function = function
        self.prefix = prefix
        self.PropertyType = FunctionAttributes if kind == "attributes" else FunctionParameters
        self.first_index = first_index

    def __getitem__(self, i):
        """
        Get a FunctionParameters or FunctionAttributes object that give access to properties of the i-th
        member function (shifted by self.firstIndex).

        For example:
            function = FunctionFactory.createInitialized('name=Gaussian,Sigma=1;name=Gaussian,Sigma=2')
            params = CompositeProperties(function, '', 'parameters', 0)
            assert params[0]['Sigma'] == 1
            assert params[1]['Sigma'] == 2
            params[1]['Sigma'] = 3
            assert params[1]['Sigma'] == 3
        Args:
            i: index of a member function to get/set parameters
        Returns:
            FunctionParameters or FunctionAttributes object.
        """
        return self.PropertyType(self.function, self.prefix + "f%s." % (i + self.first_index))

    def update(self, function):
        self.function = function

    def ties(self, ties_dict):
        """Set ties on the parameters.

        :param ties_dict: Ties as name=value pairs: name is a parameter name,
            the value is a tie string or a number. For example:
                tie({'A0': 0.1, 'A1': '2*A0'})
        """
        for param, tie in ties_dict.items():
            tie = re.sub(PARAMETER_NAME_PATTERN, "%s\\1" % self.prefix, tie)
            self.function.tie(self.prefix + param, tie)

    def constraints(self, *args):
        """
        Set constraints for the parameters.

        @param args: A list of constraints. For example:
                constraints('A0 > 0', '0.1 < A1 < 0.9')
        """
        for arg in args:
            constraint = re.sub(PARAMETER_NAME_PATTERN, "%s\\1" % self.prefix, arg)
            self.function.addConstraints(constraint)


class PeaksFunction(object):
    """A helper object that simplifies getting and setting parameters of a composite function
    containing multiple peaks of the same spectrum.
    """

    def __init__(self, function, prefix, first_index):
        """
        Constructor.
        :param function: A CrystalField function who's peaks we want to access.
        :param prefix: a prefix of the parameters of the spectrum we want to access.
        :param first_index: Index of the first peak
        """
        # Collection of all attributes
        self._attrib = CompositeProperties(function, prefix, "attributes", first_index)
        # Collection of all parameters
        self._params = CompositeProperties(function, prefix, "parameters", first_index)

    @property
    def attr(self):
        """Get or set the function attributes.
        Returns a FunctionAttributes object that accesses the peaks' attributes.
        """
        return self._attrib

    @property
    def param(self):
        """Get or set the function parameters.
        Returns a FunctionParameters object that accesses the peaks' parameters.
        """
        return self._params

    def ties(self, ties_dict):
        """Set ties on the peak parameters.

        :param ties_dict: Ties as name=value pairs: name is a parameter name,
              the value is a tie string or a number. For example:
              ties({'f1.Sigma': '0.1', 'f2.Sigma': '2*f0.Sigma'})
        """
        self._params.ties(ties_dict)

    def constraints(self, *constraints):
        """
        Set constraints for the peak parameters.

        @param constraints: A list of constraints. For example:
                constraints('f0.Sigma > 0', '0.1 < f1.Sigma < 0.9')
        """
        self._params.constraints(*constraints)

    def tieAll(self, tie, iFirstN, iLast=-1):
        """
        Tie parameters with the same name for all peaks.

        @param tie: A tie as a string. For example:
                tieAll('Sigma=0.1', 3) is equivalent to a call
                ties('f0.Sigma=0.1', 'f1.Sigma=0.1', 'f2.Sigma=0.1')

        @param iFirstN: If iLast is given then it's the index of the first peak to tie.
                Otherwise it's a number of peaks to tie.

        @param iLast: An index of the last peak to tie (inclusive).
        """
        if iLast >= 0:
            start = iFirstN
            end = iLast + 1
        else:
            start = self._params.first_index
            end = iFirstN + self._params.first_index
        name, expr = tuple(tie.split("="))
        name = "f%s." + name.strip()
        expr = expr.strip()
        ties = {(name % i): expr for i in range(start, end)}
        self.ties(ties)

    def constrainAll(self, constraint, iFirstN, iLast=-1):
        """
        Constrain parameters with the same name for all peaks.

        @param constraint: A constraint as a string. For example:
                constrainAll('0 < Sigma <= 0.1', 3) is equivalent to a call
                constrains('0 < f0.Sigma <= 0.1', '0 < f1.Sigma <= 0.1', '0 < f2.Sigma <= 0.1')

        @param iFirstN: If iLast is given then it's the index of the first peak to constrain.
                Otherwise it's a number of peaks to constrain.

        @param iLast: An index of the last peak to tie (inclusive).
        """
        if iLast >= 0:
            start = iFirstN
            end = iLast + 1
        else:
            start = self._params.first_index
            end = iFirstN + self._params.first_index

        pattern = re.sub(PARAMETER_NAME_PATTERN, "f%s.\\1", constraint)
        self.constraints(*[pattern % i for i in range(start, end)])


class Background(object):
    """Object representing spectrum background: a sum of a central peak and a
    background.
    """

    def __init__(self, peak=None, background=None, functions=[]):
        """
        Initialise new instance.
        @param peak: An instance of Function class meaning to be the elastic peak (remains for backward-compatibility).
        @param background: An instance of Function class serving as the background (remains for backward-compatibility).
        @param functions: A list of Function class instances which make up the background.
        """
        self.peak = peak
        self.background = background

        self.functions = []
        if self.peak is not None:
            self.functions.append(self.peak)
        if self.background is not None:
            self.functions.append(self.background)
        self.functions.extend(functions)

    def toString(self):
        """Returns the Background object in string format."""
        if len(self.functions) == 0:
            return ""
        elif len(self.functions) == 1:
            return self.functions[0].toString()
        else:
            return "(" + ";".join([function.toString() for function in self.functions]) + ")"

    def update(self, func, index=0):
        """
        Update values of the fitting parameters. If both arguments are given
            the first one updates the peak and the other updates the background.

        @param func: The IFunction object containing new parameter values.
        @param index: The index of the function to update in the Background object.
        """
        if index < len(self.functions):
            self.functions[index].update(func)
        else:
            raise ValueError(f"Invalid index ({index}) provided: Background is made of {len(self.functions)} Function(s).")


class ResolutionModel:
    """
    Encapsulates a resolution model.
    """

    default_accuracy = 1e-4
    max_model_size = 100

    def __init__(self, model, xstart=None, xend=None, accuracy=None):
        """
        Initialize the model.

        :param model: Either a prepared model or a single python function or a list
                    of functions. If it's functions they must have signatures:
                        func(x: ndarray) -> ndarray
                    A prepared model is a tuple of exactly two arrays of floats of
                    equal sizes or a list of such tuples. The first array in the tuple
                    is the x-values and the second array is the y-values of the resolution
                    model.
        :param xstart:
        :param xend:
        :param accuracy: (Optional) If given and model argument contains functions it's used
                    to tabulate the functions such that linear interpolation between the
                    tabulated points has this accuracy. If not given a default value is used.
        """
        errmsg = "Resolution model must be either a tuple of two arrays, a function, PyChop object or list of one of these"
        self.multi = False
        if hasattr(model, "__call__"):
            self.model = self._makeModel(model, xstart, xend, accuracy)
        elif hasattr(model, "getEi") and hasattr(model, "getResolution"):
            Ei = model.getEi()
            self.model = self._makeModel(model.getResolution, -Ei, 0.9 * Ei, 0.01)
        elif hasattr(model, "model"):
            self.model = model
        elif isinstance(model, tuple):
            self._checkModel(model)
            self.model = model
        elif hasattr(model, "__len__"):
            if len(model) == 0:
                raise RuntimeError("Resolution model cannot be initialised with an empty iterable %s" % str(model))
            if hasattr(model[0], "__call__"):
                self.model = [self._makeModel(m, xstart, xend, accuracy) for m in model]
            elif hasattr(model[0], "model"):
                self.model = [m.model for m in model]
            elif hasattr(model[0], "getEi") and hasattr(model[0], "getResolution"):
                Ei = model[0].getEi()
                self.model = [self._makeModel(m.getResolution, -Ei, 0.9 * Ei, 0.01) for m in model]
            elif isinstance(model[0], tuple):
                for m in model:
                    self._checkModel(m)
                self.model = model
            else:
                raise RuntimeError(errmsg)
            self.multi = True
        else:
            raise RuntimeError(errmsg)

    @property
    def NumberOfSpectra(self):
        if not self.multi:
            return 1
        else:
            return len(self.model)

    def _checkModel(self, model):
        if not isinstance(model, tuple):
            raise RuntimeError("Resolution model must be a tuple of two arrays of floats.\nFound instead:\n\n%s" % str(model))
        if len(model) != 2:
            raise RuntimeError("Resolution model tuple must have exactly two elements.\nFound instead %d" % len(model))
        self._checkArray(model[0])
        self._checkArray(model[1])
        if len(model[0]) != len(model[1]):
            raise RuntimeError(
                "Resolution model expects two arrays of equal sizes.\nFound sizes %d and %d" % (len(model[0]), len(model[1]))
            )

    def _checkArray(self, array):
        if not hasattr(array, "__len__"):
            raise RuntimeError("Expected an array of floats, found %s" % str(array))
        if len(array) == 0:
            raise RuntimeError("Expected a non-empty array of floats.")
        if not isinstance(array[0], float) and not isinstance(array[0], int):
            raise RuntimeError("Expected an array of floats, found %s" % str(array[0]))

    def _mergeArrays(self, a, b):
        import numpy as np

        c = np.empty(2 * len(a) - 1)
        c[::2] = a
        c[1::2] = b
        return c

    def _makeModel(self, model, xstart, xend, accuracy):
        if xstart is None or xend is None:
            raise RuntimeError("The x-range must be provided to ResolutionModel via xstart and xend parameters.")
        import numpy as np

        if accuracy is None:
            accuracy = self.default_accuracy

        n = 5
        acc = accuracy * 2
        x = []
        y = []
        while n < self.max_model_size:
            x = np.linspace(xstart, xend, n)
            y = model(x)
            dx = (x[1] - x[0]) / 2
            xx = np.linspace(xstart + dx, xend - dx, n - 1)
            yi = np.interp(xx, x, y)
            yy = model(xx)
            acc = np.max(np.abs(yy - yi))
            if acc <= accuracy:
                break
            x = self._mergeArrays(x, xx)
            y = self._mergeArrays(y, yy)
            n = len(x)
        return list(x), list(y)


class PhysicalProperties(object):
    """
    Contains information about measurement conditions of physical properties
    """

    HEATCAPACITY = 1
    SUSCEPTIBILITY = 2
    MAGNETISATION = 3
    MAGNETICMOMENT = 4

    def _str2id(self, typeid):
        mappings = [["cp", "cv", "heatcap"], ["chi", "susc"], ["mag", "m(h)"], ["mom", "m(t)"]]
        for id in range(4):
            if any([typeid.lower() in elem for elem in mappings[id]]):
                return id + 1
        return 0

    def __init__(self, typeid, *args, **kwargs):
        """
        Initialize physical properties environment.

        :param typeid: a flag or string (case insensitive) indicating the type of physical properties.
                       "Cp" or "Cv" or "HeatCap*" or 1: Data is heat capacity in J/mol/K
                       "chi" or "susc*" or 2: Data is magnetic susceptibility
                       "mag*" or "M(H)" or 3: Data is magnetisation vs field
                       "mom*" or "M(T)" or 4: Data is magnetic moment vs temperature
        :param hdir: the direction of the applied magnetic field for susceptibiliy or M(T) measurements
        :param hmag: the magnitude in Tesla of the magnetic field for M(T)
        :param temperature: the temperature in Kelvin of measurements of M(H)
        :param inverse: a boolean indicating whether susceptibility is chi or 1/chi or M(T) or 1/M(T)
        :param unit: the unit the data was measured in. Either: 'bohr', 'SI' or 'cgs'.
        :param lambda: (susceptibility only) the value of the exchange constant in inverse susc units
        :param chi0: (susceptibility only) the value of the residual (background) susceptibility

        typeid is required in all cases, and all other parameters may be specified as keyword arguments.
        otherwise the syntax is:

        PhysicalProperties('Cp')  # No further parameters required for heat capacity
        PhysicalProperties('chi', hdir, inverse, unit, lambda, chi0)
        PhysicalProperties('chi', unit)
        PhysicalProperties('mag', hdir, temp, unit)
        PhysicalProperties('mag', unit)
        PhysicalProperties('M(T)', hmag, hdir, inverse, unit)
        PhysicalProperties('M(T)', unit)

        Defaults are: hdir=[0, 0, 1]; hmag=1; temp=1; inverse=False; unit='cgs'; lambda=chi0=0.
        """
        self._physpropUnit = "cgs"
        self._suscInverseFlag = False
        self._hdir = [0.0, 0.0, 1.0]
        self._hmag = 1.0
        self._physpropTemperature = 1.0
        self._lambda = 0.0  # Exchange parameter (for susceptibility only)
        self._chi0 = 0.0  # Residual/background susceptibility (for susceptibility only)
        self._typeid = self._str2id(typeid) if isinstance(typeid, str) else int(typeid)
        try:
            initialiser = getattr(self, "init" + str(self._typeid))
        except AttributeError:
            raise ValueError("Physical property type %s not recognised" % (str(typeid)))
        initialiser(*args, **kwargs)

    def _checkmagunits(self, unit, default=None):
        """Checks that unit string is valid and converts to correct case."""
        if "cgs" in unit.lower():
            return "cgs"
        elif "bohr" in unit.lower():
            return "bohr"
        elif "SI" in unit.upper():
            return "SI"
        elif default is not None:
            return default
        else:
            raise ValueError("Unit %s not recognised" % (unit))

    def _checkhdir(self, hdir):
        import numpy as np

        try:
            if isinstance(hdir, str):
                if "powder" in hdir.lower():
                    return "powder"
                else:
                    raise TypeError()
            else:
                hdir = np.array(hdir)
                if len(hdir) != 3:
                    raise TypeError()
                hdir * hdir  # Catches most cases where elements not numeric...
        except TypeError:
            raise ValueError("Magnetic field direction %s not recognised" % (str(self._hdir)))
        return hdir

    @property
    def TypeID(self):
        return self._typeid

    @property
    def Unit(self):
        return self._physpropUnit

    @Unit.setter
    def Unit(self, value):
        self._physpropUnit = self._checkmagunits(value)

    @property
    def Inverse(self):
        return self._suscInverseFlag if (self._typeid == self.SUSCEPTIBILITY or self._typeid == self.MAGNETICMOMENT) else None

    @Inverse.setter
    def Inverse(self, value):
        if self._typeid == self.SUSCEPTIBILITY or self._typeid == self.MAGNETICMOMENT:
            if isinstance(value, str):
                self._suscInverseFlag = value.lower() in ["true", "t", "1", "yes", "y"]
            else:
                self._suscInverseFlag = bool(value)  # In some cases will always be true...
        else:
            raise NameError("This physical properties does not support the Inverse attribute")

    @property
    def Hdir(self):
        return self._hdir if (self._typeid != self.HEATCAPACITY) else None

    @Hdir.setter
    def Hdir(self, value):
        if self._typeid != self.HEATCAPACITY:
            self._hdir = self._checkhdir(value)

    @property
    def Hmag(self):
        return self._hmag if (self._typeid == self.MAGNETICMOMENT) else None

    @Hmag.setter
    def Hmag(self, value):
        if self._typeid == self.MAGNETICMOMENT:
            self._hmag = float(value)

    @property
    def Temperature(self):
        return self._physpropTemperature if (self._typeid == self.MAGNETISATION) else None

    @Temperature.setter
    def Temperature(self, value):
        if self._typeid == self.MAGNETISATION:
            self._physpropTemperature = float(value)

    @property
    def Lambda(self):
        return self._lambda if (self._typeid == self.SUSCEPTIBILITY) else None

    @Lambda.setter
    def Lambda(self, value):
        if self._typeid == self.SUSCEPTIBILITY:
            self._lambda = float(value)

    @property
    def Chi0(self):
        return self._chi0 if (self._typeid == self.SUSCEPTIBILITY) else None

    @Chi0.setter
    def Chi0(self, value):
        if self._typeid == self.SUSCEPTIBILITY:
            self._chi0 = float(value)

    def init1(self, *args, **kwargs):
        """Initialises environment for heat capacity data"""
        if len(args) > 0:
            raise ValueError("No environment arguments should be specified for heat capacity")

    def _parseargs(self, mapping, *args, **kwargs):
        args = [_f for _f in list(args) if _f]
        # Handles special case of first argument being a unit type
        if len(args) > 0:
            try:
                if self._checkmagunits(args[0], "bad") != "bad":
                    kwargs["Unit"] = args.pop(0)
            except AttributeError:
                pass
        for i in range(len(mapping)):
            if len(args) > i:
                setattr(self, mapping[i], args[i])
            elif mapping[i] in kwargs.keys():
                setattr(self, mapping[i], kwargs[mapping[i]])

    def init2(self, *args, **kwargs):
        """Initialises environment for susceptibility data"""
        mapping = ["Hdir", "Inverse", "Unit", "Lambda", "Chi0"]
        self._parseargs(mapping, *args, **kwargs)

    def init3(self, *args, **kwargs):
        """Initialises environment for M(H) data"""
        mapping = ["Hdir", "Temperature", "Unit"]
        self._parseargs(mapping, *args, **kwargs)

    def init4(self, *args, **kwargs):
        """Initialises environment for M(T) data"""
        mapping = ["Hmag", "Hdir", "Inverse", "Unit"]
        self._parseargs(mapping, *args, **kwargs)

    def toString(self):
        """Create function initialisation string"""
        types = ["CrystalFieldHeatCapacity", "CrystalFieldSusceptibility", "CrystalFieldMagnetisation", "CrystalFieldMoment"]
        out = "name=%s" % (types[self._typeid - 1])
        if self._typeid != self.HEATCAPACITY:
            out += ",Unit=%s" % (self._physpropUnit)
            if "powder" in self._hdir:
                out += ",powder=1"
            else:
                out += ",Hdir=(%s)" % (",".join([str(hh) for hh in self._hdir]))
            if self._typeid == self.MAGNETISATION:
                out += ",Temperature=%s" % (self._physpropTemperature)
            else:  # either susceptibility or M(T)
                out += ",inverse=%s" % (1 if self._suscInverseFlag else 0)
                out += (",Hmag=%s" % (self._hmag)) if self._typeid == self.MAGNETICMOMENT else ""
                if self._typeid == self.SUSCEPTIBILITY and self._lambda != 0:
                    out += ",Lambda=%s" % (self._lambda)
                if self._typeid == self.SUSCEPTIBILITY and self._chi0 != 0:
                    out += ",Chi0=%s" % (self._chi0)
        return out

    def getAttributes(self, dataset=None):
        """Returns a dictionary of PhysicalProperties attributes for use with IFunction"""
        dataset = "" if dataset is None else str(dataset)
        out = {}
        if self._typeid != self.HEATCAPACITY:
            out["Unit%s" % (dataset)] = self._physpropUnit
            if "powder" in self._hdir:
                out["powder%s" % (dataset)] = 1
            else:
                out["Hdir%s" % (dataset)] = [float(hh) for hh in self._hdir]  # needs to be list
            if self._typeid != self.MAGNETISATION:  # either susceptibility or M(T)
                out["inverse%s" % (dataset)] = 1 if self._suscInverseFlag else 0
                if self._typeid == self.MAGNETICMOMENT:
                    out["Hmag%s" % (dataset)] = self._hmag
                if self._typeid == self.SUSCEPTIBILITY:
                    out["Lambda%s" % (dataset)] = self._lambda
                    out["Chi0%s" % (dataset)] = self._chi0
        return out
