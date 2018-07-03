.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Divides the data spectra by the matching vanadium spectra according to
the following formula:

:math:`(y_i)_{Norm}=(\frac{y_i}{v_i})*\Delta w_i*\frac{\sum_i{y_i}}{\sum_i((\frac{y_i}{v_i})\Delta w_i)}`

where :math:`y_i` is the signal in the sample workspace, :math:`v_i` the
count in the corresponding vanadium bin, :math:`\Delta w_i` the bin
width, :math:`\sum_i{y_i}` the integrated data count and
:math:`\sum_i((\frac{y_i}{v_i})\Delta w_i)` the sum of the sample counts
divided by the vanadium counts multiplied by the bin width.

This leads to a normalised histogram which has unit of counts, as
before.

In order to minimise sudden jumps in the data caused by 0 counts in the
corresponding vanadium spectrum it is worth considering smoothing the
Vanadium spectrum using :ref:`algm-SmoothData` prior to using it in
this algorithm.

Valid input workspaces
######################

The input workspaces have to have the following in order to be valid
inputs for this algorithm.

-  The same number of spectra
-  Matching spectra numbers

This is normally not a problem unless the setup of the instrument has
been changed between recording the Vanadium and the sample datasets.

.. categories::

.. sourcelink::
