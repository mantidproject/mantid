.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------
This algorithm changes the sign of Q and the label of the workspace convention.


Usage
-----

**Example - an example of running ChangeQConvention with PointGroup option.**

.. testcode:: ExChangeQConventionOption

    mdws = LoadMD('MAPS_MDEW.nxs')
    dim = mdws.getXDimension()
    print "X range of Q ",dim.getX(0),dim.getX(1)
    ChangeQConvention(mdws)
    mdws = mtd['mdws']
    dim = mdws.getXDimension()
    print "X range of Q after ChangeQConvention ",dim.getX(0),dim.getX(1)

Output:

.. testoutput:: ExChangeQConventionOption

    X range of Q  0.0 10.0
    X range of Q after ChangeQConvention  -10.0 0.0


.. categories::

.. sourcelink::
