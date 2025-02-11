# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# ruff: noqa: E741  # Ambiguous variable name
import os
import sys
import json


def print_histogram_R_factors(project):
    """prints profile rfactors for all histograms"""
    print("*** Weighted profile R-factor: Rwp " + os.path.split(project.filename)[1])
    for loop_histogram in project.histograms():
        print("\t{:20s}: {:.2f}".format(loop_histogram.name, loop_histogram.get_wR()))
    print("")


def add_histograms(data_filenames, project, instruments, number_regions):
    if number_regions > len(data_filenames):  # many regions in one data and one instrument file
        if len(data_filenames) != 1:
            raise ValueError("There must be one region/bank per focused data file or many regions/banks in one focused data file")
        for loop_region_index in range(1, number_regions + 1):
            project.add_powder_histogram(
                datafile=os.path.join(data_filenames[0]),
                iparams=os.path.join(instruments[0]),
                phases=project.phases(),
                databank=loop_region_index,  # indexing starts at 1
                instbank=loop_region_index,  # indexing starts at 1
            )
    else:  # one region in each data file
        if len(data_filenames) != number_regions:
            raise ValueError("There must be one region/bank per focused data file or many regions/banks in one focused data file")
        if len(instruments) == len(data_filenames):
            for loop_index, loop_data_filename in enumerate(data_filenames):
                project.add_powder_histogram(
                    datafile=os.path.join(loop_data_filename), iparams=os.path.join(instruments[loop_index]), phases=project.phases()
                )
        elif 1 == len(instruments) < len(data_filenames):
            for loop_index, loop_data_filename in enumerate(data_filenames):
                project.add_powder_histogram(
                    datafile=os.path.join(loop_data_filename),
                    iparams=os.path.join(instruments[0]),
                    phases=project.phases(),
                    instbank=loop_index + 1,  # indexing starts at 1
                )
        else:
            raise ValueError(
                "Calling GSASII from Mantid with multiple instrument files and one focused data file is currently not supported"
            )


def add_pawley_reflections(pawley_reflections, project, d_min):
    for iphase, phase in enumerate(project.phases()):
        phase.data["General"]["doPawley"] = True
        gsas_reflections = []
        for reflection in pawley_reflections[iphase]:
            [h, k, l], d, multiplicity = reflection
            gsas_reflections.append([int(h), int(k), int(l), int(multiplicity), float(d), True, 100.0, d_min])
        phase.data["Pawley ref"] = gsas_reflections


def set_max_number_cycles(number_cycles):
    gsas_project.data["Controls"]["data"]["max cyc"] = number_cycles


def enable_background(refine, project):
    for loop_histogram in project.histograms():
        if refine:
            loop_histogram.set_refinements({"Background": {"no. coeffs": 3, "refine": True}})
        else:
            loop_histogram.set_refinements({"Background": {"no. coeffs": 0, "refine": False}})


def enable_histogram_scale_factor(refine, project):
    # GSAS-II default is enabled
    if not refine:
        for loop_histogram in project.histograms():
            loop_histogram.SampleParameters["Scale"] = [1.0, False]


def enable_unit_cell(refine, override_unit_cell_lengths, project):
    for iphase, phase in enumerate(project.phases()):
        if refine:
            phase.set_refinements({"Cell": True})
        if override_unit_cell_lengths:
            phase.data["General"]["Cell"][1:7] = tuple(override_unit_cell_lengths[iphase])


def enable_limits(x_limits, project):
    if x_limits:
        for ihist, xlim in enumerate(zip(*x_limits)):
            project.histograms()[ihist].set_refinements({"Limits": [min(xlim), max(xlim)]})  # ensure min <= max


def run_microstrain_refinement(refine, project, path_to_project):
    if refine:
        for loop_phase in project.phases():
            loop_phase.set_HAP_refinements({"Mustrain": {"type": "isotropic", "refine": True}})
        project.do_refinements()
        project.save(path_to_project)
        print_histogram_R_factors(project)


def run_parameter_refinement(refine, instrument_parameter_string, project, path_to_project):
    if refine:
        for loop_histogram in project.histograms():
            loop_histogram.set_refinements({"Instrument Parameters": [instrument_parameter_string]})
        project.do_refinements()
        project.save(path_to_project)
        print_histogram_R_factors(project)


def export_refinement_to_csv(temp_save_directory, name_of_project, project):
    for histogram_index, loop_histogram in enumerate(project.histograms()):
        loop_histogram.Export(
            os.path.join(temp_save_directory, name_of_project + f"_{histogram_index + 1}.csv"), ".csv", "histogram CSV file"
        )


def export_reflections(temp_save_directory, name_of_project, project):
    for histogram_index, loop_histogram in enumerate(project.histograms()):
        loop_histogram_name = loop_histogram.name.replace(".gss", "").replace(" ", "_")
        for phase_name in loop_histogram.reflections().keys():
            reflection_positions = loop_histogram.reflections()[phase_name]["RefList"][:, 5]
            reflection_file_path = os.path.join(
                temp_save_directory, name_of_project + f"_reflections_{histogram_index + 1}_{phase_name}.txt"
            )
            with open(reflection_file_path, "wt", encoding="utf-8") as file:
                file.write(f"{loop_histogram_name}\n")
                file.write(f"{phase_name}\n")
                for reflection in reflection_positions:
                    file.write(f"{str(reflection)}\n")


