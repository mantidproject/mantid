.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm calculates the incident energy from the time-of-flight between one monitor and some detectors. The detector spectra are summed together using :ref:`GroupDetectors <algm-GroupDetectors>` and a Gaussian is fitted to both the monitor and summed detector data, the difference in the peak positions giving the time-of-flight. If the detector peak is before the monitor one, one pulse interval is added to the time-of-flight. The pulse interval can be given by the ``PulseInterval`` property or it is read from the sample logs under the entry ``pulse_interval`` (in seconds). It is also possible to specify a maximum expected neutron energy by ``MaximumEnergy``. The pulse interval is added to the time-of-flight also if the calculated energy exceeds this value.

If the monitor peak has been previously fitted using :ref:`FindEPP <algm-FindEPP>`, one fitting step can be omitted by supplying the EPP table via ``MonitorEPPWorkspace``.

If no *MonitorWorkspace* is specified, the monitor spectrum is expected to be in the detector workspace. *DetectorWorkspaceIndexSet* understands complex expression, for example ``2,3,5-7,101`` would use detectors 2, 3, 5, 6, 7, and 101 for the computation. 

Usage
-----

**Example - simple incident energy calibration:**

.. testcode:: Simple

   import numpy

   spectrum = 'name = Gaussian, PeakCentre = 1000.0, Height = 500.0, Sigma = 30.0'
   ws = CreateSampleWorkspace(WorkspaceType='Histogram', XUnit='TOF',
       XMin=100.0, XMax=1100.0, BinWidth=10.0,
       NumBanks=1, NumMonitors=1,
       Function='User Defined', UserDefinedFunction=spectrum, BankDistanceFromSample = 1.5)
   # Shift monitor Y and E data
   shift = -70
   ws.setY(0, numpy.roll(ws.readY(0), shift))
   ws.setE(0, numpy.roll(ws.readE(0), shift))
   calibratedE_i = GetEiMonDet(DetectorWorkspace=ws,
       DetectorWorkspaceIndexSet="1-100", MonitorIndex=0)

   print('Calibrated energy: {0:.3f}'.format(calibratedE_i))

Output:

.. testoutput:: Simple

   Calibrated energy: 53.298

**Example - using separate monitor workspace and monitor EPP table:**

.. testcode:: WithMonitorAndEPPs

   import numpy
   from scipy.constants import elementary_charge, neutron_mass
   
   spectrum = 'name = Gaussian, PeakCentre = 1000.0, Height = 500.0, Sigma = 30.0'
   ws = CreateSampleWorkspace(WorkspaceType='Histogram', XUnit='TOF',
     XMin=100.0, XMax=1100.0, BinWidth=10.0,
     NumBanks=1, NumMonitors=1,
     Function='User Defined', UserDefinedFunction=spectrum, BankDistanceFromSample = 1.5)
   # Shift monitor Y and E data
   shift = -70
   ws.setY(0, numpy.roll(ws.readY(0), shift))
   ws.setE(0, numpy.roll(ws.readE(0), shift))
   ExtractMonitors(ws, DetectorWorkspace='detectors', MonitorWorkspace='monitors')
   monitorEPPs = FindEPP('monitors')
   calibratedE_i = GetEiMonDet(DetectorWorkspace='detectors',
     DetectorWorkspaceIndexSet="0-99", MonitorWorkspace='monitors', MonitorEPPTable=monitorEPPs, MonitorIndex=0)
   
   print('Calibrated energy: {0:.3f}'.format(calibratedE_i))

Output:

.. testoutput:: WithMonitorAndEPPs

   Calibrated energy: 53.298

.. categories::

.. sourcelink::
