import mantidqtpython
import mantidplottests
from mantidplottests import *


class TSVSerialiserTest(unittest.TestCase):

    def genLines(self):
        tmp = mantidqtpython.MantidQt.API.TSVSerialiser()
        tmp.writeLine("test")
        tmp.storeDouble(1.1)
        tmp.storeInt(2)

        tmp.writeLine("line2")
        tmp.storeString("Hello World")
        tmp.storeBool(True)

        return tmp.outputLines()

    def test_lines(self):
        lines = self.genLines()
        load = mantidqtpython.MantidQt.API.TSVSerialiser(lines)
        self.assertEqual(load.values("test")[0], "test")
        self.assertEqual(load.values("test")[1], "1.1")
        self.assertEqual(load.values("test")[2], "2")

    def test_numbers(self):
        lines = self.genLines()
        load = mantidqtpython.MantidQt.API.TSVSerialiser(lines)
        load.selectLine("test")
        self.assertEqual(load.readDouble(), 1.1)
        self.assertEqual(load.readInt(), 2)

    def test_stringAndBool(self):
        lines = self.genLines()
        load = mantidqtpython.MantidQt.API.TSVSerialiser(lines)
        load.selectLine("line2")
        self.assertEqual(load.readString(), "Hello World")
        self.assertEqual(load.readBool(), True)

    def test_sections(self):
        lines = self.genLines()
        tmp = mantidqtpython.MantidQt.API.TSVSerialiser()
        tmp.writeSection("Section", lines)
        lines = tmp.outputLines()

        tmp2 = mantidqtpython.MantidQt.API.TSVSerialiser()
        tmp2.writeSection("Big", lines)
        lines = tmp2.outputLines()

        # read the sections back
        load = mantidqtpython.MantidQt.API.TSVSerialiser(lines)
        secs = load.sections("Big")
        load = mantidqtpython.MantidQt.API.TSVSerialiser(secs[0])

        secs = load.sections("Section")
        load = mantidqtpython.MantidQt.API.TSVSerialiser(secs[0])

        load.selectLine("test")
        self.assertEqual(load.readDouble(), 1.1)
        self.assertEqual(load.readInt(), 2)

        load.selectLine("line2")
        self.assertEqual(load.readString(), "Hello World")
        self.assertEqual(load.readBool(), True)


mantidplottests.runTests(TSVSerialiserTest)
