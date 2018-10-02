
.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Algorithm to mask detectors, tube, or pixels in the ORNL SANS instruments. It
applies to the following instruments only: BioSANS, GPSANS and EQSANS.

If one of Detector, Tube, Pixel entries is left blank, it will apply to all
elements of that type. For example:

- SANSMaskDTP(InputWorkspace=ws, Detector="1") will completely mask all tubes and pixels in Detector 1. 
- SANSMaskDTP(InputWorkspace=ws, Pixel="1,2") will mask all pixels 1 and 2, in all tubes, in all Detectors.

The algorithm allows ranged or interleaved inputs: 

- Pixel = "1-8,121-128" is equivalent to Pixel = "1,2,3,4,5,6,7,8,121,122,123,124,125,126,127,128"
- Pixel = "1::2" is equivalent to Pixel = "1,3,5,7,9,11 (...)"


Usage
-----

.. testcode:: SANSMaskDTP

    LoadEmptyInstrument(InstrumentName='biosans', OutputWorkspace='ws')
    # mask completely left and right side of the detector:
    # tubes 1 to 10 and 183 to 192.
    mask_return = SANSMaskDTP(InputWorkspace='ws', tube='1-10,183-192')
    print("SANSMaskDTP masked {} detectors.".format(len(mask_return)))

    det_ids_masked = []
    ws = mtd['ws']
    det_info = ws.detectorInfo()
    for i in range(ws.getNumberHistograms()):
        if not det_info.isMonitor(i) and det_info.isMasked(i):
            det_ids_masked.append(ws.getDetector(i).getID())
    
    print("Found {} detectors masked.".format(len(det_ids_masked)))
    

.. testcleanup:: SANSMaskDTP

   DeleteWorkspace('ws')


Output:

.. testoutput:: SANSMaskDTP

    SANSMaskDTP masked 5120 detectors.
    Found 5120 detectors masked.


.. categories::

.. sourcelink::
