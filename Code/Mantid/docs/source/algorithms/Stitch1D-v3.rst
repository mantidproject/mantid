.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Stitches single histogram `Matrix Workspaces <http://www.mantidproject.org/MatrixWorkspace>`_
together outputting a stitched Matrix Workspace. Either the
right-hand-side or left-hand-side workspace can be chosen to be scaled.
Users must provide a Param step (single value), but the binning start
and end are calculated from the input workspaces if not provided.
Likewise, StartOverlap and EndOverlap are optional. If the StartOverlap
or EndOverlap are not provided, then these are taken to be the region of
x-axis intersection.

The workspaces must be histogrammed. Use
:ref:`algm-ConvertToHistogram` on workspaces prior to
passing them to this algorithm.

Usage
-----
**Example - a basic example using stitch1D to stitch two workspaces together.**

.. testcode:: ExStitch1DSimple

    import numpy as np

    def gaussian(x, mu, sigma):
      """Creates a gaussian peak centered on mu and with width sigma."""
      return (1/ sigma * np.sqrt(2 * np.pi)) * np.exp( - (x-mu)**2  / (2*sigma**2))

    #create two histograms with a single peak in each one
    x1 = np.arange(-1, 1, 0.02)
    x2 = np.arange(0.4, 1.6, 0.02)
    ws1 = CreateWorkspace(UnitX="1/q", DataX=x1, DataY=gaussian(x1[:-1], 0, 0.1)+1)
    ws2 = CreateWorkspace(UnitX="1/q", DataX=x2, DataY=gaussian(x2[:-1], 1, 0.05)+1)

    #stitch the histograms together
    stitched, scale = Stitch1D(LHSWorkspace=ws1, RHSWorkspace=ws2, StartOverlap=0.4, EndOverlap=0.6, Params=0.02)

Output:

.. image:: /images/Stitch1D1.png
   :scale: 65 %
   :alt: Stitch1D output
   :align: center


**Example - a practical example using reflectometry data and a scale factor.**

.. testcode:: ExStitch1DPractical

    trans1 = Load('INTER00013463')
    trans2 = Load('INTER00013464')

    trans1_wav = CreateTransmissionWorkspaceAuto(trans1)
    trans2_wav = CreateTransmissionWorkspaceAuto(trans2)

    stitched_wav, y = Stitch1D(trans1_wav, trans2_wav, UseManualScaleFactor=True, ManualScaleFactor=0.85)

Output:

.. image:: /images/Stitch1D2.png
   :scale: 65 %
   :alt: Stitch1D output
   :align: center


.. categories::
