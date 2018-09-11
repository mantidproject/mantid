.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

LoadTBl is loads ascii files in TBL format into a
tableworkspace. The Format for TBL files is as follows:

First line : Comma-separated Column headings
Subsequent lines : The data you wish to be displayed in those columns.

Example
-------
An example of a valid TBL format would be as follows::

    Run,Angle,Qmin,Qmax,Energy,Group, Options
    14455,1.5,0.333,1.44,1.4,1,
    14456,1.5,1.4,3.522,1.6,1,ProcessingInstructions="1:2"
    14457,1.5,0.83,2.44,1.2,2,



This file would produce the following TableWorkspace:

+------+-----+-----+-----+------+------+----------------------------+
|Run   |Angle|QMin |QMax |Energy|Group |Options                     |
+======+=====+=====+=====+======+======+============================+
|14455 |1.5  |0.333|1.44 |1.4   |1     |                            |
+------+-----+-----+-----+------+------+----------------------------+
|14456 |1.5  |0.14 |3.522|1.6   |1     |ProcessingInstructions="1:2"|
+------+-----+-----+-----+------+------+----------------------------+
|14457 |1.5  |0.83 |2.44 |1.2   |2     |                            |
+------+-----+-----+-----+------+------+----------------------------+


The first line of the TBL file will define your column headings. As you can see in the table above
the number of commas on each subsequent line must be equal to the number of column headings minus one.
If you wish for a cell in the table to be blank you must still provide the surrounding commas in place
of data.

.. categories::

.. sourcelink::
