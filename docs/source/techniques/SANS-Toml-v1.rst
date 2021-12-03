.. _sans_toml_v1-ref:

===============
SANS TOML Files
===============

.. contents:: Table of Contents
    :local:

General Notes
=============

- Lengths are *always* specified in meters within TOML files, unlike previous legacy formats.
- Angles are specified in degrees.
# David is this correct?


Format Changes
==============

V0 to V1
--------

- *normalisation* and *normalization* are both accepted and equivalent
- *detector.calibration* was renamed to *detector.correction*
- *mask.beamstop_shadow* and *mask.mask_pixels* were moved to
  *mask.spatial.beamstop_shadow* and *mask.spatial.mask_pixels*
- *normalisation.all_monitors* was added to support *BACK/MON/TIMES*
- *[gravity]* and *gravity.enabled* were merged into *instrument.configuration.gravity_enabled*
- *detector.configuration.selected_detector* is now mandatory
- *detector.configuration.selected_detector* accepts *front* and *rear* instead of *HAB* and *LAB* respectively.
- *detector.configuration.all_centre* has been added to set the front and rear centre at the same time.


New Fields
==========

toml_file_version
-----------------

This is always the first line of the file and represents the TOML
file version. Long-term this allows us to make changes in a backwards compatible way.

Available TOML Versions: 0

..  code-block:: none

  # First line of file
  toml_file_version = 0

  # Everything else

Metadata
--------

This is a free-form field, typically at the top of the file
to enter any user attributes. They are ignored by the TOML parser.

..  code-block:: none

  [metadata]
    created = "1980-12-31"
    weather_that_day = "sunny"
    this_is_toml = true

Instrument
----------

This is a required entry to specify the instrument name and `instrument.configuration`, documented in the conversion guide below.

..  code-block:: none

  [instrument]
    name = "LARMOR"  # or "LOQ" / "SANS2D" / "ZOOM"...etc.

  [instrument.configuration]
    # ...


Conversion From Legacy User Files
=================================

Layout
------

This section is designed like a reference that users can paste straight into
existing TOML files, but means that the sections are listed alphabetically
by the *old* command name!

*Note: TOML files use SI units rather than a mix of unit prefixes. For example,
you will need to convert any measurements in millimetres to meters.*

The following is used to note optional qualifiers which were available in
the existing user file format: ``[`` ``]``.

Examples are given in a way that they can be merged together where headers
match, for example these three examples:

..  code-block:: none

    [binning]
      wavelength = {start = 2.0, step=0.125, stop=14.0, type = "Lin"}

..  code-block:: none

    [binning]
      [binning.1d_reduction]
        binning = "0.02,0.05,0.5,-0.1,10.0"

..  code-block:: none

    [binning]
      [binning.2d_reduction]
        step = 0.002
        stop = 0.1
        type = "Lin"

Are combined into the following when writing the TOML file:

..  code-block:: none

    [binning]
      wavelength = {start = 2.0, step=0.125, stop=14.0, type = "Lin"}

      [binning.1d_reduction]
        binning = "0.02,0.05,0.5,-0.1,10.0"

      [binning.2d_reduction]
        step = 0.002
        stop = 0.1
        type = "Lin"

Tips for converting
-------------------

For converting existing files the following process is recommended:

- Make a copy of the existing (old-format) user file to work with
- Create a **blank** TOML file (file.toml instead of file.txt)
- Add the following to the start of the TOML file in the order shown:

..  code_block:: none

    toml_file_version = 1

    [metadata]

    [instrument]

    [instrument.configuration]
  
- Copy any comments from the old user file that need to be preserved
  to ``[``metadata``]`` in the TOML user file and replace any leading
  ``!`` with ``#``
- Remove any commented out lines in the old user file (lines starting
  with ``!``)
- Work down the old user file line-by-line using this guide to find
  the new replacement TOML commands
- Add the replacement TOML commands to the TOML user file
- Delete each line from the old user file as conversion proceeds
- When done, **save** the new TOML user file and delete the edited copy
  of the old user file; **do not delete the reference copy of the old
  user file!!!**
- Try the TOML user file in Mantid!


Command Set
===========

BACK/MON/TIMES t1 t2
--------------------

BACK was used to specify a time window over which to estimate the
(time-independent) background on monitor spectra. This background
is then subtracted from the specified monitor spectra before the
data are rebinned into wavelength.

This particular command subtracts the *same* background level from
*all* monitors. The continued use of this method of monitor correction
is now deprecated. See BACK/M[n]/TIMES.

Times were specified in microseconds.

