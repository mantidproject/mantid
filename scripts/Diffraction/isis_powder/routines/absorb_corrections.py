from __future__ import (absolute_import, division, print_function)

import mantid.simpleapi as mantid

from isis_powder.routines import common, common_enums, sample_details


def create_vanadium_sample_details_obj(config_dict):
    """
    Creates a SampleDetails object based on a vanadium sample which is found
    in the advanced config of an instrument.
    :param config_dict: The advanced config dictionary of the instrument for the vanadium sample
    :return: A sample details object which holds properties used in sample corrections
    """
    height_key = "cylinder_sample_height"
    radius_key = "cylinder_sample_radius"
    pos_key = "cylinder_position"
    formula_key = "chemical_formula"
    number_density_key = "number_density"

    e_msg = "The following key was not found in the advanced configuration for sample correction:\n"

    height = common.dictionary_key_helper(dictionary=config_dict, key=height_key, exception_msg=e_msg + height_key)
    radius = common.dictionary_key_helper(dictionary=config_dict, key=radius_key, exception_msg=e_msg + radius_key)
    pos = common.dictionary_key_helper(dictionary=config_dict, key=pos_key, exception_msg=e_msg + pos_key)
    formula = common.dictionary_key_helper(dictionary=config_dict, key=formula_key, exception_msg=e_msg + formula_key)
    number_density = common.dictionary_key_helper(dictionary=config_dict, key=number_density_key, throws=False)

    vanadium_sample_details = sample_details.SampleDetails(height=height, radius=radius, center=pos)
    vanadium_sample_details.set_material(chemical_formula=formula, number_density=number_density)
    return vanadium_sample_details


def run_cylinder_absorb_corrections(ws_to_correct, multiple_scattering, sample_details_obj):
    """
    Sets a cylindrical sample from the user specified config dictionary and performs Mayers
    sample correction on the workspace. The SampleDetails object defines the sample, material
    and associated properties. Additionally it checks the value of multiple_scattering to
    determine whether to take into account the effects of multiple scattering.
    :param ws_to_correct: The workspace to perform Mayers sample correction on
    :param multiple_scattering: Boolean of whether to account for the effects of multiple scattering
    :param sample_details_obj: The object containing the sample details
    :return: The corrected workspace
    """

    if not isinstance(sample_details_obj, sample_details.SampleDetails):
        raise RuntimeError("A SampleDetails object was not set or a different object type was found. Please create a"
                           " SampleDetails object and set the relevant properties it. Then set the new sample by "
                           "calling set_sample_details()")

    height = sample_details_obj.height
    radius = sample_details_obj.radius
    pos = sample_details_obj.center

    # Get the underlying material object
    if not sample_details_obj.is_material_set():
        raise RuntimeError("The material for this sample has not been set yet. Please call"
                           " set_material on the SampleDetails object to set the material")
    material_obj = sample_details_obj.material_object
    formula = material_obj.chemical_formula
    number_density = material_obj.number_density

    ws_to_correct = _calculate__cylinder_absorb_corrections(
        ws_to_correct=ws_to_correct, multiple_scattering=multiple_scattering,
        c_height=height, c_radius=radius, c_pos=pos, chemical_formula=formula, number_density=number_density)

    return ws_to_correct


def _calculate__cylinder_absorb_corrections(ws_to_correct, multiple_scattering,
                                            c_height, c_radius, c_pos, chemical_formula, number_density):
    """
    Calculates vanadium absorption corrections for the specified workspace. The workspace
    should have any monitor spectra masked before being passed into this method. Additionally
    it takes details of the sample container and sets the sample geometry to a cylinder with
    the specified geometry. This function uses Mayers Sample Correction to perform the corrections.
    :param ws_to_correct: The workspace to apply the sample corrections to
    :param multiple_scattering: True if the effects of multiple scattering should be accounted for, else False
    :param c_height: The height of the cylinder as a float
    :param c_radius: The radius of the cylinder as a float
    :param c_pos: The position as a list of three float values
    :param chemical_formula: The chemical formula of the container - usually set to 'V' for Vanadium
    :param number_density: The number density if the element is not elemental. Note it is up to the caller
    to ensure a none elemental formula has associated number density
    :return: The workspace with corrections applied
    """
    geometry_json = {'Shape': 'Cylinder', 'Height': c_height,
                     'Radius': c_radius, 'Center': c_pos}
    material_json = {'ChemicalFormula': chemical_formula}

    if number_density:
        material_json["SampleNumberDensity"] = number_density

    mantid.SetSample(InputWorkspace=ws_to_correct, Geometry=geometry_json, Material=material_json)

    previous_units = ws_to_correct.getAxis(0).getUnit().unitID()
    ws_units = common_enums.WORKSPACE_UNITS

    # Mayers Sample correction must be completed in TOF, convert if needed. Then back to original units afterwards
    if previous_units != ws_units.tof:
        ws_to_correct = mantid.ConvertUnits(InputWorkspace=ws_to_correct, OutputWorkspace=ws_to_correct,
                                            Target=ws_units.tof)
    ws_to_correct = mantid.MayersSampleCorrection(InputWorkspace=ws_to_correct, OutputWorkspace=ws_to_correct,
                                                  MultipleScattering=multiple_scattering)
    if previous_units != ws_units.tof:
        ws_to_correct = mantid.ConvertUnits(InputWorkspace=ws_to_correct, OutputWorkspace=ws_to_correct,
                                            Target=previous_units)

    return ws_to_correct
