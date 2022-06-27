.. _LeanElasticPeaksWorkspace:

LeanElasticPeaks Workspace
==========================

The LeanElasticPeaksWorkspace is a special Workspace that holds a list
of single crystal LeanElasticPeak objects. It is the equivalent to the
:ref:`PeaksWorkspace <PeaksWorkspace>` for Peak objects.

Creating a LeanElasticPeaksWorkspace
------------------------------------

* :ref:`FindPeaksMD <algm-FindPeaksMD>` will find peaks in reciprocal space in a :ref:`MDWorkspace <MDWorkspace>`.
* :ref:`PredictPeaks <algm-PredictPeaks>` will predict peaks in reciprocal space for a given UB matrix.
* :ref:`CreatePeaksWorkspace <algm-CreatePeaksWorkspace>` will create an empty LeanElasticPeaksWorkspace that you can then edit. *_e.g._* :code:`CreatePeaksWorkspace(NumberOfPeaks=0, OutputType="LeanElasticPeak", OutputWorkspace='peaks')`

FindPeaksMD example
~~~~~~~~~~~~~~~~~~~

Create a MDEventWorkspace and add some fake peaks at Q-sample of
(1,1,1), (3,3,3), and (5.5,3.14159,1) and run FindPeaksMD.

.. testcode:: findpeaksmd

   CreateMDWorkspace(Dimensions='3', Extents='0,10,0,10,0,10',
                     Names='Q_sample_x,Q_sample_y,Q_sample_z',
                     Units='rlu,rlu,rlu',
                     Frames='QSample,QSample,QSample',
		     OutputWorkspace='MDE')
   FakeMDEventData('MDE', PeakParams='10000,1,1,1,0.2')
   FakeMDEventData('MDE', PeakParams='100000,3,3,3,0.3')
   FakeMDEventData('MDE', PeakParams='1000,5.5,3.14159,1,0.1')

   peaks=FindPeaksMD('MDE',
                     DensityThresholdFactor=1,
                     PeakDistanceThreshold=1)

   for n in range(peaks.getNumberPeaks()):
       p = peaks.getPeak(n)
       print(f"Peak {n} - Q-sample = {p.getQSampleFrame()} - bin count = {p.getBinCount()}")

Output:

.. testoutput:: findpeaksmd

   Peak 0 - Q-sample = [5.50233,3.1408,1.00067] - bin count = 1000.0
   Peak 1 - Q-sample = [2.91935,3.07936,2.91857] - bin count = 503.0
   Peak 2 - Q-sample = [1.00319,1.00183,1.00097] - bin count = 181.0

Now convert that MDEventWorkspace to MDHistoWorkspace and run FindPeaksMD on that.

.. testcode:: findpeaksmd

   BinMD('MDE', OutputWorkspace='MDH',
         AlignedDim0='Q_sample_x,0,10,100',
         AlignedDim1='Q_sample_y,0,10,100',
         AlignedDim2='Q_sample_z,0,10,100')

   peaks2=FindPeaksMD('MDH',
                      DensityThresholdFactor=1,
                      PeakDistanceThreshold=1)

   for n in range(peaks2.getNumberPeaks()):
       p = peaks2.getPeak(n)
       print(f"Peak {n} - Q-sample = {p.getQSampleFrame()} - bin count = {p.getBinCount():.2f}")

Output:

.. testoutput:: findpeaksmd

   Peak 0 - Q-sample = [2.85,2.95,3.05] - bin count = 0.94
   Peak 1 - Q-sample = [1.05,0.95,1.05] - bin count = 0.35
   Peak 2 - Q-sample = [5.55,3.15,1.05] - bin count = 0.19


PredictPeaks example
~~~~~~~~~~~~~~~~~~~~

Create a workspace, set the UB and then predict all peaks in the d-spacing range.

.. testcode:: predictpeaks

   ws=CreatePeaksWorkspace()
   SetUB(ws,a=5,b=5,c=7,gamma=120)
   peaks = PredictPeaks(ws, OutputType='LeanElasticPeak', CalculateWavelength=False, MinDSpacing=2.5, MaxDSpacing=3.5)
   for n in range(peaks.getNumberPeaks()):
       p = peaks.getPeak(n)
       print(f"Peak {n:>2} - d-spacing = {p.getDSpacing():.2f} - HKL = {p.getHKL()}")

Output:

