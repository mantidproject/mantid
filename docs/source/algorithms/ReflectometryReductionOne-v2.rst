.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm reduces a single reflectometry run into a mod Q vs I/I0 workspace.
The mandatory input properties must be manually set by the user. In addition, for
the algorithm to be able to correctly convert from wavelength to momentum transfer,
instrument components must be located at the correct positions. This algorithm
expects the following experimental setup:



The figure below displays a high-level workflow diagram illustrating the main
steps taking place in the reduction.

.. diagram:: ReflectometryReductionOne_HighLvl-v2_wkflw.dot

Conversion to Wavelength
########################

First, the algorithm checks if the X units of
the input workspace. If the input workspace is already in wavelength, normalization by
monitors and direct beam is not performed, as it is considered that the input run was
already reduced using this algorithm. If the input workspace is in TOF, it will be
converted to wavelength (note that :literal:`AlignBins` will be set to :literal:`True` for this in 
:ref:`algm-ConvertUnits`), cropped according to
:literal:`WavelengthMin` and :literal:`WavelengthMax`, which are both mandatory properties, and the detectors
of interest, specified via :literal:`ProcessingInstructions`, will be grouped together using
:ref:`algm-GroupDetectors`. **Optionally**, the algorithm will perform direct beam
normalization (if :literal:`RegionOfDirectBeam` is specified) dividing the detectors of
interest by the direct beam, and monitor normalization (if :literal:`I0MonitorIndex` and
:literal:`MonitorBackgroundWavelengthMin` and :literal:`MonitorBackgroundWavelengthMax` are all specified),
in which case the detectors of interest will be divided by monitors. A summary of the
steps taking place in the reduction is shown below.

.. diagram:: ReflectometryReductionOne_ConvertToWavelength-v2_wkflw.dot

Transmission Correction
#######################

Transmission corrections can be optionally applied to the workspace resulting
from the previous step. Transmission corrections can be either specified via
transmission runs or specific correction algorithms.

.. diagram:: ReflectometryReductionOne_TransmissionCorrection-v2_wkflw.dot

When transmission runs are given, the spectrum numbers in the
transmission workspaces must be the same as those in the input run
workspace. If two transmission runs are provided then the stitching
parameters associated with the transmission runs will also be required.
If a single transmission run is provided, then no stitching parameters
will be needed.

If no transmission runs are provided, then polynomial correction can be
performed instead. Polynomial correction is enabled by setting the
:literal:`CorrectionAlgorithm` property. If set to
:literal:`PolynomialCorrection` it runs the :ref:`algm-PolynomialCorrection`
algorithm, with this algorithms :literal:`Polynomial` property used as its
:literal:`Coefficients` property. If the :literal:`CorrectionAlgorithm` property is set to
:literal:`ExponentialCorrection`, then the :Ref:`algm-ExponentialCorrection`
algorithm is used, with C0 and C1 taken from the :literal:`C0` and :literal:`C1`
properties.

Conversion to Momentum Transfer (Q)
###################################

Finally, the output workspace in wavelength is converted into momentum transfer (Q).
Optionally, this workspace can be rebinned according to :literal:`MomentumTransferMin`,
:literal:`MomentumTransferStep` and :literal:`MomentumTransferMax` and scaled if
:literal:`ScaleFactor` is given.

.. diagram:: ReflectometryReductionOne_ConvertToMomentum-v2_wkflw.dot


Usage
-----

.. categories::

.. sourcelink::
