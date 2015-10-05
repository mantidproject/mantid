import unittest
from testhelpers import run_algorithm
from mantid.api import AnalysisDataService, AnalysisDataServiceImpl, MatrixWorkspace, Workspace
from mantid import mtd

class AnalysisDataServiceTest(unittest.TestCase):

    def test_len_returns_correct_value(self):
        self.assertEquals(len(AnalysisDataService), 0)

    def test_mtd_is_same_object_type_as_analysis_data_service(self):
        self.assertTrue(isinstance(AnalysisDataService, AnalysisDataServiceImpl))
        self.assertTrue(isinstance(mtd, AnalysisDataServiceImpl))

    def test_retrieval_of_non_existent_data_raises_KeyError(self):
        try:
            AnalysisDataService['NotHere']
            self.fail('AnalysisDataService did not throw when object does not exist')
        except KeyError:
            pass

    def _run_createws(self, wsname):
        """
            Run create workspace storing the output in the named workspace
        """
        data = [1.0,2.0,3.0]
        alg = run_algorithm('CreateWorkspace',DataX=data,DataY=data,NSpec=1,UnitX='Wavelength', child=True)
        AnalysisDataService.addOrReplace(wsname, alg.getProperty("OutputWorkspace").value)

    def test_len_increases_when_item_added(self):
        wsname = 'ADSTest_test_len_increases_when_item_added'
        current_len = len(AnalysisDataService)
        self._run_createws(wsname)
        self.assertEquals(len(AnalysisDataService), current_len + 1)
        # Remove to clean the test up
        AnalysisDataService.remove(wsname)

    def test_len_decreases_when_item_removed(self):
        wsname = 'ADSTest_test_len_decreases_when_item_removed'
        self._run_createws(wsname)
        current_len = len(AnalysisDataService)
        # Remove to clean the test up
        del AnalysisDataService[wsname]
        self.assertEquals(len(AnalysisDataService), current_len - 1)

    def test_add_raises_error_if_name_exists(self):
        data = [1.0,2.0,3.0]
        alg = run_algorithm('CreateWorkspace',DataX=data,DataY=data,NSpec=1,UnitX='Wavelength', child=True)
        name = "testws"
        ws = alg.getProperty("OutputWorkspace").value
        AnalysisDataService.addOrReplace(name, ws)
        self.assertRaises(RuntimeError, AnalysisDataService.add, name, ws)
        AnalysisDataService.remove(name)

    def test_addOrReplace_replaces_workspace_with_existing_name(self):
        data = [1.0,2.0,3.0]
        alg = run_algorithm('CreateWorkspace',DataX=data,DataY=data,NSpec=1,UnitX='Wavelength', child=True)
        name = "testws"
        ws = alg.getProperty("OutputWorkspace").value
        AnalysisDataService.add(name, ws)
        len_before = len(AnalysisDataService)
        AnalysisDataService.addOrReplace(name, ws)
        len_after = len(AnalysisDataService)
        self.assertEquals(len_after, len_before)
        AnalysisDataService.remove(name)

    def do_check_for_matrix_workspace_type(self, workspace):
        self.assertTrue(isinstance(workspace, MatrixWorkspace))
        self.assertNotEquals(workspace.getName(), '')
        self.assertTrue(hasattr(workspace, 'getNumberHistograms'))
        self.assertTrue(hasattr(workspace, 'getMemorySize'))


    def test_retrieve_gives_back_derived_type_not_DataItem(self):
        wsname = 'ADSTest_test_retrieve_gives_back_derived_type_not_DataItem'
        self._run_createws(wsname)
        self.do_check_for_matrix_workspace_type(AnalysisDataService.retrieve(wsname))
        AnalysisDataService.remove(wsname)

    def test_key_operator_does_same_as_retrieve(self):
        wsname = 'ADSTest_test_key_operator_does_same_as_retrieve'
        self._run_createws(wsname)
        ws_from_op = AnalysisDataService[wsname]
        ws_from_method = AnalysisDataService.retrieve(wsname)

        self.do_check_for_matrix_workspace_type(ws_from_op)
        self.do_check_for_matrix_workspace_type(ws_from_method)

        self.assertEquals(ws_from_op.name(), ws_from_method.name())
        self.assertEquals(ws_from_op.getMemorySize(), ws_from_method.getMemorySize())

        # Remove to clean the test up
        AnalysisDataService.remove(wsname)

    def test_removing_item_invalidates_extracted_handles(self):
        # If a reference to a DataItem has been extracted from the ADS
        # and it is then removed. The extracted handle should no longer
        # be able to access the DataItem
        wsname = 'ADSTest_test_removing_item_invalidates_extracted_handles'
        self._run_createws(wsname)
        ws_handle = AnalysisDataService[wsname]
        succeeded = False
        try:
            ws_handle.id() # Should be okay
            succeeded = True
        except RuntimeError:
            pass
        self.assertTrue(succeeded, "DataItem handle should be valid and allow function calls")
        AnalysisDataService.remove(wsname)
        self.assertRaises(RuntimeError, ws_handle.id)

    def test_importAll_exists_as_member(self):
        self.assertTrue(hasattr(AnalysisDataService, "importAll"))

    def test_importAll_creates_variable_in_current_global_dict_pointing_to_each_workspace(self):
        obj_names = mtd.getObjectNames()
        extra_names = ["ADSTest_test_1", "ADSTest_test_2", "ADSTest_test_3"]
        for name in extra_names:
            self._run_createws(name)
        obj_names += extra_names

        # Check no names are in globals
        for name in obj_names:
            self.assertFalse(name in locals())

        # Pull in variables
        mtd.importAll()
        # Are they in the local namespace
        for name in obj_names:
            self.assertTrue(name in locals())

        # Clean up
        for name in obj_names:
            try:
                del locals()[name]
            except KeyError:
                pass
        for name in extra_names:
            mtd.remove(name)

if __name__ == '__main__':
    unittest.main()
