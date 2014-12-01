.. _PeaksWorkspace:

PeaksWorkspace
==============

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
* In MantidPlot, you can drag/drop a PeaksWorkspace from the list of workspaces onto the `Instrument View <http://www.mantidproject.org/MantidPlot:_Instrument_View>`__ . This will overlay the peaks onto the detector face.
* Right-click a PeaksWorkspace and select the menu to show the peak positions in 3D in the `VatesSimpleInterface <http://www.mantidproject.org/VatesSimpleInterface>`__ .
* In paraview, you can load a .Peaks file loader plugin to view a PeaksWorkspace.

The Peak Object
---------------

Each peak object contains several pieces of information. Not all of them are necessary:

* Detector position and wavelength
* Q position (calculated from the detector position/wavelength)
* H K L indices (optional)
* Goniometer rotation matrix (for finding Q in the sample frame)
* Integrated intensity and error (optional)
* Row/column of detector (only for :ref:`RectangularDetectors <RectangularDetector>` )

Using PeaksWorkspace's in Python
--------------------------------

The PeaksWorkspace and Peak objects are exposed to python.

PeaksWorkspace Python Interface
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: python

    pws = mtd['name_of_peaks_workspace']
    pws.getNumberOfPeaks()
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
    detector_distance = 2.5 # sample-detector distance in meters
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


.. categories:: Concepts