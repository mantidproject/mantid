.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Intended for use on data from engineering beamlines, this algorithm
creates a shape object for use as the 'gauge volume' (i.e. the portion
of the sample that is visible to the detectors in a given run) of a
larger sample in the :ref:`algm-AbsorptionCorrection`
algorithm. The sample shape will also need to be defined using, e.g.,
the :ref:`algm-CreateSampleShape` algorithm. Shapes are
defined using XML descriptions that can be found
`here <HowToDefineGeometricShape>`__.

Internally, this works by attaching the XML string (after validating it)
to a property called "GaugeVolume" on the workspace's `Run <Run>`__
object.

.. categories::