..  code-block:: none

    [normalisation]
      [normalisation.all_monitors]
        background = [t1, t2]
        enabled = true

**Existing Example**

..  code-block:: none

    BACK/MON/TIMES 30000 40000

**Replacement Example**

..  code-block:: none

    [normalisation]
      [normalisation.all_monitors]
        background = [30000, 40000]
        enabled = true

Note: if using this, set any instances of use_own_background to false.

BACK/M[n]/TIMES t1 t2
---------------------

This command was used to estimate and subtract the (time-independent)
background level on a specified monitor. See also BACK/MON/TIMES.

Times were specified in microseconds.

..  code-block:: none

    # Note: both "normalisation" and "normalisation" are both accepted
    [normalisation]
      [normalisation.monitor.Mn]
        spectrum_number = n
  	    use_own_background = true
        background = [t1, t2]

*OR*

..  code-block:: none

    [transmission]
      [transmission.monitor.Mn]
        spectrum_number = n
  	    use_own_background = true
        background = [t1, t2]

**Existing Example**

..  code-block:: none

    BACK/M1/TIMES 30000 40000

**Replacement Example**

..  code-block:: none

    [normalisation]
      [normalisation.monitor.M1]
        spectrum_number = 1
  	    use_own_background = true
        background = [30000.0, 40000.0]

COMPATIBILITY ON
----------------

This command was used to allow event data to be reduced in
a manner that, so far as was possible, emulated the reduction
of histogram data. The primary use of this command was as a
diagnostic. Omitting this command was equivalent to
COMPATIBILITY OFF.

**Existing Example**

..  code-block:: none

    COMPATIBILITY ON

**Replacement Example**
Unsupported

DET/CORR [FRONT][REAR] [X][Y][Z] [XTILT][YTILT][ZTILT] [ROT] [SIDE] [RADIUS] n
------------------------------------------------------------------------------

This command was used to fine tune the position of a specified
detector by applying a relative correction to the logged encoder
value. The parameter n could be a distance or an angle depending
on the specified context as shown below.

If specified, SIDE *applies a translation to the rotation axis of
the detector perpendicular to the plane of the detector*. RADIUS
*increases the apparent radius from the rotation axis of the detector
to the active plane*.

# David how are the X/Y/ZTILT commands defined? What values are they modifying?

..  code-block:: none

    [detector]
      [detector.correction.position]
        # Note fields can be added or omitted as required
        # This is the complete list of adjustments available
        front_x = a
        front_y = b
        front_z = c

        front_x_tilt = d
        front_y_tilt = e
        front_z_tilt = f

        front_rot = g
        front_side = h

        rear_x = a
        rear_y = b
        rear_z = c

        rear_x_tilt = d
        rear_y_tilt = e
        rear_z_tilt = f

        rear_rot = g
        rear_side = h

**Existing Example**

..  code-block:: none

    DET/CORR FRONT X -33
    DET/CORR FRONT Y -20
    DET/CORR FRONT Z -47
    DET/CORR FRONT XTILT -0.0850
    DET/CORR FRONT YTILT 0.1419
    DET/CORR FRONT ROT 0.0
    DET/CORR FRONT SIDE 0.19
    DET/CORR FRONT RADIUS 75.7
    DET/CORR REAR X 0.0
    DET/CORR REAR Z 58

**Replacement Example**

..  code-block:: none

    [detector]
      [detector.correction.position]
        front_x = -0.033
        front_y = -0.020
        front_z = -0.047
        front_x_tilt = -0.000085
        front_y_tilt = 0.0001419
        front_rot = 0.0
        front_side = 0.00019
        rear_x = 0.0
        rear_z = 0.058

# David how is DET/CORR FRONT RADIUS represented now?

DET/[REAR][FRONT][MERGED][BOTH]
-------------------------------

This command was used to specify which detector(s) were to be
processed during data reduction. On the LOQ instrument the
qualifier /FRONT could be equivalently replaced by /HAB (for
high-angle bank). Similarly, /MERGED and /MERGE were equivalent.

If an instrument only has one detector it is assumed to be
equivalent to the *rear* detector.

In TOML the detectors must be specified in lower case, and /BOTH
has been replaced by "all".

..  code-block:: none

    [detector.configuration]
      selected_detector = "rear"

**Existing Example**

..  code-block:: none

    DET/HAB

**Replacement Example**

..  code-block:: none

    [detector.configuration]
      # Accepts "front", "rear", "merged", or "all".
      selected_detector = "front"

DET/RESCALE n
-------------

