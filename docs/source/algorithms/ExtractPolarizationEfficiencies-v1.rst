.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm reads polarization correction parameters from the intruments parameters file and creates an efficiency workspace ready to be used with :ref:`algm-PolarizationEfficiencyCor`. It is intended to be used as a child algorithm by the reflectometry
reduction algorithms and not to be used directly.

To work with this algorithm the parameters file must contain the following parameters (all of type `string`):

1. `polarization_correction_method` to specify the method of correction. Possible values are `Fredrikze` and `Wildes`.
2. `efficiency_lambda` is a vector of wavelengths at which the efficiencies are sampled.
3. A set of four parameters for each of the four efficiencies stored as vectors of values at the wavelengths in `efficiency_lambda`. The names of the parameters are different for the two methods of correction: `Pp`, `Ap`, `Rho` and `Alpha` for `Fredrikze` and `P1`, `P2`, `F1` and `F2` for `Wildes`.

The values in the vector parameters must be separated by spaces, for example:

.. code-block:: xml

  <parameter name="efficiency_lambda" type="string">
    <value val="0 1 2 3 4 5" />
  </parameter>

.. categories::

.. sourcelink::




