from  plugins.algorithms.PeakMatching import PeakMatching
import unittest
from unittest import mock
from mantid.simpleapi import DeleteWorkspace,CreateEmptyTableWorkspace
from mantid.api import mtd

class PeakMatchingTest(unittest.TestCase):

    def setUp(self):
        self.peak_data = {'Primary energy': {'Ag': {'K(4->1)': 3177.7,
                                                    'L(4->2)': 900.7,
                                                    'M(4->3)': 304.7,
                                                    '6->5': 141.0}},

                          'Secondary energy': {'Ag': {'K(2->1)': 3140.6,
                                                      'L(8->2)': 1347.8,
                                                      'M(10->3)': 567.0,
                                                      '8->6': 122.2}},

                          'All energies': {'Ag': {'K(4->1)': 3177.7,
                                                  'L(4->2)': 900.7,
                                                  'M(4->3)': 304.7,
                                                  '6->5': 141.0,
                                                  'K(2->1)': 3140.6,
                                                  'L(8->2)': 1347.8,
                                                  'M(10->3)': 567.0,
                                                  '8->6': 122.2}}}

        self.input_peaks = [(900, 0.8), (306, 0.8), (567, 0.8), (3, 0.8)]
        self.input_table =CreateEmptyTableWorkspace(OutputWorkSpace = "input")
        self.input_table.addColumn("double","centre")
        self.input_table.addColumn("double", "sigma")
        for row in self.input_peaks:
            self.input_table.addRow(row)
        self.algo = PeakMatching()

    def tearDown(self):
        self.delete_if_present("input")
        self.algo = None
    @staticmethod
    def delete_if_present(workspace):
        if workspace in mtd:
            DeleteWorkspace(workspace)

    def test_get_input_peaks(self):
        table_data = self.algo.get_input_peaks(self.input_table)
        self.assertEqual(table_data,self.input_peaks)
        self.delete_if_present("Test")

    @mock.patch('plugins.algorithms.PeakMatching.PeakMatching.get_default_peak_data')
    def test_process_peak_data(self,mock_get_default_peak_data):
        mock_get_default_peak_data.return_value = dict({
            "Ag": {
                "Z": 47,
                "A": 107.87,
                "Primary": {
                    "K(4->1)": 3177.7,
                    "L(4->2)": 900.7,
                    "M(4->3)": 304.7,
                    "6->5": 141
                },
                "Secondary": {
                    "K(2->1)": 3140.6,
                    "L(8->2)": 1347.8,
                    "M(10->3)": 567,
                    "8->6": 122.2
                },
                "Gammas": {
                    "72Ge(n,n')72Ge": 691,
                    "73Ge(n,g)74Ge": None,
                    "74Ge(n,n')74Ge": 595.7
                }
            }
        })
        data = self.algo.process_peak_data('')

        self.assertEqual(self.peak_data, data)

    def test_get_matches(self):
        all_data = self.algo.get_matches(self.peak_data, self.input_peaks)

        primary_matches = [{'energy': 900.7, 'peak_centre': 900, 'error': 0.8,
                            'element': 'Ag', 'diff': 0.7, 'transition': 'L(4->2)'},

                           {'energy': 304.7, 'peak_centre': 306, 'error': 1.6,
                            'element': 'Ag', 'diff': 1.3, 'transition': 'M(4->3)'}]

        all_matches = [{'energy': 567, 'peak_centre': 567, 'error': 0,
                        'element': 'Ag', 'diff': 0, 'transition': 'M(10->3)'},

                       {'energy': 900.7, 'peak_centre': 900, 'error': 0.8,
                        'element': 'Ag', 'diff': 0.7, 'transition': 'L(4->2)'},

                       {'energy': 304.7, 'peak_centre': 306, 'error': 1.6,
                        'element': 'Ag', 'diff': 1.3, 'transition': 'M(4->3)'}]

        secondary_matches = [{'energy': 567, 'peak_centre': 567, 'error': 0,
                              'element': 'Ag', 'diff': 0, 'transition': 'M(10->3)'}]

        all_matches = [primary_matches, secondary_matches, all_matches]
        for j in range(len(all_data)):
            data = all_data[j]
            matches = all_matches[j]
            if len(data) != len(matches):
                raise AssertionError

            for i in range(len(data)):

                for column in data[i]:

                    if type(data[i][column]) == int or type(data[i][column]) == float:

                        self.assertAlmostEqual(matches[i][column],
                                                   data[i][column])
                    else:

                        self.assertEqual(matches[i][column], data[i][column])

    @mock.patch('plugins.algorithms.PeakMatching.PeakMatching.make_peak_table')
    @mock.patch('plugins.algorithms.PeakMatching.PeakMatching.make_count_table')
    @mock.patch('plugins.algorithms.PeakMatching.PeakMatching.setProperty')
    @mock.patch('plugins.algorithms.PeakMatching.PeakMatching.setPropertyValue')
    def test_output_data(self,mock_set_value,mock_set_property, mock_make_count_table
                        ,mock_make_peak_table):
        self.algo.output_data(1, 2, 3,['prim','secon','all','sort','count']*5)
        call = [mock.call('prim', 1), mock.call('secon', 2),
                mock.call('all', 3),
                mock.call('sort', 3,True,"energy"),
                mock.call('count',3)]
        mock_make_peak_table.assert_has_calls(call[:-1])
        mock_make_count_table.assert_has_calls(call[-1:])
        self.assertEqual(mock_make_peak_table.call_count, 4)
        self.assertEqual(mock_make_count_table.call_count, 1)
        self.assertEqual(mock_set_value.call_count,5)
        self.assertEqual(mock_set_property.call_count, 5)

    def test_algorithm_with_valid_inputs(self):
        #PeakMatching()
        pass

    def test_algorithm_with_invalid_arguments(self):
        pass

if __name__ == '__main__':
    unittest.main()