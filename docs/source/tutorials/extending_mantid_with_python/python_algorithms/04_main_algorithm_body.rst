.. _04_main_algorithm_body:

===================
Main Algorithm Body
===================

After the properties have been created within the ``PyInit`` function it is
time to turn attention to what the algorithm does with its inputs, i.e. what
happens when we execute it.

In order to define what happens when the algorithm is executed we must write
another function called ``PyExec``, whose definition should look like:

.. code-block:: python

    def PyExec(self):
        # Code to run algorithm

Inside ``PyExec`` you can write any python code that is necessary to run
your desired processing. It can of course call Mantid python functions but
you can also run any python code you like, i.e.

.. code-block:: python

    def PyExec(self):
        sum = 0
        for i in range(100):
            sum += 1

Retrieving Input
================

It is most likely that the algorithm has input properties and the values
that have been given by the user will need to be retrieved. This is done
through the ``getProperty`` method, which returns the named property. This
can then be used to query the value, units, defaults etc. See
:ref:`mantid.kernel.Property` for more details. The actual value of the
property is retrieved with ``.value`` and is returned as an appropriate type.

For example, to parameterize the loop above, instead of the hard-coded 100,
we could define an input parameter, ``NIterations``, which will be used to
constrain the loop:

.. code-block:: python

    def PyInit(self):
        self.declareProperty("NIterations", 100,
                             IntBoundedValidator(lower=0),
                             "The number of iterations of the loop to "
                             "perform (default=100)")

    def PyExec(self):
        nloops = self.getProperty("NIterations").value
        sum = 0
        for i in range(nloops):
            sum += 1

The above ``self.getProperty("[NAME]").value`` code is valid regardless of the
type of the property: *float*, *workspace* etc.

Output Properties
=================

Most algorithms will want to do something and return some output to the user.
We do this by using output properties so that they can be used in a generic
manner, without knowing what needs to be returned.

Output properties are declared in a similar manner to input properties,
with the exception that their direction is ``Direction.Output``. As an example,
we could extend the code above to output the final sum like so:

.. code-block:: python

    def PyInit(self):
        self.declareProperty(name="NIterations", defaultValue=100,
                             validator=IntBoundedValidator(lower=0),
                             doc="The number of iterations of the loop "
                                 "to perform (default=100)")
        self.declareProperty(name="SummedValue", defaultValue=0,
                             doc="Outputs the sum of the n iterations",
                             direction = Direction.Output)

    def PyExec(self):
        nloops = self.getProperty("NIterations").value
        sum = 0
        for i in range(nloops):
            sum += 1

        self.setProperty("SummedValue", sum)

The second property, ``SummedValue``, is defined as an output property
and the ``self.setProperty("SummedValue", sum)`` is used as the point to tell
the algorithm that this value is to be used for that property. It will then be
returned as part of the generated function call.