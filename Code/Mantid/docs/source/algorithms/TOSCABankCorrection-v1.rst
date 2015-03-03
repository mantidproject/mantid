.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm attempts to automatically correct TOSCA data in which the
position of the sample has been moved and has affected the alignment of features
on the spectra from forward and backscattering detector banks.

The input workspace should be an energy transfer reduction, for the default
values of SearchRange and ClosePeakTolerance the X axis is assumed to be in
cm-1, however the X axis is not restricted to this unit.

The algorithm works by finding peaks of a given shape (using the :ref:`FindPeaks
<algm-FindPeaks>`) on both the forward and backscattering banks and attempting
to match them to what is believed to be the same feature on the other bank.

The choice can then be made to either take the difference in peak centre between
the tallest matched peaks or to average the difference of all matched peaks to
determine the mount that both spectra must be moved by to correct for the change
in sample position.

The data is then corrected by providing a shift in the X axis of each bank
spectra (using :ref:`ConvertAxisByFormula <algm-ConvertAxisByFormula>`) to bring
the two individual bank spectra back into alignment.

The corrected spectra are then rebinned to the input workspace (using
:ref:`RebinToWorkspace <algm-RebinToWorkspace>`) to preserve the X range and to
maintain bin alignment.

.. categories::
