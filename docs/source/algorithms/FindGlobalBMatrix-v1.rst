.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

FindGlobalBMatrix refines the lattice parameters (encoded in the B matrix) across PeaksWorkspaces from multiple runs
with a separate U matrix (which encodes the orientation for an identity goniometer matrix) per run. This is useful for
when the goniometer matrix is inaccurate/not well known.

The quantity minimised is the average of the square of the difference in QSample between the observed peak and the peak
at integer HKL (see :ref:`algm-CalculateUMatrix` for details). The average rather than the sum of the squared residuals
is used so as not to penalise the indexing of more peaks as the UB matrix becomes more accurate.

FindGlobalBMatrix adds a different UB to each peak workspace that can be indexed - note the algorithm ensures consistent
indexing across all runs (i.e. axes are not swapped or inverted). All peaks workspaces must have more then 6 peaks in
(a requirement of some child algorithms used).

The algorithm proceeds in this way:

-  Finds an initial UB

   -  If a UB exists on any workspace the initial is the UB that indexes the most peaks.
   -  Otherwise :ref:`FindUBUsingLatticeParameters<algm-FindUBUsingLatticeParameters>` is run on the workspaces with the input lattice parameters until a UB is found.

-  Peaks in all workspaces are indexed consistently with the initial UB

   -  If a workspace has a valid UB it is transformed to preserve consistent indexing.
   -  Otherwise we try to index the peaks using the initial UB and the goniometer matrix on the workspace
   -  If the above doesn't work because e.g. the goniometer matrix is inaccurate, we find a UB using :ref:`FindUBUsingLatticeParameters<algm-FindUBUsingLatticeParameters>` and transform to preserve consistent indexing.

-  Once peaks are indexed the optimal lattice parameters are found that minimise the average of the squared residuals in QSample across all runs.

   -  The U matrix of each run is found for a given set of lattice parameters using :ref:`algm-CalculateUMatrix`.


Useage
-----------

**Example:**

.. code-block:: python


    from mantid.simpleapi import *

    # load empty instrument so can create a peak table
    ws = LoadEmptyInstrument(InstrumentName='SXD', OutputWorkspace='empty_SXD')
    axis = ws.getAxis(0)
    axis.setUnit("TOF")

    # create two peak tables with UB corresponding to different lattice constants, a
    peaks1 = CreatePeaksWorkspace(InstrumentWorkspace=ws, NumberOfPeaks=0)
    UB = np.diag([1.0/3.8, 0.25, 0.1])  # alatt = [3.8, 4, 10]
    SetUB(peaks1, UB=UB)
    peaks2 = CreatePeaksWorkspace(InstrumentWorkspace=ws, NumberOfPeaks=0)
    UB = np.diag([1.0/4.2, 0.25, 0.1])  # alatt = [4.2, 4, 10]
    SetUB(peaks2, UB=UB)
    # Add some peaks
    for h in range(0, 3):
        for k in range(0, 3):
            for peaks in [peaks1, peaks2]:
                pk = peaks.createPeakHKL([h, k, 4])
                peaks.addPeak(pk)

    FindGlobalBMatrix(PeakWorkspaces=[peaks1, peaks2], a=4.1, b=4.2, c=10, alpha=88, beta=88, gamma=89,
                      Tolerance=0.15)

    # show that both workspaces have the average of the two a lattice constants (a=4 Ang)
    print(peaks1.sample().getOrientedLattice())
    # lattice parameters: a = 4.00025 b = 3.98794 c = 9.99608 alpha = 89.9698 beta = 90.0829 gamma = 89.9336

    print(peaks2.sample().getOrientedLattice())
    # lattice parameters: a = 4.00025 b = 3.98794 c = 9.99608 alpha = 89.9698 beta = 90.0829 gamma = 89.9336

.. categories::

.. sourcelink::
