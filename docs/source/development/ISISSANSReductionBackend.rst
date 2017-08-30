.. _ISISSANSReductionBackend:

============================
ISIS SANS Reduction Back-end
============================

.. contents::
  :local:

General
#######

Introduction and Motivation
---------------------------

The ISIS SANS v2 reduction back-end is a more modern and updated version of the
original ISIS SANS reduction back-end which has been in use for almost 10 years.

Users who sets up a SANS reduction work-flow have control over a vast number of
settings (>50) in addition to settings which are extracted from the provided
workspaces and instrument specific settings. The total number of settings which
define a SANS data reduction can be close to 100.

The previous implementation of the SANS data reduction stored the settings
non-centrally and allowed the overall state to be mutable.
This made it extremely hard to reason about the overall state of a data
reduction and lead to unnecessary data reloads, degrading the overall
performance. The direct coupling of the state to the algorithms does not allow
for extending them to other facilities.

The current implementation aims to avoid these pitfalls and focusses on robustness,
maintainability and performance.

This new implementation of the SANS data reduction uses a simple state object
which stores the reduction-relevant information centrally and is immutable.
This *SANSState* approach is the corner stone of the new design.
The *SANSState* is:

- self-validating
- immmutable
- typed
- serializabvle
- easy to reason about
- modular (sub-states for units of work)


Overview
--------

The reduction back-end consists of three components:

- the *SANSState* approach to centrally store the state of the reduction
- a set of work-flow algorithms which perform the individual reduction steps
- a algorithm which orchestrates the work-flow algorithms.


*SANSState*
###########

Motivation
----------

As mentioned above, the amount of parameters that can be set by the user makes
the SANS reduction one of the more complex ones in the Mantid ecosystem. Previous
implementations stored the settings non-centrally which led to many difficult-to-find
bugs and a lot of uncertainty about the current settings of the reduction as they
were changed during the reduction.

This has been the main bottleneck of the previous reduction framework. To overcome
this the new implementation of the SANS data reduction uses a simple state object
which stores the reduction-relevant information centrally and is immutable.
This *SANSState* approach is the corner stone of the new design.

The *SANSState* is:

- self-validating
- immutable
- typed
- serializable
- easy to reason about
- modular (sub-states for units of work)

This approach allows us to identify issues with the settings before a lengthy
data reduction has been started.


Components
----------

This section describes the essential components of the state mechanism.
These include the states themselves, the parameters in a state and
the state construction.


`state_base.py`
^^^^^^^^^^^^^^^

The `state_base.py` module contains the essential ingredients for defining a
state object. These are the `StateBase` class which allows for serialization
and a set of `TypedParameter`s.

The `StateBase`'s `property_manager` property is responsible for serialization.
Due to the nature of the `PropertyManagerProperty` of algorithms it serializes
the state object to a Python dictionary and receives a Mantid `PropertyManager`
object.

States which want to fulfil the `StateBase` contract must override the
`validate` method. This method is used to ensure internal consistency
of the `TypedParameters` on the state. It is important to have comprehensive
and tight checks here.

The entries on the state objects are all of type `TypedParameter` which allows
for type checking, ensuring consistency early on. It is easy to
build custom types. The currently list of types are:

- `StringParameter`
- `BoolParameter`
- `FloatParameter`
- `PositiveFloatParameter`
- `PositiveIntegerParameter`
- `DictParameter`
- `ClassTypeParameter`
- `FloatWithNoneParameter`
- `StringWithNoneParameter`
- `PositiveFloatWithNoneParameter`
- `FloatListParameter`
- `StringListParameter`
- `PositiveIntegerListParameter`
- `ClassTypeListParameter`

Most of the  typed parameters are self-descriptive. The `ClassTypeParameter`
refers to the enum-like class definitions in `enum.py`. Note that if a parameter
is not set by the state builder, then it will return `None` when it is queried.
If it is a mandatory parameter on a state object, then this needs to be enforced
in the `validate` method of the state.


Individual states
^^^^^^^^^^^^^^^^^

The overall state object is made of sub state objects which carry all required
information for a single reduction step. This ensures that all the sub-states
are independent of each other carry all required information. Note that this
also means that some data is stored redundantly, for example the binning
for the wavelength conversion is stored in the state object used for
monitor normalization and in the state object for the transmission calculation.

In the following sections we list the different parameters on the currently
implemented states.


`state.py`
**********

The `State` class is the overarching state which contains sub-states where each
sub-state has a different responsibility (see below).

