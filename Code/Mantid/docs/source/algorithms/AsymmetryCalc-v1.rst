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

.. include:: ../usagedata-note.txt

**Example - Calculating assymetry for a MUSR run:**

.. testcode:: ExMUSR

   # Load some arbitrary MUSR run
   ws = LoadMuonNexus('MUSR0015189.nxs')

   asymmetry = AsymmetryCalc('ws_1', # Use first period only
                             ForwardSpectra=range(33,65),
                             BackwardSpectra=range(1,33),
                             Alpha=1.0)

   print 'No. of spectra in the resulting workspace:', asymmetry.getNumberHistograms()

   output_format = 'For TOF of {:.3f} asymmetry is {:.3f} with error {:.3f}'
   for i in [500, 1000, 1500]:
      print output_format.format(asymmetry.readX(0)[i], asymmetry.readY(0)[i], asymmetry.readE(0)[i])

Output:

.. testoutput:: ExMUSR

   No. of spectra in the resulting workspace: 1
   For TOF of 7.450 asymmetry is -0.045 with error 0.080
   For TOF of 15.450 asymmetry is -0.333 with error 0.609
   For TOF of 23.450 asymmetry is 1.000 with error 1.414

.. categories::
