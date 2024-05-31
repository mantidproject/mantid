# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import tempfile
import os
import shutil
import systemtesting
from mantid.simpleapi import (
    LoadEmptyInstrument,
    CreatePeaksWorkspace,
    ConvertPeaksWorkspace,
    SetUB,
    SaveNexus,
    LoadNexus,
    AnalysisDataService,
)
from mantid.kernel import V3D


class SingleCrystalPeaksIntHKLIntMNPSaveLoadTest(systemtesting.MantidSystemTest):
    def setUp(self):
        self._test_dir = tempfile.mkdtemp()

    def tearDown(self):
        AnalysisDataService.clear()
        shutil.rmtree(self._test_dir)

    def runTest(self):
        # peaks workspace
        ws = LoadEmptyInstrument(InstrumentName="TOPAZ")
        pks = CreatePeaksWorkspace(ws, NumberOfPeaks=0)

        SetUB(pks, 5, 6, 7, 90, 90, 120)
        pk = pks.createPeak([3, 2, 1])
        pk.setIntHKL(V3D(1, 3, 2))
        pk.setIntMNP(V3D(3, 2, 1))
        pks.addPeak(pk)

        file_name = os.path.join(self._test_dir, "peaks_workspace.nxs")

        SaveNexus(pks, Filename=file_name)
        ref = LoadNexus(Filename=file_name)

        self.assertAlmostEqual(ref.getPeak(0).getIntHKL(), pks.getPeak(0).getIntHKL())
        self.assertAlmostEqual(ref.getPeak(0).getIntMNP(), pks.getPeak(0).getIntMNP())

        # lean peaks workspace
        pks = ConvertPeaksWorkspace(pks)

        lean_file_name = os.path.join(self._test_dir, "peaks_workspace.nxs")

        SaveNexus(pks, Filename=lean_file_name)
        ref = LoadNexus(Filename=lean_file_name)

        self.assertAlmostEqual(ref.getPeak(0).getIntHKL(), pks.getPeak(0).getIntHKL())
        self.assertAlmostEqual(ref.getPeak(0).getIntMNP(), pks.getPeak(0).getIntMNP())


class SingleCrystalPeaksSaveLoadTest(systemtesting.MantidSystemTest):
    def setUp(self):
        self._test_dir = tempfile.mkdtemp()

    def tearDown(self):
        AnalysisDataService.clear()
        shutil.rmtree(self._test_dir)

    def runTest(self):
        # peaks workspace
        ws = LoadEmptyInstrument(InstrumentName="TOPAZ")
        pks = CreatePeaksWorkspace(ws, NumberOfPeaks=0)

        SetUB(pks, 5, 6, 7, 90, 90, 120)
        pk = pks.createPeak([3, 2, 1])
        pk.setIntHKL(V3D(1, 3, 2))
        pk.setIntMNP(V3D(3, 2, 1))
        pks.addPeak(pk)

        file_name = os.path.join(self._test_dir, "peaks_workspace.nxs")

        SaveNexus(pks, Filename=file_name)
        ref = LoadNexus(Filename=file_name)

        self.assertAlmostEqual(ref.getPeak(0).getIntHKL(), pks.getPeak(0).getIntHKL())
        self.assertAlmostEqual(ref.getPeak(0).getIntMNP(), pks.getPeak(0).getIntMNP())

        # lean peaks workspace
        pks = ConvertPeaksWorkspace(pks)

        lean_file_name = os.path.join(self._test_dir, "peaks_workspace.nxs")

        SaveNexus(pks, Filename=lean_file_name)
        ref = LoadNexus(Filename=lean_file_name)

        self.assertAlmostEqual(ref.getPeak(0).getIntHKL(), pks.getPeak(0).getIntHKL())
        self.assertAlmostEqual(ref.getPeak(0).getIntMNP(), pks.getPeak(0).getIntMNP())