============= ==================================================== ====================
Name          Comment                                              State type
============= ==================================================== ====================
data          info about runs to use (most important state)        `StateData`
move          info about the instrument component positions        `StateMove`
reduction     general reduction info                               `StateReductionMode`
slice         info about event slicing (when applicable)           `StateSliceEvent`
mask          info about masking                                   `StateMask`
wavelength    info about wavelength conversion of the scatter data `StateWavelength`
save          info about the save settings                         `StateSave`
scale         info about the absolute scale and the sample volume  `StateScale`
adjustment    info about adjustment workspaces                     `StateAdjustment`
convert_to_q  info about momentum transfer conversion              `StateConvertToQ`
compatibility used when reducing in compatibility mode             `StateCompatibility`
============= ==================================================== ====================


`data.py`
*********

This is the most important state. Since the reduction framework has a data-driven
approach it is not possible to build up most of the reduction without knowing what
the actual data for the reduction will be.

=============================== ============================================== ====================================  ========= ===============
Name                            Comment                                        Type                                  Optional? Auto-generated?
=============================== ============================================== ====================================  ========= ===============
sample_scatter                  The sample scatter file path                   `StringParameter`                     N         N
sample_scatter_period           The period to use for the sample scatter       `PositiveIntegerParameter`            Y         N
sample_transmission             The sample transmission file path              `StringParameter`                     Y         N
sample_transmission_period      The period to use for the sample transmission  `PositiveIntegerParameter`            Y         N
sample_direct                   The sample direct file path                    `StringParameter`                     Y         N
sample_direct_period            The period to use for the sample direct        `PositiveIntegerParameter`            Y         N
can_scatter                     The can scatter file path                      `StringParameter`                     Y         N
can_scatter_period              The period to use for the can scatter          `PositiveIntegerParameter`            Y         N
can_transmission                The can transmission file path                 `StringParameter`                     Y         N
can_transmission_period         The period to use for the can transmission     `PositiveIntegerParameter`            Y         N
can_direct                      The can direct file path                       `StringParameter`                     Y         N
can_direct_period               The period to use for the can direct           `PositiveIntegerParameter`            Y         N
calibration                     The path to the calibration file               `StringParameter`                     Y         N
sample_scatter_run_number       Run number of the sample scatter file          `PositiveIntegerParameter`            -         Y
sample_scatter_is_multi_period  If the sample scatter is multi-period          `BoolParameter`                       -         Y
instrument                      Enum for the SANS instrument                   `ClassTypeParameter(SANSInstrument)`  -         Y
idf_file_path                   Path to the IDF file                           `StringParameter`                     -         Y
ipf_file_path                   Path to the IPF file                           `StringParameter`                     -         Y
=============================== ============================================== ==================================== ========= =================


Note that while some parameters are optional they might become mandatory if other
optional parameters have been specified. Also note that some of the parameters
on the state are auto-generated by the builder classes.

`move.py`
*********

The move state defines how instruments are moved. This is highly individual to
the different instruments. Therefore there is most likely going to be one state
per instrument, sometimes even more when there should be different behaviour for
different run numbers.

The fundamental class is `StateMove` which has the following parameters:

=============================== ======= ========================= ========= =============== =============
Name                            Comment Type                      Optional? Auto-generated? Default value
=============================== ======= ========================= ========= =============== =============
x_translation_correction        -       `FloatParameter`          Y         N               0.0
y_translation_correction        -       `FloatParameter`          Y         N               0.0
z_translation_correction        -       `FloatParameter`          Y         N               0.0
rotation_correction             -       `FloatParameter`          Y         N               0.0
side_correction                 -       `FloatParameter`          Y         N               0.0
radius_correction               -       `FloatParameter`          Y         N               0.0
x_tilt_correction               -       `FloatParameter`          Y         N               0.0
y_tilt_correction               -       `FloatParameter`          Y         N               0.0
z_tilt_correction               -       `FloatParameter`          Y         N               0.0
sample_centre_pos1              -       `FloatParameter`          Y         N               0.0
sample_centre_pos2              -       `FloatParameter`          Y         N               0.0
detector_name                   -       `StringWithNoneParameter` -         Y               -
detector_name_short             -       `StringWithNoneParameter` -         Y               -
=============================== ================================= ========= =============== ==============

If nothing is specified, then the detector positions and movements are assumed to be 0.
Note that each instrument contains additional parameters on their individual state classes. When adding
a new instrument, this will be most likely one of the main areas to add new code.

`reduction_mode.py`
*******************

