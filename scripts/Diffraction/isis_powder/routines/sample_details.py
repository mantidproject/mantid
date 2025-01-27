# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from isis_powder.routines import common
import math
from mantid import logger

property_err_string = "The following sample property was not passed as an argument: {}"


class SampleDetails(object):
    def __init__(self, **kwargs):
        self._shape_type = common.dictionary_key_helper(dictionary=kwargs, key="shape", throws=False)
        if self._shape_type is None:
            self._shape_type = "cylinder"
            warning = 'Failed to supply parameter "shape" to SampleDetails - defaulting to "cylinder"'
            print("WARNING: {}".format(warning))  # Show warning in script window
            logger.warning(warning)  # Show warning in Mantid logging area

        if self._shape_type == "cylinder":
            self._shape = _Cylinder(kwargs)
        elif self._shape_type == "slab":
            self._shape = _Slab(kwargs)
        else:
            raise KeyError('Shape type "' + self._shape_type + '" not supported: current supported shape types are "cylinder" and "slab"')

        self.material_object = None
        self.container_material_object = None
        self._container_shape = None

    def is_material_set(self):
        return self.material_object is not None

    def print_sample_details(self):
        self._print()

    def reset_sample_material(self):
        self.material_object = None

    def set_material(self, **kwargs):
        chemical_formula = common.dictionary_key_helper(
            dictionary=kwargs,
            key="chemical_formula",
            exception_msg="The following argument is required but was not passed: chemical_formula",
        )
        number_density = common.dictionary_key_helper(dictionary=kwargs, key="number_density", throws=False)
        number_density_effective = common.dictionary_key_helper(dictionary=kwargs, key="number_density_effective", throws=False)
        packing_fraction = common.dictionary_key_helper(dictionary=kwargs, key="packing_fraction", throws=False)

        if self.material_object is not None:
            self.print_sample_details()
            raise RuntimeError(
                "The material has already been set to the above details. If the properties"
                " have not been set they can be modified with 'set_material_properties()'. Otherwise"
                " to change the material call 'reset_sample_material()'"
            )

        self.material_object = _Material(
            chemical_formula=chemical_formula,
            number_density=number_density,
            number_density_effective=number_density_effective,
            packing_fraction=packing_fraction,
        )

    def set_container(self, **kwargs):
        chemical_formula = common.dictionary_key_helper(
            dictionary=kwargs,
            key="chemical_formula",
            exception_msg="The following argument is required but was not passed: chemical_formula",
        )
        number_density = common.dictionary_key_helper(dictionary=kwargs, key="number_density", throws=False)
        number_density_effective = common.dictionary_key_helper(dictionary=kwargs, key="number_density_effective", throws=False)
        packing_fraction = common.dictionary_key_helper(dictionary=kwargs, key="packing_fraction", throws=False)
        if self.container_material_object is not None:
            self.print_container_details()
            raise RuntimeError(
                "The container material has already been set to the above details. To change the material call 'reset_sample_material()'"
            )

        self.container_material_object = _Material(
            chemical_formula=chemical_formula,
            number_density=number_density,
            number_density_effective=number_density_effective,
            packing_fraction=packing_fraction,
        )
        if self._shape_type.capitalize() == "Cylinder":
            self._container_shape = _HollowCylinder(
                kwargs, sample_height=self.height(), sample_radius=self.radius(), sample_center=self.center()
            )
        elif self._shape_type.capitalize() == "Slab":
            self._container_shape = _FlatPlateHolder(
                kwargs,
                sample_width=self.width(),
                sample_height=self.height(),
                sample_thickness=self.thickness(),
                sample_angle=self.angle(),
                sample_center=self.center(),
            )

    def set_material_properties(self, **kwargs):
        err_msg = "The following argument is required but was not set or passed: "
        absorption_cross_section = common.dictionary_key_helper(
            dictionary=kwargs, key="absorption_cross_section", exception_msg=err_msg + "absorption_cross_section"
        )
        scattering_cross_section = common.dictionary_key_helper(
            dictionary=kwargs, key="scattering_cross_section", exception_msg=err_msg + "scattering_cross_section"
        )
        if self.material_object is None:
            raise RuntimeError(
                "The material has not been set (or reset). Please set it by calling"
                " 'set_material()' to set the material details of the sample."
            )

        self.material_object.set_material_properties(
            abs_cross_sect=absorption_cross_section, scattering_cross_sect=scattering_cross_section
        )

    @staticmethod
    def validate_center(center):
        # Center has to be checked specially - it has to be a list of floating point values
        if not isinstance(center, list):
            raise ValueError("The center of the cylinder must be specified as a list of X, Y, Z co-ordinates. For example [0., 1., 2.]")

        # The center of the cylinder can be at any position of X Y Z so don't check against physical constraints
        if len(center) != 3:
            raise ValueError("The center must have three values corresponding to X, Y, Z position of the sample. For example [0. ,1., 2.]")

        for val in center:
            _check_can_convert_to_float(property_name="center", value=val)

    @staticmethod
    def validate_constructor_inputs(values_to_check):
        # Ensure we got double (or int) types and they are sane

        # Attempt to convert them all to floating point relying on the fact on
        # the way Python has aliases to an object
        for key, value in values_to_check.items():
            _check_value_is_physical(property_name=key, value=value)
            _check_can_convert_to_float(property_name=key, value=value)

    def _print(self):
        print("Sample Details")
        print("------------------------")
        print("Shape type: " + self._shape_type)
        print("Center X:{}, Y:{}, Z{}".format(self.center()[0], self.center()[1], self.center()[2]))

        self._shape.print_shape()
        print("------------------------")

        if self.material_object is None:
            print("Material has not been set (or has been reset).")
        else:
            self.material_object.print_material()
        print()  # Newline for visual spacing

    def print_container_details(self):
        print("Container Details")
        print("------------------------")
        print("Shape type: " + self._shape_type)
        print("Radius: " + str(self.container_radius))
        print("------------------------")

        if self.material_object is None:
            print("Material has not been set (or has been reset).")
        else:
            self.container_material_object.print_material()
        print()  # Newline for visual spacing

    def shape_type(self):
        return self._shape_type

    def radius(self):
        if self._shape_type.capitalize() == "Cylinder":
            return self._shape.radius
        else:
            raise RuntimeError('Radius is not applicable for the shape type "{}"'.format(self._shape_type))

    def container_radius(self):
        if self._shape_type.capitalize() == "Cylinder":
            return self._container_shape.container_radius
        else:
            raise RuntimeError('Container Radius is not applicable for the shape type "{}"'.format(self._shape_type))

    def get_front_thick(self):
        if self._shape_type.capitalize() == "Slab":
            return self._container_shape.front_thick
        else:
            raise RuntimeError('Front Thick is not applicable for the shape type "{}"'.format(self._shape_type))

    def get_back_thick(self):
        if self._shape_type.capitalize() == "Slab":
            return self._container_shape.back_thick
        else:
            raise RuntimeError('Back Thick is not applicable for the shape type "{}"'.format(self._shape_type))

    def height(self):
        return self._shape.height

    def center(self):
        return self._shape.center

    def width(self):
        if self._shape_type == "slab":
            return self._shape.width
        else:
            raise RuntimeError('Width is not applicable for the shape type "{}"'.format(self._shape_type))

    def angle(self):
        if self._shape_type == "slab":
            return self._shape.angle
        else:
            raise RuntimeError('Angle is not applicable for the shape type "{}"'.format(self._shape_type))

    def thickness(self):
        if self._shape_type == "slab":
            return self._shape.thickness
        else:
            raise RuntimeError('Thickness is not applicable for the shape type "{}"'.format(self._shape_type))

    def generate_sample_geometry(self):
        """
        Generates the expected input for sample geometry using the SampleDetails class
        :param self: Instance of SampleDetails containing details about sample geometry and material
        :return: A map of the sample geometry
        """
        return self._shape.generate_sample_geometry()

    def generate_sample_material(self):
        """
        Generates the expected input for sample material using the SampleDetails class.
        See SetSampleMaterial for documentation on this dictionary
        :param self: Instance of SampleDetails containing details about sample geometry and material
        :return: A map of the sample material
        """
        material_json = {"ChemicalFormula": self.material_object.chemical_formula}
        if self.material_object.number_density:
            material_json["NumberDensity"] = self.material_object.number_density
        if self.material_object.number_density_effective:
            material_json["EffectiveNumberDensity"] = self.material_object.number_density_effective
        if self.material_object.packing_fraction:
            material_json["PackingFraction"] = self.material_object.packing_fraction
        if self.material_object.absorption_cross_section:
            material_json["AttenuationXSection"] = self.material_object.absorption_cross_section
        if self.material_object.scattering_cross_section:
            material_json["ScatteringXSection"] = self.material_object.scattering_cross_section
        return material_json

    def generate_container_geometry(self):
        if self._container_shape:
            return self._container_shape.generate_container_geometry()
        else:
            return None

    def generate_container_material(self):
        if self.container_material_object:
            container_material_json = {"ChemicalFormula": self.container_material_object.chemical_formula}
            if self.container_material_object.number_density:
                container_material_json["NumberDensity"] = self.container_material_object.number_density
            if self.container_material_object.number_density_effective:
                container_material_json["EffectiveNumberDensity"] = self.container_material_object.number_density_effective
            if self.container_material_object.packing_fraction:
                container_material_json["PackingFraction"] = self.container_material_object.packing_fraction
            if self.container_material_object.absorption_cross_section:
                container_material_json["AttenuationXSection"] = self.container_material_object.absorption_cross_section
            if self.container_material_object.scattering_cross_section:
                container_material_json["ScatteringXSection"] = self.container_material_object.scattering_cross_section
            return container_material_json
        else:
            return None


