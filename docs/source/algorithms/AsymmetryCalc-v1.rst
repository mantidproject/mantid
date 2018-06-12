.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm is used to calculate the asymmetry for a Muon workspace.
It first groups the input workspace according to the spectra numbers
provided as *ForwardSpectra* and *BackwardSpectra*. If these properties
are not supplied, the algorithm assumes that the first spectrum in the
workspace is the forward group and the second one is the backward
group. It then calculates the asymmetry as:

.. math:: Asymmetry = \frac{F-\alpha B}{F+\alpha B}

, where :math:`F` is the front spectra, :math:`B` is the back spectra
and :math:`\alpha` is the balance parameter [1]_.

The errors in :math:`F-\alpha B` and :math:`F+\alpha B` are calculated
by adding the errors in :math:`F` and :math:`B` in quadrature; any
errors in :math:`\alpha` are ignored. The errors for the asymmetry are
then calculated using the fractional error method with the values for
the errors in :math:`F-\alpha B` and :math:`F+\alpha B`.

The output workspace contains one set of data for the time of flight:
the asymmetry and the asymmetry errors.

.. [1] See :ref:`algm-AlphaCalc`

Usage
-----

**Example - Calculating asymmetry:**

.. testcode:: ExSimple

   y = [1,2,3] + [3,1,12]
   x = [1,2,3,4] * 2
   e = [1,1,1] * 2
   input = CreateWorkspace(x, y, e, NSpec=2)

   asymmetry = AsymmetryCalc(input, Alpha=0.5)

   print('Asymmetry: {}'.format(asymmetry.readY(0)))
   print('Errors: {}'.format(asymmetry.readE(0)))

Output:

.. testoutput:: ExSimple

   Asymmetry: [-0.2         0.6        -0.33333333]
   Errors: [ 0.5396295   0.69971423  0.28688766]

.. categories::

.. sourcelink::
