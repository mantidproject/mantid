.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm is responsible for normalising data via a given incident
beam parameter. This parameter, IncidentBeamNormalisation, is controlled
from the reduction property manager. It can have the values *None*,
*ByCurrent* or *ByMonitor*. For SNS, monitor workspaces need to be
passed. Parameters in italics are controlled by the
`instrument parameter file (IPF) <http://www.mantidproject.org/InstrumentParameterFile>`_
unless provided to the algorithm via a property manager. The mappings are given
below.

+-----------------------+-----------------+
| Parameter             | IPF Mapping     |
+=======================+=================+
| MonitorIntRangeLow    | norm-mon1-min   |
+-----------------------+-----------------+
| MonitorIntRangeHigh   | norm-mon1-max   |
+-----------------------+-----------------+

Parameters in italics with dashed perimeters are only controllable by
the IPF name given. All underlined parameters are fixed via other input
methods. If normalisation is performed, a sample log called
DirectInelasticReductionNormalisedBy is added to the resulting workspace
with the normalisation procedure used.

Workflow
########

.. figure:: /images/DgsPreprocessDataWorkflow.png
   :alt: DgsPreprocessDataWorkflow.png

Usage
-----

.. warning::

    This algorithm is not really intented for use at the command line, but is used
    within :ref:`DgsReduction <algm-DgsReduction>`.

.. categories::

.. sourcelink::
