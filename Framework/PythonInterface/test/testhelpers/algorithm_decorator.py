# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import inspect
import re


def make_decorator(algorithm_to_decorate):
    """
    Dynamically create a builder pattern style decorator around a Mantid algorithm.
    This allows you to separate out setting algorithm parameters from the actual method execution. Parameters may be reset multiple times.

    Usage:
     rebin = make_decorator(Rebin)
     rebin.set_Params([0, 0.1, 1])
     ....
     rebin.execute()

    Arguments:
     algorithm_to_decorate: The mantid.simpleapi algorithm to decorate.



    """

    class Decorator(object):
        def __init__(self, alg_subject):
            self.__alg_subject = alg_subject
            self.__parameters__ = dict()

        def execute(self, additional=None, verbose=False):
            if verbose:
                print("Algorithm Parameters:")
                print(self.__parameters__)
                print()
            out = self.__alg_subject(**self.__parameters__)
            return out

        def set_additional(self, additional):
            self.__parameters__.update(**additional)

    def add_getter_setter(type, name):
        def setter(self, x):
            self.__parameters__[name] = x

        def getter(self):
            return self.__parameters__[name]

        setattr(type, "set_" + name, setter)
        setattr(type, "get_" + name, getter)

    argspec = inspect.getfullargspec(algorithm_to_decorate)
    for parameter in argspec.varargs.split(","):
        m = re.search(r"(^\w+)", parameter)  # Take the parameter key part from the defaults given as 'key=value'
        if m:
            parameter = m.group(0).strip()
        m = re.search(r"\w+$", parameter)  # strip off any leading numerical values produced by argspec
        if m:
            parameter = m.group(0).strip()
        add_getter_setter(Decorator, m.group(0).strip())

    return Decorator(algorithm_to_decorate)