This command specified the factor by which the reduced *front*
detector data should be multiplied to allow it to overlap the
reduced rear detector data. If omitted n was assumed to be 1.0
(no rescaling). See also DET/RESCALE/FIT [q1 q2] and DET/SHIFT n.

..  code-block:: none

  [reduction]
    [reduction.merged.rescale]
        factor = n
        use_fit = false  # Must be false for single value

**Existing Example**

..  code-block:: none

    DET/RESCALE 0.123

**Replacement Example**

..  code-block:: none

  [reduction]
    [reduction.merged.rescale]
        factor = 0.123
        use_fit = false

DET/RESCALE/FIT [q1 q2]
-----------------------

This command was used to automatically estimate the factor by
which the reduced *front* detector data should be multiplied to
allow it to overlap the reduced rear detector data. A specific
Q-range over which to compare intensities could be optionally
specified. If omitted, all overlapping Q values were used. See
also DET/RESCALE n.

Scattering vectors were specified in inverse Angstroms.

..  code-block:: none

  [reduction]
    [reduction.merged.rescale]
        min = q1
        max = q2
        use_fit = true  # Must be true for fitting

**Existing Example**

..  code-block:: none

    DET/RESCALE/FIT 0.14 0.24


**Replacement Example**

..  code-block:: none

  [reduction]
    [reduction.merged.rescale]
      min = 0.14
      max = 0.24
      use_fit = true

DET/SHIFT n
-----------

This command specified the relative amount (a constant) by which the
reduced *front* detector data should be shifted in intensity to allow
it to overlap the reduced rear detector data. If omitted n was assumed
to be 0.0 (no shift). See also DET/RESCALE n and DET/SHIFT/FIT [q1 q2].

..  code-block:: none

  [reduction]
    [reduction.merged.shift]
        factor = y
        use_fit = false  # Must be false for single value

**Existing Example**

..  code-block:: none

    DET/SHIFT 0.123

**Replacement Example**

..  code-block:: none

  [reduction]
    [reduction.merged.shift]
        factor = 0.123
        use_fit = false

# David your existing doc says that the shift is a distance; that is
# inaccurate so I suggest it be changed to factor as for rescale.
# I've changed this doc accordingly.

DET/SHIFT/FIT [q1 q2]
---------------------

This command was used to automatically estimate the relative amount
(a constant) by which the reduced *front* detector data should be
shifted to allow it to overlap the reduced rear detector data. A
specific Q-range over which to compare intensities could be optionally
specified. If omitted, all overlapping Q values were used. See also
DET/SHIFT y.

Scattering vectors were specified in inverse Angstroms.

..  code-block:: none

  [reduction]
    [reduction.merged.shift]
        min = q1
        max = q2
        use_fit = true  # Must be true for fitting

**Existing Example**

..  code-block:: none

    DET/SHIFT/FIT 0.1 0.2

**Replacement Example**

..  code-block:: none

  [reduction]
    [reduction.merged.shift]
        min = 0.1
        max = 0.2
        use_fit = true

DET/OVERLAP q1 q2
-----------------

This command was used to specify the Q-range over which
merging of the rear and front detectors was to be done. If
omitted, all overlapping Q values were used.

Scattering vectors were specified in inverse Angstroms.

..  code-block:: none

  [reduction]
    [reduction.merged.merge_range]
      min = q1
      max = q2
      use_fit = true

**Existing Example**

..  code-block:: none

    DET/OVERLAP 0.14 0.24


**Replacement Example**

..  code-block:: none

  [merged]
    [reduction.merged.merge_range]
        min = 0.14
        max = 0.24
        use_fit = true

FIT/CENTRE t1 t2
----------------

This command was used to specify a time window within which
the 'prompt spike' could be found in *detector* spectra. This
information was used to remove the spike by interpolating
along the time-of-flight distribution. Also see
FIT/MONITOR t1 t2. 

Times were specified in microseconds.

# David this command used to be used on LOQ. Does the note on
# FIT/MONITOR apply here too?

**Existing Example**

..  code-block:: none

    FIT/CENTRE 19900 20500

**Replacement Example**
Unsupported

FIT/MID[/HAB]/FILE=script.txt
-----------------------------

This command was used to drive automatic determination of the
coordinates of the centre of the scattering pattern on the
specified detector using a script file. It has been superseded
by the Beam Centre Finder tool in Mantid.

If /HAB (equivalent to the "front" detector) was omitted the
command applied to the "rear" detector. 

**Existing Example**

..  code-block:: none

    FIT/MID/FILE=FIND_CENTRE128SC.COM
    FIT/MID/HAB/FILE=FIND_CENTRE_HAB2.COM

