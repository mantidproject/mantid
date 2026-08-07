.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Takes a workspace with either just a sample or a sample and gauge volume defined and returns an estimation for the centre of mass of the
illuminated region of sample.

Under the hood this algorithm uses the same basic logic as :ref:`algm-AbsorptionCorrection` to rasterize the sample shape
(or any defined gauge volume). It then calculates the average position of all the raster elements which are inside both shapes.

.. categories::

.. sourcelink::
