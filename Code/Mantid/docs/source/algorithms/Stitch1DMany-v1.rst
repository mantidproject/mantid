.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Stitches single histogram :ref:`Matrix Workspaces <MatrixWorkspace>`
together outputting a stitched Matrix Workspace. This algorithm is a
wrapper over :ref:`algm-Stitch1D`.

The algorithm expects pairs of StartOverlaps and EndOverlaps values. The
order in which these are provided determines the pairing. There should
be N entries in each of these StartOverlaps and EndOverlaps lists, where
N = 1 -(No of workspaces to stitch). StartOverlaps and EndOverlaps are
in the same units as the X-axis for the workspace and are optional.

The workspaces must be histogrammed. Use
:ref:`algm-ConvertToHistogram` on workspaces prior to
passing them to this algorithm.

Usage
-----
**Example - a basic example using Stitch1DMany to stitch two workspaces together.**

.. testcode:: ExStitch1DManySimple

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
    workspaces = ws1.name() + "," + ws2.name()
    stitched, scale = Stitch1DMany(InputWorkspaces=workspaces, StartOverlaps=[0.4], EndOverlaps=[0.6], Params=[0.02])

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

  workspaces = trans1_wav.name() + ',' + trans2_wav.name()
  stitched_wav, y = Stitch1DMany(workspaces, params='1, 0.05, 17', UseManualScaleFactor=True, ManualScaleFactor=0.85)

Output:

.. image:: /images/Stitch1D2.png
   :scale: 65 %
   :alt: Stitch1D output
   :align: center

.. categories::
