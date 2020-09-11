.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm is used to remove peaks from the input data, leaving the background intact.


The input data can be optionally smoothed using a linear weighted average based on the value of
the SmoothingRange parameter.

Smaller peaks present in the data can be exaggerated before clipping with the LLSCorrection option,
which applies a log-log-sqrt function on the data. The inverse function is applied after the peaks
are clipped.

Smoothing
#########

If the smoothing window (s) is greater than 0, then the input is smoothed based on the following:

.. math::

    Y_{i}^{s} = \frac{ \sum_{r=-r_{max}}^{r_{max}} (r_{max} - \vert r \vert ) \cdot Y_{i+r} }{ \sum_{r=-r_{max}}^{r_{max}} (r_{max} - \vert r \vert ) }

where :math:`r_{max} = \frac{s}{2} + 1`.

Clipping
########

Peak clipping is done iteratively over the window size, :math:`w`. The order in which the data is clipped depends on
whether an increasing or decreasing window is chosen: :math:`s = [1, 2, \cdots , w]` or :math:`s = [w, w-1, \cdots , 1]`.

.. math::

    Y_{i}^{(k)} = \min \left( \frac{ \left[ Y_{i-s_k}^{(k-1)}, \cdots , Y_{i+s_k}^{(k-1)} \right] + \left[ Y_{i+s_k}^{(k-1)}, \cdots , Y_{i-s_k}^{(k-1)} \right] }{ 2 } \right )

where :math:`(k)` is the current iteration of :math:`Y_{i}`. If the LLS transformation is enabled, the inverse function
is applied before doing the last operation:

.. math::

    Y_{i} = Y_{i}\  \cdot \ \left(\frac{ Y_{\min ( Y^{in} - Y )}^{in} }{ Y_{\min (Y^{in} - Y)} }\right)


LLS Transformation
##################

If the "LLSCorrection" option is true, then the input :math:`Y` is transformed by the following before clipping is done:

.. math::

    Y^{\prime} = \log \left( \log \left( \sqrt{(Y + 1)} + 1 \right) + 1 \right)

After clipping, the inverse function is applied:

.. math::

    Y = ( \exp \, ( \exp( Y \, ) - 1 ) - 1)^2 - 1


Usage
-----

Example usage of using this algorithm on some data with random background noise and large peaks:

.. image:: /images/ClipPeaks_RandNoise_before.png
    :width: 50 %

.. image:: /images/ClipPeaks_RandNoise_after.png
    :width: 50 %

.. categories::

