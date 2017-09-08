from __future__ import (absolute_import, division, print_function)

from six import iteritems
from isis_powder.routines import common
import math
import warnings


class SampleDetails(object):
    def __init__(self, **kwargs):
        self.shape_type = common.dictionary_key_helper(dictionary=kwargs, key="shape",
                                                       exception_msg="Sample property \"shape\" was not passed to "
                                                                     "SampleDetails. If you have been using this class "
                                                                     "without worrying about \"shape\", you probably "
                                                                     "want to set this to \"cylinder\"")

        if self.shape_type == "cylinder":
            self.shape = _Cylinder(kwargs)
        elif self.shape_type == "slab":
            self.shape = _Slab(kwargs)
        else:
            raise KeyError("Shape type \"" + self.shape_type + "\" not supported: current supported shape types are "
                           "\"cylinder\" and \"slab\"")

        self.material_object = None

    def is_material_set(self):
        return self.material_object is not None

    def print_sample_details(self):
        self._print()

    def reset_sample_material(self):
        self.material_object = None

    def set_material(self, **kwargs):
        chemical_formula = common.dictionary_key_helper(dictionary=kwargs, key="chemical_formula",
                                                        exception_msg="The following argument is required but was not"
                                                                      " passed: chemical_formula")
        number_density = common.dictionary_key_helper(dictionary=kwargs, key="number_density", throws=False)

        if self.material_object is not None:
            self.print_sample_details()
            raise RuntimeError("The material has already been set to the above details. If the properties"
                               " have not been set they can be modified with 'set_material_properties()'. Otherwise"
                               " to change the material call 'reset_sample_material()'")

        self.material_object = _Material(chemical_formula=chemical_formula, number_density=number_density)

    def set_material_properties(self, **kwargs):
        err_msg = "The following argument is required but was not set or passed: "
        absorption_cross_section = common.dictionary_key_helper(dictionary=kwargs, key="absorption_cross_section",
                                                                exception_msg=err_msg + "absorption_cross_section")
        scattering_cross_section = common.dictionary_key_helper(dictionary=kwargs, key="scattering_cross_section",
                                                                exception_msg=err_msg + "scattering_cross_section")
        if self.material_object is None:
            raise RuntimeError("The material has not been set (or reset). Please set it by calling"
                               " 'set_material()' to set the material details of the sample.")

        self.material_object.set_material_properties(abs_cross_sect=absorption_cross_section,
                                                     scattering_cross_sect=scattering_cross_section)

    def _print(self):
        print("Sample Details")
        print("------------------------")
        print("Shape type: " + self.shape_type)

        self.shape.print_shape()
        print("------------------------")

        if self.material_object is None:
            print("Material has not been set (or has been reset).")
        else:
            self.material_object.print_material()
        print()  # Newline for visual spacing

    def radius(self):
        if self.shape_type == "cylinder":
            return self.shape.radius
        else:
            raise RuntimeError("Radius is not applicable for the shape type \"" + self.shape_type + "\"")

    def height(self):
        if self.shape_type == "cylinder":
            return self.shape.height
        else:
            raise RuntimeError("Height is not applicable for the shape type \"" + self.shape_type + "\"")

    def center(self):
        if self.shape_type == "cylinder":
            return self.shape.center
        else:
            raise RuntimeError("Center is not applicable for the shape type \"" + self.shape_type + "\"")

    def thickness(self):
        if self.shape_type == "slab":
            return self.shape.thickness
        else:
            raise RuntimeError("Thickness is not applicable for the shape type \"" + self.shape_type + "\"")


