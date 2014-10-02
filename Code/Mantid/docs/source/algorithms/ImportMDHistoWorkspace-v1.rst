.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm takes a text file (.txt extension) containing two columns
and converts it into an :ref:`MDHistoWorkspace <MDHistoWorkspace>`.

Details
#######

The columns are in the order **signal** then **error**. The file must
only contain two columns, these may be separated by any whitespace
character. The algorithm expects there to be 2\*product(nbins in each
dimension) entries in this file. So if you have set the dimensionality
to be *4,4,4* then you will need to provide 64 rows of data, in 2
columns or 128 floating point entries.

The Names, Units, Extents and NumberOfBins inputs are all linked by the
order they are provided in. For example, if you provide Names *A, B, C*
and Units *U1, U2, U3* then the dimension *A* will have units *U1*.

Signal and Error inputs are read in such that, the first entries in the
file will be entered across the first dimension specified, and the
zeroth index in the other dimensions. The second set of entries will be
entered across the first dimension and the 1st index in the second
dimension, and the zeroth index in the others.

Alternatives
------------

A very similar algorithm to this is
:ref:`CreateMDHistoWorkspace <algm-CreateMDHistoWorkspace>`, which takes it's
input signal and error values from arrays rather than a text file.
Another alternative is to use :ref:`ConvertToMD <algm-ConvertToMD>` which works
on MatrixWorkspaces, and allows log values to be included in the
dimensionality.

Usage
-----

.. include:: ../usagedata-note.txt

**A 3D Example**

.. testcode:: Ex3D

    ws = ImportMDHistoWorkspace('demo_mdhw.txt',Dimensionality='3',Extents='-1,1,-1,1,-1,1',
                                NumberOfBins='2,2,2',Names='A,B,C',Units='A,A,A')
    print "Number of Dimensions =", ws.getNumDims()
    index = (1,0,1)
    print "Signal at "+str(index)+" =", ws.getSignalArray()[index]
    print "Error Squared at "+str(index)+" =", ws.getErrorSquaredArray()[index]

Output:

.. testoutput:: Ex3D

    Number of Dimensions = 3
    Signal at (1, 0, 1) = 6.0
    Error Squared at (1, 0, 1) = 37.21

**A 2D Example**

.. testcode:: Ex2D

    ws = ImportMDHistoWorkspace('demo_mdhw.txt',Dimensionality='2',Extents='-1,1,-1,1',
                                NumberOfBins='4,2',Names='A,B',Units='A,A')
    print "Number of Dimensions =", ws.getNumDims()
    index = (1,2)
    print "Signal at "+str(index)+" =", ws.getSignalArray()[index]
    print "Error Squared at "+str(index)+" =", ws.getErrorSquaredArray()[index]

Output:

.. testoutput:: Ex2D

    Number of Dimensions = 2
    Signal at (1, 2) = 7.0
    Error Squared at (1, 2) = 50.41

.. categories::