.. testoutput:: predictpeaks

   Peak  0 - d-spacing = 2.50 - HKL = [-2,1,0]
   Peak  1 - d-spacing = 2.50 - HKL = [-1,-1,0]
   Peak  2 - d-spacing = 2.72 - HKL = [-1,0,-2]
   Peak  3 - d-spacing = 2.72 - HKL = [-1,0,2]
   Peak  4 - d-spacing = 2.72 - HKL = [-1,1,-2]
   Peak  5 - d-spacing = 2.72 - HKL = [-1,1,2]
   Peak  6 - d-spacing = 2.50 - HKL = [-1,2,0]
   Peak  7 - d-spacing = 2.72 - HKL = [0,-1,-2]
   Peak  8 - d-spacing = 2.72 - HKL = [0,-1,2]
   Peak  9 - d-spacing = 3.50 - HKL = [0,0,-2]
   Peak 10 - d-spacing = 3.50 - HKL = [0,0,2]
   Peak 11 - d-spacing = 2.72 - HKL = [0,1,-2]
   Peak 12 - d-spacing = 2.72 - HKL = [0,1,2]
   Peak 13 - d-spacing = 2.50 - HKL = [1,-2,0]
   Peak 14 - d-spacing = 2.72 - HKL = [1,-1,-2]
   Peak 15 - d-spacing = 2.72 - HKL = [1,-1,2]
   Peak 16 - d-spacing = 2.72 - HKL = [1,0,-2]
   Peak 17 - d-spacing = 2.72 - HKL = [1,0,2]
   Peak 18 - d-spacing = 2.50 - HKL = [1,1,0]
   Peak 19 - d-spacing = 2.50 - HKL = [2,-1,0]

Viewing a LeanElasticPeaksWorkspace
-----------------------------------

* Double-click a LeanElasticPeaksWorkspace to see the full list of data of each Peak object.
* The LeanElasticPeaksWorkspace can be overlay onto of data using :ref:`sliceviewer`.

The LeanElasticPeak Object
--------------------------

Each peak object contains several pieces of information. Not all of them are necessary:

* Q position (in q-sample frame)
* H K L indices (optional)
* Goniometer rotation matrix (for converting to Q in the lab frame)
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

To get peaks from an existing workspace `'name_of_peaks_workspace'`

.. code-block:: python

    pws = mtd['name_of_peaks_workspace']
    pws.getNumberPeaks()
    p = pws.getPeak(12)
    pws.removePeak(34)

The command :meth:`p = pws.getPeak
<mantid.api.IPeaksWorkspace.getPeak()>` will give you a reference to
the peak and not a copy of the peak, so any modification to `p` will
change not just `p` but that peak in the `name_of_peaks_workspace`
workspace.

To create an empty LeanElasticPeaksWorkspace then add peaks to it:

.. testsetup:: createpeaks

   from mantid.kernel import V3D

.. testcode:: createpeaks

   peaks = CreatePeaksWorkspace(NumberOfPeaks=0, OutputType="LeanElasticPeak")
   p0 = peaks.createPeakQSample([1,1,1])
   peaks.addPeak(p0)
   SetUB(peaks, a=2, b=2, c=2)
   p1 = peaks.createPeakHKL([1,2,3])
   peaks.addPeak(p1)
   for n in range(2):
       peak = peaks.getPeak(n)
       print('Peak {} hkl = {:.0f}{:.0f}{:.0f} q_sample = {}'.format(n, peak.getH(), peak.getK(), peak.getL(), peak.getQSampleFrame()))

Output:

.. testoutput:: createpeaks

   Peak 0 hkl = 000 q_sample = [1,1,1]
   Peak 1 hkl = 123 q_sample = [6.28319,9.42478,3.14159]


LeanElasticPeak Python Interface
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

See :class:`IPeak <mantid.api.IPeak>` for complete API.

You can get a handle to an existing peak with:

.. testcode:: createpeaks

    p = peaks.getPeak(1)

Or you can create a new peak in this way:

.. testcode:: createpeaks

    qsample = V3D(1.23, 3.45, 2.22) # Q in the sample frame of the peak
    p = peaks.createPeakQSample(qsample)
    # The peak can later be added to the workspace
    peaks.addPeak(p)

Once you have a handle on a peak "p" you have several methods to query/modify its values:

.. testcode:: createpeaks

    p.setHKL(-5, 4, 3)
    hkl = p.getHKL()
    print("hkl =", hkl)

    q = p.getQSampleFrame()
    print("q =", q)

    p.setIntensity(1000.0)
    p.setSigmaIntensity(31.6)
    counts = p.getIntensity()
    print("counts =", counts)

    wl = p.getWavelength()
    print("wl = {:.2f}".format(wl))
    d = p.getDSpacing()
    print("d = {:.2f}".format(d))
    shape = p.getPeakShape()
    print("shape =", shape.shapeName())

Output:

.. testoutput:: createpeaks

    hkl = [-5,4,3]
    q = [1.23,3.45,2.22]
    counts = 1000.0
    wl = 1.52
    d = 1.47
    shape = none

.. categories:: Concepts
