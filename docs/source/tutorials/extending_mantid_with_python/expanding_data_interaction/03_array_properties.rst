.. _03_array_properties:

================
Array Properties
================

So far we have dealt with properties that contain a single item: int, float,
string or workspace. In order to provide multiple items as input for a
single property, i.e. a list of values, we must use a different type of
property called an ``ArrayProperty``.

While a Python list is capable of storing items of any type in the one list,
the same is not true in ``C++``. For this reason it is only possible for an
array property to store a single type. This choice must be made when the
property is declared. The options are:

* :ref:`mantid.kernel.FloatArrayProperty` - Stores a list of floats/doubles.
* :ref:`mantid.kernel.IntArrayProperty` - Stores a list of integers.
* :ref:`mantid.kernel.StringArrayProperty` - Stores a list of strings.

The simplest use of each is where the default value is empty:

.. code-block:: python

    def PyInit(self):
        self.declareProperty(FloatArrayProperty("Floats",
                                                direction=Direction.Input),
                                                doc='Input doubles')
        self.declareProperty(IntArrayProperty("Ints",
                                                direction=Direction.Input),
                                                doc='Input integers')
        self.declareProperty(StringArrayProperty("Strings",
                                                direction=Direction.Input),
                                                doc='Input strings')

Default values for the list can be specified as a python ``list``, a numpy
array or a comma-separated string using the ``values`` keyword.

.. code-block:: python

    def PyInit(self):
        self.declareProperty(FloatArrayProperty(name="PythonListInput",
                                                values=[1.2,4.5,6.7],
                                                direction=Direction.Input))

Validation
==========

As with the other property types there is an option to supply a validator
using the ``validator`` keyword. The available validators are:

* :ref:`mantid.kernel.FloatArrayLengthValidator`,
  :ref:`mantid.kernel.IntArrayLengthValidator`,
  :ref:`mantid.kernel.StringArrayLengthValidator` - Verify that the array is
  of a given length.
* :ref:`mantid.kernel.FloatArrayBoundedValidator`,
  :ref:`mantid.kernel.IntArrayBoundedValidator` - Verify that each value in
  the array is within the given bounds.

The prefix, *Float*, *Int*, *String*, must match the property type:

.. code-block:: python

    def PyInit(self):
        length_validator = FloatArrayLengthValidator(5)
        self.declareProperty(FloatArrayProperty("Floats",
                                                validator=length_validator,
                                                direction=Direction.Input))
