.. _LeanElasticPeaksWorkspace:

LeanElasticPeaks Workspace
==========================

The LeanElasticPeaksWorkspace is a special Workspace that holds a list
of single crystal LeanElasticPeak objects. It is the equivalent to the
:ref:`PeaksWorkspace <PeaksWorkspace>` for Peak objects.

Creating a LeanElasticPeaksWorkspace
------------------------------------

* :ref:`FindPeaksMD <algm-FindPeaksMD>` will find peaks in reciprocal space in a :ref:`MDWorkspace <MDWorkspace>`.
* :ref:`CreatePeaksWorkspace <algm-CreatePeaksWorkspace>` will create an empty LeanElasticPeaksWorkspace that you can then edit.

Viewing a LeanElasticPeaksWorkspace
-----------------------------------

* Double-click a LeanElasticPeaksWorkspace to see the full list of data of each Peak object.
* The LeanElasticPeaksWorkspace can be overlay onto of data using the Mantid Workbench Sliceviewer

The LeanElasticPeak Object
--------------------------

Each peak object contains several pieces of information. Not all of them are necessary:

* Q position (in q-sample frame)
* H K L indices (optional)
* Goniometer rotation matrix (for finding Q in the lab frame)
* Wavelength
* Integrated intensity and error (optional)
* An integration shape (see below)

The LeanElasticPeak Shape
~~~~~~~~~~~~~~~~~~~~~~~~~

This is the same as the Peak object, see :ref:`PeaksWorkspace #The Peak Shape
<the-peak-shape>`.

Using LeanElasticPeaksWorkspaces in Python
------------------------------------------

The LeanElasticPeaksWorkspace and LeanElasticPeak objects are exposed to python.

LeanElasticPeaksWorkspace Python Interface
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

See :class:`IPeaksWorkspace <mantid.api.IPeaksWorkspace>` for
complete API.

.. code-block:: python

    pws = mtd['name_of_peaks_workspace']
    pws.getNumberPeaks()
    p = pws.getPeak(12)
    pws.removePeak(34)

LeanElasticPeak Python Interface
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

See :class:`IPeak <mantid.api.IPeak>` for complete API.

You can get a handle to an existing peak with:

.. code-block:: python

    p = pws.getPeak(12)

Or you can create a new peak in this way:

.. code-block:: python

    qsample = V3D(1.23, 3.45, 2.22) # Q in the lab frame of the peak
    p = pws.createPeak(qsample)
    # The peak can later be added to the workspace
    pws.addPeak(p)

Once you have a handle on a peak "p" you have several methods to query/modify its values:

.. code-block:: python

    hkl = p.getHKL()
    p.setHKL(-5, 4, 3)

    q = p.getQSampleFrame()
    q = p.getQLabFrame()

    p.setIntensity(1000.0)
    p.setSigmaIntensity(31.6)
    counts = p.getIntensity()

    wl = p.getWavelength()
    d = p.getDSpacing()
    shape = p.getPeakShape()


.. categories:: Concepts