class _Material(object):
    def __init__(self, chemical_formula, number_density=None):
        self.chemical_formula = chemical_formula

        # If it is not an element Mantid requires us to provide the number density
        # which is required for absorption corrections.
        if len(chemical_formula) > 2 and number_density is None:
                raise ValueError("A number density formula must be set on a chemical formula which is not elemental."
                                 " An element can only be a maximum of 2 characters (e.g. 'Si' or 'V'). The number"
                                 " density can be set using the following key: number_density")
        if number_density:
            # Always check value is sane if user has given one
            _check_value_is_physical(property_name="number_density", value=number_density)

        self.number_density = number_density

        # Advanced material properties
        self.absorption_cross_section = None
        self.scattering_cross_section = None

        # Internal flags so we are only allowed to set the material properties once
        self._is_material_props_set = False

    def print_material(self):
        print("Material properties:")
        print("------------------------")
        print("Chemical formula: {}".format(self.chemical_formula))

        if self.number_density:
            print("Number Density: {}".format(self.number_density))
        else:
            print("Number Density: Set from elemental properties by Mantid")
        self._print_material_properties()

    def _print_material_properties(self):
        if self._is_material_props_set:
            print("Absorption cross section: {}".format(self.absorption_cross_section))
            print("Scattering cross section: {}".format(self.scattering_cross_section))
        else:
            print("Absorption cross section: Calculated by Mantid based on chemical/elemental formula")
            print("Scattering cross section: Calculated by Mantid based on chemical/elemental formula")
            print("Note to manually override these call 'set_material_properties()'")

    def set_material_properties(self, abs_cross_sect, scattering_cross_sect):
        if self._is_material_props_set:
            self.print_material()
            raise RuntimeError("The material properties have already been set to the above."
                               " To reset the material and its properties call 'reset_sample_material()' on the "
                               "properties object.")

        _check_value_is_physical("absorption_cross_section", abs_cross_sect)
        _check_value_is_physical("scattering_cross_section", scattering_cross_sect)
        self.absorption_cross_section = float(abs_cross_sect)
        self.scattering_cross_section = float(scattering_cross_sect)
        self._is_material_props_set = True


class _Cylinder(object):
    def __init__(self, kwargs):
        # By using kwargs we get a better error than "init takes n arguments"
        err_string = "The following sample property was not passed as an argument: "
        height = common.dictionary_key_helper(dictionary=kwargs, key="height", exception_msg=err_string + "height")
        radius = common.dictionary_key_helper(dictionary=kwargs, key="radius", exception_msg=err_string + "radius")
        center = common.dictionary_key_helper(dictionary=kwargs, key="center", exception_msg=err_string + "center")

        _Cylinder._validate_constructor_inputs(height=height, radius=radius, center=center)

        self.height = float(height)
        self.radius = float(radius)
        self.center = [float(i) for i in center]  # List of X, Y, Z position
        self.shape_type = "cylinder"

    @staticmethod
    def _validate_constructor_inputs(height, radius, center):
        # Ensure we got double (or int) types and they are sane
        values_to_check = {'height': height, 'radius': radius}

        # Attempt to convert them all to floating point relying on the fact on
        # the way Python has aliases to an object
        for key, value in iteritems(values_to_check):
            _check_value_is_physical(property_name=key, value=value)

        # Center has to be checked specially - it has to be a list of floating point values
        if not isinstance(center, list):
            raise ValueError("The center of the cylinder must be specified as a list of X, Y, Z co-ordinates."
                             " For example [0., 1., 2.]")

        # The center of the cylinder can be at any position of X Y Z so don't check against physical constraints
        if len(center) != 3:
            raise ValueError("The center must have three values corresponding to X, Y, Z position of the sample."
                             " For example [0. ,1., 2.]")

        for val in center:
            _check_can_convert_to_float(property_name="center", value=val)

    def print_shape(self):
        print("Height: {}".format(self.height))
        print("Radius: {}".format(self.radius))
        print("Center X:{}, Y:{}, Z{}".format(self.center[0], self.center[1], self.center[2]))


class _Slab(object):
    def __init__(self, kwargs):
        # By using kwargs we get a better error than "init takes n arguments"
        err_string = "The following sample property was not passed as an argument: "
        thickness = common.dictionary_key_helper(dictionary=kwargs, key="thickness",
                                                 exception_msg=err_string + "thickness")
        _Slab._validate_constructor_inputs(thickness=thickness)
        self.thickness = float(thickness)
        self.shape_type = "slab"

    @staticmethod
    def _validate_constructor_inputs(thickness):
        _check_value_is_physical(property_name="thickness", value=thickness)
        _check_can_convert_to_float(property_name="thickness", value=thickness)

    def print_shape(self):
        print("Thickness: {}".format(self.thickness))


def _check_value_is_physical(property_name, value):
    original_value = value
    value = _check_can_convert_to_float(property_name=property_name, value=value)

    if value <= 0 or math.isnan(value):
        raise ValueError("The value set for " + property_name + " was: " + str(original_value)
                         + " which is impossible for a physical object")


def _check_can_convert_to_float(property_name, value):
    original_value = value
    value = convert_to_float(value)
    if value is None:
        raise ValueError("Could not convert the " + property_name + " to a number."
                         " The input was: '" + str(original_value) + "'")
    return value


def convert_to_float(val):
    try:
        val = float(val)
        return val
    except ValueError:
        return None
