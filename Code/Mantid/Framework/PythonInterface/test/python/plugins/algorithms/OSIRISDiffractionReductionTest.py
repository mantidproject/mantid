import unittest
from mantid.simpleapi import *
from mantid.api import *


class OSIRISDiffractionReductionTest(unittest.TestCase):

    def test_basic_reduction_completes(self):
        """
        Sanity test to ensure the most basic reduction actually completes.
        """

        ws = OSIRISDiffractionReduction(Sample=['OSI89813.raw'],
                                        CalFile='osiris_041_RES10.cal',
                                        Vanadium=['osi89757.raw'])

        self.assertTrue(isinstance(ws, MatrixWorkspace), 'Result workspace should be a matrix workspace.')
        self.assertEqual(ws.getAxis(0).getUnit().unitID(), 'dSpacing')


    def test_reduction_with_manual_drange_completes(self):
        """
        Test to ensure reduction with manual dRange selection completes.
        The run here is for dRange 4.
        """

        ws = OSIRISDiffractionReduction(Sample=['OSI10203.raw'],
                                        CalFile='osiris_041_RES10.cal',
                                        Vanadium=['OSI10156.raw'],
                                        DetectDRange=False,
                                        DRange=4)

        self.assertTrue(isinstance(ws, MatrixWorkspace), 'Result workspace should be a matrix workspace.')
        self.assertEqual(ws.getAxis(0).getUnit().unitID(), 'dSpacing')


    def test_reduction_with_can_subtraction(self):
        """
        Tests reduction after subtraction of an empty can.
        """

        ws = OSIRISDiffractionReduction(Sample=['OSI10203.raw'],
                                        CalFile='osiris_041_RES10.cal',
                                        Vanadium=['OSI10156.raw'],
                                        Container='OSI10241.raw',
                                        DetectDRange=False,
                                        DRange=4)

        self.assertTrue(isinstance(ws, MatrixWorkspace), 'Result workspace should be a matrix workspace.')
        self.assertEqual(ws.getAxis(0).getUnit().unitID(), 'dSpacing')


    def test_reduction_with_can_subtraction_and_scale(self):
        """
        Tests reduction after subtraction of an empty can.
        """

        ws = OSIRISDiffractionReduction(Sample=['OSI10203.raw'],
                                        CalFile='osiris_041_RES10.cal',
                                        Vanadium=['OSI10156.raw'],
                                        Container='OSI10241.raw',
                                        ContainerScaleFactor=0.5,
                                        DetectDRange=False,
                                        DRange=4)

        self.assertTrue(isinstance(ws, MatrixWorkspace), 'Result workspace should be a matrix workspace.')
        self.assertEqual(ws.getAxis(0).getUnit().unitID(), 'dSpacing')


if __name__ == '__main__':
    unittest.main()
