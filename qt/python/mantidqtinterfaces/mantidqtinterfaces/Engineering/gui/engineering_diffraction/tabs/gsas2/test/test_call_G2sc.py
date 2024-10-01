# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from unittest import mock

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
)


class GSAS2ViewTest(unittest.TestCase):
    def setUp(self) -> None:
        pass

    def tearDown(self) -> None:
        pass

    def test_add_phases(self):
        project = mock.Mock()
        phase_files = ["file_1", "file_2"]
        add_phases(project, phase_files)
        project.add_phase.assert_has_calls([mock.call("file_1"), mock.call("file_2")])

    def test_add_histograms_with_single_datafile(self):
        project = mock.Mock()
        data_filenames = ["data_file_1"]
        instruments = ["instr_1"]
        number_of_regions = 3
        add_histograms(data_filenames, project, instruments, number_of_regions)
        project.add_powder_histogram.assert_called()

    def test_add_histograms_same_number_of_instruments_and_datafiles(self):
        project = mock.Mock()
        project.phases.return_value = ["phase_1"]
        data_filenames = ["data_file_1", "data_file_2"]
        instruments = ["instr_1", "instr_2"]
        number_of_regions = 2
        add_histograms(data_filenames, project, instruments, number_of_regions)
        expected_calls = [
            mock.call(datafile="data_file_1", iparams="instr_1", phases=["phase_1"]),
            mock.call(datafile="data_file_2", iparams="instr_2", phases=["phase_1"]),
        ]
        project.add_powder_histogram.assert_has_calls(expected_calls)

    def test_add_histograms_more_datafiles_than_instruments(self):
        project = mock.Mock()
        project.phases.return_value = ["phase_1"]
        data_filenames = ["data_file_1", "data_file_2"]
        instruments = ["instr_1"]
        number_of_regions = 2
        add_histograms(data_filenames, project, instruments, number_of_regions)
        expected_calls = [
            mock.call(datafile="data_file_1", iparams="instr_1", phases=["phase_1"], instbank=1),
            mock.call(datafile="data_file_2", iparams="instr_1", phases=["phase_1"], instbank=2),
        ]
        project.add_powder_histogram.assert_has_calls(expected_calls)

    def test_add_histograms_multiple_regions_for_single_datafile(self):
        project = mock.Mock()
        project.phases.return_value = ["phase_1"]
        data_filenames = ["data_file_1"]
        instruments = ["instr_1", "instr_2"]
        number_of_regions = 3
        add_histograms(data_filenames, project, instruments, number_of_regions)
        expected_calls = [
            mock.call(datafile="data_file_1", iparams="instr_1", phases=["phase_1"], databank=1, instbank=1),
            mock.call(datafile="data_file_1", iparams="instr_1", phases=["phase_1"], databank=2, instbank=2),
            mock.call(datafile="data_file_1", iparams="instr_1", phases=["phase_1"], databank=3, instbank=3),
        ]
        project.add_powder_histogram.assert_has_calls(expected_calls)

    def test_add_histograms_throws_for_more_datafiles_than_number_of_regions(self):
        project = mock.Mock()
        data_filenames = ["data_file_1", "data_file_2"]
        instruments = ["instr_1", "instr_2"]
        number_of_regions = 3
        with self.assertRaises(ValueError):
            add_histograms(data_filenames, project, instruments, number_of_regions)

    def test_add_histograms_throws_for_less_datafiles_than_number_of_regions(self):
        project = mock.Mock()
        data_filenames = ["data_file_1", "data_file_2"]
        instruments = ["instr_1", "instr_2"]
        number_of_regions = 1
        with self.assertRaises(ValueError):
            add_histograms(data_filenames, project, instruments, number_of_regions)

    def test_add_histograms_throws_for_less_datafiles_than_instruments(self):
        project = mock.Mock()
        data_filenames = ["data_file_1", "data_file_2"]
        instruments = ["instr_1", "instr_2", "instr_3"]
        number_of_regions = 2
        with self.assertRaises(ValueError):
            add_histograms(data_filenames, project, instruments, number_of_regions)

    def test_add_pawley_reflections(self):
        project = mock.Mock()
        phase_1 = mock.Mock()
        phase_1.data = {"General": {"doPawley": False}}
        phase_2 = mock.Mock()
        phase_2.data = {"General": {"doPawley": False}}
        project.phases.return_value = [phase_1, phase_2]
        pawley_reflections = [[([1, 2, 3], 4.1, 5), ([6, 7, 8], 9.1, 10)], [([1, 2, 3], 4.1, 5)]]
        add_pawley_reflections(pawley_reflections, project, 5.5)
        self.assertEqual(phase_1.data["General"]["doPawley"], True)
        self.assertEqual(phase_1.data["Pawley ref"], [[1, 2, 3, 5, 4.1, True, 100.0, 5.5], [6, 7, 8, 10, 9.1, True, 100.0, 5.5]])
        self.assertEqual(phase_2.data["General"]["doPawley"], True)
        self.assertEqual(phase_2.data["Pawley ref"], [[1, 2, 3, 5, 4.1, True, 100.0, 5.5]])

    def test_set_max_number_cycles(self):
        project = mock.Mock()
        project.data = {"Controls": {"data": {"max cyc": 0}}}
        set_max_number_cycles(project, 5)
        self.assertEqual(project.data["Controls"]["data"]["max cyc"], 5)

    def test_enable_background(self):
        project = mock.Mock()
        hist_1 = mock.Mock()
        hist_2 = mock.Mock()
        project.histograms.return_value = [hist_1, hist_2]
        enable_background(True, project)
        hist_1.set_refinements.assert_called_with({"Background": {"no. coeffs": 3, "refine": True}})
        hist_2.set_refinements.assert_called_with({"Background": {"no. coeffs": 3, "refine": True}})
        enable_background(False, project)
        hist_1.set_refinements.assert_called_with({"Background": {"no. coeffs": 0, "refine": False}})
        hist_2.set_refinements.assert_called_with({"Background": {"no. coeffs": 0, "refine": False}})

    def test_enable_histogram_scale_factor(self):
        project = mock.Mock()
        hist_1 = mock.Mock()
        hist_1.SampleParameters = {}
        hist_2 = mock.Mock()
        hist_2.SampleParameters = {}
        project.histograms.return_value = [hist_1, hist_2]
        enable_histogram_scale_factor(True, project)
        self.assertEqual(hist_1.SampleParameters, {})
        self.assertEqual(hist_2.SampleParameters, {})
        enable_histogram_scale_factor(False, project)
        self.assertEqual(hist_1.SampleParameters, {"Scale": [1.0, False]})
        self.assertEqual(hist_2.SampleParameters, {"Scale": [1.0, False]})

    def test_enable_unit_cell(self):
        project = mock.Mock()
        phase_1 = mock.Mock()
        phase_1.data = {"General": {"Cell": [0, 0, 0, 0, 0, 0, 0, 0, 0]}}
        phase_2 = mock.Mock()
        phase_2.data = {"General": {"Cell": [0, 0, 0, 0, 0, 0, 0, 0, 0]}}
        project.phases.return_value = [phase_1, phase_2]
        enable_unit_cell(True, [[1, 1, 1, 1, 1, 1], [2, 2, 2, 2, 2, 2]], project)
        phase_1.set_refinements.assert_called_with({"Cell": True})
        self.assertEqual(phase_1.data["General"]["Cell"], [0, 1, 1, 1, 1, 1, 1, 0, 0])
        phase_2.set_refinements.assert_called_with({"Cell": True})
        self.assertEqual(phase_2.data["General"]["Cell"], [0, 2, 2, 2, 2, 2, 2, 0, 0])

    def test_enable_limits(self):
        project = mock.Mock()
        hist_1 = mock.Mock()
        hist_2 = mock.Mock()
        project.histograms.return_value = [hist_1, hist_2]
        x_limits = [(1, 5), (6, 10)]
        enable_limits(x_limits, project)
        hist_1.set_refinements.assert_called_with({"Limits": [1, 6]})
        hist_2.set_refinements.assert_called_with({"Limits": [5, 10]})

    def test_run_microstrain_refinement(self):
        project = mock.Mock()
        phase_1 = mock.Mock()
        phase_2 = mock.Mock()
        project.phases.return_value = [phase_1, phase_2]
        project.filename = "project_filename"
        project.histograms.return_value = []
        run_microstrain_refinement(True, project, "save_path")
        phase_1.set_HAP_refinements.assert_called_with({"Mustrain": {"type": "isotropic", "refine": True}})
        phase_2.set_HAP_refinements.assert_called_with({"Mustrain": {"type": "isotropic", "refine": True}})
        project.do_refinements.called_once()
        project.save.called_with("save_path")

    def test_run_parameter_refinement(self):
        project = mock.Mock()
        hist_1 = mock.Mock()
        hist_1.name = "hist_1"
        hist_1.get_wR.return_value = 5
        hist_2 = mock.Mock()
        hist_2.name = "hist_2"
        hist_2.get_wR.return_value = 5
        project.histograms.return_value = [hist_1, hist_2]
        project.filename = "project_filename"
        run_parameter_refinement(True, "instr_par_string", project, "save_path")
        hist_1.set_refinements.assert_called_with({"Instrument Parameters": ["instr_par_string"]})
        hist_2.set_refinements.assert_called_with({"Instrument Parameters": ["instr_par_string"]})
        project.do_refinements.called_once()
        project.save.called_with("save_path")