The `StateReductionMode` class contains general settings about the reduction, e.g. if we are dealing with a merged
reduction. It contains the following parameters:

=============================== =================================================== ===========================================  ========= =============== ===========================================
Name                            Comment                                             Type                                         Optional? Auto-generated? Default value
=============================== =================================================== ===========================================  ========= =============== ===========================================
reduction_mode                  The type of reduction, ie LAB, HAB, merged or both `ClassTypeParameter(ReductionMode)`           N         N               `ISISReductionMode.LAB` enum value
reduction_dimensionality        If 1D or 2D reduction                              `ClassTypeParameter(ReductionDimensionality)` N         N               `ReductionDimensionality.OneDim` enum value
merge_fit_mode                  The fit mode for merging                           `ClassTypeParameter(FitModeForMerge)`         Y         N               `FitModeForMerge.NoFit` enum value
merge_shift                     The shift value for merging                        `FloatParameter`                              Y         N               0.0
merge_scale                     The scale value for merging                        `FloatParameter`                              Y         N               1.0
merge_range_min                 The min q value for merging                        `FloatWithNoneParameter`                      Y         N               `None`
merge_range_max                 The max q value for merging                        `FloatWithNoneParameter`                      Y         N               `None`
detector_names                  A dict from detector type to detector name         `DictParameter`                               N         Y               -
=============================== ================================================== ============================================ ========== =============== ============================================


`slice.py`
**********

The `StateSliceEvent` class is only relevant when we are dealing with event-type
data and the user decides to perform an event-sliced reduction, ie one reduction per event slice.

=========== ======================================= ========================= ========= ===============
Name        Comment                                 Type                      Optional? Auto-generated?
=========== ======================================= ========================= ========= ===============
start_time  A list of start times for event slices  `FloatListParameter`      Y         N
end_time    A list of stop times for event slices   `FloatListParameter`      Y         N
=========== ======================================= ========================= ========= ===============

Note that the validation ensures that the number of `start_time` and `end_time`
entries is matched and that the end time is larger than the start time.


`mask.py`
*********

The `StateMask` class holds information regarding time and pixel masking.
It also contains two sub-states which contain detector-specific masking information.
The `StateMask` contains the following parameters:

====================== ======================================================== ========================= ========= ===============
Name                   Comment                                                  Type                      Optional? Auto-generated?
====================== ======================================================== ========================= ========= ===============
radius_min             The min radius of a circular mask on the detector        `FloatParameter`          Y         N
radius_max             The max radius of a circular mask on the detector        `FloatParameter`          Y         N
bin_mask_general_start A list of start times for general bin masks               `FloatListParameter`     Y         N
bin_mask_general_stop  A list of stop times for general bin masks                `FloatListParameter`     Y         N
mask_files             A list of mask files                                      `StringListParameter`    Y         N
phi_min                The min angle of an angle mask                            `FloatParameter`         Y         N
phi_max                The max angle of an angle mask                            `FloatParameter`         Y         N
use_mask_phi_mirror    If the mirror slice should be used                        `BoolParameter`          Y         N
beam_stop_arm_width    The width of the beam stop arm                            `PositiveFloatParameter` Y         N
beam_stop_arm_angle    The angle of the beam stop arm                            `FloatParameter`         Y         N
beam_stop_arm_pos1     The x position of the beam stop arm                       `FloatParameter`         Y         N
beam_stop_arm_pos2     The y position of the bThe lower wavelength boundaream stop arm                       `FloatParameter`         Y         N
clear                  currently not used                                        `BoolParameter`          Y         N
clear_time             currently not used                                        `BoolParameter`          Y         N
detector               A dict of detector type to `StateMaskDetector` sub-states `DictParameter`          N         Y
idf_path               The path to the IDF                                       `StringParameter`        N         Y
====================== ======================================================== ========================= ========= ===============

Validation is applied to some of the entries.

The detector-specific settings are stored in the `StateMaskDetector` which contains the following parameters:

