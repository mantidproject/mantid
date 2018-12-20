from __future__ import absolute_import, division, print_function
from mantid.simpleapi import LoadWANDSCD
import unittest


class LoadWANDTest(unittest.TestCase):

    def test(self):
        LoadWANDTest_ws = LoadWANDSCD('HB2C_7000.nxs.h5,HB2C_7001.nxs.h5')
        self.assertTrue(LoadWANDTest_ws)
        self.assertEquals(LoadWANDTest_ws.getNumDims(), 3)
        self.assertEquals(LoadWANDTest_ws.getNPoints(), 1966080*2)
        self.assertEquals(LoadWANDTest_ws.getSignalArray().max(), 7)

        d0 = LoadWANDTest_ws.getDimension(0)
        self.assertEquals(d0.name, 'y')
        self.assertEquals(d0.getNBins(), 512)
        self.assertEquals(d0.getMinimum(), 0.5)
        self.assertEquals(d0.getMaximum(), 512.5)

        d1 = LoadWANDTest_ws.getDimension(1)
        self.assertEquals(d1.name, 'x')
        self.assertEquals(d1.getNBins(), 3840)
        self.assertEquals(d1.getMinimum(), 0.5)
        self.assertEquals(d1.getMaximum(), 3840.5)

        d2 = LoadWANDTest_ws.getDimension(2)
        self.assertEquals(d2.name, 'scanIndex')
        self.assertEquals(d2.getNBins(), 2)
        self.assertEquals(d2.getMinimum(), 0.5)
        self.assertEquals(d2.getMaximum(), 2.5)

        self.assertEqual(LoadWANDTest_ws.getNumExperimentInfo(), 1)
        self.assertEquals(LoadWANDTest_ws.getExperimentInfo(0).getInstrument().getName(), 'WAND')

        run = LoadWANDTest_ws.getExperimentInfo(0).run()
        s1 = run.getProperty('s1').value
        self.assertEqual(len(s1), 2)
        self.assertEqual(s1[0], -142.6)
        self.assertEqual(s1[1], -142.5)
        run_number = run.getProperty('run_number').value
        self.assertEqual(len(run_number), 2)
        self.assertEqual(run_number[0], 7000)
        self.assertEqual(run_number[1], 7001)
        monitor_count=run.getProperty('monitor_count').value
        self.assertEqual(len(monitor_count), 2)
        self.assertEqual(monitor_count[0], 907880)
        self.assertEqual(monitor_count[1], 908651)
        duration = run.getProperty('duration').value
        self.assertEqual(len(duration), 2)
        self.assertAlmostEqual(duration[0], 40.05, 5)
        self.assertAlmostEqual(duration[1], 40.05, 5)

        LoadWANDTest_ws.delete()


if __name__ == '__main__':
    unittest.main()
