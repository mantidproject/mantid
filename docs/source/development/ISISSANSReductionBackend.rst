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

=============================== ======= ========================= ========= ===============
Name                            Comment Type                      Optional? Auto-generated? Default value
=============================== ======= ========================= ========= ===============
x_translation_correction        -       `FloatParameter`          Y         N
y_translation_correction        -       `FloatParameter`          Y         N
z_translation_correction        -       `FloatParameter`          Y         N
rotation_correction             -       `FloatParameter`          Y         N
side_correction                 -       `FloatParameter`          Y         N
radius_correction               -       `FloatParameter`          Y         N
x_tilt_correction               -       `FloatParameter`          Y         N
y_tilt_correction               -       `FloatParameter`          Y         N
z_tilt_correction               -       `FloatParameter`          Y         N
sample_centre_pos1              -       `FloatParameter`          Y         N
sample_centre_pos2              -       `FloatParameter`          Y         N
detector_name                   -       `StringWithNoneParameter` -         Y
detector_name_short             -       `StringWithNoneParameter` -         Y
=============================== ================================= ========= ========= =================

| Name  | Comment    | Type       | Is optional? | Is auto-generated? | Default value |
|-------|------------|------------|--------------|--------------------|---------------|
|x_translation_correction| - | `FloatParameter`| Yes | No | 0.0|
|y_translation_correction| - | `FloatParameter`| Yes | No | 0.0|
|z_translation_correction| - | `FloatParameter`| Yes | No | 0.0|
|rotation_correction| - | `FloatParameter`| Yes | No | 0.0|
|side_correction| - | `FloatParameter`| Yes | No | 0.0|
|radius_correction| - | `FloatParameter`| Yes | No | 0.0|
|x_tilt_correction| - | `FloatParameter`| Yes | No | 0.0|
|y_tilt_correction| - | `FloatParameter`| Yes | No | 0.0|
|z_tilt_correction| - | `FloatParameter`| Yes | No | 0.0|
|sample_centre_pos1| - | `FloatParameter`| Yes | No | 0.0|
|sample_centre_pos2| - | `FloatParameter`| Yes | No | 0.0|
|detector_name| -| `StringWithNoneParameter`| No | Yes | 0.0|
|detector_name_short| - | `FloatParameter`| No | Yes | 0.0|







.. code-block:: python

  from mantid.simpleapi import *



.. rubric:: Footnotes


.. categories:: Development
