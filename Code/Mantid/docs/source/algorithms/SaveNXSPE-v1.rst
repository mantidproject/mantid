.. algorithm::

.. summary::

.. alias::

.. properties::

.. _MatrixWorkspace: http://www.mantidproject.org/MatrixWorkspace

Description
-----------

Saves the given `MatrixWorkspace`_ to a file in the NeXus-based 'NXSPE' format.

Restrictions on the Input Workspace
###################################

The input workspace must have units of Momentum Transfer ('DeltaE') and
contain histogram data with common binning on all spectra.

Child Algorithms Used
#####################

The :ref:`algm-FindDetectorsPar` algorithm is used to calculate
detectors parameters from the instrument description.

Usage
-----

**Example - Save/Load "Roundtrip"**

.. testcode:: ExSimpleSavingRoundtrip

   import os
   import numpy
   
   # Create dummy workspace.
   out_ws = CreateSimulationWorkspace(Instrument="IRIS", BinParams="0,500,2000")
   out_ws.setY(0, numpy.array([10.0, 50.0, 30.0, 60.0]))
   AddSampleLog(out_ws, 'Ei', LogText='321', LogType='Number')
   
   file_path = os.path.join(config["defaultsave.directory"], "NXSPEData.nxspe")
   
   # Do a "roundtrip" of the data.
   SaveNXSPE(out_ws, file_path,Psi=32)
   
   # By desigghn, SaveMXSPE does not store detector's ID-s. LoadNXSPE sets detector's ID-s to defaults.
   # To compare loaded and saved workspaces here, one needs to set-up default detector's ID-s to the source workspace.
   nSpec = out_ws.getNumberHistograms()
   for i in xrange(0,nSpec):
       sp=out_ws.getSpectrum(i);
       sp.setDetectorID(i+1);
   in_ws = LoadNXSPE(file_path)
   
   ws_comparison_rez = CheckWorkspacesMatch(out_ws,in_ws,1.e-9,CheckInstrument=False)
   print "Contents of the first spectrum = " + str(in_ws.readY(0)) + "."
   print "Initial and loaded workspaces comparison is: ",ws_comparison_rez
   run = in_ws.getRun();
   print "Loaded workspace has attached incident energy Ei={0:5} and rotation angle Psi={1:5}deg".format(run.getLogData('Ei').value,run.getLogData('psi').value)
   

.. testcleanup:: ExSimpleSavingRoundtrip

   os.remove(file_path)

Output:

.. testoutput:: ExSimpleSavingRoundtrip

   Contents of the first spectrum = [ 10.  50.  30.  60.].
   Initial and loaded workspaces comparison is:  Success!
   Loaded workspace has attached incident energy Ei=321.0 and rotation angle Psi= 32.0deg
   

.. categories::
