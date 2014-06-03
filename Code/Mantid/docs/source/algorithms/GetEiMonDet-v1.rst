.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Get incident energy from one monitor and some detectors. If the minimum
distance from the sample to detectors is dmin, one will select detectors
in the range dmin to dmin\*MaximumDistanceFraction. These are grouped
together, appended to a copy of the monitor workspace, then fed to GetEi
algorithm. The output of this algorithm is identical to that of
:ref:`algm-GetEi`.

.. categories::
