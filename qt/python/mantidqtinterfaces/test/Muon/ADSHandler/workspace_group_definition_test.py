# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from mantidqtinterfaces.Muon.GUI.Common.ADSHandler.workspace_group_definition import check_not_in_group,\
        safe_to_add_to_group, add_list_to_group, add_to_group
from mantidqtinterfaces.Muon.GUI.Common.ADSHandler.ADS_calls import make_group, retrieve_ws
from mantid.api import AnalysisDataService as ADS
from mantid import simpleapi


def create_workspace(name):
    alg = simpleapi.AlgorithmManager.create("CreateWorkspace")
    alg.initialize()
    alg.setAlwaysStoreInADS(True)
    alg.setLogging(False)
    alg.setProperty("dataX", [0,1])
    alg.setProperty("dataY", [0,1])
    alg.setProperty("OutputWorkspace", name)
    alg.execute()
    return retrieve_ws(name)


def create_table_workspace(name):
    alg = simpleapi.AlgorithmManager.create("CreateEmptyTableWorkspace")
    alg.initialize()
    alg.setAlwaysStoreInADS(True)
    alg.setLogging(False)
    alg.setProperty("OutputWorkspace", name)
    alg.execute()
    return name


class WorkspaceGroupDefinitionTest(unittest.TestCase):

    def setUp(self):
        self.assertEqual(ADS.getObjectNames(),[])

    def tearDown(self):
        ADS.clear()

    def test_check_not_in_group(self):
        ws_in_group = create_workspace("in")
        ws_out_group = create_workspace("out")
        make_group([ws_in_group], "group")
        # get the group
        group = retrieve_ws("group")

        self.assertEqual(len(group.getNames()), 1)
        self.assertEqual(group.getNames()[0], "in")
        self.assertEqual(check_not_in_group([group], ws_in_group.name()), False)
        self.assertEqual(check_not_in_group([group], ws_out_group.name()), True)

    def test_safe_to_add_to_group(self):
        instrument = "MUSR"
        extension = "MA"

        ws = create_workspace(instrument+"test"+extension)
        tmp = create_workspace("dummy")
        make_group([tmp], "group")
        # get the group
        group = retrieve_ws("group")

        self.assertEqual(safe_to_add_to_group(ws, instrument, [group], extension), True)

    def test_safe_to_add_to_group_wrong_instrument(self):
        instrument = "MUSR"
        extension = "MA"

        ws = create_workspace("EMU"+"test"+extension)
        tmp = create_workspace("dummy")
        make_group([tmp], "group")
        # get the group
        group = retrieve_ws("group")

        self.assertEqual(safe_to_add_to_group(ws, instrument, [group], extension), False)

    def test_safe_to_add_to_group_already_in_a_group(self):
        instrument = "MUSR"
        extension = "MA"

        ws = create_workspace(instrument+"test"+extension)
        tmp = create_workspace("dummy")
        make_group([tmp,ws], "group")
        # get the group
        group = retrieve_ws("group")

        self.assertEqual(safe_to_add_to_group(ws, instrument, [group], extension), False)

    def test_safe_to_add_to_group_wrong_extension(self):
        instrument = "MUSR"
        extension = "MA"

        ws = create_workspace(instrument+"test"+"FD")
        tmp = create_workspace("dummy")
        make_group([tmp], "group")
        # get the group
        group = retrieve_ws("group")

        self.assertEqual(safe_to_add_to_group(ws, instrument, [group], extension), False)

    def test_add_list_to_group(self):
        ws = create_workspace("unit")
        ws2 = create_workspace("test")
        tmp = create_workspace("dummy")
        make_group([tmp], "group")
        # get the group
        group = retrieve_ws("group")

        self.assertEqual(len(group.getNames()), 1)
        self.assertEqual(group.getNames()[0], "dummy")

        add_list_to_group([ws.name(), ws2.name()], group)
        expected = ["dummy", "unit", "test"]

        self.assertEqual(len(group.getNames()), len(expected))
        for name in group.getNames():
            self.assertTrue(name in expected)
            expected.remove(name)

    def test_add_to_group_single(self):
        instrument = "MUSR"
        extension = "MA"
        run = "62260"
        ws = create_workspace(instrument+run+"fwd"+extension)
        ws2 = create_workspace(instrument+run+"bwd"+extension)
        _ = create_workspace("EMU"+run+"fwd"+extension)
        _ = create_workspace(instrument+run+"fwd"+"FD")
        # there was a bug that meant tables didnt work
        table_name = create_table_workspace(instrument+run+"table"+extension)

        add_to_group(instrument, extension)

        group = retrieve_ws(instrument+run)
        expected = [ws.name(), ws2.name(), table_name]

        self.assertEqual(len(group.getNames()), len(expected))
        for name in group.getNames():
            self.assertTrue(name in expected)
            expected.remove(name)

    def test_add_to_group_multiple(self):
        instrument = "MUSR"
        extension = "MA"
        run = "62260"
        run2 = "06226"
        ws = create_workspace(instrument+run+"fwd"+extension)
        ws2 = create_workspace(instrument+run+"bwd"+extension)
        ws3 = create_workspace(instrument+run2+"fwd"+extension)
        ws4 = create_workspace(instrument+run2+"bwd"+extension)
        _ = create_workspace("EMU"+run+"fwd"+extension)
        _ = create_workspace(instrument+run+"fwd"+"FD")
        # there was a bug that meant tables didnt work
        table_name = create_table_workspace(instrument+run+"table"+extension)

        add_to_group(instrument, extension)

        # check run
        group = retrieve_ws(instrument+run)
        expected = [ws.name(), ws2.name(), table_name]

        self.assertEqual(len(group.getNames()), len(expected))
        for name in group.getNames():
            self.assertTrue(name in expected)
            expected.remove(name)

        # check run2
        group = retrieve_ws(instrument+run2)
        expected = [ws3.name(), ws4.name()]

        self.assertEqual(len(group.getNames()), len(expected))
        for name in group.getNames():
            self.assertTrue(name in expected)
            expected.remove(name)

    def test_add_to_group_ignore_if_already_in_group(self):
        instrument = "MUSR"
        extension = "MA"
        run = "62260"
        ws = create_workspace(instrument+run+"fwd"+extension)
        ws2 = create_workspace(instrument+run+"bwd"+extension)
        _ = create_workspace("EMU"+run+"fwd"+extension)
        _ = create_workspace(instrument+run+"fwd"+"FD")
        # there was a bug that meant tables didnt work
        table_name = create_table_workspace(instrument+run+"table"+extension)

        make_group([ws2], "group")
        add_to_group(instrument, extension)

        group = retrieve_ws(instrument+run)
        expected = [ws.name(), table_name]

        self.assertEqual(len(group.getNames()), len(expected))
        for name in group.getNames():
            self.assertTrue(name in expected)
            expected.remove(name)


if __name__ == '__main__':
    unittest.main()
