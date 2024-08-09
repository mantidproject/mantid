.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Saves the given :ref:`MatrixWorkspace` to a file in the NeXus-based 'NXSPE' format.

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
   AddSampleLog(out_ws, 'Ei', LogText='321', LogType='Number', NumberType='Double')
   out_ws.setDistribution(True)

   file_path = os.path.join(config["defaultsave.directory"], "NXSPEData.nxspe")

   # Do a "roundtrip" of the data.
   SaveNXSPE(out_ws, file_path,Psi=32)

   # By design, SaveNXSPE does not store detector's ID-s. LoadNXSPE sets detector's ID-s to defaults.
   # To compare loaded and saved workspaces here, one needs to set-up default detector's ID-s to the source workspace.
   nSpec = out_ws.getNumberHistograms()
   for i in range(0,nSpec):
       sp=out_ws.getSpectrum(i);
       sp.setDetectorID(i+1);
   in_ws = LoadNXSPE(file_path)

   ws_comparison_rez = CompareWorkspaces(out_ws,in_ws,1.e-9,CheckInstrument=False)
   print("Contents of the first spectrum = {}.".format(in_ws.readY(0)))
   print("Initial and loaded workspaces comparison is: {}".format(str(ws_comparison_rez[0])))
   run = in_ws.getRun();
   print("Loaded workspace has attached incident energy Ei={0:.1f} and rotation angle Psi= {1:.1f}deg".format(run.getLogData('Ei').value,run.getLogData('psi').value))


.. testcleanup:: ExSimpleSavingRoundtrip

   os.remove(file_path)

Output:

.. testoutput:: ExSimpleSavingRoundtrip

   Contents of the first spectrum = [10. 50. 30. 60.].
   Initial and loaded workspaces comparison is: True
   Loaded workspace has attached incident energy Ei=321.0 and rotation angle Psi= 32.0deg

Note that :ref:`algm-LoadNXSPE` automatically applies the `distribution` flag to the loaded workspace.
This is because all examples of workspaces saved to `NXSPE` format by the reduction algorithms
are distributions (signal is count rate and should be multiplied by bin widths to get counts).
`SaveNXSPE` does not require its input is a distribution, however, and the `NXSPE` format does
not have a distribution flag.


.. categories::

.. sourcelink::
