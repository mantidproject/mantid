.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Reduces a single TOF reflectometry run into a mod Q vs I/I0 workspace.
Performs transmission corrections. Handles both point detector and
multidetector cases. The algorithm can correct detector locations based
on an input theta value.

Historically the work performed by this algorithm was known as the Quick
script.

Analysis Modes
##############

The default analysis mode is *PointDetectorAnalysis*. Only this mode
supports Transmission corrections (see below). For PointAnalysisMode the
analysis can be roughly reduced to IvsLam = DetectorWS / sum(I0) /
TransmissionWS / sum(I0). The normalization by tranmission run(s) is
optional. If necessary, input workspaces are converted to *Wavelength*
first via :ref:`algm-ConvertUnits`.

IvsQ is calculated via :ref:`algm-ConvertUnits` into units of
*MomentumTransfer*. Corrections may be applied prior to the
transformation to ensure that the detectors are in the correct location
according to the input Theta value. Corrections are only enabled when a
Theta input value has been provided.

Transmission Runs
#################

Transmission correction is a normalization step, which may be applied to
*PointDetectorAnalysis* reduction.

Transmission runs are expected to be in TOF. The spectra numbers in the
Transmission run workspaces must be the same as those in the Input Run
workspace. If two Transmission runs are provided then the Stitching
parameters associated with the transmission runs will also be required.
If a single Transmission run is provided, then no stitching parameters
will be needed.

.. categories::
