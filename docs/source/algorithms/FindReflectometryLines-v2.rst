
.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm finds the line position in a line detector reflectometry dataset. It integrates *InputWorkspace* over the :math:`X` axis and fits the sum of a Gaussian and a linear background to the integral. The center of the Gaussian, a fractional workspace index to the *InputWorkspace*, is returned as a single valued workspace in *OutputWorkspace* and as a plain number in the *LineCentre* output property.

The integration region can be constrained by *RangeLower* and *RangeUpper* which restrict the :math:`X` range. Further, the peak fitting can be controlled by *StartWorkspaceIndex* and *EndWorkspaceIndex* which limit the workspace index range.

If the peak fitting fails, the algorithm logs a warning and returns the workspace index of the integral maximum.

Usage
-----

.. include:: ../usagedata-note.txt

**Example - FindReflectometryLines**

.. testcode:: FindReflectometryLinesExample

   LoadISISNexus('POLREF00004699.nxs', OutputWorkspace='ws', LoadMonitors='Exclude')
   # 'ws' is a group workspace -> OutputWorkspace is a group as well
   FindReflectometryLines('ws', OutputWorkspace='pos')
   # Access individual outputs
   pos1 = mtd['pos_1']
   pos2 = mtd['pos_2']
   print('Line position in the 1st workspace: {:.3}'.format(pos1.readY(0)[0]))
   print('Line position in the 2nd workspace: {:.3}'.format(pos2.readY(0)[0]))
   # With single workspaces one can use the named tuple output
   out = FindReflectometryLines('ws_1')
   print('Line position from the returned tuple: {:.3}'.format(out.LineCentre))

Output:

.. testoutput:: FindReflectometryLinesExample

   Line position in the 1st workspace: 25.7
   Line position in the 2nd workspace: 25.7
   Line position from the returned tuple: 25.7

.. categories::

.. sourcelink::
