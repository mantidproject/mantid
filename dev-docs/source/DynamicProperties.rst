.. _DynamicProperties:

=========================
Dynamic dialog properties
=========================

.. contents::
  :local:

Overview
--------

From either C++ or Python, the ``IPropertySettings``-derived classes ``SetValueWhenProperty`` and ``SetDefaultWhenProperty`` can be used to allow a GUI dialog to set the value of a property based on the value of another upstream property.  For example, when used with the generic *algorithm dialog* in workbench (implemented via the ``GenericDialog`` class in C++) this allows pre-populating a propery's value based on the value of another property.

Unconditionally setting a property's value
------------------------------------------

``SetValueWhenProperty`` allows *unconditionally* setting a property's value when the value of an upstream property changes.
This ``IPropertySettings`` variant should be used sparingly, as these dynamic values will *overwrite* any value that may have already been entered by the user -- this is not usually acceptable behavior for a user-interface, except in very special cases.

``SetValueWhenProperty`` requires a callback of the form: ``Callable[[str, str], str]`` (from Python), or ``std::function<std::string(std::string, std::string)>`` (from C++).  The meaning of the signature is as follows:  ``<new value> = callback(<current value>, <watched value>)``.  Here the ``<current value>`` is the string value of the property which will be modified, and the ``<watched value>`` is the value of the *upstream* property.  In case no modification is desired, the *existing* string value of the current property should simply be returned.

The following is an example script declaring a simple ``PythonAlgorithm`` which uses ``SetValueWhenProperty`` to dynamically modify several properties values.  The clause which will raise an exception is provided so that you can trigger an example of the exception behavior, and see how it differs from other exception cases in the GUI dialog.

.. code-block:: python

    from mantid.api import AlgorithmFactory, AlgorithmManager, PythonAlgorithm

    from mantid.kernel import (
        Direction,
        Property, PropertyManager,
        SetValueWhenProperty,
        SetDefaultWhenProperty
    )

    class DynamicProperties(PythonAlgorithm):
        def name(self):
            return "DynamicProperties"

        def category(self):
            return "Test"

        def criterion_method(self, currentValue: str, watchedValue: str) -> str:
            """
            Callback function signatures are different for each type of `IPropertySettings`.
            For `SetValueWhenProperty` the callback signature is:
               `Callable[[str, str], str]`:
              The return value is a new string value for the current property, which might be
              the same as its old value.
            For any `IPropertySettings` the callback may be either a function or a bound method.
            """
            print(f"current: {currentValue}, watched: {watchedValue}")
            if float(watchedValue) > 3.0:
                # This clause will raise an exception, which will show up in the logs,
                #   but NOT in the dialog panel's validation indicators.
                #   In this case, the current property's value will not actually be changed.
                currentValue = 25.54 # NOT a string
            elif float(watchedValue) > 2.0:
                currentValue = str(40.0)
            elif float(watchedValue) > 1.0:
                currentValue = str(50.0)
            return currentValue



        def PyInit(self):
            self.declareProperty("A", defaultValue="1.0", direction=Direction.Input)
            self.declareProperty("B", defaultValue="2.0", direction=Direction.Input)
            self.declareProperty("C", defaultValue="3.0", direction=Direction.Input)
            self.declareProperty("D", defaultValue="4.0", direction=Direction.InOut)

            self.setPropertySettings("D", SetValueWhenProperty("A", self.criterion_method))
            self.setPropertySettings("C", SetValueWhenProperty("A", self.criterion_method))

        def validateInputs(self):
            return {}

        def PyExec(self):
            print(
                f"At execution: watched-property value: {self.getProperty('A').value}:\n"
                f"    dependent-property value 'C': {self.getProperty('C').value}\n"
                f"    dependent-property value 'D': {self.getProperty('D').value}\n"
            )

    AlgorithmFactory.subscribe(DynamicProperties)


Emulating a dynamic-default value
---------------------------------

