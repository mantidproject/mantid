.. _03_attributes:

==========
Attributes
==========

An attribute is defined as a fixed value that does not take part in the
fitting process (i.e. not part of parameter space). For example, the number
of iterations of an internal loop would be a good candidate for a function
attribute. Unlike parameters, attributes can have one of the following Python
types: int, float, string, boolean.

Attributes should be declared along with the parameters within the ``init``
function:

.. code-block:: python

    # Remember to choose either IFunction1D or IPeakFunction
    class Example1DFunction(IFunction1D): # or IPeakFunction

        def init(self):
            self.declareParameter("A0", 0.0)
            self.declareParameter("A1", 0.0)

            self.declareAttribute("NLoops", 10)


The value of an attribute does not change throughout the fitting so it is best
to store it locally once after it has been set by the user. A method called
``setAttributeValue`` is defined by the super class and is called
automatically by the framework when a user sets an attribute.

If defined in your class then you can use it to set a python attribute on your object, e.g.

.. code-block:: python

    class Example1DFunction(IFunction1D):

        def init(self):
            self.declareParameter("A0", 0.0)
            self.declareParameter("A1", 0.0)

            self.declareAttribute("NLoops", 10)

        def setAttributeValue(self, name, value):
            if name == "NLoops":
                # Can the be accessed quicker later using self._nloops
                self._nloops = value


.. _attribute_validators:

====================
Attribute Validators
====================

If desired, the possible values that an attribute can take can be restricted through the use
of a Function Attribute Validator. Such a validator must derive from the ``IValidator``
abstract class; a number of these are currently provided in the Mantid Kernel such as:
- List Validator (attribute value must be specified from a provided list of values).
- Bounded Validator (numeric attribute value must be between two provided bounds).
- String contrains validator (string attribute value must contain a provided sub-string/s)

In python, attributes can be declared with a validator using the following syntax:

.. code-block:: python

    from mantid.kernel import StringListValidator, StringContainsValidator, FloatBoundedValidator

    class Example1DFunction(IFunction1D): # or IPeakFunction

        def init(self):
            self.declareParameter("StrAttr", "acceptable", StringListValidator(["acceptable","values"]))
            self.declareAttribute("FloatAtt", 3.0, FloatBoundedValidator(0.0, 5.0))
            self.declareAttribute("StringContainsAtt", "StringContains",
                                                        StringContainsValidator(["Contains"]))


If a string list validator is used for any function called from the ``FitPropertyBrowser``, the attribute
value can be input through the browser using a combo box.
