# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from unittest import mock
import os
import shutil
import tempfile

from mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.gsas2.call_G2sc import (
    add_histograms,
    add_pawley_reflections,
    set_max_number_cycles,
    add_phases,
    enable_background,
    enable_histogram_scale_factor,
    enable_unit_cell,
    enable_limits,
    run_microstrain_refinement,
    run_parameter_refinement,
    export_refinement_to_csv,
    export_reflections,
    export_refined_instrument_parameters,
    export_lattice_parameters,
)

import numpy as np


class GSAS2ViewTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.temp_save_directory = tempfile.mkdtemp()

    @classmethod
    def tearDownClass(cls) -> None:
        shutil.rmtree(cls.temp_save_directory)

    def setUp(self):
        self.project = mock.Mock()

    def test_add_phases(self):
        phase_files = ["file_1", "file_2"]
        add_phases(self.project, phase_files)
        self.project.add_phase.assert_has_calls([mock.call("file_1"), mock.call("file_2")])

    def test_add_histograms_with_single_datafile(self):
        data_filenames = ["data_file_1"]
        instruments = ["instr_1"]
        number_of_regions = 3
        add_histograms(data_filenames, self.project, instruments, number_of_regions)
        self.project.add_powder_histogram.assert_called()

    def test_add_histograms_same_number_of_instruments_and_datafiles(self):
        self.project.phases.return_value = ["phase_1"]
        data_filenames = ["data_file_1", "data_file_2"]
        instruments = ["instr_1", "instr_2"]
        number_of_regions = 2
        add_histograms(data_filenames, self.project, instruments, number_of_regions)
        expected_calls = [
            mock.call(datafile="data_file_1", iparams="instr_1", phases=["phase_1"]),
            mock.call(datafile="data_file_2", iparams="instr_2", phases=["phase_1"]),
        ]
        self.project.add_powder_histogram.assert_has_calls(expected_calls)

    def test_add_histograms_more_datafiles_than_instruments(self):
        self.project.phases.return_value = ["phase_1"]
        data_filenames = ["data_file_1", "data_file_2"]
        instruments = ["instr_1"]
        number_of_regions = 2
        add_histograms(data_filenames, self.project, instruments, number_of_regions)
        expected_calls = [
            mock.call(datafile="data_file_1", iparams="instr_1", phases=["phase_1"], instbank=1),
            mock.call(datafile="data_file_2", iparams="instr_1", phases=["phase_1"], instbank=2),
        ]
        self.project.add_powder_histogram.assert_has_calls(expected_calls)

    def test_add_histograms_multiple_regions_for_single_datafile(self):
        self.project.phases.return_value = ["phase_1"]
        data_filenames = ["data_file_1"]
        instruments = ["instr_1", "instr_2"]
        number_of_regions = 3
        add_histograms(data_filenames, self.project, instruments, number_of_regions)
        expected_calls = [
            mock.call(datafile="data_file_1", iparams="instr_1", phases=["phase_1"], databank=1, instbank=1),
            mock.call(datafile="data_file_1", iparams="instr_1", phases=["phase_1"], databank=2, instbank=2),
            mock.call(datafile="data_file_1", iparams="instr_1", phases=["phase_1"], databank=3, instbank=3),
        ]
        self.project.add_powder_histogram.assert_has_calls(expected_calls)

    def test_add_histograms_throws_for_more_datafiles_than_number_of_regions(self):
        data_filenames = ["data_file_1", "data_file_2"]
        instruments = ["instr_1", "instr_2"]
        number_of_regions = 3
        with self.assertRaises(ValueError):
            add_histograms(data_filenames, self.project, instruments, number_of_regions)

    def test_add_histograms_throws_for_less_datafiles_than_number_of_regions(self):
        data_filenames = ["data_file_1", "data_file_2"]
        instruments = ["instr_1", "instr_2"]
        number_of_regions = 1
        with self.assertRaises(ValueError):
            add_histograms(data_filenames, self.project, instruments, number_of_regions)

    def test_add_histograms_throws_for_less_datafiles_than_instruments(self):
        data_filenames = ["data_file_1", "data_file_2"]
        instruments = ["instr_1", "instr_2", "instr_3"]
        number_of_regions = 2
        with self.assertRaises(ValueError):
            add_histograms(data_filenames, self.project, instruments, number_of_regions)

    def test_add_pawley_reflections(self):
        phase_1 = mock.Mock()
        phase_1.data = {"General": {"doPawley": False}}
        phase_2 = mock.Mock()
        phase_2.data = {"General": {"doPawley": False}}
        self.project.phases.return_value = [phase_1, phase_2]
        pawley_reflections = [[([1, 2, 3], 4.1, 5), ([6, 7, 8], 9.1, 10)], [([1, 2, 3], 4.1, 5)]]
        add_pawley_reflections(pawley_reflections, self.project, 5.5)
        self.assertEqual(phase_1.data["General"]["doPawley"], True)
        self.assertEqual(phase_1.data["Pawley ref"], [[1, 2, 3, 5, 4.1, True, 100.0, 5.5], [6, 7, 8, 10, 9.1, True, 100.0, 5.5]])
        self.assertEqual(phase_2.data["General"]["doPawley"], True)
        self.assertEqual(phase_2.data["Pawley ref"], [[1, 2, 3, 5, 4.1, True, 100.0, 5.5]])

    def test_set_max_number_cycles(self):
        self.project.data = {"Controls": {"data": {"max cyc": 0}}}
        set_max_number_cycles(self.project, 5)
        self.assertEqual(self.project.data["Controls"]["data"]["max cyc"], 5)

    def test_enable_background(self):
        hist_1 = mock.Mock()
        hist_2 = mock.Mock()
        self.project.histograms.return_value = [hist_1, hist_2]
        enable_background(True, self.project)
        hist_1.set_refinements.assert_called_with({"Background": {"no. coeffs": 3, "refine": True}})
        hist_2.set_refinements.assert_called_with({"Background": {"no. coeffs": 3, "refine": True}})
        enable_background(False, self.project)
        hist_1.set_refinements.assert_called_with({"Background": {"no. coeffs": 0, "refine": False}})
        hist_2.set_refinements.assert_called_with({"Background": {"no. coeffs": 0, "refine": False}})

    def test_enable_histogram_scale_factor(self):
        hist_1 = mock.Mock()
        hist_1.SampleParameters = {}
        hist_2 = mock.Mock()
        hist_2.SampleParameters = {}
        self.project.histograms.return_value = [hist_1, hist_2]
        enable_histogram_scale_factor(True, self.project)
        self.assertEqual(hist_1.SampleParameters, {})
        self.assertEqual(hist_2.SampleParameters, {})
        enable_histogram_scale_factor(False, self.project)
        self.assertEqual(hist_1.SampleParameters, {"Scale": [1.0, False]})
        self.assertEqual(hist_2.SampleParameters, {"Scale": [1.0, False]})

    def test_enable_unit_cell(self):
        phase_1 = mock.Mock()
        phase_1.data = {"General": {"Cell": [0, 0, 0, 0, 0, 0, 0, 0, 0]}}
        phase_2 = mock.Mock()
        phase_2.data = {"General": {"Cell": [0, 0, 0, 0, 0, 0, 0, 0, 0]}}
        self.project.phases.return_value = [phase_1, phase_2]
        enable_unit_cell(True, [[1, 1, 1, 1, 1, 1], [2, 2, 2, 2, 2, 2]], self.project)
        phase_1.set_refinements.assert_called_with({"Cell": True})
        self.assertEqual(phase_1.data["General"]["Cell"], [0, 1, 1, 1, 1, 1, 1, 0, 0])
        phase_2.set_refinements.assert_called_with({"Cell": True})
        self.assertEqual(phase_2.data["General"]["Cell"], [0, 2, 2, 2, 2, 2, 2, 0, 0])

    def test_enable_limits(self):
        hist_1 = mock.Mock()
        hist_2 = mock.Mock()
        self.project.histograms.return_value = [hist_1, hist_2]
        x_limits = [(1, 5), (6, 10)]
        enable_limits(x_limits, self.project)
        hist_1.set_refinements.assert_called_with({"Limits": [1, 6]})
        hist_2.set_refinements.assert_called_with({"Limits": [5, 10]})

    def test_run_microstrain_refinement(self):
        phase_1 = mock.Mock()
        phase_2 = mock.Mock()
        self.project.phases.return_value = [phase_1, phase_2]
        self.project.filename = "project_filename"
        self.project.histograms.return_value = []
        run_microstrain_refinement(True, self.project, "save_path")
        phase_1.set_HAP_refinements.assert_called_with({"Mustrain": {"type": "isotropic", "refine": True}})
        phase_2.set_HAP_refinements.assert_called_with({"Mustrain": {"type": "isotropic", "refine": True}})
        self.project.do_refinements.called_once()
        self.project.save.called_with("save_path")

    def test_run_parameter_refinement(self):
        hist_1 = mock.Mock()
        hist_1.name = "hist_1"
        hist_1.get_wR.return_value = 5
        hist_2 = mock.Mock()
        hist_2.name = "hist_2"
        hist_2.get_wR.return_value = 5
        self.project.histograms.return_value = [hist_1, hist_2]
        self.project.filename = "project_filename"
        run_parameter_refinement(True, "instr_par_string", self.project, "save_path")
        hist_1.set_refinements.assert_called_with({"Instrument Parameters": ["instr_par_string"]})
        hist_2.set_refinements.assert_called_with({"Instrument Parameters": ["instr_par_string"]})
        self.project.do_refinements.called_once()
        self.project.save.called_with("save_path")

    def test_export_refinement_to_csv(self):
        hist_1 = mock.Mock()
        hist_2 = mock.Mock()
        self.project.histograms.return_value = [hist_1, hist_2]
        export_refinement_to_csv("temp_save_directory", "project_name", self.project)
        hist_1.Export.called_with(os.path.join("temp_save_directory", "project_name_1.csv"), ".csv", "histogram CSV file")
        hist_2.Export.called_with(os.path.join("temp_save_directory", "project_name_2.csv"), ".csv", "histogram CSV file")

    def test_export_reflections(self):
        hist_1 = mock.Mock()
        hist_1.name = "hist 1.gss"
        hist_2 = mock.Mock()
        hist_2.name = "hist 2.gss"
        self.project.histograms.return_value = [hist_1, hist_2]
        hist_1.reflections.return_value = {
            "phase_1": {"RefList": np.arange(18).reshape(3, 6)},
            "phase_2": {"RefList": np.arange(20, 38).reshape(3, 6)},
        }
        hist_2.reflections.return_value = {}
        export_reflections(self.temp_save_directory, "project", self.project)

        with open(os.path.join(self.temp_save_directory, "project_reflections_1_phase_1.txt"), "rt", encoding="utf-8") as file:
            str_1 = file.read()
            self.assertEqual(str_1, "hist_1\nphase_1\n5\n11\n17\n")

        with open(os.path.join(self.temp_save_directory, "project_reflections_1_phase_2.txt"), "rt", encoding="utf-8") as file:
            str_2 = file.read()
            self.assertEqual(str_2, "hist_1\nphase_2\n25\n31\n37\n")

    def test_export_refined_instrument_parameters(self):
        hist_1 = mock.Mock()
        hist_1.name = "hist 1.gss"
        hist_2 = mock.Mock()
        hist_2.name = "hist 2.gss"
        self.project.histograms.return_value = [hist_1, hist_2]
        hist_1.getHistEntryList.return_value = [[0, 1, [{"sig-1": 5, "Y": 10}]]]
        hist_2.getHistEntryList.return_value = [[0, 1, [{"sig-1": 15, "Y": 20}]]]
        export_refined_instrument_parameters(self.temp_save_directory, "project", self.project)

        with open(os.path.join(self.temp_save_directory, "project_inst_parameters_hist_1.txt"), "rt", encoding="utf-8") as file:
            str_1 = file.read()
            self.assertEqual(str_1, """{"Histogram name":"hist_1","Sigma-1":5,"Gamma (Y)":10}""")

        with open(os.path.join(self.temp_save_directory, "project_inst_parameters_hist_2.txt"), "rt", encoding="utf-8") as file:
            str_2 = file.read()
            self.assertEqual(str_2, """{"Histogram name":"hist_2","Sigma-1":15,"Gamma (Y)":20}""")

    def test_export_lattice_parameters(self):
        phase_1 = mock.Mock()
        phase_1.name = "phase_1"
        phase_1.get_cell.return_value = {}
        phase_1.getPhaseEntryList.return_value = [[0, 0, [0, [10]]]]
        phase_2 = mock.Mock()
        phase_2.name = "phase_2"
        phase_2.get_cell.return_value = {}
        phase_2.getPhaseEntryList.return_value = [[0, 0, [0, [10]]]]
        self.project.phases.return_value = [phase_1, phase_2]
        export_lattice_parameters(self.temp_save_directory, "project", self.project)

        with open(os.path.join(self.temp_save_directory, "project_cell_parameters_phase_1.txt"), "rt", encoding="utf-8") as file:
            str_1 = file.read()
            self.assertEqual(str_1, """{"Microstrain":10}""")

        with open(os.path.join(self.temp_save_directory, "project_cell_parameters_phase_2.txt"), "rt", encoding="utf-8") as file:
            str_2 = file.read()
            self.assertEqual(str_2, """{"Microstrain":10}""")