class _Material(object):
    def __init__(self, chemical_formula, number_density=None, number_density_effective=None, packing_fraction=None):
        self.chemical_formula = chemical_formula
        # If it is not an element Mantid requires us to provide the number density
        # which is required for absorption corrections.
        if len(chemical_formula) > 2 and number_density is None and number_density_effective is None:
            raise ValueError(
                "A number density formula must be set on a chemical formula which is not elemental."
                " An element can only be a maximum of 2 characters (e.g. 'Si' or 'V'). The number"
                " density can be set using the following keys: number_density or number_density_effective"
            )
        if number_density:
            # Always check value is sane if user has given one
            _check_value_is_physical(property_name="number_density", value=number_density)
        self.number_density = number_density

        if number_density_effective:
            # Always check value is sane if user has given one
            _check_value_is_physical(property_name="number_density_effective", value=number_density_effective)
        self.number_density_effective = number_density_effective

        self.packing_fraction = packing_fraction

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
        if self.number_density_effective:
            print("Effective Number Density: {}".format(self.number_density_effective))
        if not self.number_density and not self.number_density_effective:
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
            raise RuntimeError(
                "The material properties have already been set to the above."
                " To reset the material and its properties call 'reset_sample_material()' on the "
                "properties object."
            )

        _check_value_is_physical("absorption_cross_section", abs_cross_sect)
        _check_value_is_physical("scattering_cross_section", scattering_cross_sect)
        self.absorption_cross_section = float(abs_cross_sect)
        self.scattering_cross_section = float(scattering_cross_sect)
        self._is_material_props_set = True


