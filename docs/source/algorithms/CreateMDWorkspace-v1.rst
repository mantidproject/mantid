.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm creates an empty MDEventWorkspace from scratch. The
workspace can have any number of dimensions (up to ~20). Each dimension
must have its name, units, extents specified as comma-spearated string.

The SplitInto parameter determines how splitting of dense boxes will be
performed. For example, if SplitInto=5 and the number of dimensions is
3, then each box will get split into 5x5x5 sub-boxes.

The SplitThreshold parameter determines how many events to keep in a box
before splitting it into sub-boxes. This value can significantly affect
performance/memory use! Too many events per box will mean unnecessary
iteration and a slowdown in general. Too few events per box will waste
memory with the overhead of boxes.

You can create a file-backed MDEventWorkspace by specifying the Filename
and Memory parameters.

Usage
-----

**Example**

.. testcode:: Example

    mdws = CreateMDWorkspace(Dimensions=3, Extents='-10,10,-10,10,-10,10', Names='A,B,C', Units='U,U,U')
    
    print("mdws is a " + mdws.id())
    print("with {0} dimensions:".format(mdws.getNumDims()))
    for i in range (mdws.getNumDims()):
        print(mdws.getDimension(i).name)

Output:

.. testoutput:: Example

    mdws is a MDEventWorkspace<MDLeanEvent,3>
    with 3 dimensions:
    A
    B
    C


.. categories::

.. sourcelink::
