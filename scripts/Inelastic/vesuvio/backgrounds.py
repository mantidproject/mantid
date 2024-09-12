# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=too-few-public-methods,redefined-builtin
"""Holds classes that define the backgrounds for fitting"""

import ast

# --------------------------------------------------------------------------------
# Background
# --------------------------------------------------------------------------------


class Background(object):
    """Base class"""

    pass


# --------------------------------------------------------------------------------
# Polynomial
# --------------------------------------------------------------------------------


class PolynomialBackground(object):
    cfunction = "Polynomial"

    def __init__(self, order):
        self.order = order

    @classmethod
    def from_str(cls, func_str):
        """Attempt to create an object of this type from a string.
        Raises a TypeError if parsing fails
        """
        profile_prefix = "function={0},".format(cls.cfunction)
        if not func_str.startswith(profile_prefix):
            raise TypeError("{0} function string should start with 'function={0},'".format(cls.cfunction))

        func_str = func_str[len(profile_prefix) :]
        # The only remaining property should be the order that we translate to n=
        order_prefix = "order="
        if func_str.startswith(order_prefix):
            # Trim off width= prefix
            poly_order = ast.literal_eval(func_str[len(order_prefix) :])
        else:
            raise TypeError("Unexpected value in function string. Expected order= following function")

        return PolynomialBackground(order=poly_order)

    def create_fit_function_str(self, param_vals=None, param_prefix=""):
        """Creates a string used by the Fit algorithm for this function

        :param param_vals: A table of values for the parameters that override those set
        on the object already. Default=None
        :param param_prefix: A string prefix for the parameter name in the params_vals list
        """
        vals_provided = param_vals is not None
        func_str = "name={0},n={1}".format(self.cfunction, str(self.order))

        if vals_provided:
            for power in range(0, self.order + 1):
                param_name = "A{0}".format(power)
                func_str += ",{0}={1:f}".format(param_name, param_vals[param_prefix + param_name])

        return func_str


# --------------------------------------------------------------------------------
# Factory function
# --------------------------------------------------------------------------------


def create_from_str(func_str):
    """Try and parse the function string to give the required background object

    :param func_str: A string of the form 'function=Name,attr1=val1,attr2=val2'
    """
    known_types = [PolynomialBackground]

    errors = dict()
    for cls in known_types:
        try:
            return cls.from_str(func_str)
        except TypeError as exc:
            errors[str(cls)] = str(exc)

    # if we get here we were unable to parse anything acceptable
    msgs = ["{0}: {1}".format(name, error) for name, error in errors.items()]
    raise ValueError("\n".join(msgs))
