# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
"""Defines a set of helpers wrapped around the C++ TestHelpers package that
are for use in unit tests only!
"""
from __future__ import (absolute_import, division,
                        print_function)

from distutils.version import LooseVersion

# Import mantid to set MANTIDPATH for any ConfigService call that may be done
import mantid  # noqa
# Add workspace creation namespace
import WorkspaceCreationHelper

import numpy

# Define some pure-Python functions to add to the mix

def run_algorithm(name, **kwargs):
    """Run a named algorithm and return the
    algorithm handle

    Parameters:
        name - The name of the algorithm
        kwargs - A dictionary of property name:value pairs
    """
    alg = create_algorithm(name, **kwargs)
    alg.execute()
    return alg


def create_algorithm(name, **kwargs):
    """Create a named algorithm, set the properties given by the keywords and return the
    algorithm handle WITHOUT executing the algorithm

    Useful keywords:
      - child: Makes algorithm a child algorithm
      - rethrow: Causes exceptions to be rethrown on execution

    Parameters:
        name - The name of the algorithm
        kwargs - A dictionary of property name:value pairs
    @returns The algorithm handle
    """
    # Initialize the whole framework
    import mantid.simpleapi  # noqa
    if 'Version' in kwargs:
        alg = mantid.api.AlgorithmManager.createUnmanaged(name, kwargs['Version'])
        del kwargs['Version']
    else:
        alg = mantid.api.AlgorithmManager.createUnmanaged(name)
    alg.initialize()
    # Avoid problem that Load needs to set Filename first if it exists
    if name == 'Load' and 'Filename' in kwargs:
        alg.setPropertyValue('Filename', kwargs['Filename'])
        del kwargs['Filename']
    if 'child'in kwargs:
        alg.setChild(True)
        del kwargs['child']
        if 'OutputWorkspace' in alg:
            alg.setPropertyValue("OutputWorkspace","UNUSED_NAME_FOR_CHILD")
    if 'rethrow' in kwargs:
        alg.setRethrows(True)
        del kwargs['rethrow']
    alg.setProperties(kwargs)
    return alg


# Case difference is to be consistent with the unittest module
def assertRaisesNothing(testobj, callable, *args, **kwargs):
    """
        unittest does not have an assertRaisesNothing. This
        provides that functionality

        Parameters:
            testobj  - A unittest object
            callable - A callable object
            *args    - Positional arguments passed to the callable as they are
            **kwargs - Keyword arguments, passed on as they are
    """
    try:
        return callable(*args, **kwargs)
    except Exception as exc:
        testobj.fail("Assertion error. An exception was caught where none was expected in %s. Message: %s"
                     % (callable.__name__, str(exc)))


def can_be_instantiated(cls):
    """The Python unittest assertRaises does not
    seem to catch the assertion raised by being unable
    to instantiate a class (or maybe it's just the boost
    python stuff).
    In any case this little function tests for it and returns
    a boolean
    """
    try:
        cls()
        result = True
    except RuntimeError:
        result = False
    return result


# Work around for slow testhelpers.assert_almost_equal that was fixed in 1.9.0:
#   - https://github.com/numpy/numpy/commit/79d3a94f41b7e3c661eceed2f26ba6cce362ba4f
# Running assert_almost_equal in a tight loop with v<1.9.0 causes major slows (read minutes)
if LooseVersion(numpy.__version__) >= LooseVersion("1.9.0"):
    assert_almost_equal = numpy.testing.assert_almost_equal
else:
    def assert_almost_equal(actual,desired,decimal=7,err_msg='',verbose=True):
        """
        Raises an AssertionError if two items are not equal up to desired
        precision.
        .. note:: It is recommended to use one of `assert_allclose`,
                  `assert_array_almost_equal_nulp` or `assert_array_max_ulp`
                  instead of this function for more consistent floating point
                  comparisons.
        The test is equivalent to ``abs(desired-actual) < 0.5 * 10**(-decimal)``.
        Given two objects (numbers or ndarrays), check that all elements of these
        objects are almost equal. An exception is raised at conflicting values.
        For ndarrays this delegates to assert_array_almost_equal
        Parameters
        ----------
        actual : array_like
            The object to check.
        desired : array_like
            The expected object.
        decimal : int, optional
            Desired precision, default is 7.
        err_msg : str, optional
            The error message to be printed in case of failure.
        verbose : bool, optional
            If True, the conflicting values are appended to the error message.
        Raises
        ------
        AssertionError
          If actual and desired are not equal up to specified precision.
        See Also
        --------
        assert_allclose: Compare two array_like objects for equality with desired
                         relative and/or absolute precision.
        assert_array_almost_equal_nulp, assert_array_max_ulp, assert_equal
        Examples
        --------
        >>> import numpy.testing as npt
        >>> npt.assert_almost_equal(2.3333333333333, 2.33333334)
        >>> npt.assert_almost_equal(2.3333333333333, 2.33333334, decimal=10)
        ...
        <type 'exceptions.AssertionError'>:
        Items are not equal:
         ACTUAL: 2.3333333333333002
         DESIRED: 2.3333333399999998
        >>> npt.assert_almost_equal(np.array([1.0,2.3333333333333]),
        ...                         np.array([1.0,2.33333334]), decimal=9)
        ...
        <type 'exceptions.AssertionError'>:
        Arrays are not almost equal
        <BLANKLINE>
        (mismatch 50.0%)
         x: array([ 1.        ,  2.33333333])
         y: array([ 1.        ,  2.33333334])
        """
        __tracebackhide__ = True  # Hide traceback for py.test
        from numpy.core import ndarray
        from numpy.lib import iscomplexobj, real, imag
        from numpy.testing.utils import (assert_array_almost_equal, build_err_msg,
                                         gisfinite, gisnan)

        # Handle complex numbers: separate into real/imag to handle
        # nan/inf/negative zero correctly
        # XXX: catch ValueError for subclasses of ndarray where iscomplex fail
        try:
            usecomplex = iscomplexobj(actual) or iscomplexobj(desired)
        except ValueError:
            usecomplex = False

        def _build_err_msg():
            header = ('Arrays are not almost equal to %d decimals' % decimal)
            return build_err_msg([actual, desired], err_msg, verbose=verbose,
                                 header=header)

        if usecomplex:
            if iscomplexobj(actual):
                actualr = real(actual)
                actuali = imag(actual)
            else:
                actualr = actual
                actuali = 0
            if iscomplexobj(desired):
                desiredr = real(desired)
                desiredi = imag(desired)
            else:
                desiredr = desired
                desiredi = 0
            try:
                assert_almost_equal(actualr, desiredr, decimal=decimal)
                assert_almost_equal(actuali, desiredi, decimal=decimal)
            except AssertionError:
                raise AssertionError(_build_err_msg())

        if isinstance(actual, (ndarray, tuple, list)) \
                or isinstance(desired, (ndarray, tuple, list)):
            return assert_array_almost_equal(actual, desired, decimal, err_msg)
        try:
            # If one of desired/actual is not finite, handle it specially here:
            # check that both are nan if any is a nan, and test for equality
            # otherwise
            if not (gisfinite(desired) and gisfinite(actual)):
                if gisnan(desired) or gisnan(actual):
                    if not (gisnan(desired) and gisnan(actual)):
                        raise AssertionError(_build_err_msg())
                else:
                    if not desired == actual:
                        raise AssertionError(_build_err_msg())
                return
        except (NotImplementedError, TypeError):
            pass
        if round(abs(desired - actual), decimal) != 0:
            raise AssertionError(_build_err_msg())
