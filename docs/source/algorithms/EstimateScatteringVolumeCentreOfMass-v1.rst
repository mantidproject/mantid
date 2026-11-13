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

Sample Orientation
##################

The centre of mass is given in the lab frame. The sample shape is turned by the run's goniometer, for example from :ref:`algm-SetGoniometer`, unless it already carries that rotation, for example from :ref:`algm-CopySample`. Unlike the absorption corrections, a sample in an environment is turned too, as the environment plays no part here. The gauge volume is in the lab frame.

.. categories::

.. sourcelink::
