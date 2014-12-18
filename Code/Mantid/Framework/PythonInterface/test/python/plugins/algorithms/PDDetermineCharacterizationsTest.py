import unittest
import numpy
from mantid.simpleapi import *
from mantid.kernel import *
from mantid.api import *

class PDDetermineCharacterizationsTest(unittest.TestCase):
    def createLogWksp(self):
        if self.logWksp is not None:
            return

        # create the log workspace
        self.logWksp = CreateWorkspace(DataX=numpy.arange(-1, 1.2, 0.2),
                                       DataY=numpy.arange(-1, 1, 0.2),
                                       OutputWorkspace="_det_char_logs")

        # add a frequency log
        freq = FloatTimeSeriesProperty("frequency")
        freq.units="Hz"
        freq.addValue(DateAndTime("2008-12-18T17:58:38"), 60.)
        self.logWksp.run().addProperty(freq.name, freq, True)

        # add a wavelength log
        wave = FloatTimeSeriesProperty("LambdaRequest")
        wave.units="Angstrom"
        wave.addValue(DateAndTime("2008-12-18T17:58:38"), 0.533)
        self.logWksp.run().addProperty(wave.name, wave, True)

    def createTableWksp(self, full):
        # pick the right name
        if full:
            if self.charWksp is not None:
                return
            name = "_det_char_table_full"
        else:
            if self.charWkspEmpty is not None:
                return
            name = "_det_char_table_empty"

        # table columns
        labels = ["frequency", "wavelength", "bank", "vanadium", "container",
                  "empty", "d_min", "d_max", "tof_min", "tof_max"]
        types =  ["double",    "double",     "int",  "int",      "int",
                  "int",   "str",   "str",   "double",  "double"]

        # create the table
        table = CreateEmptyTableWorkspace(OutputWorkspace=name)
        for (label, typ) in zip(labels, types):
            table.addColumn(typ, label)

        if full:
            rows = [[60., 0.533, 1, 17702, 17711, 0, "0.05",  "2.20",  0000.00, 16666.67],
                    [60., 1.333, 3, 17703, 17712, 0, "0.43",  "5.40", 12500.00, 29166.67],
                    [60., 2.665, 4, 17704, 17713, 0, "1.15",  "9.20", 33333.33, 50000.00],
                    [60., 4.797, 5, 17705, 17714, 0, "2.00", "15.35", 66666.67, 83333.67]]
            for row in rows:
                table.addRow(row)
            self.charWksp = table
        else:
            self.charWkspEmpty = table

    def setUp(self):
        self.mgrName = "__pd_reduction_properties"
        self.logWksp = None
        self.charWkspEmpty = None
        self.charWksp = None
        self.defaultInfo = {
            "frequency":0.,
            "wavelength":0.,
            "bank":1,
            "vanadium":0,
            "container":0,
            "empty":0,
            "d_min":"",
            "d_max":"",
            "tof_min":0.,
            "tof_max":0.
            }

    def tearDown(self):
        if self.logWksp is not None:
            DeleteWorkspace(self.logWksp)
        if self.charWksp is not None:
            DeleteWorkspace(self.charWksp)

    def test_exception(self):
        self.createLogWksp()
        self.assertRaises(RuntimeError, PDDetermineCharacterizations)
        self.assertRaises(RuntimeError, PDDetermineCharacterizations,
                          InputWorkspace=self.logWksp)

    def compareResult(self, expect, manager):
        for key in expect.keys():
            if type([]) == type(expect[key]):
                self.checkSequence(key, expect[key],
                                   manager.getProperty(key).value)
            else:
                self.assertEqual(expect[key], manager.getProperty(key).value,
                                "'%s' doesn't have expected value" % key)

    def checkSequence(self, k, tarr, rarr):
        try:
            self.assertSequenceEqual(tarr, rarr)
        except AttributeError:
            # RHEL6 doesn't have assertSequenceEqual since it runs python 2.6
            self.assertEquals(len(tarr), len(rarr),
                            "'%s' doesn't have same length" % k)
            import itertools
            for (i, (tval, rval)) in enumerate(itertools.izip(tarr, rarr)):
                try:
                    self.assertEqual(tval, rval)
                except AssertionError as detail:
                    message = "'%s' doesn't have expected value (%s) at index %d"
                    raise AssertionError(message % (k, detail, i))

    def test_emptyChar(self):
        self.createLogWksp()
        self.createTableWksp(False)
        PDDetermineCharacterizations(InputWorkspace=self.logWksp,
                                     Characterizations=self.charWkspEmpty,
                                     ReductionProperties=self.mgrName)

        self.compareResult(self.defaultInfo,
                           PropertyManagerDataService.retrieve(self.mgrName))

    def test_fullChar(self):
        self.createLogWksp()
        self.createTableWksp(True)
        PDDetermineCharacterizations(InputWorkspace=self.logWksp,
                                     Characterizations=self.charWksp,
                                     ReductionProperties=self.mgrName)

        result = {"frequency":60.,
                  "wavelength":0.533,
                  "bank":1,
                  "vanadium":17702,
                  "container":17711,
                  "empty":0,
                  "d_min":[0.05],
                  "d_max":[2.20],
                  "tof_min":0000.00,
                  "tof_max":16666.67}
        self.compareResult(result,
                           PropertyManagerDataService.retrieve(self.mgrName))

        PDDetermineCharacterizations(InputWorkspace=self.logWksp,
                                     Characterizations=self.charWksp,
                                     ReductionProperties=self.mgrName,
                                     BackRun=-1,
                                     NormRun=-1,
                                     NormBackRun=-1)
        result["vanadium"]  = 0
        result["container"] = 0
        result["empty"]     = 0
        self.compareResult(result,
                           PropertyManagerDataService.retrieve(self.mgrName))


if __name__ == "__main__":
    unittest.main()
