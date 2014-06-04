.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

PoldiPeakSearch is a peak-finding routine for POLDI auto-correlation
data. The algorithm is implemented according to the original data
analysis software and their results match closely.

The algorithm performs the following steps:

#. Map each point of the spectrum :math:`y`, except the first and the
   last to the sum of its value and its neighbor's values:

| ``   ``\ :math:`y'_i = y_{i-1} + y_{i} + y_{i+1}`
| `` The new spectrum ``\ :math:`y'`\ `` contains ``\ :math:`n-2`\ `` points when ``\ :math:`y`\ `` contains ``\ :math:`n`\ ``.``

#. Identify peak positions in :math:`y'`, which is done with a recursive
   algorithm, consisting of these steps:

   #. Find the position of the maximum, :math:`i_{max}` in the list,
      store in peak-list.
   #. Split the list in two parts,
      :math:`[i_{0} + \Delta, i_{max} - \Delta)` and
      :math:`(i_{max} + \Delta, i_{n} - \Delta]`,

where :math:`\Delta` is the mininum number of data points between two
peaks.

#. 

   #. If ranges are valid, perform algorithm on each of the sublists,
      append returned lists to peak-list.
   #. Return peak-list.

#. Sort list by value in descending order, keep the first
   :math:`N_{max}` items of the list.
#. Map peak positions from :math:`y'` back to :math:`y`
#. Perform background and fluctuation estimation:

   #. Extract all points from :math:`y` (except the first and the last)
      that are further than :math:`\Delta` elements away from any peak
      position
   #. Calculate median of these points as location estimate
      (:math:`\bar{b}`)
   #. Calculate Sn as scale estimator (:math:`\bar(s)`)

#. Estimate peak intensity as :math:`y_{i}`
#. If a minimum peak height is set, discard all peaks that are smaller
   than this, if not, discard all peaks that are lower than
   :math:`3\cdot\bar{s} + \bar{b}`

The peaks are stored in a new table workspace.

.. categories::
