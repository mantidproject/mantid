.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Stitches single histogram :ref:`Matrix Workspaces <MatrixWorkspace>`
together outputting a stitched Matrix Workspace. Note that workspaces must be histogrammed, you may
want to use :ref:`algm-ConvertToHistogram` on workspaces prior to passing them to this algorithm.

Either the right-hand-side or left-hand-side workspace can be chosen to be scaled.
Users can optionally provide :ref:`algm-Rebin` :literal:`Params`, otherwise they are calculated from the input workspaces.
Likewise, :literal:`StartOverlap` and :literal:`EndOverlap` are optional. If not provided, then these
are taken to be the region of X-axis intersection.

The algorithm workflow is as follows:

#. The workspaces are initially rebinned, as prescribed by the rebin :literal:`Params`. Note that
   rebin parameters are determined automatically if not provided. In this case, the step size is
   taken from the step size of the LHS workspace (or from the RHS workspace if :literal:`ScaleRHSWorkspace`
   was set to false) and rebin boundaries are taken from the minimum X value in the LHS workspace
   and the maximum X value in the RHS workspace respectively.
#. After rebinning, each spectrum is searched for special values. Special values refer to signal
   (Y) and error (E) values that are either infinite or NaN. These special values are masked out
   as zeroes and their positions are recorded for later reinsertion.
#. Next, if :literal:`UseManualScaleFactor` was set to false, both workspaces will be integrated
   according to the integration range defined by :literal:`StartOverlap` and :literal:`EndOverlap`.
   Note that the integration is performed without special values, as those have been masked out
   in the previous step. The scale factor is then calculated as the quotient of the integral of
   the left-hand-side workspace by the integral of the right-hand-side workspace (or the quotient
   of the right-hand-side workspace by the left-hand-side workspace if :literal:`ScaleRHSWorkspace`
   was set to false), and the right-hand-side workspace (left-hand-side if :literal:`ScaleRHSWorkspace`
   was set to false) is multiplied by the calculated factor.
#. Alternatively, if :literal:`UseManualScaleFactor` was set to true, the scale factor is applied
   to the right-hand-side workspace (left-hand-side workspace if :literal:`ScaleRHSWorkspace` was
   set to false).
#. The weighted mean of the two workspaces in range [:literal:`StartOverlap`, :literal:`EndOverlap`]
   is calculated. Note that if both workspaces have zero errors, an un-weighted mean will be
   performed instead.
#. The output workspace will be created by summing the left-hand-side workspace (values in range
   [:literal:`StartX`, :literal:`StartOverlap`], where :literal:`StartX` is the minimum X value
   specified via :literal:`Params` or calculated from the left-hand-side workspace) + weighted
   mean workspace + right-hand-side workspace (values in range [:literal:`EndOverlap`, :literal:`EndX`],
   where :literal:`EndX` is the maximum X value specified via :literal:`Params` or calculated
   from the right-hand-side workspace) multiplied by the scale factor.
#. The special values are put back in the output workspace. Note that if both the left-hand-side
   workspace and the right-hand-side workspace happen to have a different special value in the same bin, this
   bin will be set to infinite in the output workspace.

Below is a flowchart illustrating the steps in the algorithm (it assumes :literal:`ScaleRHSWorkspace`
is true). Figure on the left corresponds
to the workflow when no scale factor is provided, while figure on the right corresponds to
workflow with a manual scale factor specified by the user.

.. diagram:: Stitch1D-v3.dot

Error propagation
#################

Errors are are handled and propagated in every step according to :ref:`Error Propagation`. This
includes every child algorithm: :ref:`algm-Rebin`, :ref:`algm-Integration`, :ref:`algm-Divide`,
:ref:`algm-Multiply` and :ref:`algm-WeightedMean`. In particular, when the scale factor is calculated
as the quotient of the left-hand-side integral and the right-hand-side integral, the result is
a number with an error associated, and therefore the multiplication of the right-hand-side
workspace by this number takes into account its error.

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

.. sourcelink::
    :filename: Stitch1D