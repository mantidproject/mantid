import unittest
import os
import numpy as np
from mantid.api import AnalysisDataService
from testhelpers import run_algorithm
from mantid.simpleapi import LoadSANS1MLZ
from mantid import config
from plugins.algorithms.sansdata import SANSdata


class LoadSANSMLZTest(unittest.TestCase):

    def test_LoadValidData(self):
        """
        test: whether the workspace has been created, is the instrument correct
        """
        output_ws_name = "LoadSANS1MLZTest_Test1"
        filename = "D0511339.001"
        alg_test = run_algorithm("LoadSANS1MLZ",
                                 Filename=filename,
                                 OutputWorkspace=output_ws_name,
                                 SectionOption='EssentialData')

        ws = AnalysisDataService.retrieve(output_ws_name)
        self.assertEqual('SANS-1_MLZ', ws.getInstrument().getName())

        self.assertTrue(alg_test.isExecuted())

    def test_VerifyValues(self):
        """
        test: whether the values are correct
        """
        output_ws_name = "LoadSANS1MLZTest_Test1"
        filename = "D0511339.001"
        run_algorithm("LoadSANS1MLZ", Filename=filename, OutputWorkspace=output_ws_name, SectionOption='EssentialData')
        ws = AnalysisDataService.retrieve(output_ws_name)
        # dimensions
        self.assertEqual(16386, ws.getNumberHistograms())
        self.assertEqual(2, ws.getNumDims())
        # data array
        self.assertEqual(519, ws.readY(8502))
        self.assertEqual(427, ws.readY(8629))
        # sample logs
        run = ws.getRun()

        self.assertEqual(6, run.getProperty('wavelength').value)

        self.assertEqual('Eichproben', ws.getTitle())
        self.assertEqual(output_ws_name, ws.name())

        det = ws.getDetector(0)
        self.assertAlmostEqual(2.07, -ws.detectorSignedTwoTheta(det) * 180 / np.pi, 2)
        run_algorithm("DeleteWorkspace", Workspace=output_ws_name)

    def test_LoadInvalidData(self):
        """
        test: trying to process incorrect data file;
        check is exceptions definition is correct
        """
        output_ws_name = "LoadSANS1MLZTest_Test2"
        filename = "sans-incomplete.001"
        parameters = {
            'counts 128': "'Counts' section include incorrect data: must be 128x128",
            'counts pr': "loop of ufunc does not support argument 0 of type list which"
            " has no callable sqrt method\nprobably incorrect 'Counts' data",
            'section name': "Section name doesn't match with expected: 'Coment' != 'Comment'",
            'section amount': "Incorrect amount of sections: 6 != 7"
        }

        for param in parameters.keys():
            self._create_incomplete_dataFile(filename, param)
            self.assertRaisesRegex(RuntimeError, parameters[param], LoadSANS1MLZ,
                                   Filename=filename, OutputWorkspace=output_ws_name)
            os.remove(filename)

    def test_LoadValidData_notDefaultProperties(self):
        """
        test: create workspace with unique parameters
        """
        output_ws_name = "LoadSANS1MLZTest_Test3"
        filename = "D0511339.001"
        alg_test = run_algorithm("LoadSANS1MLZ",
                                 Filename=filename,
                                 OutputWorkspace=output_ws_name,
                                 SectionOption='CommentSection',
                                 Wavelength=3.2)
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
        self.assertEqual(3.2, run.getProperty('selector_lambda_value').value)
        self.assertEqual('error: blocked', run.getProperty('selector_tilt_status').value)
        self.assertEqual(0.00074, run.getProperty('selector_vacuum_value').value)

        run_algorithm("DeleteWorkspace", Workspace=output_ws_name)

    def test_LoadValidData_noMonitors(self):
        """
        test: create workspace with no monitors
        """
        output_ws_name = "LoadSANS1MLZTest_Test3"
        current_paths = config.getDataSearchDirs()[0]
        filename = "D0511339.001"
        filename_path = current_paths + filename
        file_inv = "sans-incomplete.001"
        with open(file_inv, "w") as f:
            with open(filename_path, "r") as fs:
                t = fs.readlines()
                f.writelines(t[:150])
                f.writelines(t[153:])
        alg_test = run_algorithm("LoadSANS1MLZ",
                                 Filename=file_inv,
                                 OutputWorkspace=output_ws_name,
                                 SectionOption='CommentSection',
                                 Wavelength=3.2)
        os.remove(file_inv)
        self.assertTrue(alg_test.isExecuted())

        # Verify some values
        ws = AnalysisDataService.retrieve(output_ws_name)
        self.assertEqual('SANS-1_MLZ', ws.getInstrument().getName())
        # dimensions
        self.assertEqual(16384, ws.getNumberHistograms())
        # self.assertEqual(2,  ws.getNumDims())
        # data array
        self.assertEqual(519, ws.readY(8502))
        self.assertEqual(427, ws.readY(8629))
        # sample logs
        run = ws.getRun()
        self.assertEqual(4, run.getProperty('det1_x_value').value)
        self.assertEqual(20000, run.getProperty('det1_z_value').value)
        self.assertEqual(3.2, run.getProperty('selector_lambda_value').value)
        self.assertEqual('error: blocked', run.getProperty('selector_tilt_status').value)
        self.assertEqual(0.00074, run.getProperty('selector_vacuum_value').value)

        run_algorithm("DeleteWorkspace", Workspace=output_ws_name)

    @staticmethod
    def _create_incomplete_dataFile(filename, param):
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
                f.write("%Counts\n\n")
                if prm[1] == 'pr':
                    s = ('1, ' * 127 + '1\n') * 50
                    s = s + ('1, ' * 126 + '1\n')
                    s = s + ('1, ' * 127 + '1\n') * 77
                elif prm[1] == '128':
                    s = ('1, ' * 127 + '1\n') * 127
                f.write(s)

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


