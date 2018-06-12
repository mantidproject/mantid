.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------
This algorithm changes the sign of Q and the label of the workspace convention.
This should only be used on MD workspaces that contain only Q or HKL dimensions.  If there are other dimensions,
user should still run this algorithm (so that the convention flag is changed), but then run TransformMD with -1 
for the non-Q dimensions.


Usage
-----

**Example - an example of running ChangeQConvention with PointGroup option.**

.. testcode:: ExChangeQConventionOption

    mdws = LoadMD('MAPS_MDEW.nxs')
    dim = mdws.getXDimension()
    print("X range of Q  {} {}".format(dim.getX(0), dim.getX(1)))
    ChangeQConvention(mdws)
    mdws = mtd['mdws']
    dim = mdws.getXDimension()
    print("X range of Q after ChangeQConvention  {} {}".format(dim.getX(0), dim.getX(1)))

Output:

.. testoutput:: ExChangeQConventionOption

    X range of Q  0.0 10.0
    X range of Q after ChangeQConvention  -10.0 0.0


.. categories::

.. sourcelink::
