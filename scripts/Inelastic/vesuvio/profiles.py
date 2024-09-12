# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=too-many-arguments,redefined-builtin
"""Holds classes that define the mass profiles.
This is all essentially about parsing the user input and putting it into a form
the Mantid fitting algorithm will understand
"""

import ast
import collections.abc
import re

from mantid import logger

# --------------------------------------------------------------------------------
# Mass profile base class
# --------------------------------------------------------------------------------


class MassProfile(object):
    cfunction = None

    def __init__(self, width, mass, intensity=None):
        self.width = width
        self.mass = mass
        self.intensity = intensity

    def create_fit_function_str(self, param_vals=None, param_prefix=""):
        raise NotImplementedError("MassProfile: Subclasses should overrode create_fitting_str")

    def create_constraint_str(self, param_prefix=""):
        """Returns a constraints string for the Fit algorithm

        :param param_prefix: An optional prefix for the parameter name
        """
        try:
            return "{0:f} < {1} < {2:f}".format(self.width[0], param_prefix + "Width", self.width[2])
        except TypeError:
            return ""

    def create_ties_str(self, param_prefix=""):
        """Return a ties string for the Fit algorithm

        :param param_prefix: An optional prefix for the parameter name
        """
        ties_str = "{0}Mass={1:f}".format(param_prefix, self.mass)

        if not isinstance(self.width, collections.abc.Iterable):
            ties_str += ",{0}={1:f}".format(param_prefix + "Width", self.width)

        return ties_str

    @classmethod
    def _parse_list(cls, func_str, prop_name):
        """
        Parse a list from a string containing 'prop_name=[]'

        :param prop_name: The string on the lhs of the equality
        :return: The parsed list
        """
        prop_re = re.compile(prop_name + r"=(\[(?:[\w.](?:,)?(?:\s)?)+\])")
        match = prop_re.search(func_str)
        if match:
            value = ast.literal_eval(match.group(1))
            if not isinstance(value, list):
                raise ValueError("Unexpected format for {0} value. Expected e.g. {0}=[1,0,1]".format(prop_name))
        else:
            raise ValueError("Cannot find {0}= in function_str (list) ({1})".format(prop_name, func_str))

        return value

    @classmethod
    def _parse_float(cls, func_str, prop_name, default=None):
        """
        :param prop_name: The string on the lhs of the equality
        :return: The parsed float
        """
        prop_re = re.compile(prop_name + r"=?(:\d+\.?\d*|\d*\.?\d+)")
        match = prop_re.search(func_str)
        if match:
            value = float(ast.literal_eval(match.group(1)))
            if not isinstance(value, float):
                raise ValueError("Unexpected format for {0} value. Expected e.g. {0}=0".format(prop_name))
        else:
            if default is None:
                raise ValueError("Cannot find {0}= in function_str (float) ({1})".format(prop_name, func_str))
            else:
                value = default

        return value

    @classmethod
    def _parse_bool_flag(cls, func_str, prop_name):
        """
        Parse an integer from a string containing 'prop_name=1'

        :param prop_name: The string on the lhs of the equality
        :return: The parsed value
        """
        prop_re = re.compile(prop_name + r"=([0,1])")
        match = prop_re.search(func_str)
        if match:
            value = ast.literal_eval(match.group(1))
            if not isinstance(value, int):
                raise ValueError("Unexpected format for {0} value. Expected e.g. {0}=1".format(prop_name))
        else:
            raise ValueError("Cannot find {0}= in function_str".format(prop_name))

        return value


# --------------------------------------------------------------------------------
# Gaussian profile
# --------------------------------------------------------------------------------


class GaussianMassProfile(MassProfile):
    cfunction = "GaussianComptonProfile"

    @classmethod
    def from_str(cls, func_str, mass):
        """Attempt to create an object of this type from a string.
        Raises a TypeError if parsing fails
        """
        profile_prefix = "function=Gaussian,"
        if not func_str.startswith(profile_prefix):
            raise TypeError("Gaussian function string should start with 'function=Gaussian,'")

        try:
            width = cls._parse_list(func_str, "width")
        except ValueError:
            width = cls._parse_float(func_str, "width")

        params = {"width": width, "mass": mass}

        return GaussianMassProfile(**params)

    def create_fit_function_str(self, param_vals=None, param_prefix=""):
        """Creates a string used by the Fit algorithm for this profile

        :param param_vals: A table of values for the parameters that override those set
        on the object already. Default=None
        :param param_prefix: A string prefix for the parameter as seen by the Mantid Fit algorithm
        """
        vals_provided = param_vals is not None

        if vals_provided:
            def_width = param_vals[param_prefix + "Width"]
        else:
            def_width = self.width
            if isinstance(def_width, list):
                def_width = def_width[1]

        fitting_str = "name={0},Mass={1:f},Width={2:f}".format(self.cfunction, self.mass, def_width)
        if vals_provided:
            param_name = "Intensity"
            intensity_str = "{0}={1:f}".format(param_name, param_vals[param_prefix + param_name])
            fitting_str += "," + intensity_str
        elif self.intensity is not None:
            fitting_str += ",Intensity={0:f}".format(self.intensity)

        logger.debug("Gaussian profile fit function string: {0}".format(fitting_str))
        return fitting_str + ";"

    def create_constraint_str(self, param_prefix=""):
        """Returns a constraints string for the Fit algorithm

        :param param_prefix: An optional prefix for the parameter name
        """
        constraints = super(GaussianMassProfile, self).create_constraint_str(param_prefix)
        if constraints != "":
            constraints += ","
        constraints += "{0}Intensity > 0.0".format(param_prefix)
        return constraints


