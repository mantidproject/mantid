# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""Test suite for the PyChop package"""

import unittest
from unittest import mock
from unittest.mock import patch
import builtins
import warnings
import numpy as np

from pychop.Instruments import Instrument
from pychop import MulpyRep


class PyChopInstrumentTests(unittest.TestCase):
    # Tests the Fermi chopper instruments
    def test_pychop_fermi(self):
        instnames = ["maps", "mari", "merlin"]
        res = []
        flux = []
        for inc, instname in enumerate(instnames):
            chopobj = Instrument(instname)
            # Code should give an error if the chopper settings and Ei have
            # not been set.
            self.assertRaises(ValueError, chopobj.getResolution)
            chopobj.setChopper("s", 200)
            chopobj.setEi(18)
            rr, ff = chopobj.getResFlux(np.linspace(0, 17, 10))
            res.append(rr)
            flux.append(ff)
        # Checks that the flux should be highest for MERLIN, MARI and MAPS in that order
        self.assertGreater(flux[2], flux[1])
        # Note that MAPS has been upgraded so now should have higher flux than MARI.
        self.assertGreater(flux[0], flux[1])
        # Checks that the resolution should be best for MAPS, MARI, and MERLIN in that order
        # actually MAPS and MARI resolutions are very close (previous error in MAPS distances
        # meant that MARI was calculated to have a better resolution, but it *should* be MAPS)
        self.assertLess(res[0][0], res[1][0])
        self.assertLess(res[1][0], res[2][0])
        # Now tests the standalone function
        for inc, instname in enumerate(instnames):
            rr, ff = Instrument.calculate(instname, "s", 200, 18, 0)
            self.assertAlmostEqual(rr[0], res[inc][0], places=7)
            self.assertAlmostEqual(ff, flux[inc], places=7)

    # Tests the different variants of LET
    def test_pychop_let(self):
        variants = ["High flux", "Intermediate", "High resolution"]
        res = []
        flux = []
        for inc, variant in enumerate(variants):
            chopobj = Instrument("LET", variant)
            # Checks that it instantiates the correct variant
            self.assertTrue(variant in chopobj.getChopper())
            # Code should give an error if the chopper settings and Ei have
            # not been set.
            self.assertRaises(ValueError, chopobj.getResolution)
            chopobj.setFrequency(200)
            chopobj.setEi(18)
            rr, ff = chopobj.getResFlux(np.linspace(0, 17, 10))
            res.append(rr)
            flux.append(ff)
        # Checks that the flux should be highest for 'High flux', then 'Intermediate', 'High resolution'
        self.assertGreater(flux[0], flux[1])
        self.assertGreaterEqual(flux[1], flux[2])
        # Checks that the resolution should be best for 'High resolution', then 'Intermediate', 'High flux'
        self.assertLessEqual(res[2][0], res[1][0])
        self.assertLessEqual(res[1][0], res[0][0])
        # Now tests the standalone function
        for inc, variant in enumerate(variants):
            rr, ff = Instrument.calculate("LET", variant, 200, 18, 0)
            self.assertAlmostEqual(rr[0], res[inc][0], places=7)
            self.assertAlmostEqual(ff, flux[inc], places=7)

    def test_pychop_invalid_ei(self):
        chopobj = Instrument("MARI", "G", 400.0)
        chopobj.setEi(120)
        with warnings.catch_warnings(record=True) as w:
            res = chopobj.getResolution(130.0)
            assert len(w) == 1
            assert issubclass(w[0].category, UserWarning)
            assert "Cannot calculate for energy transfer greater than Ei" in str(w[0].message)
            assert np.isnan(res[0])


class MockedModule(mock.MagicMock):
    # A class which is meant to act like a module
    def __init__(self, *args, mock_class=mock.MagicMock, **kwargs):
        super().__init__(*args, **kwargs)
        self.mock_class = mock_class

    def __call__(self, *args, **kwargs):
        return self.mock_class(*args, **kwargs)


