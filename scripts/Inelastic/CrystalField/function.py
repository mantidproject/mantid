from __future__ import (absolute_import, division, print_function)
import re

parNamePattern = re.compile(r'([a-zA-Z][\w.]+)')


class FunctionParameters(object):
    """
    A helper class that simplifies access to parameters of nested composite fitting functions.
    """
    def __init__(self, function, prefix=''):
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
    def __init__(self, function, prefix=''):
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
            self._name = name_or_function
            self.function = FunctionFactory.createFunction(self._name)
        else:
            self.function = name_or_function
        if 'prefix' in kwargs:
            self.prefix = kwargs['prefix']
            del kwargs['prefix']
        else:
            self.prefix = ''
        # Function attributes.
        self._attrib = FunctionAttributes(self.function, self.prefix)
        # Function parameters.
        self._params = FunctionParameters(self.function, self.prefix)
        # The rest of kw arguments are treated as function parameters
        for param in kwargs:
            self._params[param] = kwargs[param]

        # self._ties = {}
        # self._constraints = []

    # def copyFrom(self, attrib, params, ties, constraints):
    #     """Make shallow copies of the member collections"""
    #     from copy import copy
    #     self._attrib = copy(attrib)
    #     self._params = copy(params)
    #     self._ties = copy(ties)
    #     self._constraints = copy(constraints)
    #
    # def clone(self):
    #     """Make a copy of self."""
    #     function = Function(self._name)
    #     # Make shallow copies of the member collections
    #     function.copyFrom(self._attrib, self._params, self._ties, self._constraints)
    #     return function

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

    def ties(self, **kwargs):
        """Set ties on the parameters.

        @param kwargs: Ties as name=value pairs: name is a parameter name,
            the value is a tie string or a number. For example:
                tie(A0 = 0.1, A1 = '2*A0')
        """
        for param in kwargs:
            self.function.tie(self.prefix + param, str(kwargs[param]))

    def constraints(self, *args):
        """
        Set constraints for the parameters.

        @param args: A list of constraints. For example:
                constraints('A0 > 0', '0.1 < A1 < 0.9')
        """
        for arg in args:
            constraint = re.sub(parNamePattern, '%s\\1' % self.prefix, arg)
            self.function.addConstraints(constraint)

    def toString(self):
        """Create function initialisation string"""
        if self.prefix != '':
            raise RuntimeError('Cannot convert to string a part of function')
        return str(self.function)
    #     attrib = ['%s=%s' % item for item in self._attrib.items()] + \
    #              ['%s=%s' % item for item in self._params.items()]
    #     if len(attrib) > 0:
    #         out = 'name=%s,%s' % (self._name, ','.join(attrib))
    #     else:
    #         out = 'name=%s' % self._name
    #     ties = ','.join(['%s=%s' % item for item in self._ties.items()])
    #     if len(ties) > 0:
    #         out += ',ties=(%s)' % ties
    #     constraints = ','.join(self._constraints)
    #     if len(constraints) > 0:
    #         out += ',constraints=(%s)' % constraints
    #     return out
    #
    # def paramString(self, prefix):
    #     """Create a string with only parameters and attributes settings.
    #         The prefix is prepended to all attribute names.
    #     """
    #     attrib = ['%s%s=%s' % ((prefix,) + item) for item in self._attrib.items()] + \
    #              ['%s%s=%s' % ((prefix,) + item) for item in self._params.items()]
    #     return ','.join(attrib)
    #
    # def tiesString(self, prefix):
    #     """Create a string with only ties settings.
    #         The prefix is prepended to all parameter names.
    #     """
    #     ties = ['%s%s=%s' % ((prefix,) + item) for item in self._ties.items()]
    #     return ','.join(ties)
    #
    # def constraintsString(self, prefix):
    #     """Create a string with only constraints settings.
    #         The prefix is prepended to all parameter names.
    #     """
    #     if len(prefix) > 0:
    #         constraints = []
    #         for constraint in self._constraints:
    #             constraint = re.sub(parNamePattern, prefix + '\\1', constraint)
    #             constraints.append(constraint)
    #     else:
    #         constraints = self._constraints
    #     return ','.join(constraints)
    #

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
        self.PropertyType = FunctionAttributes if kind == 'attributes' else FunctionParameters
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
        return self.PropertyType(self.function, self.prefix + 'f%s.' % (i + self.first_index))

    def update(self, function):
        self.function = function

    def ties(self, **kwargs):
        """Set ties on the parameters.

        @param kwargs: Ties as name=value pairs: name is a parameter name,
            the value is a tie string or a number. For example:
                tie(A0 = 0.1, A1 = '2*A0')
        """
        for param in kwargs:
            self.function.tie(self.prefix + param, str(kwargs[param]))

    def constraints(self, *args):
        """
        Set constraints for the parameters.

        @param args: A list of constraints. For example:
                constraints('A0 > 0', '0.1 < A1 < 0.9')
        """
        for arg in args:
            constraint = re.sub(parNamePattern, '%s\\1' % self.prefix, arg)
            print (constraint)
            self.function.addConstraints(constraint)
        print (self.function)

    # def getSize(self):
    #     """Get number of maps (functions) defined here"""
    #     keys = list(self._properties.keys())
    #     if len(keys) > 0:
    #         return max(keys) + 1
    #     return 0
    #
    # def toStringList(self):
    #     """Format all properties into a list of strings where each string is a comma-separated
    #     list of name=value pairs.
    #     """
    #     prop_list = []
    #     for i in range(self.getSize()):
    #         if i in self._properties:
    #             props = self._properties[i]
    #             prop_list.append(','.join(['%s=%s' % item for item in sorted(props.items())]))
    #         else:
    #             prop_list.append('')
    #     return prop_list
    #
    # def toCompositeString(self, prefix, shift=0):
    #     """Format all properties as a comma-separated list of name=value pairs where name is formatted
    #     in the CompositeFunction style.
    #
    #     Example:
    #         'f0.Height=100,f0.Sigma=1.0,f1.Height=120,f1.Sigma=2.0,f5.Height=300,f5.Sigma=3.0'
    #     """
    #     out = ''
    #     for i in self._properties:
    #         fullPrefix = '%sf%s.' % (prefix, i + shift)
    #         props = self._properties[i]
    #         if len(out) > 0:
    #             out += ','
    #         out += ','.join(['%s%s=%s' % ((fullPrefix,) + item) for item in sorted(props.items())])
    #     return out[:]


