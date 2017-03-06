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
    chemical_key = "chemical_properties"

    e_msg = "The following key was not found in the advanced configuration for sample correction:\n"

    height = common.dictionary_key_helper(dictionary=config_dict, key=height_key, exception_msg=e_msg + height_key)
    radius = common.dictionary_key_helper(dictionary=config_dict, key=radius_key, exception_msg=e_msg + radius_key)
    pos = common.dictionary_key_helper(dictionary=config_dict, key=pos_key, exception_msg=e_msg + pos_key)

    formula = common.dictionary_key_helper(dictionary=config_dict, key=formula_key, exception_msg=e_msg + formula_key)
    chemical_properties = common.dictionary_key_helper(dictionary=config_dict, key=chemical_key, throws=False)

    position_dict = {
        "cylinder_height": height,
        "cylinder_radius": radius,
        "cylinder_pos": pos
    }

    ws_to_correct = _calculate__cylinder_absorb_corrections(
        ws_to_correct=ws_to_correct, multiple_scattering=multiple_scattering,
        position_dict=position_dict, chemical_formula=formula, material_properties=chemical_properties)

    return ws_to_correct


def _calculate__cylinder_absorb_corrections(ws_to_correct, multiple_scattering,
                                            position_dict, chemical_formula, material_properties):
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
    :return: The workspace with corrections applied
    """
    geometry_json = {'Shape': 'Cylinder', 'Height': position_dict["cylinder_height"],
                     'Radius': position_dict["cylinder_radius"], 'Center': position_dict["cylinder_pos"]}
    material_json = _get_material_json(chemical_formula=chemical_formula, material_properties=material_properties)

    mantid.SetSample(InputWorkspace=ws_to_correct, Geometry=geometry_json, Material=material_json)

    ws_to_correct = mantid.ConvertUnits(InputWorkspace=ws_to_correct, OutputWorkspace=ws_to_correct, Target="TOF")
    ws_to_correct = mantid.MayersSampleCorrection(InputWorkspace=ws_to_correct, OutputWorkspace=ws_to_correct,
                                                  MultipleScattering=multiple_scattering)
    ws_to_correct = mantid.ConvertUnits(InputWorkspace=ws_to_correct, OutputWorkspace=ws_to_correct, Target="dSpacing")

    return ws_to_correct


def _get_material_json(chemical_formula, material_properties):
    """
    Returns a material JSON with either a chemical formula when using elemental chemicals
    or uses user set chemical properties when required or the user has set them to generate
    the required input for SetMaterial
    :param chemical_formula: The formula of the chemical to use
    :param material_properties: The materials properties dictionary as set by the user
    :return: The material JSON for setSample with appropriate settings.
    """
    material_json = {'ChemicalFormula': chemical_formula}

    is_elemental_vanadium = True if chemical_formula.lower() == 'v' else False

    # Test if we can just use built in Mantid properties
    if is_elemental_vanadium and not material_properties:
        return material_json

    # Else we have to parse the chemical formula specified below
    err_message = "Custom properties was detected for the material but the required key was not set:\n"

    attenuation_key = "attenuation_cross_section"
    scattering_key = "scattering_cross_section"
    sample_density_key = "sample_number_density"

    attenuation_x_section = common.dictionary_key_helper(material_properties, attenuation_key,
                                                         exception_msg=err_message + attenuation_key)
    scattering_x_section = common.dictionary_key_helper(material_properties, scattering_key,
                                                        exception_msg=err_message + scattering_key)
    sample_density = common.dictionary_key_helper(material_properties, sample_density_key,
                                                  exception_msg=err_message + sample_density_key)

    material_json = {"SampleNumberDensity": sample_density,
                     "AttenuationXSection": attenuation_x_section,
                     "ScatteringXSection": scattering_x_section}

    print ("Using custom chemical properties:")
    for k, v in material_json.viewitems():
        print (k, v)

    return material_json