**Replacement Example**
Unsupported

FIT/MONITOR t1 t2
-----------------

This command was used to specify a time window within which
the 'prompt spike' could be found in *monitor* spectra. This
information was used to remove the spike by interpolating
along the time-of-flight distribution. Also see
FIT/CENTRE t1 t2. 

Times were specified in microseconds.

Note: This command was only ever enabled in the data reduction
source code for the LOQ instrument.

**Replacement**

..  code-block:: none

  [mask]
    prompt_peak = {start = t1, stop = t2}

**Existing Example**

..  code-block:: none

    FIT/MONITOR 19900 20500

**Replacement Example**

..  code-block:: none

  [mask]
    prompt_peak = {start = 19900.0, stop = 20500.0}

FIT/TRANS[/CLEAR][/OFF]
-----------------------

This command was used to disable fitting of the calculated
transmission data. Also see FIT/TRANS[[/SAMPLE][/CAN]][/LINEAR][/YLOG][/POLYNOMIALn] [w1 w2].

**Replacement**

..  code-block:: none

    [transmission]
      [transmission.fitting]
        enabled = false
        parameters = {lambda_min = w1, lambda_max = w2}
        # Can be: "Linear" / "Logarithmic" / "Polynomial"
        function = "Linear"
        # Only used when set to "Polynomial"
        polynomial_order = 3

**Existing Example**

..  code-block:: none

    FIT/TRANS/OFF

**Replacement Example**

..  code-block:: none

    [transmission]
      [transmission.fitting]
        enabled = false
        parameters = {lambda_min = 3.0, lambda_max = 11.0}
        function = "Linear"

FIT/TRANS[[/SAMPLE][/CAN]][/LINEAR][/YLOG][/POLYNOMIALn] [w1 w2]
----------------------------------------------------------------

This command was used to specify how the calculated transmission data
should be fitted. Subsequent data processing would then use transmission
values interpolated using the fit function. In some instances doing this
could improve the statistical quality of the transmission data. Also see
FIT/TRANS[/CLEAR][/OFF].

Wavelengths were specified in Angstroms. If w1 and w2 were omitted then the
fit was applied to the full wavelength range.

The \SAMPLE qualifier only applied the specified fit to the sample transmission
data. Similarly, the \CAN qualifier only applied the specified fit to the can
transmission data. If neither of these qualifiers was present then the same fit
function was applied to both sets of transmission data.

The \LINEAR (which could be abbreviated to \LIN) qualifier implemented a fit
function of the form Y=mX+C.

The \YLOG (which could be abbreviated to \LOG) qualifier implemented a fit
function of the form Y=exp(aX)+C.

The \POLYNOMIALn qualifier implemented a fit function of the form
Y=C0+C1X+C2X^2+...CnX^n where n>2.

**Replacement**

..  code-block:: none

    [transmission]
      [transmission.fitting]
        enabled = true
        parameters = {lambda_min = w1, lambda_max = w2}
        # Can be: "Linear" / "Logarithmic" / "Polynomial"
        function = "Linear"
        # Only used when set to "Polynomial"
        polynomial_order = 3

**Existing Example**

..  code-block:: none

    FIT/TRANS/LIN 3.0 11.0

**Replacement Example**

..  code-block:: none

    [transmission]
      [transmission.fitting]
        enabled = true
        parameters = {lambda_min = 3.0, lambda_max = 11.0}
        function = "Linear"

GRAVITY[/ON/OFF]
----------------

This command was used to specify whether the detector data should be
corrected for the ballistic effects of gravity on the neutrons. This
correction is particularly important at long sample-detector distances
and/or when using long wavelengths. Also see GRAVITY/LEXTRA x.

**Replacement**

..  code-block:: none

    [instrument.configuration]
      gravity_enabled = true

**Existing Example**

..  code-block:: none

    GRAVITY/ON

**Replacement Example**

..  code-block:: none

    [instrument.configuration]
      gravity_enabled = true


GRAVITY/LEXTRA x
----------------

This command was used to specify an extra length that can be added
to the gravity correction. The extra length is only taken into account
when the gravity correction is enabled and the default value is x=0.0.
Also see GRAVITY[/ON/OFF].

**Replacement**

..  code-block:: none

    [instrument.configuration]
      gravity_extra_length = x

**Existing Example**

..  code-block:: none

    GRAVITY/LEXTRA 2.0

**Replacement Example**

..  code-block:: none

    [instrument.configuration]
      gravity_extra_length = 2.0

