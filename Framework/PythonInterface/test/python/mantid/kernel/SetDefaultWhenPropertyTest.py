# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from mantid.api import AlgorithmFactory, AlgorithmManager, PythonAlgorithm

from mantid.kernel import Direction, SetDefaultWhenProperty

import unittest


class SetDefaultWhenPropertyTest(unittest.TestCase):
    class Algorithm1(PythonAlgorithm):
        def name(self):
            return "Algorithm1"

        def category(self):
            return "Test"

        def PyInit(self):
            self.declareProperty("A", defaultValue=1.0, direction=Direction.Input)
            self.declareProperty("B", defaultValue=2.0, direction=Direction.Input)
            self.declareProperty("C", defaultValue=3.0, direction=Direction.Input)
            self.declareProperty("D", defaultValue=4.0, direction=Direction.Output)

            def criterion(algo, currentProp, watchedProp):
                print(f"current: {currentProp.value}, watched: {watchedProp.value}")
                if watchedProp.value > 1.0:
                    currentProp.value = 40.0
                    return True
                return False

            self.setPropertySettings("D", SetDefaultWhenProperty("A", criterion))

        def validateInputs(self):
            return {}

        def PyExec(self):
            pass

    class Algorithm2(PythonAlgorithm):
        def name(self):
            return "Algorithm2"

        def category(self):
            return "Test"

        def PyInit(self):
            self.declareProperty("A", defaultValue=1.1, direction=Direction.Input)
            self.declareProperty("B", defaultValue=2.0, direction=Direction.Input)
            self.declareProperty("C", defaultValue=3.0, direction=Direction.Input)
            self.declareProperty("D", defaultValue=4.0, direction=Direction.Output)

            def criterion(algo, currentProp, watchedProp):
                print(f"current: {currentProp.value}, watched: {watchedProp.value}")
                if watchedProp.value > 2.0:
                    currentProp.value = 50.0
                elif watchedProp.value > 1.0:
                    currentProp.value = 40.0
                    return True
                return False

            self.setPropertySettings("D", SetDefaultWhenProperty("A", criterion))

        def validateInputs(self):
            return {}

        def PyExec(self):
            pass

    class Algorithm3(PythonAlgorithm):
        def name(self):
            return "Algorithm2"

        def category(self):
            return "Test"

        def PyInit(self):
            self.declareProperty("A", defaultValue=10.0, direction=Direction.Input)
            self.declareProperty("B", defaultValue=2.0, direction=Direction.Input)
            self.declareProperty("C", defaultValue=3.0, direction=Direction.Input)
            self.declareProperty("D", defaultValue=4.0, direction=Direction.Output)

            def criterion(algo, currentProp, watchedProp):
                print(f"current: {currentProp.value}, watched: {watchedProp.value}")
                if watchedProp.value > 1.0:
                    currentProp.value = 40.0
                    return True
                return False

            # Set up a circular dependency.
            self.setPropertySettings("D", SetDefaultWhenProperty("D", criterion))

        def validateInputs(self):
            return {}

        def PyExec(self):
            pass

    @classmethod
    def setUpClass(cls):
        # Register test algorithms with Mantid
        AlgorithmFactory.subscribe(SetDefaultWhenPropertyTest.Algorithm1)
        AlgorithmFactory.subscribe(SetDefaultWhenPropertyTest.Algorithm2)
        AlgorithmFactory.subscribe(SetDefaultWhenPropertyTest.Algorithm3)

    @classmethod
    def tearDownClass(cls):
        # Unregister test algorithms
        AlgorithmFactory.unsubscribe("Algorithm1", 1)
        AlgorithmFactory.unsubscribe("Algorithm2", 1)
        AlgorithmFactory.unsubscribe("Algorithm3", 1)

    def test_construction(self):
        def criterion(algo, currentProp, watchedProp):
            print(f"current: {currentProp.value}, watched: {watchedProp.value}")
            if watchedProp.value > 1.0:
                currentProp.value = 40.0
                return True
            return False

        p = SetDefaultWhenProperty("A", criterion)
        self.assertIsNotNone(p)

    def test_criterion_satisfied(self):
        # criterion => dependent properties will be set at dialog initialization:
        #   here we check the status of the separate functions that will be called.
        alg2 = AlgorithmManager.create("Algorithm2")
        alg2.initialize()
        assert alg2.getProperty("D").settings._isConditionChanged(alg2, "A")
        dependentProperty = alg2.getProperty("D")
        assert dependentProperty.value == 4.0

        dependentProperty.settings._applyChanges(alg2, dependentProperty.name)
        #  Change criterion is satisfied: property value should have been modified.
        assert alg2.getProperty("D").value == 40.0

    def test_criterion_not_satisfied(self):
        # criterion => dependent properties set at init
        alg1 = AlgorithmManager.create("Algorithm1")
        alg1.initialize()
        assert alg1.getProperty("D").settings._isConditionChanged(alg1, "A")

        dependentProperty = alg1.getProperty("D")
        assert dependentProperty.value == 4.0

        dependentProperty.settings._applyChanges(alg1, dependentProperty.name)
        # Change criterion is not satisfied: property value should NOT have been modified.
        assert dependentProperty.value == 4.0

    def test_isDynamicDefault_set(self):
        # when a dependent property is changed programmatically,
        #   its `isDynamicDefault` flag will be set

        alg2 = AlgorithmManager.create("Algorithm2")
        alg2.initialize()
        assert alg2.getProperty("D").settings._isConditionChanged(alg2, "A")
        dependentProperty = alg2.getProperty("D")
        assert dependentProperty.value == 4.0
        assert dependentProperty.isDefault
        assert not dependentProperty.isDynamicDefault

        dependentProperty.settings._applyChanges(alg2, dependentProperty.name)
        #  Change criterion is satisfied: property value should have been modified.
        assert dependentProperty.value == 40.0
        assert not dependentProperty.isDefault
        assert dependentProperty.isDynamicDefault

    def test_dynamic_default_values_are_modifiable(self):
        # when a dependent property is changed programmatically,
        #   its `isDynamicDefault` flag will be set, AND it may continue to be modified.

        alg2 = AlgorithmManager.create("Algorithm2")
        alg2.initialize()
        assert alg2.getProperty("D").settings._isConditionChanged(alg2, "A")
        dependentProperty = alg2.getProperty("D")
        assert dependentProperty.value == 4.0
        assert dependentProperty.isDefault
        assert not dependentProperty.isDynamicDefault

        dependentProperty.settings._applyChanges(alg2, dependentProperty.name)
        #  Change criterion is satisfied: property value should have been modified.
        assert dependentProperty.value == 40.0
        assert not dependentProperty.isDefault
        assert dependentProperty.isDynamicDefault

        alg2.getProperty("A").value = 3.0
        assert alg2.getProperty("D").settings._isConditionChanged(alg2, "A")
        dependentProperty.settings._applyChanges(alg2, dependentProperty.name)
        #  Change criterion is satisfied: property value should have been modified AGAIN.
        assert dependentProperty.value == 50.0
        assert not dependentProperty.isDefault
        assert dependentProperty.isDynamicDefault  # AND so on, ...

    def test_non_dynamic_default_values_are_not_modified(self):
        # when a dependent property has been changed by the user its value will NOT be changed programmatically
        alg2 = AlgorithmManager.create("Algorithm2")
        alg2.initialize()
        assert alg2.getProperty("D").settings._isConditionChanged(alg2, "A")
        dependentProperty = alg2.getProperty("D")
        assert dependentProperty.value == 4.0
        assert dependentProperty.isDefault

        dependentProperty.value = 5.0
        assert dependentProperty.value == 5.0
        assert not dependentProperty.isDefault

        dependentProperty.settings._applyChanges(alg2, dependentProperty.name)
        #  Even though change criterion is satisfied: property value should NOT have been modified.
        assert dependentProperty.value == 5.0

    def test_isDynamicDefault_only_set_when_modified(self):
        # when a dependent property has been changed by the user its `isDynamicDefault` flag will NOT be set
        alg2 = AlgorithmManager.create("Algorithm2")
        alg2.initialize()
        assert alg2.getProperty("D").settings._isConditionChanged(alg2, "A")
        dependentProperty = alg2.getProperty("D")
        assert dependentProperty.value == 4.0
        assert dependentProperty.isDefault
        assert not dependentProperty.isDynamicDefault

        dependentProperty.value = 5.0
        assert dependentProperty.value == 5.0
        assert not dependentProperty.isDefault
        assert not dependentProperty.isDynamicDefault

        dependentProperty.settings._applyChanges(alg2, dependentProperty.name)
        #  Even though change criterion is satisfied:
        #    neither property value nor `isDynamicDefault` flag should be modified.
        assert alg2.getProperty("D").value == 5.0
        assert not dependentProperty.isDefault
        assert not dependentProperty.isDynamicDefault

    def test_not_automatically_applied(self):
        # criterion satisfied => dependent properties are not set by algorithm code itself!
        alg1 = AlgorithmManager.create("Algorithm1")
        alg1.initialize()
        assert alg1.getProperty("D").value == 4.0

        alg1.setProperty("A", "10.0")
        assert alg1.getProperty("D").value == 4.0

    def test_dependsOn(self):
        # dependent properties list dependencies
        alg1 = AlgorithmManager.create("Algorithm1")
        alg1.initialize()

        dependentProperty = alg1.getProperty("D")
        assert list(dependentProperty.settings.dependsOn(dependentProperty.name)) == ["A"]

    #  ------------ Failure cases ------------------

    def test_default_construction_raises_error(self):
        try:
            SetDefaultWhenProperty()
            self.fail("Expected default constructor to raise an error")
        except TypeError as e:
            # `boost.python.ArgumentError` are exposed as `TypeError`
            if "Python argument types in" not in str(e):
                raise RuntimeError("Unexpected exception type raised")

    def test_dependsOn_circular(self):
        # dependent properties list dependencies
        alg3 = AlgorithmManager.create("Algorithm3")
        alg3.initialize()

        with self.assertRaises(RuntimeError) as context:
            dependentProperty = alg3.getProperty("D")
            alg3.getProperty("D").settings.dependsOn(dependentProperty.name)
        assert "circular dependency" in str(context.exception)


if __name__ == "__main__":
    unittest.main()
