.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm is used to calculate the asymmetry for a Muon workspace.
The asymmetry is given by:

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

.. note::
   This algorithm does not perform any grouping. The grouping must be
   done using :ref:`algm-MuonGroupDetectors` or ``AutoGroup`` option
   of :ref:`algm-LoadMuonNexus`.

.. [1] See :ref:`algm-AlphaCalc`

Usage
-----

**Example - Calculating asymmetry:**

.. testcode:: ExSimple

   y = [1,2,3] + [3,6,9]
   x = [1,2,3,4] * 2
   e = [1,1,1] * 2
   input = CreateWorkspace(x, y, e, NSpec=2)

   asymmetry = AsymmetryCalc(input, Alpha=0.5)

   print 'Asymmetry:', ', '.join('{:.3f}'.format(y) for y in asymmetry.readY(0))
   print 'Errors:', ', '.join('{:.3f}'.format(e) for e in asymmetry.readE(0))

Output:

.. testoutput:: ExSimple

   Asymmetry: -0.200, -0.200, -0.200
   Errors: 0.540, 0.382, 0.312

.. categories::
