# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""Fitting support routines

This is all essentially about parsing the user input and putting it into a form
the Mantid fitting algorithm will understand
"""

import ast

import vesuvio.backgrounds as backgrounds
import vesuvio.profiles as profiles


# --------------------------------------------------------------------------------
# Functions
# --------------------------------------------------------------------------------


def parse_fit_options(mass_values, profile_strs, background_str="", constraints_str=""):
    """Parse the function string into a more usable format"""

    # Individual functions are separated by semi-colon separators
    mass_functions = profile_strs.rstrip(";").split(";")
    if len(mass_functions) != len(mass_values):
        raise ValueError(
            "Expected the number of 'function=' definitions to equal the number of masses. "
            "Found {0} masses but {1} function definition".format(len(mass_values), len(mass_functions))
        )
    mass_profiles = []
    for mass_value, prop_str in zip(mass_values, mass_functions):
        mass_profiles.append(profiles.create_from_str(prop_str, mass_value))

    if background_str != "":
        background = backgrounds.create_from_str(background_str)
    else:
        background = None

    if constraints_str != "":
        constraint_strings = constraints_str.split(";")
        constraints = []
        for constr_str in constraint_strings:
            constraints.append(ast.literal_eval(constr_str))
    else:
        constraints = None

    return FittingOptions(mass_profiles, background, constraints)


# --------------------------------------------------------------------------------
# FittingOptions - Used internally
# --------------------------------------------------------------------------------


class FittingOptions(object):
    """Holds all of the parameters for the fitting that are not related to the domain"""

    def __init__(self, mass_profile, background=None, intensity_constraints=None):
        self.smooth_points = None
        self.bad_data_error = None

        self.mass_profiles = mass_profile
        if intensity_constraints is not None:
            # Ensure that the constraints is a "2D" list
            if hasattr(intensity_constraints[0], "__len__"):
                self.intensity_constraints = intensity_constraints
            else:
                # trailing comma is important or the list gets undone
                self.intensity_constraints = [
                    intensity_constraints,
                ]
        else:
            self.intensity_constraints = None
        self.background = background

        self.global_fit = False
        self.output_prefix = None

    def has_been_set(self, name):
        """Returns true if the given option has been set by the user"""
        return getattr(self, name) is not None

    # ----------------------------------------------------------------------------

    def create_function_str(self, default_vals=None):
        """
        Creates the function string to pass to fit

        @param default_vals: A dictionary of key/values specifying the parameter
        values that have already been calculated. If None then the
        ComptonScatteringCountRate function, along with the constraints matrix,
        is used rather than the standard CompositeFunction. It is assumed that the
        standard CompositeFunction is used when running the fit for a second time
        to compute the errors with everything free
        """
        all_free = default_vals is not None

        if all_free or (self.intensity_constraints is None):
            function_str = "composite=CompositeFunction,NumDeriv=1;"
        else:
            function_str = "composite=ComptonScatteringCountRate,NumDeriv=1%s;"
            matrix_str = self.create_matrix_string(self.intensity_constraints)
            if matrix_str == "":
                function_str = function_str % ""
            else:
                function_str = function_str % (",IntensityConstraints=" + matrix_str)

        for index, mass_profile in enumerate(self.mass_profiles):
            par_prefix = "f{0}.".format(index)
            function_str += mass_profile.create_fit_function_str(default_vals, par_prefix)

        # Add on a background polynomial if requested
        if self.background is not None:
            bkgd_index = len(self.mass_profiles)
            function_str += self.background.create_fit_function_str(default_vals, param_prefix="f{0}.".format(bkgd_index))

        return function_str.rstrip(";")

    def create_matrix_string(self, constraints_tuple):
        """Returns a string for the value of the Matrix of intensity
        constraint values
        """
        if constraints_tuple is None or len(constraints_tuple) == 0:
            return ""

        nrows = len(constraints_tuple)
        ncols = len(constraints_tuple[0])

        matrix_str = '"Matrix(%d|%d)%s"'
        values = ""
        for row in constraints_tuple:
            for val in row:
                values += "%f|" % val
        values = values.rstrip("|")
        matrix_str = matrix_str % (nrows, ncols, values)
        return matrix_str

    def create_constraints_str(self):
        """Returns the string of constraints for this Fit"""
        constraints = []
        for func_index, mass_info in enumerate(self.mass_profiles):
            # Constraints
            prefix = "f{0}.".format(func_index)
            constraint = mass_info.create_constraint_str(prefix)
            if constraint != "":
                constraints.append(constraint)

        return ",".join(constraints).rstrip(",")

    def create_ties_str(self):
        """Returns the string of ties for this Fit"""
        ties = []
        for func_index, mass_info in enumerate(self.mass_profiles):
            # Constraints
            prefix = "f{0}.".format(func_index)

            tie = mass_info.create_ties_str(prefix)
            if tie != "":
                ties.append(tie)

        return ",".join(ties).rstrip(",")

    def create_global_function_str(self, nspectra, param_values=None):
        """
        Creates the function string to pass to fit for a multi-dataset (global) fitting

        @param nspectra :: A number of datasets (spectra) to be fitted simultaneously.

        @param param_values :: A dict/tableworkspace of key/values specifying the
                               parameter values that have already been calculated. If
                               None then the ComptonScatteringCountRate function,
                               along with the constraints matrix, is used rather than
                               the standard CompositeFunction. It is assumed that the
                               standard CompositeFunction is used when running the fit
                               for a second time to compute the errors with everything
                               free
        """

        # create a local function to fit a single spectrum
        func_str = self.create_function_str(param_values)
        # insert an attribute telling the function which spectrum it should be applied to
        i = func_str.index(";")
        # $domains=i means "function index == workspace index"
        fun_str = func_str[:i] + ",$domains=i" + func_str[i:]

        # append the constrints and ties within the local function
        fun_str += ";constraints=(" + self.create_constraints_str() + ")"
        ties = self.create_ties_str()
        if len(ties) > 0:
            fun_str += ";ties=(" + ties + ")"

        # initialize a string for composing the global ties
        global_ties = "f0.f0.Width"
        # build the multi-dataset function by joining local functions of the same type
        global_fun_str = "composite=MultiDomainFunction"
        for i in range(nspectra):
            global_fun_str += ";(" + fun_str + ")"
            if i > 0:
                global_ties = "f" + str(i) + ".f0.Width=" + global_ties
        # add the global ties
        global_fun_str += ";ties=(" + global_ties + ")"

        return global_fun_str

    def __str__(self):
        """Returns a string representation of the object"""
        self.generate_function_str()
