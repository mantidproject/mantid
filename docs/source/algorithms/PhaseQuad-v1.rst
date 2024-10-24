.. algorithm::

.. summary::

Overview
--------

This algorithm provides a method for combining signals from multiple detector segments to form two output
signals in quadrature phase. It is of particular use when working with muon spin rotation signals measured
using highly segmented detector arrays that are typically found at pulsed muon sources (ISIS has instruments
with up to 600 detector elements). The method allows information from individual detectors to be combined in
a way that makes full use of the dataset.

This algorithm is frequently run as a precursor to making a Rotating Reference Frame transformation of
the dataset using the algorithm :ref:`algm-RRFMuon`. Both algorithms are fully described in the article
by T.M. Riseman and J.H. Brewer [Hyp. Int., 65, (1990), 1107].


.. relatedalgorithms::

.. properties::

Description
-----------

Assuming that the *InputWorkspace* contains measured counts as a function of time,
and *PhaseTable* contains the detector phases and asymmetries, the algorithm returns a workspace
containing two spectra (squashograms) as a function of the same time binning. If there are zero
measured counts for a detector it is ignored in the calculation.
*PhaseTable* is expected to have three columns, corresponding to the detector id, its asymmetry
and its phase.

Usage
-----

.. include:: ../usagedata-note.txt

**Example - Computing squashograms:**

.. testcode:: ExPhaseQuad

   from math import pi
   Load(Filename='MUSR00022725.nxs', OutputWorkspace='MUSR00022725')
   CropWorkspace(InputWorkspace='MUSR00022725', OutputWorkspace='MUSR00022725', XMin=0, EndWorkspaceIndex=63)

   # Create a PhaseTable with some arbitrary detector information
   tab = CreateEmptyTableWorkspace()
   tab.addColumn('int', 'DetID')
   tab.addColumn('double', 'Phase')
   tab.addColumn('double', 'Asym')
   for i in range(0,32):
       phi = 2*pi*i/32.
       tab.addRow([1, 0.2, phi])
   for i in range(0,32):
       phi = 2*pi*i/32.
       tab.addRow([1, 0.2, phi])
   ows = PhaseQuad(InputWorkspace='MUSR00022725', PhaseTable='tab')
   print("Output workspace has {} histograms".format(ows.getNumberHistograms()))

Output:

.. testoutput:: ExPhaseQuad

   Output workspace has 2 histograms

.. categories::

.. sourcelink::
    :filename: PhaseQuadMuon