L/EVENTSTIME binning_string
---------------------------

This command was used to specify a binning scheme to be applied to
event mode data. The scheme comprised a comma-separated string of the
form t1,step1,t2,step2,t3... where t1, t2, t3, etc specified event
times and step1, step2, etc specified the binning interval between
those event times.

A positive step would result in linear (ie, equally-spaced) bins, whilst
a negative step would result in logarithmic (ie, geometrically-expanding)
bins.

All times and linear steps were specified in microseconds. Logarithmic
steps were specified as %/100.

**Replacement**

..  code-block:: none

  [reduction.events]
    binning = "str"

**Existing Example**

..  code-block:: none

    L/EVENTSTIME 7000.0,500.0,60000.0

**Replacement Example**

..  code-block:: none

  [reduction.events]
    # A negative step (middle val) indicates Log
    binning = "7000.0,500.0,60000.0"

L/PHI[/NOMIRROR] a b
---------------------

This command specified the azimuthal range of 2D detector data to be
included in data reduction. Viewed along the direction of travel of
the neutrons 0 (or 360) degrees was at 3 O'clock, 90 degrees was at
12 O'clock, 180 (or -180) degrees was at 9 O'clock, and 270 (or -90)
degrees was at 6 O'clock. By default the mirror sector was always
included (ie, selecting a=-30 & b=+30 would *also* include the sector
150-210), but this could be overridden with the /NOMIRROR qualifier.

Angles were specified in degrees.

**Replacement**

..  code-block:: none

    [mask]
      [mask.phi]
        mirror = bool
        start = a
        stop = b

**Existing Example**

..  code-block:: none

    L/PHI/NOMIRROR -45 45

**Replacement Example**

..  code-block:: none

    [mask]
      [mask.phi]
        mirror = false
        start = -45
        stop = 45

L/Q binning_string
------------------

This command was used to specify a Q-binning scheme to be applied
during 1D data reduction. Also see L/QXY binning_string.

For historical reasons, several variants of this command were
implemented but they can be summarised thus:

..  code-block:: none

    L/Q q1 q2 qstep/LIN   same as   L/Q/LIN q1 q2 qstep
    L/Q q1 q2 qstep/LOG   same as   L/Q/LOG q1 q2 qstep
	L/Q q1,step1,q2,step2,q3...
	
In the first two cases the type of Q-binning is fixed by the choice of
the \LIN or \LOG qualifier. But in the last case *variable* Q-binning
is permitted if required.

A positive step would result in linear (ie, equally-spaced) bins, whilst
a negative step would result in logarithmic (ie, geometrically-expanding)
bins.

All Q-values and linear steps were specified in inverse Angstroms. Logarithmic
steps were specified as %/100.

**Replacement**

..  code-block:: none

    [binning.1d_reduction]
        # Negative indicates log
        binning = "rebin_string"

**Existing Example**

..  code-block:: none

    L/Q .02,0.05,0.5,-0.1,10

**Replacement Example**

..  code-block:: none

    [binning]
      [binning.1d_reduction]
        # Negative indicates log
        binning = "0.02,0.05,0.5,-0.1,10.0"

L/Q/RCUT r
----------

This command was used to specify the 'radius cut' value, a construct
which could be used to improve the statistical uncertainty on Q bins
suffering from poor instrumental resolution. This command would typically,
but not exclusively, be used in conjunction with L/Q/WCUT w.

