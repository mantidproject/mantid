
.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Add a peak in the HKL frame to an existing :ref:`PeaksWorkspace <PeaksWorkspace>`. The OrientedLattice must be provided and the Goniometer should be set correctly before running the algorithm. 

The peak is added to the existing PeaksWorkspace. Run information and goniometer setting and the UB matrix are taken from the provided PeaksWorkspace.


Usage
-----

**Example - AddPeakHKL**

.. testcode:: AddPeakHKLExample

   import os
   import numpy 
   import mantid.kernel

   # Create an instrument workspace 
   inst_dir = config.getInstrumentDirectory()
   sxd_ws = LoadEmptyInstrument(os.path.join(inst_dir, "SXD_Definition.xml"))
   AddSampleLog(sxd_ws, 'run_number', '1', 'Number')

   # Create a peaks workspace from the instrument workspace
   peak_ws = CreatePeaksWorkspace(InstrumentWorkspace=sxd_ws, NumberOfPeaks=0)
   peak_ws.mutableRun().addProperty('run_number', sxd_ws.run().getProperty('run_number'), True)

   # Apply a UB
   ub = numpy.array([[-0.15097235,  0.09164432,  0.00519473], [ 0.0831895,   0.14123681, -0.06719047], [-0.03845029, -0.05534039, -0.1633801 ]])
   SetUB(peak_ws, UB=ub)

   # Add a new peak
   AddPeakHKL(peak_ws, [2, 0, -4])

   # Get info on newly added peak
   peak = peak_ws.getPeak(0)
   print 'Peak wavelength', round(peak.getWavelength(), 4)
   print 'Peak detector id', peak.getDetectorID()
   print 'Peak run number', peak.getRunNumber()
   print 'Peak HKL', peak.getHKL()

Output:

.. testoutput:: AddPeakHKLExample

  Peak wavelength 1.8423
  Peak detector id 25766
  Peak run number 1
  Peak HKL [2,0,-4]

.. categories::