class PeaksFunction(object):
    """A helper object that simplifies getting and setting parameters of a composite function
    containing multiple peaks of the same spectrum.
    """

    def __init__(self, crystalField, prefix):
        """
        Constructor.
        @param crystalField: A CrystalField object who's peaks we want to access.
        @param prefix: a prefix of the parameters of the spectrum we want to access.
        """
        # Index of the first peak
        first_index = 1 if crystalField.isMultiSpectrum() else 0
        # Collection of all attributes
        self._attrib = CompositeProperties(crystalField.function, prefix, 'attributes', first_index)
        # Collection of all parameters
        self._params = CompositeProperties(crystalField.function, prefix, 'parameters', first_index)
        # # Ties
        # self._ties = []
        # # Constraints
        # self._constraints = []
        self.crystalField = crystalField


    # @property
    # def name(self):
    #     """Read only name of the peak function"""
    #     return self._name

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

    def ties(self, **kwargs):
        """Set ties on the peak parameters.

        @param ties: A list of ties. For example:
                ties('f1.Sigma=0.1', 'f2.Sigma=2*f0.Sigma')
        """
        self._params.ties(**kwargs)

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
            start = self._firstIndex
            end = iFirstN + self._firstIndex
        pattern = 'f%s.' + tie
        ties = [pattern % i for i in range(start, end)]
        self.ties(*ties)

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
            start = self._firstIndex
            end = iFirstN + self._firstIndex

        pattern = re.sub(parNamePattern, 'f%s.\\1', constraint)
        self.constraints(*[pattern % i for i in range(start, end)])

    def nPeaks(self):
        """Get the number of peaks"""
        numPeaks = max(self._attrib.getSize(), self._params.getSize())
        if numPeaks == 0:
            raise RuntimeError('PeaksFunction has no defined parameters or attributes.')
        return numPeaks

    def toString(self):
        """Create function initialisation string"""
        numPeaks = self.nPeaks()
        attribs = self._attrib.toStringList()
        params = self._params.toStringList()
        if len(attribs) < numPeaks:
            attribs += [''] * (numPeaks - len(attribs))
        if len(params) < numPeaks:
            params += [''] * (numPeaks - len(params))
        peaks = []
        for i in range(numPeaks):
            attrib = attribs[i]
            param = params[i]
            if len(attrib) != 0 or len(param) != 0:
                if len(attrib) == 0:
                    peaks.append('name=%s,%s' % (self._name, param))
                elif len(param) == 0:
                    peaks.append('name=%s,%s' % (self._name, attrib))
                else:
                    peaks.append('name=%s,%s,%s' % (self._name, attrib,param))
            else:
                peaks.append('name=%s' % self._name)
        out = ';'.join(peaks)
        if len(self._ties) > 0:
            out += ';%s' % self.tiesString()
        return out

    def paramString(self, prefix='', shift=0):
        """Format a comma-separated list of all peaks attributes and parameters in a CompositeFunction
        style.
        """
        numAttributes = self._attrib.getSize()
        numParams = self._params.getSize()
        if numAttributes == 0 and numParams == 0:
            return ''
        elif numAttributes == 0:
            return self._params.toCompositeString(prefix, shift)
        elif numParams == 0:
            return self._attrib.toCompositeString(prefix, shift)
        else:
            return '%s,%s' % (self._attrib.toCompositeString(prefix, shift),
                              self._params.toCompositeString(prefix, shift))

    def tiesString(self, prefix=''):
        if len(self._ties) > 0:
            ties = ','.join(self._ties)
            return 'ties=(%s)' % re.sub(parNamePattern, prefix + '\\1', ties)
        return ''

    def constraintsString(self, prefix=''):
        if len(self._constraints) > 0:
            constraints = ','.join(self._constraints)
            return 'constraints=(%s)' % re.sub(parNamePattern, prefix + '\\1', constraints)
        return ''


