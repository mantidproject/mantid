# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""Test suite for the PyChop package
"""
import unittest
from unittest import mock
from unittest.mock import patch
import builtins
import warnings
import numpy as np

from PyChop import PyChop2


class PyChop2Tests(unittest.TestCase):

    # Tests the Fermi chopper instruments
    def test_pychop_fermi(self):
        instnames = ['maps', 'mari', 'merlin']
        res = []
        flux = []
        for inc, instname in enumerate(instnames):
            chopobj = PyChop2(instname)
            # Code should give an error if the chopper settings and Ei have
            # not been set.
            self.assertRaises(ValueError, chopobj.getResolution)
            chopobj.setChopper('s', 200)
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
            rr, ff = PyChop2.calculate(instname, 's', 200, 18, 0)
            self.assertAlmostEqual(rr[0], res[inc][0], places=7)
            self.assertAlmostEqual(ff, flux[inc], places=7)

    # Tests the different variants of LET
    def test_pychop_let(self):
        variants = ['High flux', 'Intermediate', 'High resolution']
        res = []
        flux = []
        for inc, variant in enumerate(variants):
            chopobj = PyChop2('LET', variant)
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
            rr, ff = PyChop2.calculate('LET', variant, 200, 18, 0)
            self.assertAlmostEqual(rr[0], res[inc][0], places=7)
            self.assertAlmostEqual(ff, flux[inc], places=7)

    def test_pychop_invalid_ei(self):
        chopobj = PyChop2('MARI', 'G', 400.)
        chopobj.setEi(120)
        with warnings.catch_warnings(record=True) as w:
            res = chopobj.getResolution(130.)
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


class MockImports():
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
        self.loaded_from = {}     # Stores mocks of "from" syntax
        if replace:
            for replacement_mock in replace:
                if replacement_mock not in self.include:
                    self.include.append(replacement_mock)

    def _check_dot_names(self, name, ref_list):
        for ref_name in ref_list:
            level = ref_name.count('.') + 1
            requested = '.'.join(name.split('.')[:level])
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
        root_name = name.split('.')[0]
        save = False
        if replacement is not None:
            rv = self.replace[replacement]
        elif root_name in self.loaded_modules:
            rv = self.loaded_modules[root_name]
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
        root_name = name.split('.')[0]
        self.loaded_modules[root_name] = mock_object
        if fromlist:
            for mods in fromlist:
                self.loaded_from[mods] = mock_object

    def import_func(self, name, globals=None, locals=None, fromlist=(), level=0):
        prf = f'name:{name}, from:{fromlist}, level:{level}'
        if self.include:
            if self.is_module_included(name):
                return self.get_mock(name, fromlist)
            else:
                return self.real_import(name, globals, locals, fromlist, level)
        else:
            return MockedModule()

    def __getattr__(self, module_name):
        if module_name in self.loaded_modules:
            return self.loaded_modules[module_name]
        if module_name in self.loaded_from:
            return getattr(self.loaded_from[module_name], module_name)


class PyChopGuiTests(unittest.TestCase):
    # Tests GUI routines

    @classmethod
    def setUpClass(cls):
        class fake_QMainWindow():
            def __init__(self, *args, **kwargs):
                self.menuBar = mock.MagicMock()
                self.setCentralWidget = mock.MagicMock()
                self.setWindowTitle = mock.MagicMock()
            def setWindowFlags(self, *args, **kwargs):            # noqa: E306
                pass
            def show(self):                                       # noqa: E306
                pass
        class fake_QCombo(mock.MagicMock):                        # noqa: E306
            def __init__(self, *args, **kwargs):
                super().__init__(*args, **kwargs)
                self.clear()
            def clear(self):                                      # noqa: E306
                self.items = []
                self.currentIndex = 0
            def addItem(self, item):                              # noqa: E306
                self.items.append(item)
            def currentText(self):                                # noqa: E306
                return self.items[self.currentIndex]
            def count(self):                                      # noqa: E306
                return len(self.items)
            def itemText(self, idx):                              # noqa: E306
                return self.items[idx]
            def setCurrentIndex(self, idx):                       # noqa: E306
                self.currentIndex = idx
            def __getattr__(self, attribute):                     # noqa: E306
                if attribute not in self.__dict__:
                    self.__dict__[attribute] = mock.MagicMock()
                return self.__dict__[attribute]
        class fake_Line(mock.MagicMock):                          # noqa: E306
            def __init__(self, parent, *args, **kwargs):
                super().__init__(*args, **kwargs)
                self.parent = parent
            def set_label(self, label):                           # noqa: E306
                parent.legends[self] = label
        class fake_Axes(mock.MagicMock):                          # noqa: E306
            def __init__(self, *args, **kwargs):
                super().__init__(*args, **kwargs)
                self.legends = {}
            def plot(self, *args, **kwargs):                      # noqa: E306
                self.lines.append(fake_Line(self))
                return self.lines[-1],
            def get_legend_handles_labels(self):                  # noqa: E306
                labels = [self.legends[line] for line in self.lines]
                return self.lines, labels
        class fake_Figure(mock.MagicMock):                        # noqa: E306
            def add_subplot(self, *args, **kwargs):
                return fake_Axes()
        class fake_Slider(mock.MagicMock):                        # noqa: E306
            def __init__(self, parent, label, valmin, valmax, **kwargs):
                super().__init__(parent, label, valmin, valmax, **kwargs)
                self.parent, self.label, self.valmin, self.valmax = parent, label, valmin, valmax
                self.val = kwargs.pop('valinit', 0.5)
                self.valtext = mock.MagicMock()
                self.on_changed = mock.MagicMock()
        cls.mock_modules = MockImports(include=['qtpy', 'matplotlib', 'mantidqt', 'mantid.plots'],
                                       replace={'QMainWindow':fake_QMainWindow,
                                                'QComboBox':MockedModule(mock_class=fake_QCombo),
                                                'Figure':MockedModule(mock_class=fake_Figure),
                                                'Slider':MockedModule(mock_class=fake_Slider)})
        # Mess around with import mechanism _inside_ PyChopGui so GUI libs not really imported
        with patch('builtins.__import__', cls.mock_modules.import_func):
            from PyChop import PyChopGui
            cls.window = PyChopGui.PyChopGui()
            cls.window.eiPlots.isChecked = mock.MagicMock(return_value=False)
            cls.mock_modules.matplotlib.__version__ = '2.1.0'

    def test_hyspec(self):
        # Tests that Hyspec routines are only called when the instrument is Hyspec
        with patch.object(self.window, 'setS2') as setS2:
            self.window.setInstrument('MAPS')
            self.window.calc_callback()
            setS2.assert_not_called()
            self.window.setInstrument('HYSPEC')
            self.window.calc_callback()
            setS2.assert_called()

    def test_plot_flux_ei(self):
        # Tests that Hyspec routines are only called when the instrument is Hyspec
        with patch.object(self.window, 'plot_flux_ei') as plot_flux_ei:
            self.window.widgets['EiEdit']['Edit'].text = mock.MagicMock(return_value='')
            self.window.calc_callback()
            plot_flux_ei.assert_not_called()
            self.window.widgets['EiEdit']['Edit'].text = mock.MagicMock(return_value=120)
            self.window.calc_callback()
            plot_flux_ei.assert_called()


if __name__ == "__main__":
    unittest.main()
