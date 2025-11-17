# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantid.simpleapi import AnalysisDataService as ADS, AlgorithmManager, CreateGroupingByComponent
import numpy as np


class CreateGroupingByComponentTest(unittest.TestCase):
    def setUp(self):
        self.default_kwargs = {"InstrumentName": "ENGINX", "OutputWorkspace": "test_group", "StoreInADS": False}

    def tearDown(self):
        ADS.clear()

    def assertCorrectGroupingLabels(self, target_inds, target_bounds, y_dat):
        y_test = create_grouping_from_indices_and_bounds(target_inds, target_bounds, y_dat.size)
        self.assertTrue(np.all(y_dat == y_test))

    def assertCorrectNumberOfGroups(self, y_dat, n_groups):
        self.assertEqual(len(np.unique(y_dat)), n_groups)

    def test_alg_with_default_parameters(self):
        out_ws = CreateGroupingByComponent(**self.default_kwargs)

        # by default, include is "" which all components satisfy, so there should be 1 unique group
        y_dat = out_ws.extractY().reshape(-1)
        self.assertEqual(len(np.unique(y_dat)), 1)
        self.assertTrue(np.all(y_dat == np.ones(len(y_dat))))

    def test_alg_with_no_search_term_splits_into_n_groups(self):
        out_ws = CreateGroupingByComponent(**self.default_kwargs, GroupSubdivision=2)

        # by default, include is "" which all components satisfy, so sub-dividing this into 2 gives 2 unique groups
        y_dat = out_ws.extractY().reshape(-1)
        target_inds = [1, 2, 1, 2]
        target_bounds = [(0, 1200), (1200, 2160), (2160, 2210), (2210, 2500)]

        self.assertCorrectNumberOfGroups(y_dat, 2)
        self.assertCorrectGroupingLabels(target_inds, target_bounds, y_dat)

    def test_alg_with_include_term_splits_into_expected_groups(self):
        out_ws = CreateGroupingByComponent(**self.default_kwargs, ComponentNameIncludes="detector-block")

        # ENGINX has structure banks -> modules -> blocks -> pixels
        # with 2 banks, each containing 5 modules (each module then made up of 9 blocks)

        # here we have requested that we group the detector-blocks together.
        # As this is done under each parent component we should have 10 groups + 1 for the null group

        y_dat = out_ws.extractY().reshape(-1)
        target_inds = [10, 9, 8, 7, 6, 1, 2, 3, 4, 5, 0]
        target_bounds = [
            (0, 240),
            (240, 480),
            (480, 720),
            (720, 960),
            (960, 1200),
            (1200, 1440),
            (1440, 1680),
            (1680, 1920),
            (1920, 2160),
            (2160, 2400),
            (2400, 2500),
        ]

        self.assertCorrectNumberOfGroups(y_dat, 11)
        self.assertCorrectGroupingLabels(target_inds, target_bounds, y_dat)

    def test_alg_with_exclude_term_splits_into_expected_groups(self):
        out_ws = CreateGroupingByComponent(**self.default_kwargs, ComponentNameIncludes="pixel")

        # ENGINX has diffraction structure: banks -> modules -> blocks -> pixels
        # with 2 banks, each containing 5 modules (each module then made up of 9 blocks)

        # AND forward transmission structure: bank -> columns -> pixels
        # with 1 banks, containing 10 columns (each module then made up of 10 pixels)

        # here we have requested that we group the pixel together.
        # As this is done under each parent component we should have 2*5*9 groups for the diffraction
        # and 10 for the transmission, so 100 total

        y_dat = out_ws.extractY().reshape(-1)
        target_inds = [
            100,
            99,
            98,
            97,
            96,
            95,
            94,
            93,
            92,
            91,
            90,
            89,
            88,
            87,
            86,
            85,
            84,
            83,
            82,
            81,
            80,
            79,
            78,
            77,
            76,
            75,
            74,
            73,
            72,
            71,
            70,
            69,
            68,
            67,
            66,
            65,
            64,
            63,
            62,
            61,
            60,
            59,
            58,
            57,
            56,
            19,
            18,
            17,
            16,
            15,
            14,
            13,
            12,
            11,
            28,
            27,
            26,
            25,
            24,
            23,
            22,
            21,
            20,
            37,
            36,
            35,
            34,
            33,
            32,
            31,
            30,
            29,
            46,
            45,
            44,
            43,
            42,
            41,
            40,
            39,
            38,
            55,
            54,
            53,
            52,
            51,
            50,
            49,
            48,
            47,
            10,
            9,
            8,
            7,
            6,
            5,
            4,
            3,
            2,
            1,
        ]
        target_bounds = [
            (0, 26),
            (26, 53),
            (53, 80),
            (80, 107),
            (107, 134),
            (134, 161),
            (161, 188),
            (188, 215),
            (215, 240),
            (240, 266),
            (266, 293),
            (293, 320),
            (320, 347),
            (347, 374),
            (374, 401),
            (401, 428),
            (428, 455),
            (455, 480),
            (480, 506),
            (506, 533),
            (533, 560),
            (560, 587),
            (587, 614),
            (614, 641),
            (641, 668),
            (668, 695),
            (695, 720),
            (720, 746),
            (746, 773),
            (773, 800),
            (800, 827),
            (827, 854),
            (854, 881),
            (881, 908),
            (908, 935),
            (935, 960),
            (960, 986),
            (986, 1013),
            (1013, 1040),
            (1040, 1067),
            (1067, 1094),
            (1094, 1121),
            (1121, 1148),
            (1148, 1175),
            (1175, 1200),
            (1200, 1226),
            (1226, 1253),
            (1253, 1280),
            (1280, 1307),
            (1307, 1334),
            (1334, 1361),
            (1361, 1388),
            (1388, 1415),
            (1415, 1440),
            (1440, 1466),
            (1466, 1493),
            (1493, 1520),
            (1520, 1547),
            (1547, 1574),
            (1574, 1601),
            (1601, 1628),
            (1628, 1655),
            (1655, 1680),
            (1680, 1706),
            (1706, 1733),
            (1733, 1760),
            (1760, 1787),
            (1787, 1814),
            (1814, 1841),
            (1841, 1868),
            (1868, 1895),
            (1895, 1920),
            (1920, 1946),
            (1946, 1973),
            (1973, 2000),
            (2000, 2027),
            (2027, 2054),
            (2054, 2081),
            (2081, 2108),
            (2108, 2135),
            (2135, 2160),
            (2160, 2186),
            (2186, 2213),
            (2213, 2240),
            (2240, 2267),
            (2267, 2294),
            (2294, 2321),
            (2321, 2348),
            (2348, 2375),
            (2375, 2400),
            (2400, 2410),
            (2410, 2420),
            (2420, 2430),
            (2430, 2440),
            (2440, 2450),
            (2450, 2460),
            (2460, 2470),
            (2470, 2480),
            (2480, 2490),
            (2490, 2500),
        ]

        self.assertCorrectNumberOfGroups(y_dat, 100)
        self.assertCorrectGroupingLabels(target_inds, target_bounds, y_dat)

        # Now we run again and exclude any component which has 'transmission' in the name

        new_ws = CreateGroupingByComponent(**self.default_kwargs, ComponentNameIncludes="pixel", ComponentNameExcludes="transmission")

        # Now only expect the 90 groups for the diffraction banks (+1 for the null group)

        y_dat = new_ws.extractY().reshape(-1)
        target_inds = [
            90,
            89,
            88,
            87,
            86,
            85,
            84,
            83,
            82,
            81,
            80,
            79,
            78,
            77,
            76,
            75,
            74,
            73,
            72,
            71,
            70,
            69,
            68,
            67,
            66,
            65,
            64,
            63,
            62,
            61,
            60,
            59,
            58,
            57,
            56,
            55,
            54,
            53,
            52,
            51,
            50,
            49,
            48,
            47,
            46,
            9,
            8,
            7,
            6,
            5,
            4,
            3,
            2,
            1,
            18,
            17,
            16,
            15,
            14,
            13,
            12,
            11,
            10,
            27,
            26,
            25,
            24,
            23,
            22,
            21,
            20,
            19,
            36,
            35,
            34,
            33,
            32,
            31,
            30,
            29,
            28,
            45,
            44,
            43,
            42,
            41,
            40,
            39,
            38,
            37,
            0,
        ]
        target_bounds = [
            (0, 26),
            (26, 53),
            (53, 80),
            (80, 107),
            (107, 134),
            (134, 161),
            (161, 188),
            (188, 215),
            (215, 240),
            (240, 266),
            (266, 293),
            (293, 320),
            (320, 347),
            (347, 374),
            (374, 401),
            (401, 428),
            (428, 455),
            (455, 480),
            (480, 506),
            (506, 533),
            (533, 560),
            (560, 587),
            (587, 614),
            (614, 641),
            (641, 668),
            (668, 695),
            (695, 720),
            (720, 746),
            (746, 773),
            (773, 800),
            (800, 827),
            (827, 854),
            (854, 881),
            (881, 908),
            (908, 935),
            (935, 960),
            (960, 986),
            (986, 1013),
            (1013, 1040),
            (1040, 1067),
            (1067, 1094),
            (1094, 1121),
            (1121, 1148),
            (1148, 1175),
            (1175, 1200),
            (1200, 1226),
            (1226, 1253),
            (1253, 1280),
            (1280, 1307),
            (1307, 1334),
            (1334, 1361),
            (1361, 1388),
            (1388, 1415),
            (1415, 1440),
            (1440, 1466),
            (1466, 1493),
            (1493, 1520),
            (1520, 1547),
            (1547, 1574),
            (1574, 1601),
            (1601, 1628),
            (1628, 1655),
            (1655, 1680),
            (1680, 1706),
            (1706, 1733),
            (1733, 1760),
            (1760, 1787),
            (1787, 1814),
            (1814, 1841),
            (1841, 1868),
            (1868, 1895),
            (1895, 1920),
            (1920, 1946),
            (1946, 1973),
            (1973, 2000),
            (2000, 2027),
            (2027, 2054),
            (2054, 2081),
            (2081, 2108),
            (2108, 2135),
            (2135, 2160),
            (2160, 2186),
            (2186, 2213),
            (2213, 2240),
            (2240, 2267),
            (2267, 2294),
            (2294, 2321),
            (2321, 2348),
            (2348, 2375),
            (2375, 2400),
            (2400, 2500),
        ]

        self.assertCorrectNumberOfGroups(y_dat, 91)
        self.assertCorrectGroupingLabels(target_inds, target_bounds, y_dat)

    def test_alg_with_single_exclude_branches_splits_into_expected_groups(self):
        out_ws = CreateGroupingByComponent(**self.default_kwargs, ComponentNameIncludes="block", ExcludeBranches="SouthBank")

        # ENGINX has diffraction structure: banks -> modules -> blocks -> pixels
        # with 2 banks (NorthBank and SouthBank), each containing 5 modules (each module then made up of 9 blocks)

        # here we have requested that we group the blocks together but exclude anything under SouthBank
        # so we expect the 5 groups +1 for null group

        y_dat = out_ws.extractY().reshape(-1)
        target_inds = [5, 4, 3, 2, 1, 0]
        target_bounds = [(0, 240), (240, 480), (480, 720), (720, 960), (960, 1200), (1200, 2500)]

        self.assertCorrectNumberOfGroups(y_dat, 6)
        self.assertCorrectGroupingLabels(target_inds, target_bounds, y_dat)

    def test_alg_with_multiple_exclude_branches_splits_into_expected_groups(self):
        out_ws = CreateGroupingByComponent(
            **self.default_kwargs, ComponentNameIncludes="pixel", ExcludeBranches="SouthBank, TransmissionBank"
        )

        # ENGINX has diffraction structure: banks -> modules -> blocks -> pixels
        # with 2 banks (NorthBank and SouthBank), each containing 5 modules (each module then made up of 9 blocks)

        # AND forward transmission structure: bank -> columns -> pixels
        # with 1 banks (TransmissionBank), containing 10 columns (each module then made up of 10 pixels)

        # here we have requested that we group the pixels together but exclude anything under NorthBank or TransmissionBank
        # so we expect the 45 groups (North Bank 5*9) +1 for null group

        y_dat = out_ws.extractY().reshape(-1)
        target_inds = [
            45,
            44,
            43,
            42,
            41,
            40,
            39,
            38,
            37,
            36,
            35,
            34,
            33,
            32,
            31,
            30,
            29,
            28,
            27,
            26,
            25,
            24,
            23,
            22,
            21,
            20,
            19,
            18,
            17,
            16,
            15,
            14,
            13,
            12,
            11,
            10,
            9,
            8,
            7,
            6,
            5,
            4,
            3,
            2,
            1,
            0,
        ]
        target_bounds = [
            (0, 26),
            (26, 53),
            (53, 80),
            (80, 107),
            (107, 134),
            (134, 161),
            (161, 188),
            (188, 215),
            (215, 240),
            (240, 266),
            (266, 293),
            (293, 320),
            (320, 347),
            (347, 374),
            (374, 401),
            (401, 428),
            (428, 455),
            (455, 480),
            (480, 506),
            (506, 533),
            (533, 560),
            (560, 587),
            (587, 614),
            (614, 641),
            (641, 668),
            (668, 695),
            (695, 720),
            (720, 746),
            (746, 773),
            (773, 800),
            (800, 827),
            (827, 854),
            (854, 881),
            (881, 908),
            (908, 935),
            (935, 960),
            (960, 986),
            (986, 1013),
            (1013, 1040),
            (1040, 1067),
            (1067, 1094),
            (1094, 1121),
            (1121, 1148),
            (1148, 1175),
            (1175, 1200),
            (1200, 2500),
        ]

        self.assertCorrectNumberOfGroups(y_dat, 46)
        self.assertCorrectGroupingLabels(target_inds, target_bounds, y_dat)

    def test_alg_with_bad_instrument_name_fails(self):
        alg = _init_alg(InstrumentName="FakeInstrument", OutputWorkspace="test")
        with self.assertRaisesRegex(RuntimeError, "Failed to find a matching instrument to the provided input:"):
            alg.execute()


def _init_alg(**kwargs):
    alg = AlgorithmManager.create("CreateGroupingByComponent")
    alg.initialize()
    for prop, value in kwargs.items():
        alg.setProperty(prop, value)
    return alg


def create_grouping_from_indices_and_bounds(group_indices, bounds, size):
    """
    helper for constructing grouping from the boundaries of groups and the group indices
    """
    arr = np.zeros((size,))
    for i, bound in enumerate(bounds):
        arr[bound[0] : bound[1]] = group_indices[i]
    return arr


if __name__ == "__main__":
    unittest.main()
