import unittest
from testhelpers import run_algorithm
from mantid.kernel import std_vector_str
from mantid.api import WorkspaceFactory
import numpy

class ITableWorkspaceTest(unittest.TestCase):

    _test_ws = None

    def setUp(self):
        if self._test_ws is None:
            alg = run_algorithm('RawFileInfo', Filename='LOQ48127.raw',GetRunParameters=True, child=True)
            self.__class__._test_ws = alg.getProperty('RunParameterTable').value

    def test_meta_information_is_correct(self):
        self.assertEquals(self._test_ws.columnCount(), 19)
        self.assertEquals(self._test_ws.rowCount(), 1)

        column_names = self._test_ws.getColumnNames()
        self.assertEquals(len(column_names), 19)
        self.assertEquals(type(column_names), std_vector_str)

    def test_cell_access_returns_variables_as_native_python_types(self):
        self.assertAlmostEquals(self._test_ws.cell('r_gd_prtn_chrg',0), 10.040912628173828, 15)
        self.assertAlmostEquals(self._test_ws.cell(0, 7), 10.040912628173828, 15)

        self.assertEquals(self._test_ws.cell('r_goodfrm', 0), 9229)
        self.assertEquals(self._test_ws.cell(0, 9), 9229)

        self.assertEquals(self._test_ws.cell('r_enddate', 0), "18-DEC-2008")
        self.assertEquals(self._test_ws.cell(0, 16), "18-DEC-2008")

    def _create_test_table(self):
        table = WorkspaceFactory.createTable()
        table.addColumn(type='int', name='index')
        table.addColumn(type='str', name='name')
        table.addRow([0,'1'])
        table.addRow([0,'2'])
        table.addRow([0,'3'])
        return table

    def test_iteration_over_table_gives_all_rows(self):
        test_table = self._create_test_table()
        expected_nrows = len(test_table)
        found_rows = 0
        for i in test_table:
            found_rows += 1
        self.assertEquals(found_rows, expected_nrows)

    def test_table_is_resized_correctly(self):
        table = WorkspaceFactory.createTable()
        self.assertEquals(len(table), 0)
        table.setRowCount(5)
        self.assertEquals(len(table), 5)
        self.assertTrue(table.addColumn(type="int",name="index"))
        self.assertEquals(table.columnCount(), 1)

    def test_setcell_sets_the_correct_cell(self):
        test_table = self._create_test_table()
        data = '11'
        col = 1
        row = 2
        test_table.setCell(row, col, data)
        self.assertEquals(test_table.cell(row,col), data)
        data = '12'
        col = 'name'
        test_table.setCell(col, row, data)
        self.assertEquals(test_table.cell(col,row), data)

    def test_adding_table_data_using_dictionary(self):
        table = WorkspaceFactory.createTable()
        table.addColumn(type="int",name="index")
        self.assertEquals(table.columnCount(), 1)
        table.addColumn(type="str",name="value")
        self.assertEquals(table.columnCount(), 2)

        nextrow = {'index':1, 'value':'10'}
        table.addRow(nextrow)
        self.assertEquals(len(table), 1)
        insertedrow = table.row(0)
        self.assertEquals(insertedrow, nextrow)

        incorrect_type = {'index':1, 'value':10}
        self.assertRaises(ValueError, table.addRow, incorrect_type)


    def test_adding_table_data_using_list(self):
        table = WorkspaceFactory.createTable()
        table.addColumn(type="int",name="index")
        self.assertEquals(table.columnCount(), 1)
        table.addColumn(type="str",name="value")
        self.assertEquals(table.columnCount(), 2)

        nextrow = {'index':1, 'value':'10'}
        values = nextrow.values()
        table.addRow(nextrow)
        self.assertEquals(len(table), 1)
        insertedrow = table.row(0)
        self.assertEquals(insertedrow, nextrow)
        insertedrow = table.row(0)
        self.assertEquals(insertedrow, nextrow)

    def test_set_and_extract_boolean_columns(self):
        table = WorkspaceFactory.createTable()
        table.addColumn(type='bool', name='yes_no')
        table.addRow([True])
        table.addRow([False])

        self.assertTrue(table.cell(0, 0))
        self.assertFalse(table.cell(1, 0))

    def test_set_and_extract_v3d_columns(self):
        from mantid.kernel import V3D

        table = WorkspaceFactory.createTable()
        table.addColumn(type='V3D', name='pos')
        table.addRow([V3D(1,1,1)])

        self.assertEquals(V3D(1,1,1), table.cell(0, 0))

    def test_set_and_extract_vector_columns(self):
        table = WorkspaceFactory.createTable()
        table.addColumn(type='vector_int', name='values')

        # Settings from general Python list
        table.addRow([ [1,2,3,4,5] ])
        # Setting from numpy array
        table.addRow([ numpy.array([6,7,8,9,10]) ])

        self.assertTrue( numpy.array_equal( table.cell(0,0), numpy.array([1,2,3,4,5]) ) )
        self.assertTrue( numpy.array_equal( table.cell(1,0), numpy.array([6,7,8,9,10]) ) )

if __name__ == '__main__':
    unittest.main()
