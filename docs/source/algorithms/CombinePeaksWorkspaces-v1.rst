.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm can be used to combine lists of single crystal peaks,
possibly obtained by different methods, in to a single list (contained
in a :ref:`PeaksWorkspace <PeaksWorkspace>` or
:ref:`LeanElasticPeaksWorkspace <LeanElasticPeaksWorkspace>`).  With
the default options, this will simply append the lists of peaks.  If
CombineMatchingPeaks is selected then an attempt will be made to
identify identical peaks by matching them in Q within the specified
tolerance. The peaks in each workspace are traversed in the order they
are found in the workspace (RHSWorkspace first) and if a match is
found (the search stops at the first match for each RHSWorkspace peak)
then the peak in the LHSWorkspace is retained.

A PeaksWorkspace can be combined with a LeanElasticPeaksWorkspace only
if the LHSWorkspace is the LeanElasticPeaksWorkspace in which case all
peaks are converted into LeanElasticPeak.

.. categories::

.. sourcelink::
