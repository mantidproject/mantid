
.. algorithm::

.. warning::

    This algorithm is currently for the VULCAN instrument testing purposes


.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This is a simplified version of :ref:`algm-AlignAndFocusPowderFromFiles` which uses very few child algorithms.
The main feature is that this reads the events, filters and adjusts their time-of-flight, then increments the correct bin in the output workspace.
As a result, there is a significantly smaller memory usage and the processing is significantly faster.

Current limitations compared to ``AlignAndFocusPowderFromFiles``

- only supports the VULCAN instrument
- hard coded for 6 particular groups
- common binning across all output spectra
- only specify binning in time-of-flight
- does not support event filtering
- does not support copping data
- does not support removing prompt pulse
- does not support removing bad pulses

Child algorithms used are

- :ref:`algm-LoadDiffCal`
- :ref:`algm-LoadIDFFromNexus-v1`
- :ref:`algm-EditInstrumentGeometry`
- :ref:`algm-LoadNexusLogs`


.. categories::

.. sourcelink::
