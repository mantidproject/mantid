.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

|Instrument view of grouping using ReadFromFile with
ShowUnselected=True| All of the detectors in each group are given the a
value equal to that of the group number. Unselected detectors are given
a value of 0 if ShowUnselected is true.

The instrumentView is the best way to visualize the grouping using the
"show Instrument" context menu option on the output workspace.

.. |Instrument view of grouping using ReadFromFile with ShowUnselected=True| image:: /images/ReadFromFile-Grouping.png


Usage
-----

.. include:: ../usagedata-note.txt

**Example - Read 9 groups for INES instrument:**

.. testcode:: ExReadGroupsFromFileSimple

   # Create workspace with INES instrument in it
   ws1 = Load("INES_Definition.xml")

   # Run algorithm
   ws2 = ReadGroupsFromFile( ws1, "INES_example.cal")

   # Print the value of selected sprectra. Each corresponds to the group of the corresponding detector.
   for i in range(9):
       print(" ".join(str(ws2.readY(16 * i + j)) for j in range(0, 20, 5)))

Output:

.. testoutput:: ExReadGroupsFromFileSimple

   [1.] [1.] [1.] [1.]
   [2.] [2.] [2.] [2.]
   [3.] [3.] [3.] [3.]
   [4.] [4.] [4.] [4.]
   [5.] [5.] [5.] [5.]
   [6.] [6.] [6.] [6.]
   [7.] [7.] [7.] [7.]
   [8.] [8.] [8.] [8.]
   [9.] [9.] [9.] [9.]


.. categories::

.. sourcelink::
