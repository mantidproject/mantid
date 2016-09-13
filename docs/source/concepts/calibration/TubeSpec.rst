.. _TubeSpec:

TubeSpec
========

The python class **TubeSpec** provides a way of specifying a set of
tubes for calibration, so that the necessary information about detectors
etc. is forthcoming. This class is provide by the python file
**tube_spec.py**. The function **getCalibration** of
:ref:`Tube Calib <Tube_calib>` needs such an object.

Constructor
-----------

The construct TubeSpec takes one argument, which is a workspace that
contains the instrument with tubes to be selected.

.. _tubespec-setTubeSpecByString:

setTubeSpecByString
-------------------

The function takes one argument **tubeSpecString** and it is a string
specifying the whole instrument or a component of the instrument
containing the tubes you wish to calibrate.

It may be the full path name for the component or steps may be missed
out provided it remains unique.

For example panel03 of WISH can be specified by just **panel03**,
because panel03 is unique within the instrument. Also tube012 of this
panel, which is unique within the panel but not within the instrument
can be specified by **panel03/tube012** but not **tube012**. If the
specification is not unique, the first found will be used and there will
be no error message. So if in doubt don't skip a step.

SetTubeSpecByStringArray
------------------------

This function allows you to calibrate a set of tubes that is not defined
by a single component. For example a set of windows. It takes an array
of strings as its argument. Each string specifies a component such as a
window or a single tube in the same manner as for
:ref:`setTubeSpecByString <tubespec-setTubeSpecByString>`. The components must be
disjoint.

Getting Information about the Tube Specification
------------------------------------------------

There are some functions useful for getting information about a tube
specification. It is not necessary to call them in a calibration script,
but they may be useful for checking the specification.

**getNumTubes** tells you the number of tubes in the specification.

**getDetectorInfoFromTube** returns information about detectors in the
(N+1)st tube in the specification, where N is the argument. Three
integers are returned:

-  the ID for the first detector,
-  the number of detectors in the tube and
-  the increment step of detector IDs in the tube (usually 1, but may be
   -1).

**getTubeLength** returns the length of the (N+1)st tube in the
specification in meters, where N is the argument.

**getTubeName** returns the name of the (N+1)st tube in the
specification, where N is the argument.

**getTubeByString** returns a list of workspace indices for the (N+1)st
tube in the specification, where N is the argument. The name of this
function needs updating. This list is inside :ref:`tube
calib <tube_calib>`.

Tubes are currently ordered in the specification in the same order as
they appear in the IDF. This may differ from the order they
appear in the workspace indices.
