# Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.simpleapi import LoadEmptyInstrument
from instrumentview.FullInstrumentViewWindow import FullInstrumentViewWindow
import unittest
from qtpy.QtWidgets import QApplication


class TestEmptyInstruments(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls._app = QApplication([])

    @classmethod
    def tearDownClass(cls):
        cls._app.quit()

    def _create_empty_instrument_and_draw(self, instrument: str):
        ws = LoadEmptyInstrument(InstrumentName=instrument)
        FullInstrumentViewWindow(ws, off_screen=True)

    def test_gem(self):
        self._create_empty_instrument_and_draw("GEM")

    def test_mari(self):
        self._create_empty_instrument_and_draw("Mari")

    def test_merlin(self):
        self._create_empty_instrument_and_draw("Merlin")

    def test_vesuvio(self):
        self._create_empty_instrument_and_draw("Vesuvio")

    def test_maps(self):
        self._create_empty_instrument_and_draw("Maps")

    def test_sandals(self):
        self._create_empty_instrument_and_draw("Sandals")

    def test_alf(self):
        self._create_empty_instrument_and_draw("Alf")

    def test_surf(self):
        self._create_empty_instrument_and_draw("Surf")

    def test_crisp(self):
        self._create_empty_instrument_and_draw("Crisp")

    def test_loq(self):
        self._create_empty_instrument_and_draw("LOQ")

    def test_osiris(self):
        self._create_empty_instrument_and_draw("Osiris")

    def test_iris(self):
        self._create_empty_instrument_and_draw("Iris")

    def test_polaris(self):
        self._create_empty_instrument_and_draw("Polaris")

    def test_ines(self):
        self._create_empty_instrument_and_draw("INES")

    def test_tosca(self):
        self._create_empty_instrument_and_draw("Tosca")

    def test_larmor(self):
        self._create_empty_instrument_and_draw("Larmor")

    def test_offspec(self):
        self._create_empty_instrument_and_draw("Offspec")

    def test_inter(self):
        self._create_empty_instrument_and_draw("Inter")

    def test_polref(self):
        self._create_empty_instrument_and_draw("Polref")

    def test_sans2d(self):
        self._create_empty_instrument_and_draw("Sans2d")

    def test_let(self):
        self._create_empty_instrument_and_draw("Let")

    def test_nimrod(self):
        self._create_empty_instrument_and_draw("Nimrod")

    def test_hifi(self):
        self._create_empty_instrument_and_draw("HiFi")

    def test_musr(self):
        self._create_empty_instrument_and_draw("MuSR")

    def test_emu(self):
        self._create_empty_instrument_and_draw("Emu")

    def test_argus(self):
        self._create_empty_instrument_and_draw("Argus")

    def test_chronus(self):
        self._create_empty_instrument_and_draw("Chronus")

    def test_enginx(self):
        self._create_empty_instrument_and_draw("Engin-X")

    def test_hrpd(self):
        self._create_empty_instrument_and_draw("HRPD")

    def test_pearl(self):
        self._create_empty_instrument_and_draw("Pearl")

    def test_sxd(self):
        self._create_empty_instrument_and_draw("SXD")

    def test_wish(self):
        self._create_empty_instrument_and_draw("WISH")

    def test_zoom(self):
        self._create_empty_instrument_and_draw("ZOOM")