# --------------------------------------------------------------------------------
# MultivariateGaussian profile
# --------------------------------------------------------------------------------


class MultivariateGaussianMassProfile(MassProfile):
    cfunction = "MultivariateGaussianComptonProfile"
    integration_steps = 64

    def __init__(self, width, mass, sigma_x=1.0, sigma_y=1.0, sigma_z=1.0):
        super(MultivariateGaussianMassProfile, self).__init__(width, mass)

        self._sigma_x = sigma_x
        self._sigma_y = sigma_y
        self._sigma_z = sigma_z

    @classmethod
    def from_str(cls, func_str, mass):
        """Attempt to create an object of this type from a string.
        Raises a TypeError if parsing fails

        Sigma values are taken as the initial value of the parameter, not the
        fixed value of the parameter.
        """
        profile_prefix = "function=MultivariateGaussian"
        if not func_str.startswith(profile_prefix):
            raise TypeError("Multivariate Gaussian function string should start with 'function=MultivariateGaussian'")

        params = {
            "width": 0.0,
            "mass": mass,
            "sigma_x": cls._parse_float(func_str, "SigmaX", 1.0),
            "sigma_y": cls._parse_float(func_str, "SigmaY", 1.0),
            "sigma_z": cls._parse_float(func_str, "SigmaZ", 1.0),
        }

        return MultivariateGaussianMassProfile(**params)

    def create_fit_function_str(self, param_vals=None, param_prefix=""):
        """Creates a string used by the Fit algorithm for this profile

        :param param_vals: A table of values for the parameters that override those set
        on the object already. Default=None
        :param param_prefix: A string prefix for the parameter as seen by the Mantid Fit algorithm
        """
        vals_provided = param_vals is not None

        intensity = None

        if vals_provided:
            intensity = param_vals[param_prefix + "Intensity"]
            sig_x = param_vals[param_prefix + "SigmaX"]
            sig_y = param_vals[param_prefix + "SigmaY"]
            sig_z = param_vals[param_prefix + "SigmaZ"]
        else:
            intensity = self.intensity
            sig_x = self._sigma_x
            sig_y = self._sigma_y
            sig_z = self._sigma_z

        fitting_str = "name={0},IntegrationSteps={1},Mass={2:f},SigmaX={3:f},SigmaY={4:f},SigmaZ={5:f}"
        fitting_str = fitting_str.format(self.cfunction, self.integration_steps, self.mass, sig_x, sig_y, sig_z)

        if intensity is not None:
            fitting_str += ",Intensity={0:f}".format(intensity)

        logger.debug("Multivariate Gaussian profile fit function string: {0}".format(fitting_str))
        return fitting_str + ";"

    def create_constraint_str(self, param_prefix=""):
        """Returns a constraints string for the Fit algorithm

        :param param_prefix: An optional prefix for the parameter name
        """
        constraints = (
            "{0}Intensity > 0.0,".format(param_prefix)
            + "{0}SigmaX > 0.0,".format(param_prefix)
            + "{0}SigmaY > 0.0,".format(param_prefix)
            + "{0}SigmaZ > 0.0".format(param_prefix)
        )
        return constraints

    def create_ties_str(self, param_prefix=""):
        ties_str = "{0}Mass={1:f}".format(param_prefix, self.mass)
        return ties_str


# --------------------------------------------------------------------------------
# GramCharlier profile
# --------------------------------------------------------------------------------


