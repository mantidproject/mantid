.. _PeaksWorkspace:

Peaks Workspace
===============

The PeaksWorkspace is a special Workspace that holds a list of single crystal Peak objects.

Creating a PeaksWorkspace
-------------------------

* :ref:`FindPeaksMD <algm-FindPeaksMD>` will find peaks in reciprocal space in a :ref:`MDWorkspace <MDWorkspace>`.
* :ref:`FindSXPeaks <algm-FindSXPeaks>` will find peaks in detector space.
* :ref:`PredictPeaks <algm-PredictPeaks>` will predict peak positions in a workspace given a UB matrix.
* The :ref:`LoadIsawPeaks <algm-LoadIsawPeaks>` algorithm will load a PeaksWorkspace from file.
* The :ref:`SaveIsawPeaks <algm-SaveIsawPeaks>` algorithm will save a PeaksWorkspace to a file.
* :ref:`CreatePeaksWorkspace <algm-CreatePeaksWorkspace>` will create an empty PeaksWorkspace that you can then edit.

Viewing a PeaksWorkspace
------------------------

* Double-click a PeaksWorkspace to see the full list of data of each Peak object.
* In MantidWorkbench, you can drag/drop a PeaksWorkspace from the list of workspaces onto the
  :ref:`Instrument View <InstrumentViewer>`. This will overlay the peaks onto the detector face.
* :ref:`Peaks overlay <sliceviewer_peaks_overlay>` in the :ref:`sliceviewer`.

Each peak object contains several pieces of information. Not all of them are necessary

* Detector position and wavelength
* Q position (calculated from the detector position/wavelength)
* H K L indices (optional)
* Goniometer rotation matrix (for finding Q in the sample frame)
* Integrated intensity and error (optional)
* Row/column of detector (only for :ref:`RectangularDetectors <RectangularDetector>` )
* An integration shape (see below)

.. _the-peak-shape:

The Peak Shape
~~~~~~~~~~~~~~

Each Peak object contains a PeakShape. Typically the integration algorithms which act on, and return PeaksWorkspaces set the shape of the peaks. The PeakShape is owned by the Peak, not the PeaksWorkspace, so when PeaksWorkspaces are split, or concatinated, the integration shapes are unaltered. Aside from the Null Peak Shape, each peak shape contains at least the following information.

* The algorithm used to perform the integration
* The version of the algorithm used to perform the integration
* The frame in which the integration has been performed

Subtypes of PeakShape will then provide additional information. For example PeakShapeSpherical provides the radius as well as background inner, and background outer radius.

Creating peak shapes in Python
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

You can manually create the peak shape and set it on a peak from Python. This peak shape is not used by any integration algorithm but will allow you to visualize the shapes with :ref:`SliceViewer`.

.. code-block:: python

    from mantid.kernel import V3D
    from mantid.dataobjects import NoShape, PeakShapeSpherical, PeakShapeEllipsoid

    pws = mtd['name_of_peaks_workspace']

    no_shape = NoShape()
    pws.getPeak(0).setPeakShape(no_shape)

    sphere = PeakShapeSpherical(peakRadius=0.5)
    pws.getPeak(1).setPeakShape(sphere)

    sphere_with_background = PeakShapeSpherical(peakRadius=0.5,
                                                backgroundInnerRadius=0.6,
                                                backgroundOuterRadius=0.7)
    pws.getPeak(2).setPeakShape(sphere_with_background)

    ellipse = PeakShapeEllipsoid(directions=[V3D(1, 0, 0), V3D(0, 1, 0), V3D(0, 0, 1)],
                                 abcRadii=[0.1, 0.2, 0.3],
                                 abcRadiiBackgroundInner=[0.4, 0.5, 0.6],
                                 abcRadiiBackgroundOuter=[0.7, 0.8, 0.9])
    pws.getPeak(3).setPeakShape(ellipse)


Calculate Goniometer For Constant Wavelength
--------------------------------------------

If you set the `wavelength` (in Å) or `energy` (in meV) property on a
PeaksWorkspace, or if the instrument on the PeaksWorkspace has the
`wavelength` parameter, the goniometer rotation will be calculated
when the createPeak method is used. This allows you to use one
instrument definition for multiple goniometer rotations, for example
adding peaks in Slice Viewer from multiple combined MD workspaces. It
only works for a constant wavelength source and only for Q sample
workspaces. It also assumes the goniometer rotation is around the
y-axis only. For details on the calculation see "Calculate Goniometer
For Constant Wavelength" at :ref:`FindPeaksMD <algm-FindPeaksMD>`.

.. code-block:: python

    pws = mtd['name_of_peaks_workspace']
    pws.run().addProperty('wavelength', 1.54, True)
    # or
    pws.run().addProperty('energy', 34.48, True)


Using PeaksWorkspaces in Python
---------------------------------

The PeaksWorkspace and Peak objects are exposed to python.

PeaksWorkspace Python Interface
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: python

    pws = mtd['name_of_peaks_workspace']
    pws.getNumberPeaks()
    p = pws.getPeak(12)
    pws.removePeak(34)

Peak Python Interface
~~~~~~~~~~~~~~~~~~~~~

You can get a handle to an existing peak with:

.. code-block:: python

    p = pws.getPeak(12)

Or you can create a new peak in this way:

.. code-block:: python

    qlab = V3D(1.23, 3.45, 2.22) # Q in the lab frame of the peak
    detector_distance = 2.5 # sample-detector distance in meters. Detector distances are optional. Calculated in not provided.
    p = pws.createPeak(qlab, detector_distance)
    # The peak can later be added to the workspace
    pws.addPeak(p)

Once you have a handle on a peak "p" you have several methods to query/modify its values:

.. code-block:: python

    hkl = p.getHKL()
    p.setHKL(-5, 4, 3)

    q = p.getQSampleFrame()
    q = p.getQLabFrame()
    detid = p.getDetectorID()

    p.setIntensity(1000.0)
    p.setSigmaIntensity(31.6)
    counts = p.getIntensity()

    wl = p.getWavelength()
    tof = p.getTOF()
    d = p.getDSpacing()
    shape = p.getPeakShape()


.. categories:: Concepts
