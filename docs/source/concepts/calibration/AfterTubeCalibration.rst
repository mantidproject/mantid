.. _After_Tube_Calibration:

After Tube Calibration
======================

After calibration, there is an option is to save the workspace to a :ref:`Nexus file <Nexus file>`.
This saved file will include the calibration (in fact any modification
done to the instrument). Reloading this file in Mantid will recreate the
workspace which was saved.

Within a Mantid session, you can copy the calibration to another
workspace using the same instrument by means of the
:ref:`CopyInstrumentParameters  <algm-CopyInstrumentParameters>` algorithm. To do
so execute :ref:`CopyInstrumentParameters  <algm-CopyInstrumentParameters>` with
the following input: select the workspace, which you have calibrated as
the InputWorkspace and the workspace you want to copy the calibration
to, as the OutputWorkspace.

Also, specific to the ISIS facility, the algorithm
:ref:`ModifyDetectorDotDatFile  <algm-ModifyDetectorDotDatFile>` can take an ISIS
.dat file and create a new version of that .dat file with the
calibration of the workspace put into it.


.. categories:: Calibration
