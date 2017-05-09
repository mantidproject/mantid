from __future__ import (absolute_import, division, print_function)

from six import iteritems
import math


class SampleDetails(object):
    def __init__(self, height, radius, center):
        # Currently we only support cylinders
        self.shape_type = "cylinder"
        validate_constructor_inputs(height=height, radius=radius, center=center)
        self.height = height
        self.radius = radius
        self.center = center  # List of X, Y, Z position

        # Internal flags so we are only allowed to set the material properties once
        self._is_material_set = False
        self._is_material_props_set = False

    def setMaterial(self, todo):
        pass

    def setMaterialProperties(self, todo):
        pass


def validate_constructor_inputs(height, radius, center):
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
    if len(center) != 3:
        raise ValueError("The center must have three values corresponding to X, Y, Z position of the sample."
                         " For example [0. ,1., 2.]")
    center_name = "center"
    for value in center:
        _check_value_is_physical(property_name=center_name, value=value)
    # All properties validated at this point


def _check_value_is_physical(property_name, value):
    value = convert_to_float(value)
    if not value:
        raise ValueError("Could not convert the " + property_name + " to a number. The input was: " + str(value))
    if value <= 0 or math.isnan(value):
        raise ValueError("The value set for " + property_name + " was: " + str(value)
                         + " which is impossible for a physical object")


def convert_to_float(val):
    try:
        val = float(val)
        return val
    except ValueError:
        return None
