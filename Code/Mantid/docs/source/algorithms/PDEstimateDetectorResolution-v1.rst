.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Instrument resolution
---------------------

Resolution of a detector in d-spacing is defined as
:math:`\frac{\Delta d}{d}`, which is constant for an individual
detector.

Starting from the Bragg equation for T.O.F. diffractometer,

.. math:: d = \frac{t}{252.777\cdot L\cdot2\sin\theta}

as

.. math:: \Delta d = \sqrt{(\Delta T)^2 + (\Delta L)^2 + (\Delta\theta)^2}

and thus

.. math:: \frac{\Delta d}{d} = \sqrt{(\frac{\Delta T}{T})^2 + (\frac{\Delta L}{L})^2 + (\Delta\theta\cdot\cot(\theta))^2}

where,

-  :math:`\Delta T` is the time resolution from modulator;
-  :math:`\Delta\theta` is the coverage of the detector, and can be
   approximated from the square root of the solid angle of the detector
   to sample;
-  :math:`L` is the flight path of the neutron from source to detector.

Factor Sheet
------------

NOMAD
#####

Detector size

-  vertical: 1 meter / 128 pixel
-  Horizontal: half inch or 1 inch

POWGEN
######

Detector size: 0.005 x 0.0543

Range of :math:`\Delta\theta\cot\theta`: :math:`(0.00170783, 0.0167497)`


Usage
-----

**Example - estimate PG3 partial detectors' resolution:**

.. testcode:: ExHistSimple

  # Load a Nexus file
  Load(Filename="PG3_2538_2k.nxs", OutputWorkspace="PG3_2538")
  # Run the algorithm to estimate detector's resolution
  PDEstimateDetectorResolution(InputWorkspace="PG3_2538", DeltaTOF=40.0, OutputWorkspace="PG3_Resolution")
  resws = mtd["PG3_Resolution"]

  print "Size of workspace 'PG3_Resolution' = ", resws.getNumberHistograms()
  print "Estimated resolution of detector of spectrum 0 = ", resws.readY(0)[0]
  print "Estimated resolution of detector of spectrum 100 = ", resws.readY(100)[0]
  print "Estimated resolution of detector of spectrum 999 = ", resws.readY(999)[0]

.. testcleanup:: ExHistSimple

   DeleteWorkspace(resws)

Output:

.. testoutput:: ExHistSimple

  Size of workspace 'PG3_Resolution' =  1000
  Estimated resolution of detector of spectrum 0 =  0.00323913250277
  Estimated resolution of detector of spectrum 100 =  0.00323608373204
  Estimated resolution of detector of spectrum 999 =  0.00354849279137

.. categories::
