import unittest
from mantid.kernel import DateAndTime
from mantid.geometry import(Detector, Instrument, ObjComponent, ReferenceFrame)
from testhelpers import can_be_instantiated, WorkspaceCreationHelper

class InstrumentTest(unittest.TestCase):

    __testws = None

    def setUp(self):
        if self.__testws is None:
            self.__class__.__testws = \
                WorkspaceCreationHelper.create2DWorkspaceWithFullInstrument(1,1)

    def test_Instrument_cannot_be_instantiated(self):
        self.assertFalse(can_be_instantiated(Instrument))

    def test_getSample(self):
        sample_pos = self.__testws.getInstrument().getSample()
        self.assertTrue(isinstance(sample_pos, ObjComponent))

    def test_getSource(self):
        source_pos = self.__testws.getInstrument().getSource()
        self.assertTrue(isinstance(source_pos, ObjComponent))

    def test_getComponentByName(self):
        comp = self.__testws.getInstrument().getComponentByName("pixel-0)")
        self.assertTrue(isinstance(comp, Detector))

    def test_getDetector(self):
        comp = self.__testws.getInstrument().getDetector(1)
        self.assertTrue(isinstance(comp, Detector))

    def test_getReferenceFrame(self):
        frame = self.__testws.getInstrument().getReferenceFrame()
        self.assertTrue(isinstance(frame, ReferenceFrame))

    def test_ValidDates(self):
        inst = self.__testws.getInstrument()
        valid_from = inst.getValidFromDate()
        valid_to = inst.getValidToDate()

        self.assertTrue(isinstance(valid_from, DateAndTime))
        self.assertTrue(isinstance(valid_to, DateAndTime))

    def test_baseInstrument_Can_Be_Retrieved(self):
        inst = self.__testws.getInstrument()
        base_inst = inst.getBaseInstrument()
        self.assertEquals("testInst", base_inst.getName())

if __name__ == '__main__':
    unittest.main()