class _Cylinder(object):
    def __init__(self, kwargs):
        # By using kwargs we get a better error than "init takes n arguments"
        height = common.dictionary_key_helper(dictionary=kwargs, key="height", exception_msg=property_err_string.format("height"))
        radius = common.dictionary_key_helper(dictionary=kwargs, key="radius", exception_msg=property_err_string.format("radius"))

        _Cylinder._validate_constructor_inputs(height=height, radius=radius)
        SampleDetails.validate_constructor_inputs({"height": height, "radius": radius})
        self.height = float(height)
        self.radius = float(radius)
        self.shape_type = "cylinder"

        center = common.dictionary_key_helper(dictionary=kwargs, key="center", exception_msg=property_err_string.format("center"))
        SampleDetails.validate_center(center)
        self.center = [float(i) for i in center]  # List of X, Y, Z position

    def generate_sample_geometry(self):
        return {"Shape": "Cylinder", "Height": self.height, "Radius": self.radius, "Center": self.center}

    @staticmethod
    def _validate_constructor_inputs(height, radius):
        # Ensure we got double (or int) types and they are sane
        values_to_check = {"height": height, "radius": radius}

        # Attempt to convert them all to floating point relying on the fact on
        # the way Python has aliases to an object
        for key, value in values_to_check.items():
            _check_value_is_physical(property_name=key, value=value)
            _check_can_convert_to_float(property_name=key, value=value)

    def print_shape(self):
        print("Height: {}".format(self.height))
        print("Radius: {}".format(self.radius))
        print("Center: {}".format(self.center))


