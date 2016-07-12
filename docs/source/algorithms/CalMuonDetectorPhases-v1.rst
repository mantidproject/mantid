
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
detector-dependent.

Before the spectra are fitted, :math:`\omega` is determined by grouping the detectors,
calculating the asymmetry and fitting this to get the frequency. This value of :math:`\omega`
is then treated as a fixed constant when fitting the spectra to the function above.

The algorithm outputs a table workspace containing the spectrum number, the asymmetry and the phase.
This table is intended to be used as the input
*PhaseTable* to :ref:`PhaseQuad <algm-PhaseQuad>`. 
Usually for muon instruments, each spectrum will correspond to one detector (spectrum number = detector ID).

In addition, the fitting results are returned
in a workspace group, where each of the items stores the original data (after removing the
exponential decay), the data simulated with the fitting function and the difference between data
and fit as spectra 0, 1 and 2 respectively.

There are five optional input properties: *FirstGoodData* and *LastGoodData* define the fitting range.
When left blank, *FirstGoodData* is set to the value stored in the input workspace and *LastGoodData*
is set to the last available bin. The optional property *Frequency* allows the user to select an
initial value for :math:`\omega` (a starting value for the fit). If this property is not supplied, the 
algorithm takes this value from the *sample_magn_field* log multiplied by :math:`2\pi\cdot g_\mu`, where
:math:`g_\mu` is the muon gyromagnetic ratio (0.01355 MHz/G).
Finally, the optional properties *ForwardSpectra* and *BackwardSpectra* are the sets of spectra in the 
forward and backward groups. If these are not supplied, the algorithm will find the instrument from the
input workspace and use the default grouping for this instrument.

Usage
-----

.. include:: ../usagedata-note.txt

**Example - CalMuonDetectorPhases**

.. testcode:: CalMuonDetectorPhasesExample

   # Load four spectra from a muon nexus file
   ws = Load(Filename='MUSR00022725.nxs', SpectrumMin=1, SpectrumMax=4)
   # Calibrate the phases and amplitudes
   detectorTable, fittingResults = CalMuonDetectorPhases(InputWorkspace='ws', LastGoodData=4, ForwardSpectra="1,2", BackwardSpectra="3,4")

   # Print the result
   for i in range(0,4):
     print "Detector %i has phase %f and amplitude %f" % (detectorTable.cell(i,0), detectorTable.cell(i,2), detectorTable.cell(i,1))

Output:

.. testoutput:: CalMuonDetectorPhasesExample

  Detector 1 has phase 0.620299 and amplitude 0.133113
  Detector 2 has phase 0.399003 and amplitude 0.134679
  Detector 3 has phase 0.214079 and amplitude 0.149431
  Detector 4 has phase 0.086315 and amplitude 0.152870

.. categories::

.. sourcelink::
    :filename: CalMuonDetectorPhases
