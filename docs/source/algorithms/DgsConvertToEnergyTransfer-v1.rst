.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm is responsible for making the conversion from
time-of-flight to energy transfer for direct geometry spectrometers. The
diagram below shows the workflow for the algorithm. The SNS instruments
have a log called EnergyRequest which allows the IncidentEnergyGuess
parameter to be left blank. Also, SNS instruments need to pass a monitor
workspace to :ref:`GetEi <algm-GetEi>`
since they are separate from the sample workspace.
Parameters in italics are controlled by the
`instrument parameter file (IPF) <http://www.mantidproject.org/InstrumentParameterFile>`_
unless provided to the algorithm via a property manager. The mappings are given
below.

+--------------------+------------------+
| Parameter          | IPF Mapping      |
+====================+==================+
| TibTofRangeStart   | bkgd-range-min   |
+--------------------+------------------+
| TibTofRangeEnd     | bkgd-range-max   |
+--------------------+------------------+

Parameters in italics with dashed perimeters are only controllable by
the IPF name given. All underlined parameters are fixed and not
controllable. The EnergyTransferRange parameter takes the canonical
Mantid notation of (start, step, stop). However, it can be left blank
and the following values will be used
:math:`(-0.5E^{Guess}_{i}, 0.01E^{Guess}_{i}, 0.99E^{Guess}_{i})`.

The use of the SofPhiEIsDistribution parameter in the last Rebin call is
used to set the :ref:`Rebin <algm-Rebin>`
algorithm parameter PreserveEvents.

Workflow
########

.. figure:: /images/DgsConvertToEnergyTransferWorkflow.png
   :alt: DgsConvertToEnergyTransferWorkflow.png

Usage
-----

.. warning::

    This algorithm is not really intented for use at the command line, but is used
    within :ref:`DgsReduction <algm-DgsReduction>`.

.. categories::

.. sourcelink::
