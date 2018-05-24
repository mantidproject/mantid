.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Determines which peaks intersect a defined box region in either QLab,
QSample or HKL space. Similar to :ref:`algm-PeaksOnSurface`.

Usage
-----

**Example - Peaks inside and outside of a defined region**

.. testcode:: PeaksInRegionExample

   # Load an MDEventWorkspace (QLab) containing some SC diffration peaks
   mdew = Load("TOPAZ_3680_5_sec_MDEW.nxs")
   # Find some peaks. These are all unindexed so will have HKL = [0,0,0]
   peaks = FindPeaksMD(InputWorkspace=mdew, MaxPeaks=1)
   
   # Find peaks in region when the Peak sits in the centre of a box
   in_box_table = PeaksInRegion(peaks, CoordinateFrame='HKL', PeakRadius=0.1, Extents=[-1.0,1.0,-1.0,1.0,-1.0,1.0], CheckPeakExtents=True)
   print("{{'Distance': {Distance}, 'PeakIndex': {PeakIndex}, 'Intersecting': {Intersecting}}}".format(**in_box_table.row(0)))
   
   # Find peaks in region when the peak is just outside the box (by radius)
   just_outside_box_table = PeaksInRegion(peaks, CoordinateFrame='HKL', PeakRadius=0.999, Extents=[1.0,2.0,-1.0,1.0,-1.0,1.0], CheckPeakExtents=True)
   print("{{'Distance': {Distance}, 'PeakIndex': {PeakIndex}, 'Intersecting': {Intersecting}}}".format(**just_outside_box_table.row(0)))
   
   # Find peaks in region when the peak is just inside the box (by radius)
   just_intersecting_box_table = PeaksInRegion(peaks, CoordinateFrame='HKL', PeakRadius=1.00, Extents=[1.0,2.0,-1.0,1.0,-1.0,1.0], CheckPeakExtents=True)
   print("{{'Distance': {Distance}, 'PeakIndex': {PeakIndex}, 'Intersecting': {Intersecting}}}".format(**just_intersecting_box_table.row(0)))
   
Output:

.. testoutput:: PeaksInRegionExample

   {'Distance': 0.0, 'PeakIndex': 0, 'Intersecting': True}
   {'Distance': -1.0, 'PeakIndex': 0, 'Intersecting': False}
   {'Distance': -1.0, 'PeakIndex': 0, 'Intersecting': True}

.. categories::

.. sourcelink::
