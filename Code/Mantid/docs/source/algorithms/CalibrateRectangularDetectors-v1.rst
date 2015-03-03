.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Here are examples of input and output from PG3 and SNAP:

.. figure:: /images/PG3_Calibrate.png

.. figure:: /images/SNAP_Calibrate.png

The purpose of this algorithm is to calibrate the detector pixels and
write a calibration file. The calibration file name contains the
instrument, run number, and date of calibration. A binary Dspacemap file
that converts from TOF to d-space including the calculated offsets is
also an output option. For ``CrossCorrelation`` option: If one peak is not
in the spectra of all the detectors, you can specify the first n
detectors to be calibrated with one peak and the next n detectors to be
calibrated with the second peak. If a color fill plot of the calibrated
workspace does not look good, do a color fill plot of the workspace that
ends in cc (cross correlation) to see if the ``CrossCorrelationPoints``
and/or ``PeakHalfWidth`` should be increased or decreased. Also plot the 
reference spectra from the cc workspace.

Features to improve performance of peak finding
-----------------------------------------------

Define peak fit-window
######################

There are two exclusive approaches to define peak's fit-window.

- ``PeakWindowMax`` All peaks will use this value to define their fitting 
  range.
- ``FitwindowTableWorkspace`` This is a table workspace by which each peak 
  will have its individual fit window defined.

Define accepted range of peak's width
#####################################

Optional input property ``DetectorResolutionWorkspace`` is a matrix
workspace containing the detector resolution :math:`\Delta(d)/d` for
each spectrum. Combining with property ``AllowedResRange``, it defines the
lower and upper limit for accepted fitted peak width.

Let :math:`c_l = AllowedResRange[0]`, :math:`c_h = AllowedResRange[1]`
and :math:`fwhm` as the peak's fitted width. Then,

.. math:: c_l\times\frac{\Delta(d)}{d} < fwhm < c_h\times\frac{\Delta(d)}{d}

.. seealso :: Algorithm :ref:`algm-PDEstimateDetectorResolution`

.. categories::
