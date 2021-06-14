.. _03_validating_input:

================
Validating Input
================

By default there is no validation performed for input properties so any
default value will be accepted by the algorithm.

Property validation can be added using the ``validator`` keyword within
the ``declareProperty`` method. The validation is performed by an object
and due to the restrictions placed on us by ``C++`` we must know the type
and pick the appropriate class for the job.

For the basic python types (``int``, ``float``, ``string``) the following
validators are defined:

* :ref:`mantid.kernel.IntBoundedValidator` - Restricts an integer to be
  bounded by either lower, upper or both limits
* :ref:`mantid.kernel.IntMandatoryValidator` - Requires a value for the
  int property
* :ref:`mantid.kernel.IntListValidator` - The value must be one of a given
  list of integers
* :ref:`mantid.kernel.FloatBoundedValidator` - Restricts a float to be
  bounded by by either lower, upper or both limits
* :ref:`mantid.kernel.FloatMandatoryValidator` - Requires a value for
  the float property
* :ref:`mantid.kernel.StringMandatoryValidator` - Requires a value for
  the string property
* :ref:`mantid.kernel.StringListValidator` - The value must be one of a
  given list of strings

To use a validator, create it with the appropriate code as discussed above
and pass it as the validator argument of ``declareProperty`` e.g.

.. code-block:: python

    def PyInit(self):
    # Force the value to be positive or zero
    self.declareProperty("Parameter", -1.0, FloatBoundedValidator(lower=0))

    # Require the user to provide a non-empty string as input
    self.declareProperty("Prefix", "", StringMandatoryValidator())

    # Require the property to have one of the listed values
    self.declareProperty("ProcessOption","Full",
                         StringListValidator(["Full", "QuickEstimate"]))

Validation can also be done when executing the algorithm by raising an
exception if the value is invalid.

It should be preferred to validate in ``PyInit`` if possible as:

#. The execution section can assume the values are valid.
#. The GUI can flag up errors sooner.
