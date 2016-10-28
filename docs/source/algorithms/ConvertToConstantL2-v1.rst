.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Moves the instrument and then corrects the flight paths such that a flat detector appears spherical with a constant l2 value.

Both time-of-flight sample-detector time and sample to detector distance are corrected to constant values.

The sample to detector distance must be specified as **l2** in the instrument parameters file.

So far this has only be tested on the ILL IN5 instrument.

Note
###################################
This algorithm is intended for visualisaion only. It is not recommended as part of any reduction process.

**Example - Add multiple sample logs**

.. testcode:: IN5Example

   # Load an IN5 file
   ws = Load('ILL/IN5/104007.nxs')

   print("Before conversion:")
   print("Monitor {0} distance from origin: {1:.3f}".format(0, ws.getDetector(0).getPos().norm()))
   for i in range(1, 6):
     print("Detector {0} distance from origin: {1:.3f}".format(i, ws.getDetector(i).getPos().norm()))

   # Convert to a detector with constant L2
   converted_ws = ConvertToConstantL2(ws)

   print("After conversion:")
   print("Monitor {0} distance from origin: {1:.3f}".format(0, converted_ws.getDetector(0).getPos().norm()))
   for i in range(1, 6):
     print("Detector {0} distance from origin: {1:.3f}".format(i, converted_ws.getDetector(i).getPos().norm()))

Output:

.. testoutput:: IN5Example

    Before conversion:
    Monitor 0 distance from origin: 0.500
    Detector 1 distance from origin: 4.296
    Detector 2 distance from origin: 4.291
    Detector 3 distance from origin: 4.287
    Detector 4 distance from origin: 4.283
    Detector 5 distance from origin: 4.278
    After conversion:
    Monitor 0 distance from origin: 4.000
    Detector 1 distance from origin: 4.000
    Detector 2 distance from origin: 4.000
    Detector 3 distance from origin: 4.000
    Detector 4 distance from origin: 4.000
    Detector 5 distance from origin: 4.000

.. categories::

.. sourcelink::
