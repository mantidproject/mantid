from __future__ import (absolute_import, division, print_function)

import mantid.simpleapi as mantid

from isis_powder.routines import common


def run_cylinder_absorb_corrections(ws_to_correct, multiple_scattering, config_dict):
    """
    Sets a cylindrical sample from the user specified config dictionary and performs Mayers
    sample correction on the workspace. The config dictionary must be for a cylinder and contain
    the keys "cylinder_sample_height", "cylinder_sample_radius", "cylinder_position" and
    "chemical_formula". If any of these keys are not found an exception is raise informing
    the user that the key was not in the advanced configuration file.
    Additionally it checks the value of multiple_scattering to determine whether to take
    into account the effects of multiple scattering.
    :param ws_to_correct: The workspace to perform Mayers sample correction on
    :param multiple_scattering: Boolean of whether to account for the effects of multiple scattering
    :param config_dict: A dictionary containing the required keys to set a cylinder sample
    :return: The corrected workspace
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
    if len(formula) > 1:
        # Not a trivial element so we need them to manually specify number density as Mantid
        # struggles to calculate it
        number_density = common.dictionary_key_helper(
            dictionary=config_dict, key=number_density_key,
            exception_msg="The number density is required as the chemical formula (" + str(formula) + ")"
            " is not a single element. The number density was not found. Please add the following key: " +
                          number_density_key)
    else:
        number_density = None

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

    ws_to_correct = mantid.ConvertUnits(InputWorkspace=ws_to_correct, OutputWorkspace=ws_to_correct, Target="TOF")
    ws_to_correct = mantid.MayersSampleCorrection(InputWorkspace=ws_to_correct, OutputWorkspace=ws_to_correct,
                                                  MultipleScattering=multiple_scattering)
    ws_to_correct = mantid.ConvertUnits(InputWorkspace=ws_to_correct, OutputWorkspace=ws_to_correct, Target="dSpacing")

    return ws_to_correct