class GramCharlierMassProfile(MassProfile):
    cfunction = "GramCharlierComptonProfile"

    def __init__(self, width, mass, hermite_coeffs, k_free, sears_flag, hermite_coeff_vals=None, fsecoeff=None):
        super(GramCharlierMassProfile, self).__init__(width, mass)

        self.hermite_co = hermite_coeffs
        self.k_free = k_free
        self.sears_flag = sears_flag
        self.hermite_coeff_vals = hermite_coeff_vals
        self.fsecoeff = fsecoeff

    @classmethod
    def from_str(cls, func_str, mass):
        """Attempt to create an object of this type from a string.
        Raises a TypeError if parsing fails
        """
        profile_prefix = "function=GramCharlier,"
        if not func_str.startswith(profile_prefix):
            raise TypeError("GramCharlier function string should start with 'function=GramCharlier,'")

        key_names = [
            ("width", cls._parse_list),
            ("hermite_coeffs", cls._parse_list),
            ("k_free", cls._parse_bool_flag),
            ("sears_flag", cls._parse_bool_flag),
        ]

        # Possible key names:
        parsed_values = []
        for key, parser in key_names:
            try:
                parsed_values.append(parser(func_str, key))
            except ValueError as exc:
                raise TypeError(str(exc))

        params = {
            "width": parsed_values[0],
            "mass": mass,
            "hermite_coeffs": parsed_values[1],
            "k_free": parsed_values[2],
            "sears_flag": parsed_values[3],
        }

        hermite_coeff_val_regex = re.compile("[Cc]_([0-9]+)=([0-9.]+)")
        hermite_vals = {}
        for hermite_val in hermite_coeff_val_regex.findall(func_str):
            hermite_vals[hermite_val[0].upper()] = ast.literal_eval(hermite_val[1])
        if len(hermite_vals) > 0:
            params["hermite_coeff_vals"] = hermite_vals

        fsecoeff_regex = re.compile("fsecoeff=([0-9.]+)")
        fsecoeff_match = fsecoeff_regex.match(func_str)
        if fsecoeff_match:
            params["fsecoeff"] = ast.literal_eval(fsecoeff_match.group(1))

        return GramCharlierMassProfile(**params)

    # pylint: disable=too-many-branches
    def create_fit_function_str(self, param_vals=None, param_prefix=""):
        """Creates a string used by the Fit algorithm for this profile

        :param param_vals: A table of values for the parameters that override those set
        on the object already. Default=None
        :param param_prefix: A string prefix for the parameter as seen by the Mantid Fit algorithm
        """
        vals_provided = param_vals is not None

        if vals_provided:
            def_width = param_vals[param_prefix + "Width"]
        else:
            def_width = self.width
            if isinstance(def_width, list):
                def_width = def_width[1]

        def to_space_sep_str(collection):
            _str = ""
            for item in collection:
                _str += " " + str(item)
            return _str.lstrip()

        hermite_str = to_space_sep_str(self.hermite_co)

        fitting_str = "name={0},Mass={1:f},HermiteCoeffs={2},Width={3:f}".format(self.cfunction, self.mass, hermite_str, def_width)
        if vals_provided:
            par_names = ["FSECoeff"]
            for i, coeff in enumerate(self.hermite_co):
                if coeff > 0:
                    par_names.append("C_{0}".format(2 * i))
            for par_name in par_names:
                fitting_str += ",{0}={1:f}".format(par_name, param_vals[param_prefix + par_name])
        else:
            if self.fsecoeff is not None:
                fitting_str += "FSECoeff={0}".format(self.fsecoeff)
            if self.hermite_coeff_vals is not None:
                for i, coeff in list(self.hermite_coeff_vals.items()):
                    if coeff > 0:
                        fitting_str += ",C_{0}={1:f}".format(i, coeff)

        logger.debug("Gram Charlier profile fit function string: {0}".format(fitting_str))
        return fitting_str + ";"

    def create_constraint_str(self, param_prefix=""):
        """Returns a constraints string for the Fit algorithm

        :param param_prefix: An optional prefix for the parameter name
        """
        constraints = super(GramCharlierMassProfile, self).create_constraint_str(param_prefix)
        if constraints != "":
            constraints += ","
        # All coefficients should be greater than zero
        for i, coeff in enumerate(self.hermite_co):
            if coeff > 0:
                constraints += "{0}C_{1} > 0.0,".format(param_prefix, 2 * i)
        return constraints.rstrip(",")

    def create_ties_str(self, param_prefix=""):
        """Return a ties string for the Fit algorithm

        :param param_prefix: An optional prefix for the parameter name
        """
        ties = super(GramCharlierMassProfile, self).create_ties_str(param_prefix)
        if not self.k_free:
            # Sears flag controls value of FSECoeff
            param_name = param_prefix + "FSECoeff"
            if self.sears_flag == 1:
                # tie to multiple of the width
                tied_value = param_prefix + "Width*sqrt(2)/12"
            else:
                tied_value = "0"
            if ties != "":
                ties += ","
            ties += "{0}={1}".format(param_name, tied_value)

        return ties


# --------------------------------------------------------------------------------
# Factory function
# --------------------------------------------------------------------------------


def create_from_str(func_str, mass):
    """Try and parse the function string to give the required profile function

    :param func_str: A string of the form 'function=Name,attr1=val1,attr2=val2'
    :param mass: The value of the mass for the profile
    """
    known_types = [GaussianMassProfile, MultivariateGaussianMassProfile, GramCharlierMassProfile]
    logger.debug("Profile factory string: {0}".format(func_str))
    errors = dict()
    for cls in known_types:
        try:
            return cls.from_str(func_str, mass)
        except TypeError as exc:
            errors[str(cls)] = str(exc)

    # if we get here we were unable to parse anything acceptable
    msgs = ["{0}: {1}".format(name, error) for name, error in errors.items()]
    raise ValueError("\n".join(msgs))
