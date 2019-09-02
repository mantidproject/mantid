# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from mantidqtpython import MantidQt
import pythonTSV

Name = "test"

"""
Returns a TSV
to read from the input TSV
"""
def load_TSV(TSV):
    string = TSV.outputLines()
    return MantidQt.API.TSVSerialiser(string)

"""
Returns a TSV on the line Name
"""
def get_loaded_data(TSV,value):
    load = load_TSV(TSV)
    load.selectLine(Name)
    return load

# dummy class
class Class(object):
    def __init__(self):
        self.a = "a"

class PythonTSVTest(unittest.TestCase):

    def test_saveDouble(self):
        value = 1.1
        TSV = MantidQt.API.TSVSerialiser()
        TSV.writeLine(Name)
        pythonTSV.saveToTSV(TSV,value)
        load =get_loaded_data(TSV,value)
        self.assertEqual(value, load.readDouble())

    def test_saveInt(self):
        value = 1
        TSV = MantidQt.API.TSVSerialiser()
        TSV.writeLine(Name)
        pythonTSV.saveToTSV(TSV,value)
        load =get_loaded_data(TSV,value)
        self.assertEqual(value, load.readInt())

    def test_saveString(self):
        value = "string"
        TSV = MantidQt.API.TSVSerialiser()
        TSV.writeLine(Name)
        pythonTSV.saveToTSV(TSV,value)
        load =get_loaded_data(TSV,value)
        self.assertEqual(value, load.readString())
 
    def test_saveBool(self):
        value = False
        TSV = MantidQt.API.TSVSerialiser()
        TSV.writeLine(Name)
        pythonTSV.saveToTSV(TSV,value)
        load =get_loaded_data(TSV,value)
        self.assertEqual(value, load.readBool())

    def test_saveClassFails(self):
        value = Class()
        TSV = MantidQt.API.TSVSerialiser()
        TSV.writeLine(Name)
        with self.assertRaises(TypeError):
             pythonTSV.saveToTSV(TSV,value)

    def test_readDouble(self):
        value = 1.1
        TSV = MantidQt.API.TSVSerialiser()
        TSV.writeLine(Name)
        TSV.storeDouble(value)
        load =load_TSV(TSV)
        self.assertEqual(value, pythonTSV.loadFromTSV(load,Name,3.3))


    def test_readInt(self):
        value = 1
        TSV = MantidQt.API.TSVSerialiser()
        TSV.writeLine(Name)
        TSV.storeInt(value)
        load =load_TSV(TSV)
        self.assertEqual(value, pythonTSV.loadFromTSV(load,Name,3))

    def test_saveString(self):
        value = "string"
        TSV = MantidQt.API.TSVSerialiser()
        TSV.writeLine(Name)
        TSV.storeString(value)
        load =load_TSV(TSV)
        self.assertEqual(value, pythonTSV.loadFromTSV(load,Name,"test"))

    def test_saveBool(self):
        value = False
        TSV = MantidQt.API.TSVSerialiser()
        TSV.writeLine(Name)
        TSV.storeBool(value)
        load =load_TSV(TSV)
        self.assertEqual(value, pythonTSV.loadFromTSV(load,Name,True))

 
    def test_saveClassFails(self):
        value = Class()
        TSV = MantidQt.API.TSVSerialiser()
        TSV.writeLine(Name)
        TSV.storeString("doesn't matter what is stored")
        load =load_TSV(TSV)
        with self.assertRaises(TypeError):
            pythonTSV.loadFromTSV(load,Name,value)

    def test_makeLineNameSafeSpaces(self):
        badName = "spaces fail"
        newName = pythonTSV.makeLineNameSafe(badName)
        self.assertEqual(newName,"SpacesFail")

    def test_makeLineNameSafeUnderscores(self):
        badName = "underscores_fail"
        newName = pythonTSV.makeLineNameSafe(badName)
        self.assertEqual(newName,"UnderscoresFail")

    def test_makeLineNameSafeDashes(self):
        badName = "dashes-fail"
        newName = pythonTSV.makeLineNameSafe(badName)
        self.assertEqual(newName,"DashesFail")

    def test_writeLine(self):
        badName = "bad name_example-here"
        TSV = MantidQt.API.TSVSerialiser()
        pythonTSV.writeLine(TSV, badName)
        pi = 3.14159
        TSV.storeDouble(pi)
        load = load_TSV(TSV)
        safeName = pythonTSV.makeLineNameSafe(badName)
        load.selectLine(safeName)
        self.assertEqual(pi, load.readDouble())
        

if __name__ == '__main__':
    unittest.main()

