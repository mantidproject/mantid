.. _FractionalRebinning:

================================================
Fractional Rebinning (RebinnedOutput Workspaces)
================================================

.. contents::
  :local:

In some instances of rebinning data, the information of the counts in the bins
prior rebinning should be preserved in the output of rebinning in some way.
This is because some algorithms depend on how the original data was treated.
We achieve this by storing a weight for each bin that corresponds to the fraction
between the area of the output bin and the area of the input bin (i.e. prior to rebinning).
This kind of **Workspace2D** that contains information of the areas used during rebinning
is called a **RebinnedOutput**. Some algorithms, such as
:ref:`SofQWNormalisedPolygon <algm-SofQWNormalisedPolygon>`
and :ref:`ConvertToReflectometryQ <algm-ConvertToReflectometryQ>`, create this special type of
:ref:`Workspace2D <Workspace2D>` workspace in which
each bin contains both a value and the fractional overlap area of the this bin over
that of the original data. This fractional overlap area value is referred to as
a fractional weight, since it has the role of controlling the proportion of the counts
in output bins that underwent some coordinate transformation.

To illustrate why using these fractional weights is necessary we explain how rebinning works
through the technique of *NormalisedPolygon*.

NormalisedPolygon Technique
---------------------------

*NormalisedPolygon* constructs
a polygon using the boundaries of the input bin, and transforms that polygon
into the output coordinates. In the output coordinates, we look for the intersections
between the transformed input bin and the output bin:

.. figure:: /images/RebinnedOutputStep1.png
   :align: center

As shown in the figure, the input bin (pink-shaded parallelopiped)
has been transformed to the output bin coordinates and so is not parallel to the
output grid.
This means the output bins will only ever partially overlap the input data.
The resulting value of the output bin is proportional to the
overlap between the purple area :math:`I_0` and the pink-shaded area :math:`A_0`.
In the general case where each output bin intersects several input bins,
the signal :math:`Y` and errors :math:`E` in the new bins are calculated as:

.. math:: Y^{\mathrm{new}} = \sum_i Y^{\mathrm{old}}_i F_i
.. math:: E^{\mathrm{new}} = \sqrt{\sum_i (E^{\mathrm{old}}_i)^2 F_i}

And the fractional weight of the new bin is stored as the sum of all the
weights with intersecting input areas:

.. math:: F^{\mathrm{new}} = \sum_i F_i

The output data will be a workspace with type: **RebinnedOutput**, which means that
it is a **Workspace2D** with the information of the weigthts on the bins.
To see why this is needed, consider the case where another rebin is perfomed after the
previous rebinning. This time the output bin sits on a larger grid:

.. figure:: /images/RebinnedOutputStep2.png
   :align: center

If the fractional weight of the initial rebin were not stored, this
rebin operation would have :math:`F_0=1`, and the resulting weight :math:`F_1`
would be overestimated.
In other words, if the fractional weights are not chained as shown, then the area
shaded in a lighter blue under :math:`A_1` (where originally there was
no data) would be included in the weights, which would lead to an
overestimate of the actual weights, and ultimately to an overestimate of the
signal and error.

Rebin2D on RebinnedOutput workspaces
------------------------------------

As mentioned before, the distinction between Fractional Rebinning
and normal Rebinning lies in the storage of the fractional weights relative to the original data.

**RebinnedOutput** workspaces have the fractional weight resulting from previous rebins stored
for each input bin. When :ref:`algm-Rebin2D` is called on RebinnedOutput workspaces, the argument
*UseFractionalArea* is always automatically turned on, to ensure the weight fractions are alwasy propagated accross
several rebins and that the best possible signal and error estimates are achieved.

On the other hand, when :ref:`algm-Rebin2D` is called on a **Workspace2D** workspace,
by default the argument *UseFractionalArea* is set to False, and no fractional weights
relative to the original bins are used. If you explicitly set *UseFractionalArea* to True,
then the algorithm assumes that the Rebin being performed is the very first Rebin on the
original data, and any fractional weights are set to unity to ensure that the new weights
calculted on this rebinning are the first fractional weights stored in the output.
The resulting **RebinnedOutput** workspace will be equal to the **Workspace2D** output of a Rebin
that does not use fractional weights, with the only difference being that **RebinnedOutput** has
started tracking the fractional weights to be propagated through any further rebins.

If you try to check this is the case with the following script, however, you will be
surprised to discover that this is seemingly not the case:

.. testcode:: ExRebinTwice

    import numpy as np
    # prepare an input workspace
    theta_tof = CreateSampleWorkspace()
    theta_tof = ConvertSpectrumAxis(theta_tof, "theta")

    theta_tof_fa_false = Rebin2D(theta_tof, '100,400,20000', '0, 0.004, 1', UseFractionalArea=False)
    theta_tof_fa_true = Rebin2D(theta_tof,  '100,400,20000', '0, 0.004, 1', UseFractionalArea=True)
    print(f'Signal difference = {np.median(np.abs(theta_tof_fa_true.readY(0) - theta_tof_fa_false.readY(0))):.3f}')
    print(f'Errors difference = {np.median(np.abs(theta_tof_fa_true.readE(0) - theta_tof_fa_false.readE(0))):.3f}')

.. testoutput:: ExRebinTwice

    Signal difference = 0.195
    Errors difference = 0.603

This discrepancy is actually only present for display purposes, and is not part of
the inner workings of the code. This behaviour was chosen to cover the cases where
the output grid has a very small overlap with the input grid (for example at the edges of the
detector coverage), resulting in a small fractional weight :math:`F` of this bin, and
hence its signal :math:`Y` and error :math:`E` would also be very small compared to its neighbours.
Thus, for display purposes, the actual signal and errors stored internally in a RebinnedOutput are
displayed in the workspace by renormalising by the fractional weights:

.. math:: Y^{\mathrm{display}} = Y^{\mathrm{new}} / F^{\mathrm{new}}
.. math:: E^{\mathrm{display}} = E^{\mathrm{new}} / F^{\mathrm{new}}

at the end of the algorithm. The biggest consequence of this method is
that in places where there are no counts (:math:`Y=0`) and no acceptance
(no fractional areas, :math:`F=0`), :math:`Y/F=`\ **nan**\ -s will
result.

Integration on RebinnedOutput workspaces
----------------------------------------

The :ref:`algm-Integration` algorithm operates differently on **RebinnedOutput** workspaces and
**Workspace2D** workspaces. For **Workspace2D** workspaces, the integrated counts per spectra is simply the
sum of the counts in the bins within the Integration range:

.. math::
   I = \left. \sum_i Y_i \right.

In the case of **RebinnedOutput**, we take into the accout the fractional area weights :math:`F_i`:

.. math::
   I = \left. \sum_i Y_i F_i \middle/ \left(\frac{1}{n} \sum_i F_i \right) \right.

where :math:`Y_i` and :math:`F_i` are the values and fractions for the :math:`i^{\mathrm{th}}`
bin and :math:`n` is the number of bins in the range which is not ``NaN``.
We can check that the factor :math:`1/n` is needed by looking at the special case where the fractional
weights are all set to :math:`F_i = 1`. In this case, the result of the integral yields
:math:`\sum_i Y_i`, which is what we expect for an integral over bins with no fractional area weights.

.. categories:: Concepts
