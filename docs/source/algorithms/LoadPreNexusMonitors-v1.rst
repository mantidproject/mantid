.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm reads in the filenames of the monitors from the runinfo file.
It will only work with histogram monitors and assumes that all monitors are on
the same time axis. It also assumes that the beam monitor files are in
the same directory as the runinfo file.

Usage
-----

**Example - Loading PreNexus Monitors**

.. testcode:: ExSimpleLoading

   # CNCS_7860_runinfo.xml references 3 beam monitor files.
   monitor_ws = LoadPreNexusMonitors("CNCS_7860_runinfo.xml")

   print("The resulting workspace contains {} spectra -- one for each monitor.".format(monitor_ws.getNumberHistograms()))

Output:

.. testoutput:: ExSimpleLoading

   The resulting workspace contains 3 spectra -- one for each monitor.

.. categories::

.. sourcelink::
