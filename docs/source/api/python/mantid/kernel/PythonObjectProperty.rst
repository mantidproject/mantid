.. _PythonObjectProperty:

=======================
 PythonObjectProperty
=======================

This is a Python binding to the C++ class Mantid::PythonInterface::PythonObjectProperty.
It is meant to hold a generic python object as a property for use in algorithms defined from python.

*bases:* :py:obj:`mantid.kernel.PropertyWithValue`

.. module:`mantid.kernel`

.. autoclass:: mantid.kernel.PythonObjectProperty
    :members:
    :undoc-members:
    :inherited-members:

Example Usage
------------------

The following example shows how to use this property type, both with and without a ``PythonObjectTypeValidator``.

.. code-block:: python

    from mantid.kernel import PythonObjectProperty, PythonObjectTypeValidator
    from mantid.api import PythonAlgorithm

    class FakeClass:
        pass

    class FakeAlgorithm(PythonAlgorithm):
        def PyInit(self):
            self.declareProperty(PythonObjectProperty("UnvalidatedPyObject"))
            self.declareProperty(PythonObjectProperty("ValidatedPyObject", None, PythonObjectTypeValidator(FakeClass)))

        def PyExec(self):
            pass

    value1 = {"key1": "value one", "key2": ["one", "two", "three"]}
    value2 = FakeClass()

    # run the algorithm
    fake = FakeAlgorithm()
    fake.initialize()
    fake.setProperty("UnvalidatedPyObject", value1)
    fake.setProperty("ValidatedPyObject", value2)
    fake.execute()

Notes
-----

When string values are required, this will first attempt to write them using ``json.dumps``.
If that does not work then serialization will recursively build a json-like object, including replacing custom classes with `__dict__` property.
Note that for very complicated objects, this can result in enormous string outputs.  It is recommended to stick to
simpler python types, or classes made from simple python types.  This will not work on objects that use python tuples for dictionary keys.
If the recursive serialization fails, the string value will be set to ``"<unrepresentable object>"``.
