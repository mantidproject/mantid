
.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Sets up a sample shape, along with the required material properties, and runs
the :ref:`DiscusMultipleScatteringCorrection <algm-DiscusMultipleScatteringCorrection-v1>` algorithm. This algorithm merely
serves as a simpler interface to define the shape & material of the sample without having
to resort to the more complex :ref:`SetSample <algm-SetSample-v1>` algorithm.
The computational part is all taken care of by :ref:`DiscusMultipleScatteringCorrection <algm-DiscusMultipleScatteringCorrection-v1>`. Please see that
documentation for more details.
Currently the shape geometries supported are:

* Flat Plate
* Cylinder
* Annulus

.. categories::

.. sourcelink::