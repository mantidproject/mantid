.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Transfer an instrument parameters from a giving workspace to a receiving
workspace.

The instrument parameters in the receiving workspace are REPLACED
(despite you can assume from the name of the algorithm) by a copy of the
instrument parameters in the giving workspace so gaining any
manipulations such as calibration done to the instrument in the giving
workspace.

Does not work on workspaces with grouped detectors if some of the
detectors were calibrated.

Usage
-----
**Example - Copy parameters that contain the movement of 3 detectors**

.. include:: ../usagedata-note.txt

.. testcode:: ExCopyInstrumentParametersSimple

   # We load two workspaces with the same instrument
   # the first of which has had some detectors moved by calibration.
   # Then run CopyInstrumentParameters and show that the same detectors have been moved.
   #
   # We use HRPD workspaces, which have detectors that are not rectangular.
   ws1 = Load("HRP38094Calib.nxs")
   ws2 = Load("HRP39180.RAW")

   spectra = [0, 1, 3] # Sprectra of detectors moved

   # Show positions in 1st workspace
   specInfo1 = ws1.spectrumInfo()
   for i in spectra:
        detID = ws1.getSpectrum(i).getDetectorIDs()[0]
        pos = specInfo1.position(i)
        print("Position of Detector ID={} in 1st workspace: {:.0f},{:.0f},{:.0f}".
               format(detID, pos.X(), pos.Y(), pos.Z()))

   # Show positions in 2nd workspace before CopyInstrumrentParameters
   specInfo2 = ws2.spectrumInfo()
   for i in spectra:
        detID = ws2.getSpectrum(i).getDetectorIDs()[0]
        pos = specInfo2.position(i)
        print("Position of Detector ID=%i in 2nd workspace before CopyInstrumentParameters: %.0f,%.0f,%.0f" % (detID,
                pos.X(), pos.Y(), pos.Z()))


   # Copy parameters from 1st workspace to 2nd workspace
   CopyInstrumentParameters( ws1, ws2 )


   # Show positions in 2nd workspace after CopyInstrumrentParameters
   specInfo2 = ws2.spectrumInfo()
   for i in spectra:
        detID = ws2.getSpectrum(i).getDetectorIDs()[0]
        pos = specInfo2.position(i)
        print("Position of Detector ID={} in 2nd workspace after CopyInstrumentParameters: {:.0f},{:.0f},{:.0f}".
              format(detID, pos.X(), pos.Y(), pos.Z()))

Output:

.. testoutput:: ExCopyInstrumentParametersSimple

   Position of Detector ID=1100 in 1st workspace: ...,...,...
   Position of Detector ID=1101 in 1st workspace: ...,...,...
   Position of Detector ID=1103 in 1st workspace: ...,...,...
   Position of Detector ID=1100 in 2nd workspace before CopyInstrumentParameters: ...,...,...
   Position of Detector ID=1101 in 2nd workspace before CopyInstrumentParameters: ...,...,...
   Position of Detector ID=1103 in 2nd workspace before CopyInstrumentParameters: ...,...,...
   Position of Detector ID=1100 in 2nd workspace after CopyInstrumentParameters: ...,...,...
   Position of Detector ID=1101 in 2nd workspace after CopyInstrumentParameters: ...,...,...
   Position of Detector ID=1103 in 2nd workspace after CopyInstrumentParameters: ...,...,...

.. categories::

.. sourcelink::
