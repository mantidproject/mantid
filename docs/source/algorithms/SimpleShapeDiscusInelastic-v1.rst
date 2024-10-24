
.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Sets up a sample shape and optionally a container shape, along with the required material properties, and runs
the :ref:`DiscusMultipleScatteringCorrection <algm-DiscusMultipleScatteringCorrection-v1>` algorithm. This algorithm merely
serves as a simpler interface to define the shape & material of the sample\\container without having
to resort to the more complex :ref:`SetSample <algm-SetSample-v1>` algorithm.
The computational part is all taken care of by :ref:`DiscusMultipleScatteringCorrection <algm-DiscusMultipleScatteringCorrection-v1>`. Please see that
documentation for more details.
Currently the shape geometries supported are:

* Flat Plate
* Cylinder
* Annulus

Below are the properties applicable to each specific shape:

FlatPlate
#########

- *Height*
- *Width*
- *Thickness*
- *Angle*
- *Front*
- *Back*

Cylinder
########

- *Height*
- *SampleRadius*
- *CanRadius* stands for the outer radius of the annular container, inner radius of which is the same as the radius of the sample.

Annulus
#######

- *Height*
- *CanInnerRadius*
- *SampleInnerRadius*
- *SampleOuterRadius*
- *CanOuterRadius*

.. categories::

.. sourcelink::