class MockImports:
    """
    Class to mock imports. Meant to be used with patch on `builtins.__import__`
    Fake modules can be access using the dot notation on this object, e.g.
    >>> mockmods = MockImports(include='numpy')
    >>> with patch('builtins.__import__', mockmods.import_func):
    >>>     import numpy.sin
    >>>     print(numpy.sin)
    >>> print(mockmods.numpy.sin)
    """

    def __init__(self, include=None, replace=None):
        # By default all imports will be mocked.
        # Use the include list to mock only specified modules (and submodules)
        # Use replace to specify a object to substitute for the real module
        self.include = include
        self.replace = replace
        self.real_import = builtins.__import__
        self.loaded_modules = {}  # Stores mocks of loaded fake modules
        self.loaded_from = {}  # Stores mocks of "from" syntax
        if replace:
            for replacement_mock in replace:
                if replacement_mock not in self.include:
                    self.include.append(replacement_mock)

    def _check_dot_names(self, name, ref_list):
        for ref_name in ref_list:
            level = ref_name.count(".") + 1
            requested = ".".join(name.split(".")[:level])
            if ref_name == requested:
                return name
        return None

    def is_module_included(self, module_name):
        if not self.include:
            return True
        check_includes = self._check_dot_names(module_name, self.include)
        return True if check_includes is not None else False

    def get_mock(self, name, fromlist):
        replacement = self._check_dot_names(name, self.replace)
        root_name = name.split(".")[0]
        save = False
        if replacement is not None:
            rv = self.replace[replacement]
        elif root_name in self.loaded_modules:
            rv = self.loaded_modules[root_name]
            for mods in fromlist:
                if mods not in self.loaded_from:
                    self.loaded_from[mods] = rv
        elif name in self.loaded_from:
            rv = self.loaded_from[name]
        else:
            rv = mock.MagicMock()
            save = True
        if fromlist and replacement is None:
            for mods in fromlist:
                replacement = self._check_dot_names(mods, self.replace)
                if replacement is not None:
                    setattr(rv, mods, self.replace[replacement])
        if save:
            self.save_module(name, fromlist, rv)
        return rv

    def save_module(self, name, fromlist, mock_object):
        root_name = name.split(".")[0]
        self.loaded_modules[root_name] = mock_object
        if fromlist:
            for mods in fromlist:
                self.loaded_from[mods] = mock_object

    def import_func(self, name, globals=None, locals=None, fromlist=(), level=0):
        if self.include:
            if self.is_module_included(name):
                return self.get_mock(name, fromlist)
            else:
                return self.real_import(name, globals, locals, fromlist, level)
        else:
            return mock.MagicMock()

    def __getattr__(self, module_name):
        if module_name in self.loaded_modules:
            return self.loaded_modules[module_name]
        if module_name in self.loaded_from:
            return getattr(self.loaded_from[module_name], module_name)


class fake_QMainWindow:
    def __init__(self, *args, **kwargs):
        self.menuBar = mock.MagicMock()
        self.setCentralWidget = mock.MagicMock()
        self.setWindowTitle = mock.MagicMock()

    def setWindowFlags(self, *args, **kwargs):
        pass

    def show(self):
        pass


class fake_QCombo(mock.MagicMock):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.clear()

    def clear(self):
        self.items = []
        self.currentIndex = 0

    def addItem(self, item):
        self.items.append(item)

    def currentText(self):
        return self.items[self.currentIndex]

    def count(self):
        return len(self.items)

    def itemText(self, idx):
        return self.items[idx]

    def setCurrentIndex(self, idx):
        self.currentIndex = idx

    def __getattr__(self, attribute):
        if attribute not in self.__dict__:
            self.__dict__[attribute] = mock.MagicMock()
        return self.__dict__[attribute]


class fake_QLineEdit(mock.MagicMock):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.value = "1.0"

    def setText(self, value):
        self.value = value

    def text(self):
        return self.value

    def __getattr__(self, attribute):
        if attribute not in self.__dict__:
            self.__dict__[attribute] = mock.MagicMock()
        return self.__dict__[attribute]


