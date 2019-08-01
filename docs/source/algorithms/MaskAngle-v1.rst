.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Algorithm to mask detectors with scattering angles in a given interval
(in degrees) By default MinAngle=0, MaxAngle=180, so if no keywords are
set, all detectors are going to be masked Returns a list of detectors
that were masked

When masking the phi angle the absolute value of phi is used.

The three angle options are subtly different:
* ``TwoTheta`` is the detector's :math:`2 \theta` value as calcualted from the downstream beam. This is the angle from the pole and not constrained to be in-plane.
* ``Phi`` is the detector's angle with respect to the positive z-axis not constrained to be in-plane. For many geometries this will be the same as the ``TwoTheta`` option.
* ``InPlane`` is the detector's angle with respect to the positive z-axis constrained within the x-z-plane.

For the ``TwoTheta`` and ``Phi`` options, only positive angles are allowed.

Usage
-----

.. include:: ../usagedata-note.txt

**Two theta example**

.. testcode:: MaskAngle

    #Load a workspace
    ws = Load("CNCS_7860")

    #Do the masking for direct beam
    mask = MaskAngle(ws, MinAngle=0, MaxAngle=10)
    print("The algorithm has masked {} detectors".format(mask.size))

    #to test check a couple of detectors
    inst = ws.getInstrument()
    print("Is the minimum element in the mask list (detector {}) masked?  {}".format(mask.min(), inst.getDetector(int(mask.min())).isMasked()))
    print("Is the maximum element in the mask list (detector {}) masked?  {}".format(mask.max(), inst.getDetector(int(mask.max())).isMasked()))
    print("Is a detector outside the list masked (for example detector 100)?  {}".format(inst.getDetector(100).isMasked()  ))

.. testcleanup:: MaskAngle

   DeleteWorkspace(ws)

Output:

.. testoutput:: MaskAngle

    The algorithm has masked 1516 detectors
    Is the minimum element in the mask list (detector 35126) masked?  True
    Is the maximum element in the mask list (detector 38601) masked?  True
    Is a detector outside the list masked (for example detector 100)?  False


The instrument view would look like:

.. figure:: /images/MaskAngle.png
   :alt: MaskAngle.png

**Phi example**

.. testcode:: MaskAngle_phi

    #Load a workspace
    ws = Load("CNCS_7860")

    #Do the masking for direct beam
    mask = MaskAngle(ws, MinAngle=30, MaxAngle=150, Angle='Phi')
    print("The algorithm has masked {} detectors".format(mask.size))

    #to test check a couple of detectors
    inst = ws.getInstrument()
    print("Is the minimum element in the mask list (detector {}) masked?  {}".format(mask.min(), inst.getDetector(int(mask.min())).isMasked()))
    print("Is the maximum element in the mask list (detector {}) masked?  {}".format(mask.max(), inst.getDetector(int(mask.max())).isMasked()))
    print("Is a detector outside the list masked (for example detector 100)?  {}".format(inst.getDetector(100).isMasked()  ))

.. testcleanup:: MaskAngle_phi

   DeleteWorkspace(ws)

Output:

.. testoutput:: MaskAngle_phi

    The algorithm has masked 6348 detectors
    Is the minimum element in the mask list (detector 29568) masked?  True
    Is the maximum element in the mask list (detector 44287) masked?  True
    Is a detector outside the list masked (for example detector 100)?  False


The instrument view would look like:

.. figure:: /images/MaskAngle_phi.png
   :alt: MaskAngle_phi.png

.. categories::

.. sourcelink::
