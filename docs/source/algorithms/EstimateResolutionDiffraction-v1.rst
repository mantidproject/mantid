.. algorithm::

.. summary::

.. relatedalgorithms::

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

.. math:: \Delta d = \sqrt{\left(\Delta T \frac{\partial d}{\partial T}\right)^2 + \left(\Delta L \frac{\partial d}{\partial L}\right)^2 + \left(\Delta \theta \frac{\partial d}{\partial \theta}\right)^2}

and thus

.. math:: \frac{\Delta d}{d} = \sqrt{\left(\frac{\Delta T}{T}\right)^2 + \left(\frac{\Delta L}{L}\right)^2 + \left(\Delta\theta\cdot\cot(\theta)\right)^2}

where,

-  :math:`\Delta T` is the time resolution from moderator
-  :math:`\Delta\theta` is the coverage of the detector, and can be
   approximated from the square root of the solid angle of the detector
   to sample
-  :math:`L` is the flight path of the neutron from source to detector
-  :math:`\theta` is half the Bragg angle :math:`2 \theta`, or half of the angle from the downstream beam

The optional ``DivergenceWorkspace`` specifies the values of
:math:`\Delta\theta` to use rather than those derived from the solid
angle of the detectors. :ref:`EstimateDivergence
<algm-EstimateDivergence>` can be used for estimating the divergence.

``PartialResolutionWorkspaces`` is a collection of partial resolution
functions where ``_tof`` is the time-of-flight term, ``_length`` is
the path length term, and ``_angle`` is the angular term. Note that
the total resolution is these terms added in quadriture.

Note that :math:`\frac{\Delta d}{d} = \frac{\Delta Q}{Q}`. When fitting peaks in time-of-flight the resolution is :math:`\frac{\Delta T}{T} = \frac{\Delta d}{d}`.

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
  EstimateResolutionDiffraction(InputWorkspace="PG3_2538", DeltaTOF=40.0, OutputWorkspace="PG3_Resolution",
                                PartialResolutionWorkspaces="PG3_Resolution_partials")
  resws = mtd["PG3_Resolution"]

  print("Size of workspace 'PG3_Resolution' =  {}".format(resws.getNumberHistograms()))
  print("Estimated resolution of detector of spectrum 0 =  {:.14f}".format(resws.readY(0)[0]))
  print("Estimated resolution of detector of spectrum 100 =  {:.14f}".format(resws.readY(100)[0]))
  print("Estimated resolution of detector of spectrum 999 =  {:.14f}".format(resws.readY(999)[0]))

.. testcleanup:: ExHistSimple

   DeleteWorkspace(resws)

Output:

.. testoutput:: ExHistSimple

  Size of workspace 'PG3_Resolution' =  1000
  Estimated resolution of detector of spectrum 0 =  0.00323913250277
  Estimated resolution of detector of spectrum 100 =  0.00323608373204
  Estimated resolution of detector of spectrum 999 =  0.00354849279137

.. seealso :: Algorithms :ref:`algm-EstimateDivergence`, :ref:`algm-CalibrateRectangularDetectors` and
   :ref:`algm-GetDetOffsetsMultiPeaks`

.. categories::

.. sourcelink::
