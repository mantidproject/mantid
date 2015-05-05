.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Creates a TableWorkspace with offsets of h,k,and l from an integer along
with bank and run number.

The maximum of these offsets is also included.

Histograms, scatterplots, etc. of this data can be used to detect
problems.

Usage
-----

**Example - Run LoadprofResolution for both TableWorkspace and workspace with MUSR Instrument**

.. include:: ../usagedata-note.txt

.. testcode:: ExShowHKLOffsetsSimple

   #
   # Load Peaks found in SXD23767.raw 
   #
   Load(Filename='SXD23767.peaks',OutputWorkspace='peaks_qLab')

   #Set orientated lattice
   ublist = [-0.06452276,  0.2478114,  -0.23742194, 0.29161678, -0.00914316, -0.12523779, 0.06958942, -0.1802702,  -0.14649001]
   SetUB('peaks_qLab',UB=ublist)

   # Run Algorithm 
   table = ShowPeakHKLOffsets('peaks_qLab')

   #Print part of first four rows
   for i in [0,1,2,3]:
      row = table.row(i)
      #print row
      print "{H offset from int: %.3f, K offset from int: %.3f, L offset from int: %.3f }" % ( row["H offset from int"],  row["K offset from int"], row["L offset from int"])


Output:

.. testoutput:: ExShowHKLOffsetsSimple

   {H offset from int: -0.003, K offset from int: 0.012, L offset from int: 0.063 }
   {H offset from int: -0.220, K offset from int: 0.431, L offset from int: -0.193 }
   {H offset from int: -0.105, K offset from int: -0.066, L offset from int: -0.232 }
   {H offset from int: 0.174, K offset from int: 0.095, L offset from int: 0.440 }


.. categories::
