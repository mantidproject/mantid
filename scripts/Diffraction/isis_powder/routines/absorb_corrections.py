# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
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

    vanadium_sample_details = sample_details.SampleDetails(height=height, radius=radius, center=pos, shape="cylinder")
    vanadium_sample_details.set_material(chemical_formula=formula, number_density=number_density)
    return vanadium_sample_details


def run_cylinder_absorb_corrections(ws_to_correct, multiple_scattering, sample_details_obj, is_vanadium, msevents=None):
    """
    Sets a cylindrical sample from the user specified config dictionary and performs Mayers
    sample correction on the workspace. The SampleDetails object defines the sample, material
    and associated properties. Additionally it checks the value of multiple_scattering to
    determine whether to take into account the effects of multiple scattering.
    :param ws_to_correct: The workspace to perform Mayers sample correction on
    :param multiple_scattering: Boolean of whether to account for the effects of multiple scattering
    :param sample_details_obj: The object containing the sample details
    :param is_vanadium: Whether the sample is a vanadium
    :param msevents: parameter MSEvents in MayerSampleCorrection algorithm (number of events in mult. scat. calc.)
    :return: The corrected workspace
    """

    if not isinstance(sample_details_obj, sample_details.SampleDetails):
        raise RuntimeError(
            "A SampleDetails object was not set or a different object type was found when sample"
            " absorption corrections were requested. If you want sample absorption corrections please "
            "create a SampleDetails object and set the relevant properties it. "
            "Then set the new sample by calling set_sample_details()."
        )

    # Get the underlying material object
    if not sample_details_obj.is_material_set():
        raise RuntimeError(
            "The material for this sample has not been set yet. Please call set_material on the SampleDetails object to set the material"
        )
    if multiple_scattering and not is_vanadium:
        raise NotImplementedError("Multiple scattering absorption corrections are not yet implemented for anisotropic samples")
    ws_to_correct = _calculate__cylinder_absorb_corrections(
        ws_to_correct=ws_to_correct,
        multiple_scattering=multiple_scattering,
        sample_details_obj=sample_details_obj,
        is_vanadium=is_vanadium,
        msevents=msevents,
    )
    return ws_to_correct


def _calculate__cylinder_absorb_corrections(ws_to_correct, multiple_scattering, sample_details_obj, is_vanadium, msevents):
    """
    Calculates vanadium absorption corrections for the specified workspace. The workspace
    should have any monitor spectra masked before being passed into this method. Additionally
    it takes details of the sample container and sets the sample geometry to a cylinder with
    the specified geometry. This function uses Mayers Sample Correction to perform the corrections.
    :param ws_to_correct: The workspace to apply the sample corrections to
    :param multiple_scattering: True if the effects of multiple scattering should be accounted for, else False
    :param sample_details_obj: The SampleDetails object in a checked state which describes the sample
    to ensure a none elemental formula has associated number density
    :param is_vanadium: Whether the sample is a vanadium
    :return: The workspace with corrections applied
    """
    _setup_sample_for_cylinder_absorb_corrections(ws_to_correct=ws_to_correct, sample_details_obj=sample_details_obj)
    ws_to_correct = _do_cylinder_absorb_corrections(
        ws_to_correct=ws_to_correct,
        multiple_scattering=multiple_scattering,
        is_vanadium=is_vanadium,
        msevents=msevents,
    )
    return ws_to_correct


def _setup_sample_for_cylinder_absorb_corrections(ws_to_correct, sample_details_obj):
    geometry_json = sample_details_obj.generate_sample_geometry()
    material_json = sample_details_obj.generate_sample_material()
    mantid.SetSample(InputWorkspace=ws_to_correct, Geometry=geometry_json, Material=material_json)


def _do_cylinder_absorb_corrections(ws_to_correct, multiple_scattering, is_vanadium, msevents):
    previous_units = ws_to_correct.getAxis(0).getUnit().unitID()
    ws_units = common_enums.WORKSPACE_UNITS

    # Mayers Sample correction must be completed in TOF, convert if needed. Then back to original units afterwards
    if previous_units != ws_units.tof:
        ws_to_correct = mantid.ConvertUnits(InputWorkspace=ws_to_correct, Target=ws_units.tof, OutputWorkspace=ws_to_correct)

    if is_vanadium:
        ws_to_correct = mantid.MayersSampleCorrection(
            InputWorkspace=ws_to_correct, MultipleScattering=multiple_scattering, OutputWorkspace=ws_to_correct, MSEvents=msevents
        )
    else:
        # Ensure we never do multiple scattering if the sample is not isotropic (e.g. not a Vanadium)
        ws_to_correct = mantid.MayersSampleCorrection(InputWorkspace=ws_to_correct, MultipleScattering=False, OutputWorkspace=ws_to_correct)
    if previous_units != ws_units.tof:
        ws_to_correct = mantid.ConvertUnits(InputWorkspace=ws_to_correct, Target=previous_units, OutputWorkspace=ws_to_correct)
    return ws_to_correct


def apply_paalmanpings_absorb_and_subtract_empty(workspace, summed_empty, sample_details, paalman_pings_events_per_point=None):
    if paalman_pings_events_per_point:
        events_per_point = int(paalman_pings_events_per_point)
    else:
        events_per_point = 1000

    container_geometry = sample_details.generate_container_geometry()
    container_material = sample_details.generate_container_material()
    if container_geometry and container_material:
        mantid.SetSample(
            workspace,
            Geometry=sample_details.generate_sample_geometry(),
            Material=sample_details.generate_sample_material(),
            ContainerGeometry=container_geometry,
            ContainerMaterial=container_material,
        )
        corrections = mantid.PaalmanPingsMonteCarloAbsorption(InputWorkspace=workspace, Shape="Preset", EventsPerPoint=events_per_point)
        workspace = mantid.ApplyPaalmanPingsCorrection(
            SampleWorkspace=workspace, CorrectionsWorkspace=corrections, CanWorkspace=summed_empty
        )
    else:
        mantid.SetSample(workspace, Geometry=sample_details.generate_sample_geometry(), Material=sample_details.generate_sample_material())
        corrections = mantid.PaalmanPingsMonteCarloAbsorption(InputWorkspace=workspace, Shape="Preset", EventsPerPoint=events_per_point)
        workspace = mantid.ApplyPaalmanPingsCorrection(SampleWorkspace=workspace, CorrectionsWorkspace=corrections)
    return workspace
