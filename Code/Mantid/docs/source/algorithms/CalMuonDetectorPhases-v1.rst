
.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Calculates detector asymmetries and phases from a reference dataset. The algorithm fits each of
the spectra in the input workspace to:

.. math:: f_i(t) = A_i \sin\left(\omega t + \phi_i\right)

where :math:`\omega` is shared across spectra and :math:`A_i` and :math:`\phi_i` are
detector-dependent. The algorithm outputs a table workspace containing the detector ID (i.e. the
spectrum index), the asymmetry and the phase. This table is intended to be used as the input
*PhaseTable* to :ref:`PhaseQuad <algm-PhaseQuad>`. In addition, the fitting results are returned
in a workspace group, where each of the items stores the original data (after removing the
exponential decay), the data simulated with the fitting function and the difference between data
and fit as spectrum 0, 1 and 2 respectively.

There are three optional input properties: *FirstGoodData* and *LastGoodData* define the fitting range.
When left blank, *FirstGoodData* is set to the value stored in the input workspace and *LastGoodData*
is set to the last available bin. The optional property *Frequency* allows the user to select an
initial value for :math:`\omega`. If this property is not supplied, the algortihm takes this
value from the *sample_magn_field* log multiplied by :math:`0.01355` MHz/G.

Usage
-----
..  Try not to use files in your examples,
    but if you cannot avoid it then the (small) files must be added to
    autotestdata\UsageData and the following tag unindented
    .. include:: ../usagedata-note.txt

**Example - CalMuonDetectorPhases**

.. testcode:: CalMuonDetectorPhasesExample

   # Create a host workspace
   ws = CreateWorkspace(DataX=range(0,3), DataY=(0,2))
   or
   ws = CreateSampleWorkspace()

   wsOut = CalMuonDetectorPhases()

   # Print the result
   print "The output workspace has %i spectra" % wsOut.getNumberHistograms()

Output:

.. testoutput:: CalMuonDetectorPhasesExample

  The output workspace has ?? spectra

.. categories::

.. sourcelink::

