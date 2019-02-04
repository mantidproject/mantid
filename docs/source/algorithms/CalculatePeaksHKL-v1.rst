.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Uses the :ref:`UB matrix <Lattice>` on the sample to calculate the Miller indices for all
peaks in the peaks workspace. Unlike :ref:`algm-IndexPeaks` this
algorithm does not perform any mandatory optimization. This algorithm
does not round the Miller indices to the nearest integer.

Alternatives
------------

:ref:`algm-IndexPeaks`

Usage
-----

**Example - Calculate peak HKL values based on UB matrix**

.. testcode:: CalculatePeaksHKLExample

  mdew = Load("TOPAZ_3680_5_sec_MDEW.nxs")
  # Find some peaks. These are all unindexed so will have HKL = [0,0,0]
  peaks = FindPeaksMD(InputWorkspace=mdew, MaxPeaks=1)

  # Set the UB to unity
  SetUB(peaks, UB=[1,0,0,0,1,0,0,0,1])

  # Run the algorithm
  indexed = CalculatePeaksHKL(PeaksWorkspace=peaks, OverWrite=True)

  print("Number of Indexed Peaks:  {}".format(indexed))

Output:

.. testoutput:: CalculatePeaksHKLExample

  Number of Indexed Peaks:  1

.. categories::

.. sourcelink::