class Background(object):
    """Object representing spectrum background: a sum of a central peak and a
    background.
    """

    def __init__(self, peak=None, background=None):
        """
        Initialise new instance.
        @param peak: An instance of Function class meaning to be the elastic peak.
        @param background: An instance of Function class serving as the background.
        """
        self.peak = peak
        self.background = background

    def clone(self):
        """Make a copy of self."""
        aCopy = Background()
        if self.peak is not None:
            aCopy.peak = self.peak.clone()
        if self.background is not None:
            aCopy.background = self.background.clone()
        return aCopy

    # def __mul__(self, nCopies):
    #     """Make expressions like Background(...) * 8 return a list of 8 identical backgrounds."""
    #     copies = [self] * nCopies
    #     return list(map(Background.clone, copies))
    #     # return [self.clone() for i in range(nCopies)]
    #
    # def __rmul__(self, nCopies):
    #     """Make expressions like 2 * Background(...) return a list of 2 identical backgrounds."""
    #     return self.__mul__(nCopies)

    def toString(self):
        if self.peak is None and self.background is None:
            return ''
        if self.peak is None:
            return self.background.toString()
        if self.background is None:
            return self.peak.toString()
        return '(%s;%s)' % (self.peak.toString(), self.background.toString())

    def nameString(self):
        if self.peak is None and self.background is None:
            return ''
        if self.peak is None:
            return self.background.name
        if self.background is None:
            return self.peak.name
        return '"name=%s;name=%s"' % (self.peak.name, self.background.name)

    def paramString(self, prefix):
        if self.peak is None and self.background is None:
            return ''
        if self.peak is None:
            return self.background.paramString(prefix)
        if self.background is None:
            return self.peak.paramString(prefix)
        return '%s,%s' % (self.peak.paramString(prefix + 'f0.'), self.background.paramString(prefix + 'f1.'))

    def tiesString(self, prefix):
        if self.peak is None and self.background is None:
            return ''
        if self.peak is None:
            return self.background.tiesString(prefix)
        if self.background is None:
            return self.peak.tiesString(prefix)
        peakString = self.peak.tiesString(prefix + 'f0.')
        backgroundString = self.background.tiesString(prefix + 'f1.')
        if len(peakString) == 0:
            return backgroundString
        elif len(backgroundString) == 0:
            return peakString
        else:
            return '%s,%s' % (peakString, backgroundString)

    def constraintsString(self, prefix):
        if self.peak is None and self.background is None:
            return ''
        if self.peak is None:
            return self.background.constraintsString(prefix)
        if self.background is None:
            return self.peak.constraintsString(prefix)
        peakString = self.peak.constraintsString(prefix + 'f0.')
        backgroundString = self.background.constraintsString(prefix + 'f1.')
        if len(peakString) == 0:
            return backgroundString
        elif len(backgroundString) == 0:
            return peakString
        else:
            return '%s,%s' % (peakString, backgroundString)

    def update(self, func1, func2=None):
        """
        Update values of the fitting parameters. If both arguments are given
            the first one updates the peak and the other updates the background.

        @param func1: First IFunction object containing new parameter values.
        @param func2: Second IFunction object containing new parameter values.
        """
        if func2 is not None:
            if self.peak is None or self.background is None:
                raise RuntimeError('Background has peak or background undefined.')
            self.peak.update(func1)
            self.background.update(func2)
        elif self.peak is None:
            self.background.update(func1)
        else:
            self.peak.update(func1)


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
        self.multi = False
        if hasattr(model, '__call__'):
            self.model = self._makeModel(model, xstart, xend, accuracy)
            return
        elif hasattr(model, '__len__'):
            if len(model) == 0:
                raise RuntimeError('Resolution model cannot be initialised with an empty iterable %s' %
                                   str(model))
            if hasattr(model[0], '__call__'):
                self.model = [self._makeModel(m, xstart, xend, accuracy) for m in model]
                self.multi = True
                return
            elif isinstance(model[0], tuple):
                for m in model:
                    self._checkModel(m)
                self.model = model
                self.multi = True
                return
        self._checkModel(model)
        self.model = model

    def _checkModel(self, model):
        if not isinstance(model, tuple):
            raise RuntimeError('Resolution model must be a tuple of two arrays of floats.\n'
                               'Found instead:\n\n%s' % str(model))
        if len(model) != 2:
            raise RuntimeError('Resolution model tuple must have exactly two elements.\n'
                               'Found instead %d' % len(model))
        self._checkArray(model[0])
        self._checkArray(model[1])
        if len(model[0]) != len(model[1]):
            raise RuntimeError('Resolution model expects two arrays of equal sizes.\n'
                               'Found sizes %d and %d' % (len(model[0]), len(model[1])))

    def _checkArray(self, array):
        if not hasattr(array, '__len__'):
            raise RuntimeError('Expected an array of floats, found %s' % str(array))
        if len(array) == 0:
            raise RuntimeError('Expected a non-empty array of floats.')
        if not isinstance(array[0], float) and not isinstance(array[0], int):
            raise RuntimeError('Expected an array of floats, found %s' % str(array[0]))

    def _mergeArrays(self, a, b):
        import numpy as np
        c = np.empty(2 * len(a) - 1)
        c[::2] = a
        c[1::2] = b
        return c

    def _makeModel(self, model, xstart, xend, accuracy):
        if xstart is None or xend is None:
            raise RuntimeError('The x-range must be provided to ResolutionModel via '
                               'xstart and xend parameters.')
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
        return x, y
