# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

from mantid.api import AlgorithmFactory, AlgorithmManager, PythonAlgorithm

from mantid.kernel import Direction, SetValueWhenProperty

import unittest


class SetValueWhenPropertyTest(unittest.TestCase):
    class Algorithm1(PythonAlgorithm):
        def name(self):
            return "Algorithm1"

        def category(self):
            return "Test"

        def PyInit(self):
            self.declareProperty("A", defaultValue="1.0", direction=Direction.Input)
            self.declareProperty("B", defaultValue="2.0", direction=Direction.Input)
            self.declareProperty("C", defaultValue="3.0", direction=Direction.Input)
            self.declareProperty("D", defaultValue="4.0", direction=Direction.Output)

            def criterion(currentValue, watchedValue):
                print(f"current: {currentValue}, watched: {watchedValue}")
                return str(40.0) if float(watchedValue) > 1.0 else str(currentValue)

            self.setPropertySettings("D", SetValueWhenProperty("A", criterion))

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
            self.declareProperty("A", defaultValue="10.0", direction=Direction.Input)
            self.declareProperty("B", defaultValue="2.0", direction=Direction.Input)
            self.declareProperty("C", defaultValue="3.0", direction=Direction.Input)
            self.declareProperty("D", defaultValue="4.0", direction=Direction.Output)

            def criterion(currentValue, watchedValue):
                print(f"current: {currentValue}, watched: {watchedValue}")
                return str(40.0) if float(watchedValue) > 1.0 else str(currentValue)

            self.setPropertySettings("D", SetValueWhenProperty("A", criterion))

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
            self.declareProperty("A", defaultValue="10.0", direction=Direction.Input)
            self.declareProperty("B", defaultValue="2.0", direction=Direction.Input)
            self.declareProperty("C", defaultValue="3.0", direction=Direction.Input)
            self.declareProperty("D", defaultValue="4.0", direction=Direction.Output)

            def criterion(currentValue, watchedValue):
                print(f"current: {currentValue}, watched: {watchedValue}")
                return str(40.0) if float(watchedValue) > 1.0 else str(currentValue)

            # Set up a circular dependency.
            self.setPropertySettings("D", SetValueWhenProperty("D", criterion))

        def validateInputs(self):
            return {}

        def PyExec(self):
            pass

    @classmethod
    def setUpClass(cls):
        # Register test algorithms with Mantid
        AlgorithmFactory.subscribe(SetValueWhenPropertyTest.Algorithm1)
        AlgorithmFactory.subscribe(SetValueWhenPropertyTest.Algorithm2)
        AlgorithmFactory.subscribe(SetValueWhenPropertyTest.Algorithm3)

    @classmethod
    def tearDownClass(cls):
        # Unregister test algorithms
        AlgorithmFactory.unsubscribe("Algorithm1", 1)
        AlgorithmFactory.unsubscribe("Algorithm2", 1)
        AlgorithmFactory.unsubscribe("Algorithm3", 1)

    def test_construction(self):
        def criterion(currentValue: str, watchedValue: str) -> str:
            if int(watchedValue) == 10.0:
                return 5.0
            return currentValue

        p = SetValueWhenProperty("A", criterion)
        self.assertIsNotNone(p)

    def test_criterion_satisfied(self):
        # criterion => dependent properties set at init
        alg2 = AlgorithmManager.create("Algorithm2")
        alg2.initialize()
        assert alg2.getProperty("D").settings._isConditionChanged(alg2, "A")
        dependentProperty = alg2.getProperty("D")
        assert dependentProperty.value == "4.0"

        dependentProperty.settings._applyChanges(alg2, dependentProperty.name)
        #  Change criterion is satisfied: property value should have been modified.
        assert alg2.getPropertyValue("D") == "40.0"

    def test_criterion_not_satisfied(self):
        # criterion => dependent properties set at init
        alg1 = AlgorithmManager.create("Algorithm1")
        alg1.initialize()
        assert alg1.getProperty("D").settings._isConditionChanged(alg1, "A")

        dependentProperty = alg1.getProperty("D")
        assert dependentProperty.value == "4.0"

        dependentProperty.settings._applyChanges(alg1, dependentProperty.name)
        # Change criterion is not satisfied: property value should NOT have been modified.
        assert dependentProperty.value == "4.0"

    def test_not_automatically_applied(self):
        # criterion satisfied => dependent properties are not set by algorithm code!
        alg1 = AlgorithmManager.create("Algorithm1")
        alg1.initialize()
        assert alg1.getPropertyValue("D") == "4.0"

        alg1.setProperty("A", "10.0")
        assert alg1.getPropertyValue("D") == "4.0"

    def test_dependsOn(self):
        # dependent properties list dependencies
        alg1 = AlgorithmManager.create("Algorithm1")
        alg1.initialize()

        dependentProperty = alg1.getProperty("D")
        assert list(dependentProperty.settings.dependsOn(dependentProperty.name)) == ["A"]

    #  ------------ Failure cases ------------------

    def test_default_construction_raises_error(self):
        try:
            SetValueWhenProperty()
            self.fail("Expected default constructor to raise an error")
        except Exception as e:
            # boost.python.ArgumentError are not catchable
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
