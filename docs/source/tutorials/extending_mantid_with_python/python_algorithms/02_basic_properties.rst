.. _02_basic_properties:

================
Basic Properties
================

To be of any use our algorithm should be able to accept input and give back output.
This is achieved through declaring named :class:`properties <mantid.kernel.Property>` that are set by the user and/or the algorithm itself.


The inputs/outputs for the algorithm are specified in the ``PyInit`` section using a method called ``declareProperty`` that is defined on the :class:`PythonAlgorithm <mantid.api.Algorithm>` super class.

Basic type properties such as *numbers*, *booleans* and *strings* are declared using ``declareProperty`` and a default value, e.g.

.. code-block:: python

     def PyInit(self):
         self.declareProperty('InputValue', -1)

This will create an ``int`` input property called *InputValue* with a default value of -1.

A property is an input by default.
To change this, use the ``direction`` keyword within ``declareProperty`` call:

.. code-block:: python

     from mantid.kernel import Direction

     ...

     def PyInit(self):
         # input direction specified explicitly
         self.declareProperty('InputValue', -1, direction=Direction.Input)
         # output direction specified explicitly
         self.declareProperty('OutputValue', -1, direction=Direction.Output)

Documentation can be provided using the ``doc`` keyword.
This sets the text in the tooltip on the GUI:

.. code-block:: python

     def PyInit(self):
         self.declareProperty('InputValue', -1, direction=Direction.Input,
                              doc="A input value for this algorithm. Extra details.")


Everything until the first "." of the docstring will appear in the automated GUI as the mouseover help.
The full docstring will appear in the algorithm documentation.