class _Slab(object):
    def __init__(self, kwargs):
        # By using kwargs we get a better error than "init takes n arguments"
        thickness = common.dictionary_key_helper(dictionary=kwargs, key="thickness", exception_msg=property_err_string.format("thickness"))
        width = common.dictionary_key_helper(dictionary=kwargs, key="width", exception_msg=property_err_string.format("width"))
        height = common.dictionary_key_helper(dictionary=kwargs, key="height", exception_msg=property_err_string.format("height"))
        angle = common.dictionary_key_helper(dictionary=kwargs, key="angle", exception_msg=property_err_string.format("angle"))

        SampleDetails.validate_constructor_inputs({"thickness": thickness, "width": width, "height": height, "angle": angle})
        self.thickness = float(thickness)
        self.width = float(width)
        self.height = float(height)
        self.angle = float(angle)
        self.shape_type = "slab"

        center = common.dictionary_key_helper(dictionary=kwargs, key="center", exception_msg=property_err_string.format("center"))
        SampleDetails.validate_center(center)
        self.center = [float(i) for i in center]  # List of X, Y, Z position

    def generate_sample_geometry(self):
        return {
            "Shape": "FlatPlate",
            "Width": self.width,
            "Height": self.height,
            "Thick": self.thickness,
            "Center": self.center,
            "Angle": self.angle,
        }

    def print_shape(self):
        print(f"Thickness: {self.thickness} \n Width: {self.width} \n Height: {self.height} \nAngle: {self.angle} \n Center: {self.center}")


class _HollowCylinder(object):
    def __init__(self, kwargs, sample_height, sample_radius, sample_center):
        # By using kwargs we get a better error than "init takes n arguments"
        container_radius = common.dictionary_key_helper(dictionary=kwargs, key="radius", exception_msg=property_err_string.format("radius"))

        SampleDetails.validate_constructor_inputs({"container_radius": container_radius})
        self.container_radius = float(container_radius)
        self.shape_type = "HollowCylinder"

        self.sample_radius = sample_radius
        self.sample_height = sample_height
        self.sample_center = sample_center

    def generate_container_geometry(self):
        return {
            "Shape": "HollowCylinder",
            "Height": self.sample_height,
            "InnerRadius": self.sample_radius,
            "OuterRadius": self.container_radius,
            "Center": self.sample_center,
        }


class _FlatPlateHolder(object):
    def __init__(self, kwargs, sample_height, sample_width, sample_thickness, sample_center, sample_angle):
        # By using kwargs we get a better error than "init takes n arguments"
        front_thick = common.dictionary_key_helper(
            dictionary=kwargs, key="front_thick", exception_msg=property_err_string.format("front_thick")
        )
        back_thick = common.dictionary_key_helper(
            dictionary=kwargs, key="back_thick", exception_msg=property_err_string.format("back_thick")
        )

        SampleDetails.validate_constructor_inputs({"front_thick": front_thick, "back_thick": back_thick})
        self.front_thick = float(front_thick)
        self.back_thick = float(back_thick)
        self.shape_type = "FlatPlateHolder"

        self.sample_width = sample_width
        self.sample_height = sample_height
        self.sample_center = sample_center
        self.sample_thickness = sample_thickness
        self.sample_angle = sample_angle

    def generate_container_geometry(self):
        return {
            "Shape": "FlatPlate",
            "Width": self.sample_width,
            "Height": self.sample_height,
            "Thick": self.sample_thickness,
            "Center": self.sample_center,
            "Angle": self.sample_angle,
            "FrontThick": self.front_thick,
            "BackThick": self.back_thick,
        }


def _check_value_is_physical(property_name, value):
    original_value = value
    value = _check_can_convert_to_float(property_name=property_name, value=value)

    if property_name == "angle":
        if math.isnan(value):
            raise ValueError("The value set for {} was: {} which is impossible for a physical object".format(property_name, original_value))
    else:
        if value <= 0 or math.isnan(value):
            raise ValueError("The value set for {} was: {} which is impossible for a physical object".format(property_name, original_value))


def _check_can_convert_to_float(property_name, value):
    original_value = value
    value = convert_to_float(value)
    if value is None:
        raise ValueError("Could not convert the {} to a number. The input was: '{}'".format(property_name, original_value))
    return value


def convert_to_float(val):
    try:
        val = float(val)
        return val
    except ValueError:
        return None
