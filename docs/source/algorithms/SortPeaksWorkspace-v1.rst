.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Sort a peaks workspace by a single column. Sorting of that
PeaksWorkspace by that column can either happen in an ascending or
descending fashion. The algorithm can either be used to generate a new
OutputWorkspace, which is sorted as requested, or to perform an in-place
sort of the InputWorkspace.

Usage
-----

**Example - sort a peaks workspace by column name:**  

.. testcode:: ExSortPeaksWorkspaceSimple

    peaks_ws = LoadIsawPeaks(Filename='TOPAZ_1204.peaks')

    #sort the table by column k
    peaks_ws = SortPeaksWorkspace(peaks_ws, ColumnNameToSortBy='k')
    print("Column k in ascending order: " + str(peaks_ws.column('k')))

    #the algorithm can also sort in descending order
    peaks_ws = SortPeaksWorkspace(peaks_ws, ColumnNameToSortBy='k', SortAscending=False)
    print("Column k in descending order: " + str(peaks_ws.column('k')))

Output:

.. testoutput:: ExSortPeaksWorkspaceSimple

   Column k in ascending order: [-3.0, -2.0, -2.0, -2.0, -2.0, -2.0, -2.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -0.0, -0.0, -0.0, 0.0, -0.0, -0.0, 0.0, -0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 2.0, 4.0, 5.0, 5.0]
   Column k in descending order: [5.0, 5.0, 4.0, 2.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, -0.0, -0.0, -0.0, 0.0, -0.0, -0.0, 0.0, -0.0, 0.0, 0.0, 0.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -2.0, -2.0, -2.0, -2.0, -2.0, -2.0, -3.0]

.. categories::

.. sourcelink::