class fake_Line(mock.MagicMock):
    def __init__(self, parent, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.parent = parent

    def set_label(self, label):
        self.parent.legends[self] = label


class fake_Axes(mock.MagicMock):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.legends = {}

    def plot(self, *args, **kwargs):
        self.lines.append(fake_Line(self))
        return (self.lines[-1],)

    def get_legend_handles_labels(self):
        labels = [self.legends[line] for line in self.lines]
        return self.lines, labels


class fake_Figure(mock.MagicMock):
    def add_subplot(self, *args, **kwargs):
        return fake_Axes()


class fake_Slider(mock.MagicMock):
    def __init__(self, parent, label, valmin, valmax, **kwargs):
        super().__init__(parent, label, valmin, valmax, **kwargs)
        self.parent, self.label, self.valmin, self.valmax = parent, label, valmin, valmax
        self.val = kwargs.pop("valinit", 0.5)
        self.valtext = mock.MagicMock()
        self.on_changed = mock.MagicMock()


class PyChopGuiTests(unittest.TestCase):
    # Tests GUI routines

    @classmethod
    def setUpClass(cls):
        cls.mock_modules = MockImports(
            include=["qtpy", "matplotlib", "mantidqt", "mantid.plots"],
            replace={
                "QMainWindow": fake_QMainWindow,
                "QComboBox": MockedModule(mock_class=fake_QCombo),
                "QLineEdit": MockedModule(mock_class=fake_QLineEdit),
                "Figure": MockedModule(mock_class=fake_Figure),
                "Slider": MockedModule(mock_class=fake_Slider),
            },
        )
        # Mess around with import mechanism _inside_ PyChopGui so GUI libs not really imported
        with patch("builtins.__import__", cls.mock_modules.import_func):
            from mantidqtinterfaces.PyChop import PyChopGui

            cls.window = PyChopGui.PyChopGui()
            cls.window.eiPlots.isChecked = mock.MagicMock(return_value=False)
            cls.mock_modules.matplotlib.__version__ = "2.1.0"

    def test_hyspec(self):
        # Tests that Hyspec routines are only called when the instrument is Hyspec
        with patch.object(self.window, "setS2") as setS2:
            self.window.setInstrument("MAPS")
            self.window.calc_callback()
            setS2.assert_not_called()
            self.window.setInstrument("HYSPEC")
            self.window.widgets["EiEdit"]["Edit"].setText("50")
            self.window.setEi()
            self.window.calc_callback()
            setS2.assert_called()
        # Test that the value of S2 is set correctly
        with patch.object(self.window.widgets["S2Edit"]["Edit"], "text") as S2txt:
            S2txt.return_value = "55"
            self.window.setS2()
            assert self.window.hyspecS2 == 55
            # Valid values are from -150 to +150
            S2txt.return_value = "155"
            with self.assertRaises(ValueError):
                self.window.setS2()

    def test_plot_flux_ei(self):
        # Tests that Hyspec routines are only called when the instrument is Hyspec
        with patch.object(self.window, "plot_flux_ei") as plot_flux_ei:
            self.window.widgets["EiEdit"]["Edit"].text = mock.MagicMock(return_value="")
            self.window.calc_callback()
            plot_flux_ei.assert_not_called()
            self.window.widgets["EiEdit"]["Edit"].text = mock.MagicMock(return_value=120)
            self.window.calc_callback()
            plot_flux_ei.assert_called()

    def test_independent_phases(self):
        # Test that if we have N independent phases, we get N text boxes to input them
        # There are no instruments which currently has more than 1 independent phases
        # So we just use a modified version of LET
        let_choppers = self.window.instruments["LET"].chopper_system
        let_choppers.isPhaseIndependent = [1, 2]
        let_choppers.defaultPhase = [5, 2200]
        let_choppers.phaseNames = ["Chopper 2 delay", "Chopper 3 delay"]
        self.window.setInstrument("LET")
        # 2 calls per chopper to insert a label and an edit box each
        assert self.window.leftPanel.insertWidget.call_count == 4
        # All `QLabel`s are the same mock object
        calls = self.mock_modules.QLabel().setText.call_args_list
        assert calls[-2] == mock.call("Chopper 2 delay")
        assert calls[-1] == mock.call("Chopper 3 delay")

    def test_update_on_Ei_press_enter(self):
        # Test that when we press "enter" in the Ei box, that plots are generated
        # if the "eiPlots" option is selected
        with patch.object(self.window, "calc_callback") as calc:
            self.window.eiPlots.isChecked = mock.MagicMock(return_value=True)
            self.window.setInstrument("ARCS")
            self.window.widgets["EiEdit"]["Edit"].setText("50")
            self.window.setEi()
            calc.assert_called_once()

    def test_merlin_specials(self):
        # Tests Merlin special routines are called for Merlin
        self.window.setInstrument("MERLIN")
        # Checks that the pulse removal phase is hiden from users unless the
        # "instrument_scientist" mode is enabled
        self.window.instSciAct.isChecked = mock.MagicMock(return_value=False)
        self.window.widgets["Chopper0Phase"]["Edit"].hide.assert_called()
        # Merlin can run with either the "G" chopper in RRM-mode
        # or the "S" chopper without RRM
        self.window.widgets["MultiRepCheck"].setEnabled.reset_mock()
        self.window.setChopper("G")
        self.window.widgets["MultiRepCheck"].setEnabled.assert_called_with(True)
        with patch.object(self.window, "_hide_phases") as hide_phases:
            self.window.setChopper("S")
            self.window.widgets["MultiRepCheck"].setEnabled.assert_called_with(False)
            hide_phases.assert_called()
        # Now check that with inst sci mode on, user can change the pulse removal phase
        self.window.widgets["Chopper0Phase"]["Edit"].show.reset_mock()
        self.window.instSciAct.isChecked = mock.MagicMock(return_value=True)
        self.window.setChopper("G")
        self.window.widgets["Chopper0Phase"]["Edit"].show.assert_called()

    def test_pychop_numerics(self):
        # Tests all instruments resolution and flux values against reference
        instruments = ["ARCS", "CNCS", "HYSPEC", "LET", "MAPS", "MARI", "MERLIN", "SEQUOIA"]
        choppers = ["ARCS-100-1.5-AST", "High Flux", "OnlyOne", "High Flux", "S", "S", "G", "SEQ-100-2.0-AST"]
        freqs = [[300], [300, 60], [180], [240, 120], [400, 50], [400], [400], [300]]
        eis = [120, 3.7, 45, 3.7, 120, 80, 120, 120]
        ref_res = [
            10.278744237772832,
            0.13188102618129077,
            3.6751279831313703,
            0.08079912729715726,
            4.9611687063450995,
            2.6049587487601764,
            6.8755979524827255,
            5.396705255853653,
        ]
        ref_flux = [
            2055.562054927915,
            128986.24972543867,
            0.014779264739956933,
            45438.33797146135,
            24196.496233770937,
            5747.118187298609,
            22287.647098883135,
            4063.3113893387676,
        ]
        for inst, ch, frq, ei, res0, flux0 in zip(instruments, choppers, freqs, eis, ref_res, ref_flux):
            res, flux = Instrument.calculate(inst, ch, frq, ei, 0)
            np.testing.assert_allclose(res[0], res0, rtol=1e-4, atol=0)
            np.testing.assert_allclose(flux, flux0, rtol=1e-4, atol=0)

    def test_erange(self):
        # Tests MARI raises if Ei outside range [0, 180meV] with G chopper only ('S' chopper ok up to 1000meV)
        obj = Instrument("MARI", "S", 400)
        obj.setEi(500)
        obj.setChopper("G", 400)
        with self.assertRaises(ValueError):
            obj.setEi(500)
        obj.setEi(180)

    def test_phase_offset(self):
        # Tests calcChopTimes applies new phase offset parameter
        instpars = [[9.3, 9.995], [1, 2], None, [950, 10.0], [64, 10.0], [250, 290.0], [1, 1], 2.5, 1.925, [50, 1], 0, 0.9, [0]]
        phaseoffset = None
        Eis_none, _, _, _, _ = MulpyRep.calcChopTimes(50.0, [50.0, 400.0], instpars, [17000], phaseoffset)
        self.assertAlmostEqual(Eis_none[5], 10.71281984, places=7)
        self.assertAlmostEqual(Eis_none[6], 7.70630740, places=7)
        self.assertAlmostEqual(Eis_none[7], 5.80834507, places=7)
        phaseoffset = 4500
        Eis_offset, _, _, _, _ = MulpyRep.calcChopTimes(50.0, [50.0, 400.0], instpars, [17000], phaseoffset)
        self.assertAlmostEqual(Eis_offset[5], 2.48991457, places=7)
        self.assertAlmostEqual(Eis_offset[6], 2.10994937, places=7)
        self.assertAlmostEqual(Eis_offset[7], 1.81075983, places=7)


if __name__ == "__main__":
    unittest.main()
