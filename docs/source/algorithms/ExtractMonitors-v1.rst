.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------
This algorithm takes a workspace that contains monitors and detectors, and outputs two workspaces, one with only
detectors and one with only the monitors. The workspaces are linked such that a call to *getMonitorWorkspace* on the
new detector workspace will return the new monitor workspace.

The algorithm can also output only the detector workspace without monitors, or the monitors on their own.

Usage
-----

.. include:: ../usagedata-note.txt

**Example - ExtractMonitorsExample**

.. testcode:: ExtractMonitorsExample

  ws = Load('ENGINX00213855.nxs')

  ExtractMonitors(InputWorkspace = ws,
                  DetectorWorkspace = 'Detectors',
                  MonitorWorkspace = 'Monitors')

  detector_ws = mtd['Detectors']
  monitor_ws = mtd['Monitors']

  # Detector histograms
  print("Number of spectra in input workspace: {0}").format(ws.getNumberHistograms())
  # Detector histograms (spectra missing detectors generate warnings)
  print("Number of spectra in detector workspace: {0}").format(detector_ws.getNumberHistograms())
  # Monitor histograms
  print("Number of spectra in monitor workspace: {0}").format(monitor_ws.getNumberHistograms())
  # Check if the first spectrum in the detector workspace is a monitor
  print("Detector workspace isMonitor for spectrum 0: {0}").format(detector_ws.getDetector(0).isMonitor())
  # Check if the first spectrum in the monitor workspace is a monitor
  print("Monitor workspace isMonitor for spectrum 0: {0}").format(monitor_ws.getDetector(0).isMonitor())
  # See the monitor workspace is set
  print("Name of monitor workspace: {0}").format(detector_ws.getMonitorWorkspace().name())

Output:

.. testoutput:: ExtractMonitorsExample
  :options: +NORMALIZE_WHITESPACE

  Number of spectra in input workspace: 2513
  Number of spectra in detector workspace: 2500
  Number of spectra in monitor workspace: 2
  Detector workspace isMonitor for spectrum 0: False
  Monitor workspace isMonitor for spectrum 0: True
  Name of monitor workspace: Monitors


.. categories::

.. sourcelink::
