# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)
import six

import unittest
import json
from mantid.api import AlgorithmID, AlgorithmManager, FrameworkManagerImpl
from testhelpers import run_algorithm

class AlgorithmTest(unittest.TestCase):

    _load = None

    def setUp(self):
        FrameworkManagerImpl.Instance()
        if self._load is None:
            self.__class__._load = AlgorithmManager.createUnmanaged('Load')
            self._load.initialize()

    def test_alg_attrs_are_correct(self):
        self.assertEqual('Load', self._load.name())
        self.assertEqual(1, self._load.version())
        self.assertEqual('DataHandling', self._load.category())
        self.assertEqual(1, len(self._load.categories()))
        self.assertEqual('DataHandling', self._load.categories()[0])
        self.assertEqual('', self._load.helpURL())
        self.assertEqual(["LoadNexus", "LoadRaw", "LoadBBY"], self._load.seeAlso())

    def test_get_unknown_property_raises_error(self):
        self.assertRaises(RuntimeError, self._load.getProperty, "NotAProperty")

    def test_alg_set_valid_prop_succeeds(self):
        self._load.setProperty('Filename', 'LOQ48127.raw')

    def test_alg_set_invalid_prop_raises_error(self):
        alg = AlgorithmManager.createUnmanaged('Load')
        alg.initialize()
        args = ('Filename', 'nonexistent.txt')
        self.assertRaises(ValueError, alg.setProperty, *args)

    def test_cannot_execute_with_invalid_properties(self):
        alg = AlgorithmManager.createUnmanaged('Load')
        alg.initialize()
        self.assertRaises(RuntimeError, alg.execute)

    def test_execute_succeeds_with_valid_props(self):
        data = [1.5,2.5,3.5]
        alg = run_algorithm('CreateWorkspace',DataX=data,DataY=data,NSpec=1,UnitX='Wavelength',child=True)
        self.assertEqual(alg.isExecuted(), True)
        self.assertEqual(alg.isRunning(), False)
        self.assertEqual(alg.getProperty('NSpec').value, 1)
        self.assertEqual(type(alg.getProperty('NSpec').value), int)
        self.assertEqual(alg.getProperty('NSpec').name, 'NSpec')
        ws = alg.getProperty('OutputWorkspace').value
        self.assertTrue(ws.getMemorySize() > 0.0 )

        as_str = str(alg)
        self.assertEqual(as_str, '{"name":"CreateWorkspace","properties":{"DataX":[1.5,2.5,3.5],"DataY":[1.5,2.5,3.5],'
                          '"OutputWorkspace":"UNUSED_NAME_FOR_CHILD","UnitX":"Wavelength"},"version":1}\n')

    def test_execute_succeeds_with_unicode_props(self):
        data = [1.5,2.5,3.5]
        kwargs = {'child':True}
        unitx = 'Wavelength'
        if six.PY2:
            # force a property value to be unicode to assert conversion happens correctly
            # this is only an issue in python2
            kwargs[unicode('UnitX')] = unicode(unitx)
        else:
            kwargs['UnitX'] = unitx


        alg = run_algorithm('CreateWorkspace',DataX=data,DataY=data,NSpec=1,**kwargs)
        self.assertEqual(alg.isExecuted(), True)
        self.assertEqual(alg.isRunning(), False)
        self.assertEqual(alg.getProperty('NSpec').value, 1)
        self.assertEqual(type(alg.getProperty('NSpec').value), int)
        self.assertEqual(alg.getProperty('NSpec').name, 'NSpec')
        ws = alg.getProperty('OutputWorkspace').value
        self.assertTrue(ws.getMemorySize() > 0.0 )

        as_str = str(alg)
        self.assertEqual(as_str, '{"name":"CreateWorkspace","properties":{"DataX":[1.5,2.5,3.5],"DataY":[1.5,2.5,3.5],'
                          '"OutputWorkspace":"UNUSED_NAME_FOR_CHILD","UnitX":"Wavelength"},"version":1}\n')

    def test_execute_succeeds_with_unicode_kwargs(self):
        props = json.loads('{"DryRun":true}') # this is always unicode
        alg = run_algorithm('Segfault', **props)

    def test_getAlgorithmID_returns_AlgorithmID_object(self):
        alg = AlgorithmManager.createUnmanaged('Load')
        self.assertEqual(AlgorithmID, type(alg.getAlgorithmID()))

    def test_AlgorithmID_compares_by_value(self):
        alg = AlgorithmManager.createUnmanaged('Load')
        id = alg.getAlgorithmID()
        self.assertEqual(id, id) # equals itself
        alg2 = AlgorithmManager.createUnmanaged('Load')
        id2 = alg2.getAlgorithmID()
        self.assertNotEqual(id2, id)

    def test_cancel_does_nothing_to_executed_algorithm(self):
        data = [1.0]
        alg = run_algorithm('CreateWorkspace',DataX=data,DataY=data,NSpec=1,UnitX='Wavelength',child=True)
        self.assertEqual(alg.isExecuted(), True)
        self.assertEqual(alg.isRunning(), False)
        alg.cancel()
        self.assertEqual(alg.isExecuted(), True)
        self.assertEqual(alg.isRunning(), False)

    def test_createChildAlgorithm_creates_new_algorithm_that_is_set_as_child(self):
        parent_alg = AlgorithmManager.createUnmanaged('Load')
        child_alg = parent_alg.createChildAlgorithm('Rebin')

        self.assertTrue(child_alg.isChild())

    def test_createChildAlgorithm_respects_keyword_arguments(self):
        parent_alg = AlgorithmManager.createUnmanaged('Load')
        try:
            child_alg = parent_alg.createChildAlgorithm(name='Rebin',version=1,startProgress=0.5,
                                                        endProgress=0.9,enableLogging=True)
        except Exception as exc:
            self.fail("Expected createChildAlgorithm not to throw but it did: %s" % (str(exc)))

        # Unknown keyword
        self.assertRaises(Exception, parent_alg.createChildAlgorithm, name='Rebin',version=1,startProgress=0.5,
                          endProgress=0.9,enableLogging=True, unknownKW=1)

if __name__ == '__main__':
    unittest.main()
