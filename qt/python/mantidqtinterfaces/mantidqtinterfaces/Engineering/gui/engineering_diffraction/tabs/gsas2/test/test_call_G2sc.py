# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from unittest import mock

from mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.gsas2.call_G2sc import add_histograms


class GSAS2ViewTest(unittest.TestCase):
    def setUp(self) -> None:
        pass

    def tearDown(self) -> None:
        pass

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
