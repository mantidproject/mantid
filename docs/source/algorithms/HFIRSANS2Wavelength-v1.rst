
.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Transforms the event files with fake time of flight from the SANS instruments at HFIR into histograms in wavelength.

Input event workspace in units of TOF is rebinned from -20000 to 20000 microseconds into one bin. The bin boundaries
are then set to wavelength +/- wavelength_spread/2, where wavelength and wavelength_spread are logs in the nexus file.
The units are set to "Wavelength".

Usage
-----

**Example - HFIRSANS2Wavelength**

.. testcode:: HFIRSANS2Wavelength

  ws = CreateWorkspace(DataX='1,11,111,1,11,111',
                       DataY='2,22,22,22',
                       DataE='1,5,5,5',
                       UnitX="TOF",
                       NSpec=2)
  AddSampleLog(ws, LogName='wavelength', LogText='6.5', LogType='Number Series')
  AddSampleLog(ws, LogName='wavelength_spread', LogText='1.0', LogType='Number Series')
  out = HFIRSANS2Wavelength(InputWorkspace=ws)
  print(out.blocksize())
  print(out.readX(0)[0])
  print(out.readX(0)[1])
  print(out.getAxis(0).getUnit().caption())

Output:

.. testoutput:: HFIRSANS2Wavelength

  1
  6.0
  7.0
  Wavelength

.. categories::

.. sourcelink::
