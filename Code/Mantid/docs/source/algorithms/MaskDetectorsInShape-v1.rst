.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Masks detectors that are contained within a user defined 3 dimensional
shape within the instrument.

The algorithm places the user defined geometric shape within the virtual
instrument and masks any detector detectors that in contained within it.
A detector is considered to be contained it its central location point
is contained within the shape.

ChildAlgorithms used
####################

MaskDetectorsInShape runs the following algorithms as child algorithms:

-  :ref:`algm-FindDetectorsInShape` - To determine the
   detectors that are contained in the user defined shape.
-  :ref:`algm-MaskDetectors` - To mask the detectors found.

.. categories::