class SANS1DataClassTestHelper:

    @staticmethod
    def set_up():
        metadata = SANSdata()
        current_paths = config.getDataSearchDirs()[0]
        filename = "D0511339.001"
        filename_path = current_paths + filename
        metadata.analyze_source(filename_path)
        return metadata, filename


class SANS1DataClassFileSectionTest(unittest.TestCase):

    def setUp(self) -> None:
        self.metadata, self.filename = SANS1DataClassTestHelper.set_up()

    def test_StartTime(self):
        date = np.datetime64('2018-01-23T15:33:20')
        self.assertEqual(date, self.metadata.file.run_start())
        date = np.datetime64('2010-04-27T07:30:25')
        self.metadata.file.info['FromDate'] = '04/27/2010'
        self.metadata.file.info['FromTime'] = '07:30:25 AM'
        self.assertEqual(date, self.metadata.file.run_start())

    def test_EndTime(self):
        date = np.datetime64('2018-01-23T15:33:51')
        self.assertEqual(date, self.metadata.file.run_end())
        date = np.datetime64('2010-04-27T07:30:25')
        self.metadata.file.info['ToDate'] = '04/27/2010'
        self.metadata.file.info['ToTime'] = '07:30:25 AM'
        self.assertEqual(date, self.metadata.file.run_end())

    def test_GetTitleName(self):
        title = 'Eichproben'
        self.assertEqual(title, self.metadata.file.get_title())
        self.metadata.file.info['Title'] = ''
        title = self.filename
        self.assertEqual(title, self.metadata.file.get_title())

    def test_CheckSomeValues(self):
        self.assertEqual('SANSDRaw', self.metadata.file.info['Type'])
        self.assertEqual('p9113', self.metadata.file.info['Proposal'])
        self.assertEqual('128', self.metadata.file.info['DataSizeX'])
        self.assertEqual('16384', self.metadata.file.info['DataSize'])


class SANS1DataClassSampleSectionTest(unittest.TestCase):

    def setUp(self) -> None:
        self.metadata, self.filename = SANS1DataClassTestHelper.set_up()

    def test_CheckSomeValues(self):
        self.assertEqual('0.00', self.metadata.sample.info['Omega'])
        self.assertEqual('-150.00', self.metadata.sample.info['BTableX'])
        self.assertEqual('50.00', self.metadata.sample.info['BTableY'])
        self.assertEqual('-10.00', self.metadata.sample.info['BTableZ'])
        self.assertEqual('-0.00002', self.metadata.sample.info['Magnet'])


class SANS1DataClassSetupSectionTest(unittest.TestCase):

    def setUp(self) -> None:
        self.metadata, self.filename = SANS1DataClassTestHelper.set_up()

    def test_CheckSomeValues(self):
        self.assertEqual('0.000000', self.metadata.setup.info['DetHAngle'])
        self.assertEqual('500.0', self.metadata.setup.info['BeamstopX'])
        self.assertEqual('500.0', self.metadata.setup.info['BeamstopY'])
        self.assertEqual('1.060', self.metadata.setup.info['Polarization_m'])
        self.assertEqual('1.064', self.metadata.setup.info['Polarization_c'])


class SANS1DataClassCounterSectionTest(unittest.TestCase):

    def setUp(self) -> None:
        self.metadata, self.filename = SANS1DataClassTestHelper.set_up()

    def test_MonitorExist(self):
        self.assertEqual(True, self.metadata.counter.is_monitors_exist())
        self.metadata.counter.monitor1 = 0
        self.metadata.counter.monitor2 = 0
        self.assertEqual(False, self.metadata.counter.is_monitors_exist())

    def test_CheckValues(self):
        self.assertEqual(97318, self.metadata.counter.monitor1)
        self.assertEqual(201028, self.metadata.counter.monitor2)
        self.assertEqual(30, self.metadata.counter.duration)
        self.assertEqual(261624, self.metadata.counter.sum_all_counts)


class SANS1DataClassCommentSectionTest(unittest.TestCase):

    def setUp(self) -> None:
        self.metadata, self.filename = SANS1DataClassTestHelper.set_up()

    def test_ChangeWavelength(self):
        self.assertEqual(6, self.metadata.comment.wavelength)
        self.metadata.comment.set_wavelength(4)
        self.assertEqual(4, self.metadata.comment.wavelength)
        self.assertEqual(4, self.metadata.comment.info['selector_lambda_value'])

    def test_CheckValues(self):
        # self.assertEqual(4, self.metadata.comment.det1_x_value)
        # self.assertEqual(-150, self.metadata.comment.st1_x_value)
        # self.assertEqual(0, self.metadata.comment.det1_omg_value)
        pass


class SANS1DataClassCountsSectionTest(unittest.TestCase):

    def setUp(self) -> None:
        self.metadata, self.filename = SANS1DataClassTestHelper.set_up()

    def test_Dimensions(self):
        dim = sum([len(i) for i in self.metadata.counts.data])
        self.assertEqual(16384, dim)


if __name__ == '__main__':
    unittest.main()