``SetDefaultWhenProperty`` allows *conditionally* setting a property's value when the value of an upstream property changes.
This ``IPropertySettings`` variant is designed to only allow the modification of a value that has not already been entered by the user.
It is designed to provide the effect of a dynamic *default* value for the property, which depends on the value of an upstream property.

``SetDefaultWhenProperty`` requires a callback of the form: ``Callable[[PropertyManager, Property, Property], bool]`` (from Python), or ``std::function<bool(const IPropertyManager *, Property *, Property *)>`` (from C++).  The meaning of the signature is as follows:  ``<flag: value was modified> = callback(<algorithm>, <current property>, <watched property>)``.  Here the ``<current property>`` is the property which will be modified, and the ``<watched property>`` is the *upstream* property.  This callback should return ``true`` if the property has actually been modified.

The following is an example script declaring a simple ``PythonAlgorithm`` which uses ``SetDefaultWhenProperty`` to emulate properties with dynamic-default values.  As previously, the clause which will raise an exception is provided so that you can trigger an example of the exception behavior.

.. code-block:: python

    from mantid.api import AlgorithmFactory, AlgorithmManager, PythonAlgorithm

    from mantid.kernel import (
        Direction,
        Property, PropertyManager,
        SetValueWhenProperty,
        SetDefaultWhenProperty
    )

    class DynamicProperties(PythonAlgorithm):
        def name(self):
            return "DynamicProperties"

        def category(self):
            return "Test"

        def PyInit(self):
            self.declareProperty("A", defaultValue=1.0, direction=Direction.Input)
            self.declareProperty("B", defaultValue=2.0, direction=Direction.Input)
            self.declareProperty("C", defaultValue=3.0, direction=Direction.Input)
            self.declareProperty("D", defaultValue=4.0, direction=Direction.InOut)

            def criterion(algo: PropertyManager, currentProperty: Property, watchedProperty: Property) -> bool:
                """
                Callback function signatures are different for each type of `IPropertySettings`.
                For `SetDefaultWhenProperty` the callback signature is:
                    `Callable[[IPropertyManager, Property, Property], bool]`.
                  The return value indicates whether or not the current property has been changed.
                For any `IPropertySettings` the callback may be either a function or a bound method.
                """
                print(f"current: {currentProperty.value}, watched: {watchedProperty.value}")
                if watchedProperty.value > 3.0:
                    # This clause will raise an exception, which will show up in the logs,
                    #   but NOT in the dialog panel's validation indicators.
                    #   In this case, the current property's value will not actually be changed.
                    currentProperty.value = "not-a-float"
                    return True
                elif watchedProperty.value > 2.0:
                    currentProperty.value = 40.0
                    return True
                elif watchedProperty.value > 1.0:
                    currentProperty.value = 50.0
                    return True
                return False

            self.setPropertySettings("D", SetDefaultWhenProperty("A", criterion))
            self.setPropertySettings("C", SetDefaultWhenProperty("A", criterion))

        def validateInputs(self):
            return {}

        def PyExec(self):
            print(
                f"At execution: watched-property value: {self.getProperty('A').value}:\n"
                f"    dependent-property value 'C': {self.getProperty('C').value}\n"
                f"    dependent-property value 'D': {self.getProperty('D').value}\n"
            )

    AlgorithmFactory.subscribe(DynamicProperties)


Fine points: validation and exceptions
--------------------------------------

When a property's value is set inside of any ``IPropertySettings``-derived class it does make use of the propery's *validator*, assuming that it has one.  However, responding to any validation error in this case will result in an exception throw, which will be wrapped in a logged *warning* message, and the dependent property's value will not be modified.  It will be noted that this behavior is distinct from what happens when a validation error is encountered when the dialog triggers algorithm execution in the normal sequence of GUI actions.

``SetValueWhenProperty`` and ``SetDefaultWhenProperty`` each require the specification of a *callback* function, which will be used to calculate the dependent property's new value.  Any problem with the specification of this callback (e.g. argument compatibility differing from what is expected, or return-value type differing from what is expected), or any exception raised during the execution of this callback will also be trapped and presented as a logged *warning* message.