============================ ============ =============================== ========= ===============
Name                           Comment      Type                          Optional? Auto-generated?
============================ ============ =============================== ========= ===============
single_vertical_strip_mask   -            `PositiveIntegerListParameter`  Y         N
range_vertical_strip_start   -            `PositiveIntegerListParameter`  Y         N
range_vertical_strip_stop    -            `PositiveIntegerListParameter`  Y         N
single_horizontal_strip_mask -            `PositiveIntegerListParameter`  Y         N
range_horizontal_strip_start -            `PositiveIntegerListParameter`  Y         N
range_horizontal_strip_stop  -            `PositiveIntegerListParameter`  Y         N
block_horizontal_start       -            `PositiveIntegerListParameter`  Y         N
block_horizontal_stop        -            `PositiveIntegerListParameter`  Y         N
block_vertical_start         -            `PositiveIntegerListParameter`  Y         N
block_vertical_stop          -            `PositiveIntegerListParameter`  Y         N
block_cross_horizontal       -            `PositiveIntegerListParameter`  Y         N
block_cross_vertical         -            `PositiveIntegerListParameter`  Y         N
bin_mask_start               -            `FloatListParameter`            Y         N
bin_mask_stop                -            `FloatListParameter`            Y         N
detector_name                -            `StringParameter`               Y         N
detector_name_short          -            `StringParameter`               Y         N
single_spectra               -            `PositiveIntegerListParameter`  Y         N
spectrum_range_start         -            `PositiveIntegerListParameter`  Y         N
spectrum_range_stop          -            `PositiveIntegerListParameter`  Y         N
============================ ============ =============================== ========= ===============

Again the detector-specific settings contain multiple validation steps on the state.


`wavelength.py`
**************
The `StateWavelength` class contains the information required to perform the conversion of the scatter data
 from time-of-flight to wavelength units. The parameters are:

===================== ==================================== =================================== ========= ===============
Name                  Comment                              Type                                Optional? Auto-generated?
===================== ==================================== =================================== ========= ===============
rebin_type            The type of rebinning                `ClassTypeParameter(RebinType)      N         N
wavelength_low        The lower wavelength boundary        `PositiveFloatParameter`            N         N
wavelength_high       The upper wavelength boundary        `PositiveFloatParameter`            N         N
wavelength_step       The wavelength step                  `PositiveFloatParameter`            N         N
wavelength_step_type  This is either linear or logarithmic `ClassTypeParameter(RangeStepType)` N         N
===================== ==================================== =================================== ========= ===============

The validation ensures that all entries are specified and that the lower wavelength boundary is smaller than the upper wavelength boundary.

`save.py`
*********

The `StateSave` class does not hold information which is direclty related to the reduction but contains the all the required information about saving the reduced data. The relevant parameters are:

================================== ================================================== =================================== ========= =============== =======
Name                               Comment                                            Type                                Optional? Auto-generated? Default
================================== ================================================== =================================== ========= =============== =======
zero_free_correction               If zero error correction (inflation) should happen `BoolParameter`                     Y         N               True
file_format                        A list of file formats to save into                `ClassTypeListParameter(SaveType)`  Y         N               True
user_specified_output_name         A custom user-specified name for the saved file    `StringWithNoneParameter`           Y         N               -
user_specified_output_name_suffix  A custom user-specified suffix for the saved file  `StringParameter`                   Y         N               -
use_reduction_mode_as_suffix       If the reduction mode should be used as a suffix   `BoolParameter`                     Y         N               -
================================== ================================================== =================================== ========= =============== =======


`scale.py`
*********

The `StateScale` class contains the information which is required for the absolute value scaling
and the volume information. The parameters are:


===================== ======================================== ================================== ========= ===============
Name                  Comment                                  Type                               Optional? Auto-generated?
===================== ======================================== ================================== ========= ===============
shape                 The user-specified shape of the sample   `ClassTypeParameter(SampleShape)`  N         Y
thickness             The user-specified sample thickness      `PositiveFloatParameter`           N         Y
width                 The user-specified sample width          `PositiveFloatParameter`           N         Y
height                The user-specified sample height         `PositiveFloatParameter`           N         Y
scale                 The user-specified absolute scale        `PositiveFloatParameter`           N         Y
shape_from_file       The file-extracted shape of the sample   `ClassTypeParameter(SampleShape)`  N         Y
thickness_from_file   The file-extracted sample thickness      `PositiveFloatParameter`           N         Y
width_from_file       The file-extracted sample width          `PositiveFloatParameter`           N         Y
height_from_file      The file-extracted sample height         `PositiveFloatParameter`           N         Y
===================== ======================================== ================================== ========= ===============


`adjustment.py`
***************

Adjustment workspaces are generated to be consumed in the momentum transfer conversion step.
There are three types of adjustments

- Pure wavelength adjustments, ie adjustments which only affect the bins
- Pure pixel adjustments, ie adjustments which only affect the spectra
- Pixel-and-wavelength adjustments, ie adjustments which affect both the bins and spectra

The `StateAdjustment` class is a composite state which is made of information
relating to the different types of adjustments

The parameters are:

================================ ==================================================== =================================================== ========= =============== =======
Name                             Comment                                              Type                                                Optional? Auto-generated? Default
================================ ==================================================== =================================================== ========= =============== =======
calculate_transmission           Information for the transmission calculation         `TypedParameter(StateCalculateTransmission)`        N         N               -
normalize_to_monitor             Information for the monitor normalization            `TypedParameter(StateNormalizeToMonitor)`           N         N               -
wavelength_and_pixel_adjustment  Information for combining different adjustment       `TypedParameter(StateWavelengthAndPixelAdjustment)` N         N               -
wide_angle_correction            If wide angle calculation should be performed.       `BoolParameter`                                     Y         N               False
                                 Note that this will produce the pixel-and-wavelength
                                 adjustment
================================ ==================================================== =================================================== ========= =============== =======


The transmission calculation state
----------------------------------

The transmission calculation produces one of the wavelength adjustment workspaces.
This reduction step is one of the more complicated bits of the reduction and hence has a
large variety of settings. The `StateCalculateTransmission` class contains the
following parameters parameters are:

================================ ================================================================================================ =============================== ========= =============== =======
Name                             Comment                                                                                          Type                            Optional? Auto-generated? Default
================================ ================================================================================================ =============================== ========= =============== =======
transmission_radius_on_detector  A radius around the beam centre for transmission ROI on the bank                                 `PositiveFloatParameter`        Y         N               -
transmission_roi_files           A list of ROI files for transmission ROI on the bank                                             `StringListParameter`           Y         N               -
transmission_mask_files          A list of mask files for transmission ROI on the bank                                            `StringListParameter`           Y         N               -
default_transmission_monitor     The default transmission monitor (if nothing else has been specified)                            `PositiveIntegerParameter`      N         Y               -
transmission_monitor             The relevant transmission monitor (if no ROI is being used)                                      `PositiveIntegerParameter`      Y         N               -
default_incident_monitor         The default incident monitor (if nothing else has been specified)                                `PositiveIntegerParameter`      N         Y
incident_monitor                 The incident monitor                                                                             `PositiveIntegerParameter`      Y         N               -
prompt_peak_correction_min       The start time of a prompt peak correction                                                       `PositiveFloatParameter`        Y         N               -
prompt_peak_correction_max       The stop time of a prompt peak correction                                                        `PositiveFloatParameter`        Y         N               -
prompt_peak_correction_enabled   If the prompt peak correction should occur                                                       `BoolParameter`                 Y         N               True
rebin_type                       The type of wavelength rebinning, ie standard or interpolating                                   `ClassTypeParameter(RebinType)` Y         N               -
wavelength_low                   The lower wavelength boundary                                                                    `PositiveFloatParameter         Y         N               -
wavelength_high                  The upper wavelength boundary                                                                    `PositiveFloatParameter         Y         N               -
wavelength_step                  The wavelength step                                                                              `PositiveFloatParameter         Y         N               -
wavelength_step_type             The wavelength step type, ie lin or log                                                          `ClassTypeParameter(RebinType)` Y         N               -
use_full_wavelength_range        If the full wavelength range of the instrument should be used                                    `BoolParameter`                 Y         N               -
wavelength_full_range_low        The lower wavelength boundary of the full wavelength range                                       `PositiveFloatParameter`        Y         N               -
wavelength_full_range_high       The upper wavelength boundary of the full wavelength range                                       `PositiveFloatParameter`        Y         N               -
background_TOF_general_start     General lower boundary for background correction                                                 `FloatParameter`                Y         N               -
background_TOF_general_stop      General upper boundary for background correction                                                 `FloatParameter`                Y         N               -
background_TOF_monitor_start     Monitor specific lower boundary for background correction (monitor vs. start value)              `DictParameter`                 Y         N               -
background_TOF_monitor_stop      Monitor specific upper boundary for background correction (monitor vs. stop value)               `DictParameter`                 Y         N               -
background_TOF_roi_start         Lower bound of background correction when using ROI on detector                                  `FloatParameter`                Y         N               -
background_TOF_roi_stop          Upper bound of background correction when using ROI on detector                                  `FloatParameter`                Y         N               -
fit                              A dict for each data type (sample and can) to the state of fit settings (`StateTransmissionFit`) `DictParameter`                 Y         N               -
================================ ================================================================================================ =============================== ========= =============== =======



.. code-block:: python

  from mantid.simpleapi import *



.. rubric:: Footnotes


.. categories:: Development
