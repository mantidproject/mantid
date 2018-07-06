from __future__ import (absolute_import, division, print_function)

import unittest

import mantid

from reduction_gui.reduction.toftof.toftof_reduction import TOFTOFScriptElement, OptionalFloat
from reduction_gui.widgets.toftof.toftof_setup import TOFTOFSetupWidget

import sys


class TOFTOFScriptElementTest(unittest.TestCase):
    @staticmethod
    def setValidInputs(scriptElement):
        scriptElement.reset()
        scriptElement.facility_name   = ''
        scriptElement.instrument_name = ''

        # prefix of (some) workspace names
        scriptElement.prefix   = 'ws'

        # data files are here
        scriptElement.dataDir  = '/Somepath/somewhere/'

        # vanadium runs & comment
        scriptElement.vanRuns  = '1:3'
        scriptElement.vanCmnt  = 'vanadium comment'
        scriptElement.vanTemp  = OptionalFloat(None)

        # empty can runs, comment, and factor
        scriptElement.ecRuns   = ''
        scriptElement.ecTemp   = OptionalFloat(None)
        scriptElement.ecFactor = 1

        # data runs: [(runs,comment, temperature), ...]
        scriptElement.dataRuns = [[u'0:5', u'Comment for Run 0:5', OptionalFloat(None)]]

        # additional parameters
        scriptElement.binEon        = True
        scriptElement.binEstart     = 0.0
        scriptElement.binEstep      = 0.1
        scriptElement.binEend       = 1.0

        scriptElement.binQon        = True
        scriptElement.binQstart     = 0.0
        scriptElement.binQstep      = 0.1
        scriptElement.binQend       = 1.0

        scriptElement.maskDetectors = ''

        # options
        scriptElement.subtractECVan = False
        scriptElement.normalise     = TOFTOFScriptElement.NORM_NONE
        scriptElement.correctTof    = TOFTOFScriptElement.CORR_TOF_NONE
        scriptElement.replaceNaNs   = False
        scriptElement.createDiff    = False
        scriptElement.keepSteps     = False

        # save data
        scriptElement.saveDir      = ''
        scriptElement.saveSofQW    = False
        scriptElement.saveSofTW    = False
        scriptElement.saveNXSPE    = False
        scriptElement.saveNexus    = False
        scriptElement.saveAscii    = False

    def setUp(self):
        self.scriptElement = TOFTOFScriptElement()
        self.setValidInputs(self.scriptElement)

    def tearDown(self):
        self.scriptElement = None

    @unittest.skip('wrongTest')
    def test_that_inputs_are_validated_correctly(self):
        pass
        
    def test_that_inputs_saved_and_loaded_correctly(self):
        scriptElement2 = TOFTOFScriptElement()
        scriptElement2.from_xml(self.scriptElement.to_xml())
        for name in dir(self.scriptElement):
            if not name.startswith('__') and not hasattr(getattr(self.scriptElement, name), '__call__'):
                self.assertEqual(getattr(self.scriptElement, name), getattr(scriptElement2, name))
        
    @unittest.skip('wrongTest')
    def test_that_script_is_generated_correctly(self):
        pass
        
    def test_that_script_has_correct_syntax(self):
        self.scriptElement.binEon = False
        self.scriptElement.binQon = False
        self.scriptElement.dataRuns = [('0:5', 'Comment for Run 0:5', None)]

        script = self.scriptElement.to_script()
        try:
            compiledScript = compile(script, '<string>', 'exec')
        except SyntaxError as e:
            self.fail("generated script has a syntax error: '{}'".format(e))
        except ValueError as e:
            if sys.version_info >= (3, 5):
                self.fail("generated script has null bytes: '{}'".format(e))
        except TypeError as e:
            if sys.version_info < (3, 5):
                self.fail("generated script has null bytes: '{}'".format(e))
        self.assertIsNotNone(compiledScript)


    def test_that_inputs_are_validated_correctly(self):
        try:
            self.scriptElement.validate_inputs()
        except RuntimeError as e:
            self.fail(e)

        # [(inputDict, causesException=True), ...]
        modifiedValues = [
        ({}, False),
        ({'binEstart' : 1.0}, True),
        ({'binEstep'  : 2.0}, True),
        ({'binEstep'  : 0.0}, True),
        ({'binEstep'  : -0.1}, True),
        ({'binEstep'  : -0.1, 'binEon' : False}, False),
        ({'binQstart' : 1.0}, True),
        ({'binQstep'  : 2.0}, True),
        ({'binQstep'  : 0.0}, True),
        ({'binQstep'  : -0.1}, True),
        ({'binQstep'  : -0.1, 'binQon' : False}, False),
        ({'dataRuns'  : []}, True),
        ]

        for inputs, shoudThrow in modifiedValues:
            self.setValidInputs(self.scriptElement)
            for name, value in inputs.items():
                setattr(self.scriptElement, name, value)

            try:
                self.scriptElement.validate_inputs()
            except RuntimeError as e:
                if not shoudThrow:
                    self.fail("valid input did cause an exception: '{}'".format(e))
            else:
                if shoudThrow:
                    self.fail("invalid input did not cause an exception!")


    @unittest.skip('wrongTest')
    def test_that_the_white_list_is_correct(self):
        presenter = MainPresenter(SANSFacility.ISIS)
        self.assertTrue(presenter.get_number_of_white_list_items() == 0)
        white_list = presenter.get_white_list(show_periods=True)
        self.assertEqual(presenter.get_number_of_white_list_items(), 20)

        self.assertTrue(white_list[0].algorithm_property == "SampleScatter")
        self.assertTrue(white_list[1].algorithm_property == "SampleScatterPeriod")
        self.assertTrue(white_list[2].algorithm_property == "SampleTransmission")
        self.assertTrue(white_list[3].algorithm_property == "SampleTransmissionPeriod")
        self.assertTrue(white_list[4].algorithm_property == "SampleDirect")
        self.assertTrue(white_list[5].algorithm_property == "SampleDirectPeriod")
        self.assertTrue(white_list[6].algorithm_property == "CanScatter")
        self.assertTrue(white_list[7].algorithm_property == "CanScatterPeriod")
        self.assertTrue(white_list[8].algorithm_property == "CanTransmission")
        self.assertTrue(white_list[9].algorithm_property == "CanTransmissionPeriod")
        self.assertTrue(white_list[10].algorithm_property == "CanDirect")
        self.assertTrue(white_list[11].algorithm_property == "CanDirectPeriod")
        self.assertTrue(white_list[12].algorithm_property == "UseOptimizations")
        self.assertTrue(white_list[13].algorithm_property == "PlotResults")
        self.assertTrue(white_list[14].algorithm_property == "OutputName")
        self.assertTrue(white_list[15].algorithm_property == "UserFile")
        self.assertEqual(white_list[16].algorithm_property, "SampleThickness")
        self.assertTrue(white_list[17].algorithm_property == "RowIndex")
        self.assertTrue(white_list[18].algorithm_property == "OutputMode")
        self.assertTrue(white_list[19].algorithm_property == "OutputGraph")

    @unittest.skip('wrongTest')
    def test_that_black_list_is_correct(self):
        presenter = MainPresenter(SANSFacility.ISIS)
        expected = "InputWorkspace,OutputWorkspace,SampleScatter,SampleScatterPeriod,SampleTransmission," \
                   "SampleTransmissionPeriod,SampleDirect,SampleDirectPeriod,CanScatter,CanScatterPeriod," \
                   "CanTransmission,CanTransmissionPeriod,CanDirect,CanDirectPeriod," \
                   "UseOptimizations,PlotResults,OutputName,UserFile,SampleThickness,RowIndex,OutputMode,OutputGraph,"
        self.assertEqual(expected, presenter.get_black_list())

    @unittest.skip('wrongTest')
    def test_that_gets_pre_processing_options_are_valid_and_other_options_are_empty(self):
        # Arrange
        presenter = MainPresenter(SANSFacility.ISIS)
        content = "# MANTID_BATCH_FILE add more text here\n" \
                  "sample_sans,SANS2D00022024,sample_trans,SANS2D00022048," \
                  "sample_direct_beam,SANS2D00022048,output_as,test_file\n" \
                  "sample_sans,SANS2D00022024,output_as,test_file2\n"
        batch_file_path = save_to_csv(content)
        user_file_path = create_user_file(sample_user_file)
        view = create_mock_view2(user_file_path, batch_file_path)
        presenter.set_view(view)

        # Act
        pre_processing_options = presenter.getProcessingOptions()

        # Assert
        expected = {'UseOptimizations':'1','OutputMode':'PublishToADS','PlotResults':'1', \
                    'OutputGraph':'SANS-Latest'}
        self.assertEqual(expected, pre_processing_options)
        self.assertFalse(presenter.getPreprocessingOptions())
        self.assertFalse(presenter.getPostprocessingOptionsAsString())

        # Clean up
        remove_file(sample_user_file)
        remove_file(user_file_path)


if __name__ == '__main__':
    unittest.main()


