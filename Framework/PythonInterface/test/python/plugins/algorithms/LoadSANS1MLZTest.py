import unittest
from testhelpers import run_algorithm
from mantid.api import AnalysisDataService
import os
from mantid.simpleapi import LoadSANS1MLZ


class LoadSANS1MLZTest(unittest.TestCase):

    def test_LoadValidData(self):
        output_ws_name = "LoadSANS1MLZTest_Test1"
        filename = "D0511339.001"
        alg_test = run_algorithm("LoadSANS1MLZ", Filename=filename,
                                 OutputWorkspace=output_ws_name,
                                 SectionOption='CommentSection')
        self.assertTrue(alg_test.isExecuted())

        # Verify some values
        ws = AnalysisDataService.retrieve(output_ws_name)
        self.assertEqual('SANS-1_MLZ', ws.getInstrument().getName())
        # dimensions
        self.assertEqual(16386, ws.getNumberHistograms())
        # self.assertEqual(2,  ws.getNumDims())
        # data array
        self.assertEqual(519, ws.readY(8502))
        self.assertEqual(427, ws.readY(8629))
        # sample logs
        run = ws.getRun()
        self.assertEqual(4, run.getProperty('det1_x_value').value)
        self.assertEqual(20000, run.getProperty('det1_z_value').value)
        self.assertEqual(6, run.getProperty('wavelength').value)
        # check whether detector bank is rotated
        # det = ws.getDetector(0)
        # self.assertAlmostEqual(8.54, ws.detectorSignedTwoTheta(det)*180/pi)
        run_algorithm("DeleteWorkspace", Workspace=output_ws_name)

    def test_LoadInvalidData(self):
        output_ws_name = "LoadSANS1MLZTest_Test2"
        filename = "sans-incomplete.001"
        parameters = {'counts 128': "'Counts' section include incorrect data: must be 128x128",
                      'counts pr': "loop of ufunc does not support argument 0 of type list which"
                                   " has no callable sqrt method\nprobably incorrect 'Counts' data",
                      'section name': "Section name doesn't match with expected: 'Coment' != 'Comment'",
                      'section amount': "Incorrect amount of sections: 6 != 7"}

        for param in parameters.keys():
            self._create_incomplete_dataFile(filename, param)
            self.assertRaisesRegex(RuntimeError,
                                   parameters[param],
                                   LoadSANS1MLZ, Filename=filename,
                                   OutputWorkspace=output_ws_name)
            os.remove(filename)

    def test_aaaa(self):
        self.assertTrue(1 == 1)

    def _create_incomplete_dataFile(self, filename, param):
        """
        creates an incomplete data file
        """
        prm = param.split(' ')
        if prm[0] == 'counts':
            with open(filename, "w") as f:
                f.write("\n\n\n\n")
                f.write("%File\n\n")
                f.write("%Sample\n\n")
                f.write("%Setup\n\n")
                f.write("%Counter\n\n")
                f.write("%History\n\n")
                f.write("%Comment\n\n")
                # f.write("selector_lambda_value=1\n\n")
                f.write("%Counts\n\n")
                if prm[1] == 'pr':
                    s = ('1, ' * 127 + '1\n') * 50
                    s = s + ('1, ' * 126 + '1\n')
                    s = s + ('1, ' * 127 + '1\n') * 77
                elif prm[1] == '128':
                    s = ('1, ' * 127 + '1\n') * 127
                f.write(s)
                f.close()
        elif prm[0] == 'section':
            with open(filename, "w") as f:
                f.write("\n\n\n\n")
                f.write("%File\n\n")
                f.write("%Sample\n\n")
                f.write("%Setup\n\n")
                f.write("%Counter\n\n")
                f.write("%History\n\n")
                if prm[1] == 'name':
                    f.write("%Coment\n\n")
                elif prm[1] == 'amount':
                    pass
                f.write("%Counts\n\n")
                s = ('1, ' * 127 + '1\n') * 128
                f.write(s)
                f.close()
        return


if __name__ == '__main__':
    unittest.main()
