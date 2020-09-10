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

Usage
-----

Example usage of using this algorithm on some data with random background noise and large peaks:

.. image:: /images/ClipPeaks_RandNoise_before.png
    :width: 50 %

.. image:: /images/ClipPeaks_RandNoise_after.png
    :width: 50 %

.. categories::

