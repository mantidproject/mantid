
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
and fit as spectra 0, 1 and 2 respectively.

There are three optional input properties: *FirstGoodData* and *LastGoodData* define the fitting range.
When left blank, *FirstGoodData* is set to the value stored in the input workspace and *LastGoodData*
is set to the last available bin. The optional property *Frequency* allows the user to select an
initial value for :math:`\omega`. If this property is not supplied, the algortihm takes this
value from the *sample_magn_field* log multiplied by :math:`2\pi\cdot g_\mu`, where :math:`g_\mu` is
the muon gyromagnetic ratio (0.01355 MHz/G).

Usage
-----

.. include:: ../usagedata-note.txt

**Example - CalMuonDetectorPhases**

.. testcode:: CalMuonDetectorPhasesExample

   # Load four spectra from a muon nexus file
   ws = Load(Filename='MUSR00022725.nxs', SpectrumMin=1, SpectrumMax=4)
   # Calibrate the phases and amplituds
   detectorTable, fittingResults = CalMuonDetectorPhases(InputWorkspace='ws', LastGoodData=4)

   # Print the result
   print "Detector 1 has phase %f and amplitude %f" % (detectorTable.cell(0,2), detectorTable.cell(0,1))
   print "Detector 2 has phase %f and amplitude %f" % (detectorTable.cell(1,2), detectorTable.cell(1,1))
   print "Detector 3 has phase %f and amplitude %f" % (detectorTable.cell(2,2), detectorTable.cell(2,1))
   print "Detector 4 has phase %f and amplitude %f" % (detectorTable.cell(3,2), detectorTable.cell(3,1))

Output:

.. testoutput:: CalMuonDetectorPhasesExample

  Detector 1 has phase 0.673861 and amplitude 0.133419
  Detector 2 has phase 0.452013 and amplitude 0.134742
  Detector 3 has phase 0.269103 and amplitude 0.149764
  Detector 4 has phase 0.140418 and amplitude 0.153004

.. categories::

.. sourcelink::
    :filename: CalMuonDetectorPhases
