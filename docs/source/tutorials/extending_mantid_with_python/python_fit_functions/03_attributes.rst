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