def export_refined_instrument_parameters(temp_save_directory, name_of_project, project):
    for loop_histogram in project.histograms():
        loop_histogram_name = loop_histogram.name.replace(".gss", "").replace(" ", "_")
        instrument_parameter_dict = loop_histogram.getHistEntryList(keyname="Instrument")[0][2][0]
        sigma_one = instrument_parameter_dict["sig-1"]  # [initial value, final value, bool whether refined]
        gamma_y = instrument_parameter_dict["Y"]  # as above
        inst_parameters = {"Histogram name": loop_histogram_name, "Sigma-1": sigma_one, "Gamma (Y)": gamma_y}
        inst_parameters_json = json.dumps(inst_parameters, separators=(",", ":"))
        inst_parameters_file_path = os.path.join(temp_save_directory, name_of_project + f"_inst_parameters_{loop_histogram_name}.txt")
        with open(inst_parameters_file_path, "wt", encoding="utf-8") as file:
            file.write(inst_parameters_json)


def export_lattice_parameters(temp_save_directory, name_of_project, project):
    for loop_phase in project.phases():
        cell_parameters = loop_phase.get_cell()
        mustrain = loop_phase.getPhaseEntryList(keyname="Mustrain")[0][2][1][0]  # Users happy this has no unit
        cell_parameters["Microstrain"] = mustrain
        parameters_json = json.dumps(cell_parameters, separators=(",", ":"))
        parameters_file_path = os.path.join(temp_save_directory, name_of_project + f"_cell_parameters_{loop_phase.name}.txt")
        with open(parameters_file_path, "wt", encoding="utf-8") as file:
            file.write(parameters_json)


"""Parse Inputs from Mantid"""
inputs_dict = json.loads(sys.argv[1])

path_to_gsas2 = inputs_dict["path_to_gsas2"]
temporary_save_directory = inputs_dict["temporary_save_directory"]
project_name = inputs_dict["project_name"]
refinement_method = inputs_dict["refinement_method"]
refine_background = inputs_dict["refine_background"]
refine_microstrain = inputs_dict["refine_microstrain"]
refine_sigma_one = inputs_dict["refine_sigma_one"]
refine_gamma = inputs_dict["refine_gamma"]
refine_histogram_scale_factor = inputs_dict["refine_histogram_scale_factor"]
refine_unit_cell = inputs_dict["refine_unit_cell"]
override_cell_lengths = inputs_dict["override_cell_lengths"]
data_files = inputs_dict["data_files"]
phase_files = inputs_dict["phase_files"]
instrument_files = inputs_dict["instrument_files"]
limits = inputs_dict["limits"]
mantid_pawley_reflections = inputs_dict["mantid_pawley_reflections"]
d_spacing_min = inputs_dict["d_spacing_min"]
number_of_regions = inputs_dict["number_of_regions"]

"""Call GSASIIscriptable"""
import_path = None
try:
    import_path = os.path.join(path_to_gsas2, "GSASII")
    sys.path.insert(0, import_path)
    import GSASIIscriptable as G2sc
except ModuleNotFoundError:
    raise ImportError(f"GSAS-II was not found at {import_path}")

project_path = os.path.join(temporary_save_directory, project_name + ".gpx")
gsas_project = G2sc.G2Project(filename=project_path)

for phase_file in phase_files:
    gsas_project.add_phase(os.path.join(phase_file))

add_histograms(data_files, gsas_project, instrument_files, number_of_regions)

if refinement_method == "Pawley" and mantid_pawley_reflections:
    add_pawley_reflections(mantid_pawley_reflections, gsas_project, d_spacing_min)

set_max_number_cycles(3)
enable_background(refine_background, gsas_project)
enable_histogram_scale_factor(refine_histogram_scale_factor, gsas_project)

enable_unit_cell(refine_unit_cell, override_cell_lengths, gsas_project)
enable_limits(limits, gsas_project)

gsas_project.save(project_path)
gsas_project.do_refinements()
gsas_project.save(project_path)
print_histogram_R_factors(gsas_project)

run_microstrain_refinement(refine_microstrain, gsas_project, project_path)
run_parameter_refinement(refine_sigma_one, "sig-1", gsas_project, project_path)
run_parameter_refinement(refine_gamma, "Y", gsas_project, project_path)

export_refinement_to_csv(temporary_save_directory, project_name, gsas_project)
export_lattice_parameters(temporary_save_directory, project_name, gsas_project)
export_refined_instrument_parameters(temporary_save_directory, project_name, gsas_project)
if refinement_method == "Pawley":
    export_reflections(temporary_save_directory, project_name, gsas_project)