For more information, see the [Q1D](https://docs.mantidproject.org/nightly/algorithms/Q1D-v2.html)
algorithm description.

**Replacement**

..  code-block:: none

    [binning.1d_reduction]
        radius_cut = r

**Existing Example**

..  code-block:: none

    L/Q/RCUT 100

**Replacement Example**

..  code-block:: none

    [binning]
      [binning.1d_reduction]
        radius_cut = 0.1

L/Q/WCUT w
----------

This command was used to specify the 'wavelength cut' value, a construct
which could be used to improve the statistical uncertainty on Q bins
suffering from poor instrumental resolution. This command would typically,
but not exclusively, be used in conjunction with L/Q/RCUT r.

For more information, see the [Q1D](https://docs.mantidproject.org/nightly/algorithms/Q1D-v2.html)
algorithm description.

The cut-off wavelength was specified in Angstroms.

**Replacement**

..  code-block:: none

    [binning.1d_reduction]
        wavelength_cut = w

**Existing Example**

..  code-block:: none

    L/Q/WCUT 8

**Replacement Example**

..  code-block:: none

    [binning]
      [binning.1d_reduction]
        wavelength_cut = 8.0

L/QXY binning_string
--------------------

This command was used to specify a Q-binning scheme to be applied
during 2D data reduction. Also see L/Q binning_string.

For historical reasons, several variants of this command were
implemented but they can be summarised thus:

..  code-block:: none

    L/QXY 0 q2 qstep/LIN   same as   L/QXY/LIN 0 q2 qstep
    L/QXY 0 q2 qstep/LOG   same as   L/QXY/LOG 0 q2 qstep
	
The type of Q-binning is fixed by the choice of the \LIN or \LOG
qualifier but variable binning is **not** permitted during 2D reductions.
Also note that the Q-range *must* start at zero.

All Q-values and linear steps were specified in inverse Angstroms. Logarithmic
steps were specified as %/100.

**Replacement**

..  code-block:: none

    [binning]
      [binning.2d_reduction]
        #binning MUST start at 0.0
        step = step
        stop = stop
        #type can be "Lin" or "Log"
        type = "Lin"

**Existing Example**

..  code-block:: none

    L/QXY 0 0.1 .002/lin

**Replacement Example**

..  code-block:: none

    [binning]
      [binning.2d_reduction]
        step = 0.002
        stop = 0.1
        type = "Lin"

# Steve pick up here

L/R x y [step]
--------------

Note step was ignored previously.

..  code-block:: none

    [detector]
      radius_limit = {min = 0.038, max = -0.001}

**Existing Example**

..  code-block:: none

    L/R 38 -1

**Replacement Example**

..  code-block:: none

    [detector]
      radius_limit = {min = 0.038, max = -0.001}

L/SP
----

**Replacement**
Unsupported

L/WAV min max step [/LIN]
--------------------------

**Replacement**

..  code-block:: none

    wavelength = {start = min, step = step, stop = max, type = "Lin"}
    # Alternative for ranges
    wavelength = {binning = "min,max", step = step, type = "RangeLin"}


**Existing Example**

..  code-block:: none

    L/WAV 2.0 14.0 0.125/LIN

**Replacement Example**

..  code-block:: none

    [binning]
      # Only for "Lin", "Log"
      wavelength = {start = 2.0, step=0.125, stop=14.0, type = "Lin"}
      # Only for "RangeLin" or "RangeLog"
      wavelength = {binning="2.0-7.0, 7.0-14.0", type = "RangeLin"}

MASKFILE str
------------

**Replacement**

..  code-block:: none

    [mask]
    mask_files = ["a", "b", "c"]

**Existing Example**

..  code-block:: none

    MASKFILE=a.xml,b.xml,c.xml

**Replacement Example**

..  code-block:: none

    [mask]
    mask_files = ["a.xml", "b.xml", "c.xml"]


MASK h
------

**Replacement**

..  code-block:: none

    [mask]
      [mask.spatial.rear]  # Or front
        detector_rows = [h1, h2, h3, ...hn]

**Existing Example**

..  code-block:: none

    mask/rear h100
    mask/rear h200

**Replacement Example**

..  code-block:: none

    [mask]
      [mask.spatial.rear]
        # Masks horizontal 100 and 200
        detector_rows = [100, 200]

MASK hx>hy
----------

**Replacement**

..  code-block:: none

    [mask]
      [mask.spatial.rear]  # Or front
        detector_row_ranges = [[x, y]]

**Existing Example**

..  code-block:: none

    mask h126>h127

**Replacement Example**

..  code-block:: none

    [mask]
      [mask.spatial.rear]
        # Masks horizontal 126 AND 127
        # Also includes 130-135 to show multiple can be masked
        detector_row_ranges = [[126, 127], [130, 135]]


MASK v
------

**Replacement**

..  code-block:: none

    [mask]
      [mask.spatial.rear]  # Or front
        detector_rows = [v1, v2, v3, ...vn]

**Existing Example**

..  code-block:: none

    mask/rear v100
    mask/rear v200

**Replacement Example**

..  code-block:: none

    [mask]
      [mask.spatial.rear]
        # Masks vertical 100 and 200
        detector_columns = [100, 200]

MASK vx>vy
----------

**Replacement**

..  code-block:: none

    [mask]
      [mask.spatial.rear]  # Or front
        detector_column_ranges = [[x, y]]

**Existing Example**

..  code-block:: none

    mask v126>v127

**Replacement Example**

..  code-block:: none

    [mask]
      [mask.spatial.rear]
        # Masks vertical 126 AND 127
        # Also includes 130-135 to show multiple can be masked
        detector_column_ranges = [[126, 127], [130, 135]]

MASK Sn
-------

**Replacement**

..  code-block:: none

    [mask]
      mask_pixels = [n1, n2, ...n]

**Existing Example**

..  code-block:: none

    MASK S123
    MASK S456

**Replacement Example**

..  code-block:: none

    [mask]
      mask_pixels = [123, 456]

MASK/T x y
----------

**Replacement**

..  code-block:: none

    [mask]
      [mask.time]
        tof = [
            {start = x1, stop = y1},
            {start = x2, stop = y2},
            # ...etc
        ]

**Existing Example**

..  code-block:: none

    # Note multiple lines can be collapsed into one section
    MASK/T 19711.5 21228.5
    MASK/T 39354.5 41348.5

**Replacement Example**

..  code-block:: none

    [mask]
      [mask.time]
        tof = [
          {start = 19711.5, stop = 21228.5},
          {start = 39354.5, stop = 41348.5}
        ]


MASK/CLEAR[/TIME]
-----------------

**Replacement**
Unsupported

MASK/LINE x y
-------------

**Replacement**

..  code-block:: none

    beamstop_shadow = {width = x, angle = y}

**Existing Example:**

..  code-block:: none

    MASK/LINE 30 170

**Replacement Example**

..  code-block:: none

    [mask]
      beamstop_shadow = {width = 0.03, angle = 170.0}

MASK/LINE a b c d
-----------------

Note: *c* and *d* representing x and y positions are already in meters in
legacy files.

**Replacement**

..  code-block:: none

    beamstop_shadow = {width = a, angle = b, x_pos = c, y_pos = d}

**Existing Example:**

..  code-block:: none

    MASK/LINE 30 170 0.3 0.1

**Replacement Example**

..  code-block:: none

    [mask]
      beamstop_shadow = {width = 0.03, angle = 170.0, x_pos=0.3, y_pos=0.1}

MON/DIRECT="filename"
---------------------

**Replacement**

..  code-block:: none

    [detector]
      [detector.correction.direct]
        rear_file = "filename"
        front_file = "filename"


**Existing Example:**

..  code-block:: none

    MON/DIRECT=DIRECT_RUN524.dat

**Replacement Example**

..  code-block:: none

    [detector]
      [detector.correction.direct]
        rear_file = "DIRECT_RUN524.dat"
        front_file = "DIRECT_RUN524.dat"

MON/FLAT=str
------------

**Replacement**

..  code-block:: none

    [detector]
      [detector.correction.flat]
        rear_file = "str"


**Existing Example:**

..  code-block:: none

    MON/FLAT="flat_file.091"

**Replacement Example**

..  code-block:: none

    [detector]
      [detector.correction.flat]
        rear_file = "flat_file.091"


MON/LENGTH
----------

**Replacement**
Unsupported

MON [/TRANS] /SPECTRUM=n [/INTERPOLATE]
---------------------------------------

..  code-block:: none

  [normalisation]
    #Normalisation monitor

    # This name is used below so if there was a monitor called FOO1
    # this would work with it
    selected_monitor = "M1"

    [normalisation.monitor.M1]
      spectrum_number = n


**Existing Example:**

..  code-block:: none

    MON/SPECTRUM=1

**Replacement Example**

..  code-block:: none

  [normalisation]
    #Normalisation monitor

    # This name is used below so if there was a monitor called FOO1
    # this would work with it
    selected_monitor = "M1"

    [normalisation.monitor.M1]
      spectrum_number = 1

QRESOL/A1=x
--------------

**Replacement**

..  code-block:: none

  [q_resolution]
    source_aperture = x

**Existing Example:**

..  code-block:: none

    QRESOL/A1=30

**Replacement Example**

..  code-block:: none

  [q_resolution]
    source_aperture = 0.03

QRESOL/A2=x
--------------

**Replacement**

..  code-block:: none

  [instrument.configuration]
    sample_aperture_diameter = x

**Existing Example:**

..  code-block:: none

    QRESOL/A2=20

**Replacement Example**

..  code-block:: none

  [instrument.configuration]
    sample_aperture_diameter = 0.02

QRESOL/DELTAR=x
---------------

The virtual ring width of the detector in meters.
This is used to calculate the Q Resolution from TOF SANS Data on a per-pixel
in :ref:`algm-TOFSANSResolutionByPixel`.

**Replacement**

..  code-block:: none

  [q_resolution]
    delta_r = x

**Existing Example:**

..  code-block:: none

  QRESOL/DELTAR=10  # mm

**Replacement Example**

..  code-block:: none

  [q_resolution]
    delta_r = 0.01  # m

QRESOL/MODERATOR=filename.txt
-----------------------------

**Replacement**

..  code-block:: none

  [q_resolution]
    moderator_file = filename.txt

**Existing Example:**

..  code-block:: none

    QRESOL/MODERATOR=moderator_rkh_file.txt

**Replacement Example**

..  code-block:: none

  [q_resolution]
    moderator_file = moderator_rkh_file.txt


QRESOL[/ON][/OFF]
-----------------

**Replacement**

..  code-block:: none

  [q_resolution]
    enabled = true  # Or false

**Existing Example:**

..  code-block:: none

    QRESOL/ON

**Replacement Example**

..  code-block:: none

  [q_resolution]
    enabled = true  # Or false


QRESOL[/H1=x][/H2=wx][/W1=x][/W2=x]
-----------------------------------

**Replacement**

..  code-block:: none

  [q_resolution]
    h1 = x
    h2 = x
    w1 = x
    w2 = x

**Existing Example:**

..  code-block:: none

    QRESOL/H1=16.0
    QRESOL/H2=8.0
    QRESOL/W1=16.0
    QRESOL/W2=8.0

**Replacement Example**

..  code-block:: none

  [q_resolution]
    h1 = 16.0
    h2 = 8.0
    w1 = 16.0
    w2 = 8.0

SAMPLE/OFFSET x
---------------

**Replacement**

..  code-block:: none

  [instrument.configuration]
    sample_offset = n

**Existing Example:**

..  code-block:: none

    SAMPLE/OFFSET -60

**Replacement Example**

..  code-block:: none

  [instrument.configuration]
    sample_offset = -0.06


set centre a b
--------------

..  code-block:: none

    [detector]
      [detector.configuration]
        all_centre = {x=a, y=b}

**Existing Example:**

..  code-block:: none

    set centre 84.2 -196.5

**Replacement Example**

..  code-block:: none

    [detector]
      [detector.configuration]
        # This will set both front and rear to the same centre values.
        all_centre = {x=a, y=b}


set centre a b c d [/MAIN] [/HAB]
---------------------------------

..  code-block:: none

    [detector]
      [detector.configuration]
        front_centre = {x=a, y=b}
        rear_centre = {x=c, y=d}

**Existing Example:**

..  code-block:: none

    set centre 84.2 -196.5 5.1 5.1 /MAIN
    set centre 84.2 -196.5 /HAB

**Replacement Example**

..  code-block:: none

    [detector]
      [detector.configuration]
        # Note for identical results the values will
        # only take a and b in the above example due to a bug
        # with the legacy user file parser
        front_centre = {x=0.0842, y=-0.1965}
        rear_centre = {x=0.0842, y=-0.1965}

set scales a b c d
------------------

..  code-block:: none

    [detector]
      [detector.configuration]
        front_scale = b
        rear_scale = a

**Existing Example:**

..  code-block:: none

    set scales 1.497 1.0 1.0 1.0 1.0

**Replacement Example**

..  code-block:: none

    [detector]
      [detector.configuration]
        front_scale = 1.0
        rear_scale = 1.497

TRANS/TRANSSPEC=n
-----------------

**Replacement**

..  code-block:: none

    [transmission]
      # Where Mn is arbitrary but must match the section label
      selected_monitor = "Mn"

      [transmission.monitor.Mn]
        spectrum_number = n

**Existing Example:**

..  code-block:: none

    TRANS/TRANSPEC=3

**Replacement Example**

..  code-block:: none

    [transmission]
      selected_monitor = "M3"

      [transmission.monitor.M3]
        spectrum_number = 3

TRANS/TRANSPEC=n/SHIFT=a
------------------------

**Replacement**

..  code-block:: none

    [transmission]
      # Where Mn is arbitrary but must match the section label
      selected_monitor = "Mn"

      [transmission.monitor.Mn]
        spectrum_number = n

**Existing Example:**

..  code-block:: none

    TRANS/TRANSPEC=3/SHIFT=-58

**Replacement Example**

..  code-block:: none

    [transmission]
      selected_monitor = "M3"

      [transmission.monitor.M3]
        spectrum_number = 3
        shift = -0.058

TUBECALIBFILE=str
-----------------

**Replacement**

..  code-block:: none

  [detector]

  [detector.correction.tube]
    file = "str"

**Existing Example:**

..  code-block:: none

  TUBECALIBFILE=Tube.nxs

**Replacement Example**

..  code-block:: none

  [detector]

  [detector.correction.tube]
    file = "Tube.nxs"

.. categories:: Techniques
