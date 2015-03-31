.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Calculates the sample transmission using the raw data files of the sample and
its background or container by dividing the monitor spectrum of the sample
workspace by that of the container.

If the instrument has both incident and transmission monitors then the incident
monitor is first divided by the transmission monitor.

It is assumed that the name of the incident monitor is *monitor2* and the name
of the transmission monitor is *monitor1*.

Usage
-----

**Example - Create mapping file for IRIS**

.. testcode:: exIRISTransmission

   sample_ws = Load('IRS26176.RAW')
   can_ws = Load('IRS26173.RAW')

   transmission_ws = IndirectTransmissionMonitor(SampleWorkspace=sample_ws, CanWorkspace=can_ws)

   print ', '.join(transmission_ws.getNames())

**Output:**

.. testoutput:: exIRISTransmission

   sample_ws_Sam, sample_ws_Can, sample_ws_Trans

.. categories::
