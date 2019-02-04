.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Determine whether a peak intersects a surface. Similar to
:ref:`algm-PeaksInRegion`. The vertexes of the surface must be
provided. The vertexes must be provided in clockwise ordering starting
at the lower left.

Usage
------

**Example - Peaks on and off a finite surface**

.. testcode:: PeaksOnSurfaceExample

   # Load an MDEventWorkspace (QLab) containing some SC diffration peaks
   mdew = Load("TOPAZ_3680_5_sec_MDEW.nxs")
   # Find some peaks. These are all unindexed so will have HKL = [0,0,0]
   peaks = FindPeaksMD(InputWorkspace=mdew, MaxPeaks=1)
   
   # Peak is on the plane
   out_of_plane_offset = 0
   tbl = PeaksOnSurface(InputWorkspace=peaks, PeakRadius=1.0, CoordinateFrame='HKL',
                        Vertex1=[1.0, -1.0, out_of_plane_offset], Vertex2=[-1.0,-1.0,out_of_plane_offset],
                        Vertex3=[-1.0, 1.0,out_of_plane_offset], Vertex4=[1.0, 1.0,out_of_plane_offset])
   print("{{'Distance': {Distance}, 'PeakIndex': {PeakIndex}, 'Intersecting': {Intersecting}}}".format(**tbl.row(0)))
   
   # Peak is off the plane, and does not intesect it
   out_of_plane_offset = 1.000
   tbl = PeaksOnSurface(InputWorkspace=peaks, PeakRadius=0.999,  CoordinateFrame='HKL',
                        Vertex1=[1.0, -1.0, out_of_plane_offset], Vertex2=[-1.0,-1.0,out_of_plane_offset],
                        Vertex3=[-1.0, 1.0,out_of_plane_offset], Vertex4=[1.0, 1.0,out_of_plane_offset])
   print("{{'Distance': {Distance}, 'PeakIndex': {PeakIndex}, 'Intersecting': {Intersecting}}}".format(**tbl.row(0)))
   
   # Peak is off the plane, but does intesect it when radius is made larger
   tbl = PeaksOnSurface(InputWorkspace=peaks, PeakRadius=1.000,  CoordinateFrame='HKL',
                        Vertex1=[1.0, -1.0, out_of_plane_offset], Vertex2=[-1.0,-1.0,out_of_plane_offset],
                        Vertex3=[-1.0, 1.0,out_of_plane_offset], Vertex4=[1.0, 1.0,out_of_plane_offset])
   print("{{'Distance': {Distance}, 'PeakIndex': {PeakIndex}, 'Intersecting': {Intersecting}}}".format(**tbl.row(0)))
   
Output:

.. testoutput:: PeaksOnSurfaceExample

   {'Distance': 0.0, 'PeakIndex': 0, 'Intersecting': True}
   {'Distance': -1.0, 'PeakIndex': 0, 'Intersecting': False}
   {'Distance': -1.0, 'PeakIndex': 0, 'Intersecting': True}

.. categories::

.. sourcelink::
