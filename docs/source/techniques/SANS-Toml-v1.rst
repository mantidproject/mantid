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

Format Changes
==============

V1 (Mantid 6.4+) to V2 (Mantid 6.13+)
-------------------------------------
**NOTE: These are valid user file entries but have not yet been implemented as part of the reduction process.**

*polarization* options have been added to the TOML format.

- *flipper_configuration*
- *spin_configuration*
- *polarization.flipper.NAME*

  - *idf_component_name*
  - *device_name*
  - *location*

    - *x*, *y*, and *z*

  - *transmission*
  - *efficiency*

- *polarization.polarizer* and *polarization.analyzer*

  - All fields from the flippers plus:
  - *cell_length*
  - *gas_pressure*
  - *empty_cell*
  - *initial_polarization*

- *polarization.magnetic_field* and *polarization.electric_field*

  - *sample_strength_log*
  - *sample_direction*

    - *a*, *p*, and *d*

  - *sample_direction_log*

V0 (Mantid 6.3+) to V1 (Mantid 6.4+)
--------------------------------------

- *norm_monitor* and *trans_monitor* in *instrument.configuration* now take a monitor name (e.g. "M1") instead of a spectrum number
- *selected_monitor* in *normalisation* and *transmission* has been removed in favour of this change in *instrument.configuration*
- *trans_monitor* can now have the value *"ROI"*

V0 (Mantid 6.1.x) to V0 (Mantid 6.3+)
--------------------------------------

- *normalisation* and *normalization* are both accepted and equivalent
- *detector.calibration* was renamed to *detector.correction*
- *mask.beamstop_shadow* and *mask.mask_pixels* were moved to
  *mask.spatial.beamstop_shadow* and *mask.spatial.mask_pixels*
- *normalisation.all_monitors* was added to support *BACK/MON/TIMES*
- *[gravity]* and *gravity.enabled* were merged into *instrument.configuration.gravity_enabled*
- *detector.configuration.selected_detector* is now mandatory
- *detector.configuration.selected_detector* accepts *front* and *rear* instead of *HAB* and *LAB* respectively.
- *detector.configuration.all_centre* has been added to set the front and rear centre at the same time.
- *reduction.merged.shift.distance* was renamed from `distance` to `factor`


New Fields
==========

toml_file_version
-----------------

This is always the first line of the file and represents the TOML
file version. Long-term this allows us to make changes in a backwards compatible way.

Available TOML Versions:

- 1
- 2

..  code-block:: none

  # First line of file
  toml_file_version = 1

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
the existing user file format: `[ ]`.

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

..  code-block:: none

    toml_file_version = 1

    [metadata]

    [instrument]
      name = "instrument"  # give name of instrument

    [instrument.configuration]

- Copy any comments from the old user file that need to be preserved
  to `[metadata]` in the TOML user file and replace any leading
  `!` with `#`
- Remove any commented out lines in the old user file (lines starting
  with `!`)
- Work down the old user file line-by-line using this guide to find
  the new replacement TOML commands
- Add the replacement TOML commands to the TOML user file
- Delete each line from the old user file as conversion proceeds
- When done, **save** the new TOML user file and delete the edited copy
  of the old user file; **do not delete the reference copy of the old
  user file!!!**
- Try the TOML user file in Mantid!


Legacy Command Set
==================

.. _back_mon_times-ref:

BACK/MON/TIMES t1 t2
--------------------

BACK was used to specify a time window over which to estimate the
(time-independent) background on monitor spectra. This background
is then subtracted from the specified monitor spectra before the
data are rebinned into wavelength.

This particular command subtracts the *same* background level from
*all* monitors. The continued use of this method of monitor correction
is now deprecated. See also :ref:`back_mn_times-ref`.

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

.. _back_mn_times-ref:

BACK/M[n]/TIMES t1 t2
---------------------

This command was used to estimate and subtract the (time-independent)
background level on a specified monitor. See also :ref:`back_mon_times-ref`.

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

    BACK/M1 30000 40000

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

XYZ applies a translation to in the specified direction to a given bank
in the specified axis.

Tilt rotates a bank by the given number of degrees along the axis specified.

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
        front_radius = 0.0757
        front_rot = 0.0
        front_side = 0.00019
        rear_x = 0.0
        rear_z = 0.058

DET/[REAR][FRONT][MERGED][BOTH]
-------------------------------

This command was used to specify which detector(s) were to be
processed during data reduction.

On the LOQ instrument the qualifier `/FRONT` could be  equivalently replaced by `/HAB` (for
high-angle bank) in existing user files. Similarly, /MERGED and /MERGE were equivalent.

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
(no rescaling). See also :ref:`det_rescale_fit-ref` and :ref:`det_shift_y-ref`.

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

.. _det_rescale_fit-ref:

DET/RESCALE/FIT [q1 q2]
-----------------------

This command was used to automatically estimate the factor by
which the reduced *front* detector data should be multiplied to
allow it to overlap the reduced rear detector data. A specific
Q-range over which to compare intensities could be optionally
specified. If omitted, all overlapping Q values were used. See
also :ref:`det_rescale_fit-ref`.

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

.. _det_shift_y-ref:

DET/SHIFT y
-----------

This command specified the relative amount (a constant) by which the
reduced *front* detector data should be shifted in intensity to allow
it to overlap the reduced rear detector data. If omitted n was assumed
to be 0.0 (no shift). See also :ref:`det_rescale_fit-ref` and :ref:`det_shift_y-ref`.

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

DET/SHIFT/FIT [q1 q2]
---------------------

This command was used to automatically estimate the relative amount
(a constant) by which the reduced *front* detector data should be
shifted to allow it to overlap the reduced rear detector data. A
specific Q-range over which to compare intensities could be optionally
specified. If omitted, all overlapping Q values were used. See also
:ref:`det_shift_y-ref`.

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

.. _fit_centre-ref:

FIT/CENTRE t1 t2
----------------

This command was used to specify a time window within which
the 'prompt spike' could be found in *detector* spectra. This
information was used to remove the spike by interpolating
along the time-of-flight distribution. See also :ref:`fit_monitor-ref`.

Times were specified in microseconds.

**This command was never implemented in Mantid (but was in COLETTE)!**

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

.. _fit_monitor-ref:

FIT/MONITOR t1 t2
-----------------

This command was used to specify a time window within which
the 'prompt spike' could be found in *monitor* spectra. This
information was used to remove the spike by interpolating
along the time-of-flight distribution. See also :ref:`fit_centre-ref`.

Times were specified in microseconds.

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

.. _trans_fitting_off-ref:

FIT/TRANS[/CLEAR][/OFF]
-----------------------

This command was used to disable fitting of the calculated
transmission data. See also :ref:`fitting_on-ref`.

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

.. _fitting_on-ref:

FIT/TRANS[[/SAMPLE][/CAN]][/LINEAR][/YLOG][/POLYNOMIALn] [w1 w2]
----------------------------------------------------------------

This command was used to specify how the calculated transmission data
should be fitted. Subsequent data processing would then use transmission
values interpolated using the fit function. In some instances doing this
could improve the statistical quality of the transmission data. See also
:ref:`trans_fitting_off-ref`.

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

.. _gravity_on-ref:

GRAVITY[/ON/OFF]
----------------

This command was used to specify whether the detector data should be
corrected for the ballistic effects of gravity on the neutrons. This
correction is particularly important at long sample-detector distances
and/or when using long wavelengths. See also :ref:`gravity_extra_len-ref`.

If Q-resolution estimation is enabled (see QRESOL[/ON][/OFF]) any gravity
corrections will be included in that calculation too.

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

.. _gravity_extra_len-ref:

GRAVITY/LEXTRA x
----------------

This command was used to specify an extra length that can be added
to the gravity correction. The extra length is only taken into account
when the gravity correction is enabled and the default value is x=0.0.
See also :ref:`gravity_on-ref`.

Unless there is a reason not too, set the value of *gravity_extra_length* equal
to 0.5 * collimation_length. See also :ref:`QRESOL/LCOLLIM=z <qresol-lcollim-z>`.

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

L was an accepted abbreviation for LIMIT.

This command was used to specify a binning scheme to be applied to
event mode data. The scheme comprised a comma-separated string of the
form t1,tstep1,t2,tstep2,t3... where t1, t2, t3, etc specified event
times and tstep1, tstep2, etc specified the binning interval between
those event times.

A positive tstep would result in linear (ie, equally-spaced) bins, whilst
a negative tstep would result in logarithmic (ie, geometrically-expanding)
bins.

All times and linear tsteps were specified in microseconds. Logarithmic
tsteps were specified as %/100.

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
    # Therefore this is linear binning
    binning = "7000.0,500.0,60000.0"

L/PHI[/NOMIRROR] a b
---------------------

L was an accepted abbreviation for LIMIT.

This command specified the azimuthal range of 2D detector data to be
included in data reduction.
Viewed along the direction of travel of
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

.. _q-ref:

L/Q binning_string
------------------

L was an accepted abbreviation for LIMIT.

This command was used to specify a Q-binning scheme to be applied
during 1D data reduction. See also :ref:`qxy-ref`.

For historical reasons, several variants of this command were
implemented but they can be summarised thus:

..  code-block:: none

    L/Q q1 q2 qstep/LIN   same as   L/Q/LIN q1 q2 qstep
    L/Q q1 q2 qstep/LOG   same as   L/Q/LOG q1 q2 qstep
	L/Q q1,qstep1,q2,qstep2,q3...

In the first two cases the type of Q-binning is fixed by the choice of
the \LIN or \LOG qualifier. But in the last case *variable* Q-binning
is permitted if required.

A positive qstep would result in linear (ie, equally-spaced) bins, whilst
a negative qstep would result in logarithmic (ie, geometrically-expanding)
bins.

All Q-values and linear qsteps were specified in inverse Angstroms. Logarithmic
qsteps were specified as %/100.

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

.. _q_rcut-ref:

L/Q/RCUT r
----------

L was an accepted abbreviation for LIMIT.

This command was used to specify the 'radius cut' value, a construct
which could be used to improve the statistical uncertainty on Q bins
suffering from poor instrumental resolution. This command would typically,
but not exclusively, be used in conjunction with :ref:`q_wcut-ref`.

For more information, see the `Q1D <https://docs.mantidproject.org/nightly/algorithms/Q1D-v2.html>`_
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

.. _q_wcut-ref:

L/Q/WCUT w
----------

L was an accepted abbreviation for LIMIT.

This command was used to specify the 'wavelength cut' value, a construct
which could be used to improve the statistical uncertainty on Q bins
suffering from poor instrumental resolution. This command would typically,
but not exclusively, be used in conjunction with :ref:`q_rcut-ref`.

For more information, see the `Q1D <https://docs.mantidproject.org/nightly/algorithms/Q1D-v2.html>`_
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

.. _qxy-ref:

L/QXY binning_string
--------------------

L was an accepted abbreviation for LIMIT.

This command was used to specify a Q-binning scheme to be applied
during 2D data reduction. See also :ref:`q-ref`.

For historical reasons, several variants of this command were
implemented but they can be summarised thus:

..  code-block:: none

    L/QXY 0 q2 qstep/LIN   same as   L/QXY/LIN 0 q2 qstep
    L/QXY 0 q2 qstep/LOG   same as   L/QXY/LOG 0 q2 qstep

The type of Q-binning is fixed by the choice of the \LIN or \LOG
qualifier but variable binning is **not** permitted during 2D reductions.
Also note that the Q-range *must* start at zero.

All Q-values and linear qsteps were specified in inverse Angstroms.
Logarithmic qsteps were specified as %/100.

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

L/R r1 r2 [rstep]
-----------------

L was an accepted abbreviation for LIMIT.

This command was used to specify the radii on the detector between
which the radial integration of the data was to be performed. Typically,
r1 would be set to be just outside the radius of the beamstop in use.

On the LOQ instrument the maximum values of r2 on the rear and front
detectors are 0.419 m and 0.750 m, respectively. But with the advent of the
TS2 SANS instruments with moving detectors a convenience was introduced to
make setting r2 easier and less prone to error: setting r2 = -0.001 m is
equivalent to using the maximum radius. **But note it is not clear how this
is now achieved!**

On LOQ the rstep parameter originally specified the width of the virtual rings
used for the radial integration, a value of rstep = 0.003 m was typical.
However, at some point this rstep seemed to become optional, and indeed was
never used on some the TS2 instruments. **How the virtual ring width was decided
in such cases is also unclear!**

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

L was an accepted abbreviation for LIMIT.

This command was used to specify the detector spectra (ie, pixels) to be
included in the data reduction. Historically this mitigated computation
challenges. This command has effectively been superseded by the
DET/[REAR][FRONT][MERGED][BOTH] command.

**Existing Example**

..  code-block:: none

    L/SP 3 16386

**Replacement Example**

Unsupported

L/WAV binning_string
--------------------

L was an accepted abbreviation for LIMIT.

This command was used to specify a wavelength-binning scheme to be
applied during data reduction.

For historical reasons, several variants of this command were
implemented but they can be summarised thus:

..  code-block:: none

    L/WAV w1 w2 wstep/LIN   same as   L/WAV/LIN w1 w2 wstep
    L/WAV w1 w2 wstep/LOG   same as   L/WAV/LOG w1 w2 wstep

The /LIN qualifier would result in linear (ie, equally-spaced) bins,
whilst the /LOG qualifier would result in logarithmic (ie,
geometrically-expanding) bins.

All wavelength-values and linear wsteps were specified in Angstroms.
Logarithmic wsteps were specified as %/100.

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
      wavelength = {binning="2.0-7.0, 7.0-14.0", step=0.125, type = "RangeLin"}

MASK/CLEAR[/TIME]
-----------------

This command was used to clear any detector masks in operation. Without
the \TIME qualifier only *spatial* masks were cleared; with the \TIME
qualifier only time masks were cleared.

**Existing Example**

..  code-block:: none

    MASK/CLEAR
    MASK/CLEAR/TIME

**Replacement Example**

Unsupported

MASKFILE=mask_file_list
-----------------------

This command was used to specify one or more detector mask files to be
applied during data reduction to omit individual detector pixels or
regions of pixels from the calculation.

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

.. _mask_h-ref:

MASK[/FRONT][/REAR] Hn
----------------------

This command was used to specify a **horizontal row** of detector pixels
to be omitted from the calculation during data reduction. See also
:ref:`mask_h-ref`.

The TOML replacement command actually permits several rows to be
specified at once.

**Replacement**

..  code-block:: none

    [mask]
      [mask.spatial.rear]  # Or front
        detector_rows = [h1, h2, h3, ...hn]

**Existing Example**

..  code-block:: none

    MASK/REAR H100
    MASK/REAR H200

**Replacement Example**

..  code-block:: none

    [mask]
      [mask.spatial.rear]
        # Masks horizontal 100 and 200
        detector_rows = [100, 200]

:ref:`mask_h-ref`

MASK[/FRONT][/REAR] Hn>Hm
-------------------------

This command was used to specify several **contiguous horizontal rows**
of detector pixels to be omitted from the calculation during data reduction.
See also :ref:`mask_h-ref`.

The TOML replacement command actually permits multiple ranges of rows to be
specified at once.

**Replacement**

..  code-block:: none

    [mask]
      [mask.spatial.rear]  # Or front
        detector_row_ranges = [[x, y]]

**Existing Example**

..  code-block:: none

    MASK/REAR H126>H127

**Replacement Example**

..  code-block:: none

    [mask]
      [mask.spatial.rear]
        # Masks horizontal 126 AND 127
        # Also includes 130-135 to show multiple can be masked
        detector_row_ranges = [[126, 127], [130, 135]]

.. _mask_v-ref:

MASK[/FRONT][/REAR] Vn
----------------------

This command was used to specify a **vertical column** of detector pixels
to be omitted from the calculation during data reduction. See also
:ref:`mask_v-ref`.

The TOML replacement command actually permits several columns to be
specified at once.

**Replacement**

..  code-block:: none

    [mask]
      [mask.spatial.rear]  # Or front
        detector_rows = [v1, v2, v3, ...vn]

**Existing Example**

..  code-block:: none

    MASK/REAR V100
    MASK/REAR V200

**Replacement Example**

..  code-block:: none

    [mask]
      [mask.spatial.rear]
        # Masks vertical 100 and 200
        detector_columns = [100, 200]

:ref:`mask_v-ref`

MASK[/FRONT][/REAR] Vn>Vm
-------------------------

This command was used to specify several **contiguous vertical columns**
of detector pixels to be omitted from the calculation during data reduction.
See also :ref:`mask_v-ref`.

The TOML replacement command actually permits multiple ranges of columns to be
specified at once.

**Replacement**

..  code-block:: none

    [mask]
      [mask.spatial.rear]  # Or front
        detector_column_ranges = [[x, y]]

**Existing Example**

..  code-block:: none

    MASK/REAR V126>V127

**Replacement Example**

..  code-block:: none

    [mask]
      [mask.spatial.rear]
        # Masks vertical 126 AND 127
        # Also includes 130-135 to show multiple can be masked
        detector_column_ranges = [[126, 127], [130, 135]]

MASK x1 x2 y1 y2
----------------

This command was used to specify a **rectangular box** of detector pixels
to be omitted from the calculation during data reduction.

The parameters were specified in mm.

**This command is not implemented in Mantid as there are other ways to
achieve the same outcome (eg, using the Instrument View tools).** Also, a
combination of MASK[/FRONT][/REAR] Hn>Hm and MASK[/FRONT][/REAR] Vn>Vm
could be used to replicate some of the same functionality.

**Existing Example**

..  code-block:: none

    MASK 0 40 0 40

**Replacement Example**

Unsupported

.. _mask_line_two_params:

MASK/LINE a b
-------------

This command was used to specify a **diagonal line** of detector pixels
to be omitted from the calculation during data reduction. See also
:ref:`mask_line_four_params-ref`.

The line started at the centre of the scattering pattern (see SET CENTRE a b)
and extended to the edge of the pattern at the specified angle b with the
specified width a in mm. Only pixels wholly within the line were excluded. The
angle was defined in the same way as for L/PHI.

An infinite cylinder (length 100m) with the angle and width set by the user is
created in the plane of the detector from the point at which the transmitted
beam is incident on the detector.

:ref:`algm-MaskDetectorsInShape` is subsequently used the apply the generated shape.
The central point of each detector must lie within the shape to be masked, partially
overlapping detectors (whose centre does not sit in the masked region) will not be masked.

The primary use of this command was to mask out the beamstop support arm on some
instruments.

**Replacement**

..  code-block:: none

    beamstop_shadow = {width = a, angle = b}

**Existing Example:**

..  code-block:: none

    MASK/LINE 30 170

**Replacement Example**

..  code-block:: none

    [mask]
      beamstop_shadow = {width = 0.03, angle = 170.0}

.. _mask_line_four_params-ref:

MASK/LINE a b c d
-----------------

This command was used to specify a **diagonal line** of detector pixels
to be omitted from the calculation during data reduction. See also :ref:`mask_line_two_params`.

This command works identically to :ref:`mask_line_two_params`. Instead of starting at (0, 0)
the coordinates for x and y (represented by c and d) are given by the user.

Note that whilst parameter a was given in mm, c and d were specified in metres
even in legacy files!

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

MASK Sn
-------

This command was used to specify individual detector spectra (ie, pixels)
to be omitted from the calculation during data reduction.

The TOML replacement command actually permits several spectra to be
specified at once.

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

MASK/T t1 t2
------------

This command was used to specify regions of the time-of-flight spectrum
**in all spectra** to be omitted from the calculation during data reduction.
Note that the action of this command differs from FIT/CENTRE and FIT/MONITOR.

Times were specified in microseconds.

The TOML replacement command actually permits multiple time ranges to be
specified at once.

**Replacement**

..  code-block:: none

    [mask]
      [mask.time]
        tof = [
            {start = t1, stop = t2},
            {start = t3, stop = t4},
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

MON/DIRECT[/FRONT][/HAB]=filename
---------------------------------

This command was used to specify the name of a file containing the ratio
of the efficiency of the detector to that of the incident beam monitor as
a function of wavelength.

If the /FRONT or /HAB qualifiers, which were equivalent (/HAB was retained
for backward compatibility), are omitted then the command was assumed to
refer to the rear detector.

The efficiency file was required to be in 1D RKH text format with data arranged
as wavelength (in Angstroms), efficiency ratio, uncertainty on efficiency ratio.

**Replacement**

..  code-block:: none

    [detector]
      [detector.correction.direct]
        rear_file = "filename"
        front_file = "filename"

**Existing Example:**

..  code-block:: none

    MON/DIRECT=DIRECT_RUN524.dat
    MON/DIRECT/HAB=DIRECT_RUN524.dat

**Replacement Example**

..  code-block:: none

    [detector]
      [detector.correction.direct]
        rear_file = "DIRECT_RUN524.dat"
        front_file = "DIRECT_RUN524.dat"

MON/FLAT[/FRONT][/REAR]=filename
--------------------------------

This command was used to specify the name of a file containing the relative
efficiency of the individual detector pixels, also known as the 'flat cell' or
'flood source' file.

If the /FRONT qualifier was omitted then the command was assumed to refer to
the rear detector.

The relative efficiency file was required to be in 1D RKH text format with
data arranged as spectrum number, relative efficiency, uncertainty on relative
efficiency.

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

.. _habeff-ref:

MON/HABEFF=a
------------

This command was used to specify an approximate correction to the LOQ
instrument high-angle detector efficiencies arising from the longer path
length through the detection volume at high angles. See also :ref:`habpath-ref`.

The correction assumed a value (parameter a) for the efficiency at
1 Angstrom, the default value of which was 0.2. Setting a=1.0 was akin
to ignoring this correction.

**This command was never (knowingly) implemented in Mantid (but was in COLETTE)!**

**Existing Example:**

..  code-block:: none

    MON/HABEFF=0.2

**Replacement Example**

Unsupported

.. _habpath-ref:

MON/HABPATH[/ON][/OFF]
----------------------

This command was used to activate a correction to calculated transmissions
on the LOQ instrument arising from the longer path length through the
sample/can at high angles. See also :ref:`habeff-ref`.

**This command was never implemented in Mantid (but was in COLETTE)! But see
SAMPLE/PATH[/ON][/OFF].**

**Existing Example:**

..  code-block:: none

    MON/HABPATH/ON

**Replacement Example**

Unsupported

MON/LENGTH=z s
--------------

This command was intended to override the default distance of the specified
monitor s stored in the Mantid Instrument Definition File in instances where
a very accurate time-of-flight calculation was required. The parameter z was
the moderator-monitor distance.

**This command was never (knowingly) implemented in Mantid!** But see :ref:`trans_transpec-ref`.

**Replacement Example**

Unsupported

MON/SPECTRUM=n
------------------------------------

This command was used to specify which monitor *spectrum* (not number) was to
be used for normalisation during data reduction.

..  code-block:: none

  [instrument.configuration]
    norm_monitor = "Mn"

  [normalisation]
    #Normalisation monitor

    [normalisation.monitor.Ma]
      spectrum_number = n1

    [normalisation.monitor.Mb]
      spectrum_number = n2

**Existing Example:**

..  code-block:: none

    MON/SPECTRUM=1

**Replacement Example**

..  code-block:: none

  [instrument.configuration]
    norm_monitor = "M1"

  [normalisation]
    [normalisation.monitor.M1]
      spectrum_number = 1

.. _mon_interpolate-ref:

MON [/INTERPOLATE]
------------------
The optional /INTERPOLATE qualifier could be used to apply an interpolating
rebin of the specified monitor spectrum. This could be useful as a means of
'smoothing' noisy monitor spectra where the normal rebin command generated
'stepped' histograms.

This command has been been made obsolete by the switch to monitors running
in Event mode.

**Existing Example:**

..  code-block:: none

    MON/SPECTRUM=1/INTERPOLATE

**Replacement Example**

Unsupported - Obsolete

MON/TRANS/SPECTRUM=n
------------------------------------
This command could also be used to specify which monitor *spectrum* (not number) was to
be used for normalisation during data reduction. As the /TRANS qualifier was
present the command only applied to the normalisation of *transmission*
spectra.

..  code-block:: none

  [instrument.configuration]
    norm_monitor = "Ma"
    trans_monitor = "Mb"

  [normalisation]
    #Normalisation monitor

    [normalisation.monitor.Ma]
      spectrum_number = n1

    [normalisation.monitor.Mb]
      spectrum_number = n2

    [normalisation.monitor.Mc]
      spectrum_number = n3

  [transmission]
    [transmission.monitor.Mb]
      use_different_norm_monitor = true
      trans_norm_monitor = "Mc"

**Existing Example:**

..  code-block:: none

    MON/SPECTRUM=1
    TRANS/TRANSPEC=2
    MON/TRANS/SPECTRUM=4

**Replacement Example**

..  code-block:: none

  [instrument.configuration]
    norm_monitor = "M1"
    trans_monitor = "M2"

  [normalisation]
    [normalisation.monitor.M1]
      spectrum_number = 1

    [normalisation.monitor.M4]
      spectrum_number = 4

  [transmission]
    [transmission.monitor.M2]
      spectrum_number = 2
      use_different_norm_monitor = true
      trans_norm_monitor = "M4"

  # If interpolation is also required:
  [binning]
    [binning.2d_reduction]
      interpolate = true

MON/TRANS[/INTERPOLATE]
-----------------------

See :ref:`mon_interpolate-ref`.

PRINT string
------------

This command was used to write an arbitrary string to the Mantid Results
Log or Messages windows (depending on the Mantid version).

There is no equivalent of this command in TOML User Files but there are
still two ways to include equivalent information in the file:

- The first is within the [metadata] block at the top of the file; for
  example:

..  code-block:: none

    [metadata]
      name = "Using beamstop M4 for transmissions"
      created = "2021-09-09"
      original_file = "USER_LOQ_211G_M4_hab_log.toml"
      mantid_interface = "SANS v2"

- The other is in the form of comments; for example:

..  code-block:: none

    [instrument.configuration]
      #Remember to use METRES!
      sample_aperture_diameter = 0.008  # Used for q_resolution
      sample_offset = 0.00              #(-11.0 + Lms) = -0.355 + flange-to-sample-distance

**Existing Example:**

..  code-block:: none

    PRINT Using beamstop M4 for transmissions

**Replacement Example**

Unsupported

QRESOL[/ON][/OFF]
-----------------

This command was used to specify whether data reduction should *also* calculate
an estimate of the Q-resolution. If gravity corrections are also enabled (see
GRAVITY[/ON/OFF]) these are included in the calculation.

For more information, see the
`TOFSANSResolutionByPixel <https://docs.mantidproject.org/nightly/algorithms/TOFSANSResolutionByPixel-v1.html>`_
algorithm description.

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

QRESOL/MODERATOR=filename
-------------------------

This command was used to specify the name of a file containing the
moderator time spread as a function of wavelength. At ISIS these
data were produced from moderator performance simulations conducted
by R Bewley & S Ansell. **For sensible estimates of the Q-resolution
it is imperative that the moderator file be for the moderator in use!**

For more information, see the
`TOFSANSResolutionByPixel <https://docs.mantidproject.org/nightly/algorithms/TOFSANSResolutionByPixel-v1.html>`_
algorithm description.

The moderator file was required to be in 1D RKH text format with
data arranged as wavelength (in Angstroms), time spread (in microseconds),
uncertainty on time spread (zero if unknown).

**Replacement**

..  code-block:: none

  [q_resolution]
    moderator_file = "filename"

**Existing Example:**

..  code-block:: none

    QRESOL/MODERATOR=ModeratorStdDev_TS2_SANS_LETexptl_07Aug2015.txt

**Replacement Example**

..  code-block:: none

  [q_resolution]
    moderator_file = "ModeratorStdDev_TS2_SANS_LETexptl_07Aug2015.txt"

QRESOL/DELTAR=dr
----------------

This command was used to specify the width of the virtual rings used
for the radial integration. A value of 3 mm would be typical, otherwise
it would be sensible to use the rstep value specified in the
L/R r1 r2 [rstep] command if present.

For more information, see the
`TOFSANSResolutionByPixel <https://docs.mantidproject.org/nightly/algorithms/TOFSANSResolutionByPixel-v1.html>`_
algorithm description.

The virtual ring width of the detector in meters.
This is used to calculate the Q Resolution from TOF SANS Data on a per-pixel
in :ref:`algm-TOFSANSResolutionByPixel`.

**Replacement**

..  code-block:: none

  [q_resolution]
    delta_r = dr

**Existing Example:**

..  code-block:: none

  QRESOL/DELTAR=3  # m

**Replacement Example**

..  code-block:: none

  [q_resolution]
    delta_r = 0.003  # mm

.. _a1-ref:

QRESOL/A1=x
-----------

This command was used to specify the **source** aperture *diameter* to be
used in the estimation of the Q-resolution. See also :ref:`a2-ref` and
:ref:`h1_w1_h2_w2-ref`.

For more information, see the
`TOFSANSResolutionByPixel <https://docs.mantidproject.org/nightly/algorithms/TOFSANSResolutionByPixel-v1.html>`_
algorithm description.

**This command assumes that the data were collected on an instrument with
pinhole collimation!**

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

.. _a2-ref:

QRESOL/A2=x
-----------

This command was used to specify the **sample** aperture *diameter* to be
used in the estimation of the Q-resolution. See also :ref:`a1-ref` and
:ref:`h1_w1_h2_w2-ref`.

For more information, see the
`TOFSANSResolutionByPixel <https://docs.mantidproject.org/nightly/algorithms/TOFSANSResolutionByPixel-v1.html>`_
algorithm description.

**This command assumes that the data were collected on an instrument with
pinhole collimation!**

The sample aperture will normally be smaller than the source aperture!

Note that because the source aperture size is frequently altered, the ISIS
SANS Group decided to place the TOML replacement in the [instrument.configuration]
block at the top of TOML User Files instead of the [q_resolution] block.

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

.. _h1_w1_h2_w2-ref:

QRESOL[/H1=x1][/W1=y1][/H2=x2][/W2=y2]
--------------------------------------

This command was used to specify the **source and sample** slit sizes to be
used in the estimation of the Q-resolution. See also :ref:`a1-ref` and
:ref:`a2-ref`.

For more information, see the
`TOFSANSResolutionByPixel <https://docs.mantidproject.org/nightly/algorithms/TOFSANSResolutionByPixel-v1.html>`_
algorithm description.

**This command assumes that the data were collected on an instrument with
slit/jaw collimation!**

The sample slit size will normally be smaller than the source slit size! But
the heights and widths of a slit do not have to be the same.

**Replacement**

..  code-block:: none

  [q_resolution]
    h1 = x1
    w1 = y1
    h2 = x2
    w2 = y2

**Existing Example:**

..  code-block:: none

    QRESOL/H1=16.0
    QRESOL/W1=16.0
    QRESOL/H2=8.0
    QRESOL/W2=8.0

**Replacement Example**

..  code-block:: none

  [q_resolution]
    h1 = 0.016
    w1 = 0.016
    h2 = 0.008
    w2 = 0.008

.. _qresol-lcollim-z:

QRESOL/LCOLLIM=z
----------------

This command was used to specify the length of the collimation - the distance
between the source and sample apertures/slits/jaws - to be used in the
estimation of the Q-resolution.

For more information, see the
`TOFSANSResolutionByPixel <https://docs.mantidproject.org/nightly/algorithms/TOFSANSResolutionByPixel-v1.html>`_
algorithm description.

Note that because the collimation length is frequently altered, the ISIS
SANS Group decided to place the TOML replacement in the [instrument.configuration]
block at the top of TOML User Files instead of the [q_resolution] block.

Also note that the collimation length was historically specified in metres too.

**Replacement**

..  code-block:: none

  [instrument.configuration]
    collimation_length = z

**Existing Example:**

..  code-block:: none

    QRESOL/LCOLLIM=4.0

**Replacement Example**

..  code-block:: none

  [instrument.configuration]
    collimation_length = 4.0

SAMPLE/OFFSET z
---------------

This command was used to specify any correction to the default Z coordinate
in the Mantid Instrument Definition File defining the nominal position of the
sample. The offset is a relative value with positive offsets translating the
sample position *towards* the detector(s).

**Replacement**

..  code-block:: none

  [instrument.configuration]
    sample_offset = z

**Existing Example:**

..  code-block:: none

    SAMPLE/OFFSET -60

**Replacement Example**

..  code-block:: none

  [instrument.configuration]
    sample_offset = -0.06

SAMPLE/PATH[/ON][/OFF]
----------------------

This command was used to activate a correction to calculated transmissions
arising from the longer path length through the sample/can at high angles.
Unlike MON/HABPATH[/ON][/OFF] this command was generic.

For more information, see the
`SANSWideAngleCorrection <https://docs.mantidproject.org/nightly/algorithms/SANSWideAngleCorrection-v1.html>`_
algorithm description.

**Existing Example:**

..  code-block:: none

    SAMPLE/PATH/ON

**Replacement Example**

Unsupported, pending future discussion.

.. _set_centre-ref:

SET CENTRE a b
--------------

This command was used to specify the (x,y) coordinates (in real-space) of
the centre of the scattering pattern on the rear (ie, main) detector. See
also SET CENTRE[/MAIN][/HAB] a b [c d].

**Warning: the TOML replacement for this command will apply the same
centre coordinates to a front detector if present. In most instances
this will not be sensible.**

..  code-block:: none

    [detector]
      [detector.configuration]
        all_centre = {x=a, y=b}

**Existing Example:**

..  code-block:: none

    SET CENTRE 84.2 -196.5

**Replacement Example**

..  code-block:: none

    [detector]
      [detector.configuration]
        # This will set both front and rear to the same centre values.
        all_centre = {x=0.0842, y=-0.1965}

SET CENTRE[/MAIN][/HAB] a b [c d]
---------------------------------

This command was used to specify the (x,y) coordinates (in real-space) of
the centre of the scattering pattern on a specific detector. Compare with
SET CENTRE a b.

If the /MAIN qualifier was omitted the command was assumed to apply to
the main (ie, rear) detector anyway. The /HAB qualifier was required to
specify the beam centre coordinates for a high-angle (ie, front) detector.

The parameters c and d allowed the size of the detector pixels in x & y
to be passed to the data reduction.

Approximate centre coordinates on the ISIS SANS instruments (which should
be optimised using the beam centre finder tool!) are:

..  code-block:: none

    LARMOR: ( 0.020,  1.000)
    LOQ:    ( 0.320,  0.320)
    SANS2D: ( 0.100, -0.080)
    ZOOM:   (-0.170, -0.050)

..  code-block:: none

    [detector]
      [detector.configuration]
        front_centre = {x=a, y=b}
        rear_centre = {x=a, y=b}

**Existing Example:**

..  code-block:: none

    SET CENTRE 324.31 328.547 5.00 5.00
    SET CENTRE/HAB 317.92 325.498

**Replacement Example**

..  code-block:: none

    [detector]
      [detector.configuration]
        # Note for identical results the values will
        # only take a and b in the above example due to a bug
        # with the legacy user file parser
        front_centre = {x=0.31792, y=0.325498}
        rear_centre = {x=0.32431, y=0.328547}

SET SCALES a b c d e
--------------------

This command was used to specify the absolute intensity calibration scale
factor (parameter a) to be applied to all intensity values at the end of
the data reduction calculation.

In the case of the LOQ instrument, it also allowed the relative scaling of
the four high-angle detector banks (parameters b, c, d & e) to be accounted
for (as a*b, a*c, a*d & a*e). For all other ISIS SANS instruments these
four parameters should be set to unity.

**Note: In 2020 it was discovered that due to a forever bug in the legacy
User File command parser the parameters b, c, d & e have never been implemented
in Mantid.** See this `issue <https://github.com/mantidproject/mantid/issues/27948>`_.

All workspaces are currently scaled by the value represented by `a` for all values,
rather than on a per-bank basis.

The TOML replacement command allows separate but single scaling factors for
both rear and front detectors to be specified. But to maintain compatibility
`front_scale` is ignored by the parser and will not do anything.

..  code-block:: none

    [detector]
      [detector.configuration]
        front_scale = a
        rear_scale = a

**Existing Example:**

..  code-block:: none

    SET SCALES 0.02938 1.0 1.0 1.0 1.0

**Replacement Example**

..  code-block:: none

    [detector]
      [detector.configuration]
        front_scale = 1.0
        rear_scale = 0.02938

SET[/NOTABLES] BANK a b c d e
-----------------------------

This command was used to specify the physical location and orientation
of the four LOQ instrument high-angle detector modules.

The parameters were: an ISIS Detector ID Code, the distance from the
moderator (in metres), an anticlockwise rotation angle, and the x & y
coordinates (in mm) of the first pixel on the specified module. Viewed
from the direction of travel of the neutrons, positive values of x & y
corresponded to right and up, respectively.

The /NOTABLES (/NOTAB was also supported) qualifier could be used to
stop a redundant call to the routine mapping detectors.

**This command became redundant with improvements in the Mantid Instrument
Definition File.**

**Existing Example:**

..  code-block:: none

    SET/NOTAB BANK 305 11.582 0. 112.28 -245.19
    SET/NOTAB BANK 304 11.582 90. 244.28 114.82
    SET/NOTAB BANK 306 11.582 180. -115.72 246.82
    SET/NOTAB BANK 307 11.582 270. -247.72 -113.19

**Replacement Example**

Unsupported

.. _clear_xcor_ycor-ref:

SET[/XCOR][/YCOR][/ON][/OFF][/CLEAR]
------------------------------------

This command was used to specify if non-linear coordinate
corrections to LOQ instrument detector data should be applied
during data reduction. The /XCOR (/XC was also supported) qualifier
specified that detector x coordinates were to be corrected. Similarly,
the /YCOR (or /YC) qualifier specified that detector y coordinates
were to be corrected. See also :ref:`set_xcor_ycor-ref`.

The /CLEAR qualifier was equivalent to /OFF.

**Existing Example:**

..  code-block:: none

    SET/XCOR/ON

**Replacement Example**

Unsupported

.. _set_xcor_ycor-ref:

SET[/NOTABLES][/XCOR][/YCOR]=filename
-------------------------------------

This command was used to specify a file containing non-linear
coordinate corrections to LOQ instrument detector data. Separate files
were required for the x and y coordinates. See also :ref:`clear_xcor_ycor-ref`.

The /NOTABLES (/NOTAB was also supported) qualifier could be used to
stop a redundant call to the routine mapping detectors if both x and y1
coordinates were being corrected (see example below).

**This command became redundant from Mantid 1.1.9556 and LOQ_Definition.xml valid from 2002-02-26.**

**Existing Example:**

..  code-block:: none

    SET/NOTAB/XC=xcorr.991_994
    SET/YC=ycorr.991_994

**Replacement Example**

Unsupported

.. _trans_mask-ref:

TRANS/MASK=filename
--------------------

This command was used in conjunction with TRANS/RADIUS=r or, more likely,
TRANS/ROI=filename to *exclude* regions of the detector specified by those
commands. See also :ref:`trans_radius-ref` and :ref:`trans_roi-ref`.

filename was expected to be a Mantid mask file in XML format.

**Note that if also present a TRANS/TRANSPEC=s command would always supersede a
TRANS/MASK=filename command.** See also :ref:`trans_transpec-ref`.

**Existing Example:**

..  code-block:: none

    TRANS/ROI=select.xml
    TRANS/MASK=exclude.xml

**Replacement Example**

Unsupported, see :ref:`trans_roi-ref`.

.. _trans_radius-ref:

TRANS/RADIUS=r
--------------

This command was used to specify a circular region-of-interest (ROI) of radius r
on the detector taking the transmitted beam which was to be used in place of a
dedicated transmission monitor. The ROI was assumed to be centred on the beam
centre coordinates (see :ref:`set_centre-ref`). See also :ref:`trans_mask-ref`
and :ref:`trans_roi-ref`.

For this command to have had any sensible purpose, it would have been necessary
for the detector beamstop to have been removed for transmission measurements.

The radius was specified in mm.

**Note that if also present a TRANS/TRANSPEC=s command would always supersede a
TRANS/RADIUS=r command.** See also :ref:`trans_transpec-ref`.

**Existing Example:**

..  code-block:: none

    TRANS/RADIUS=30

**Replacement Example**

Unsupported, pending future discussion.

.. _trans_roi-ref:

TRANS/ROI=filename
------------------

This command was used to specify an arbitrary region-of-interest (ROI) on the
detector taking the transmitted beam which was to be used in place of a dedicated
transmission monitor. See also :ref:`trans_mask-ref` and :ref:`trans_radius-ref`.

For this command to have had any sensible purpose, the ROI would have been needed
to have been outside of any masked regions of the detector (eg, the beamstop
and/or beamstop support arm shadows).

filename was expected to be a Mantid mask file in XML format.

**Note that if also present a TRANS/TRANSPEC=s command would always supersede a
TRANS/ROI=filename command.** See also :ref:`trans_transpec-ref`.

**Replacement**

..  code-block:: none

    [instrument.configuration]
      trans_monitor = "ROI"

    [transmission]
      # This will be ignored:
      [transmission.monitor.Mn]
        spectrum_number = s

      [transmission.ROI]
        file = "roi_file.xml"


**Existing Example:**

..  code-block:: none

    TRANS/ROI=filename.xml

**Replacement Example**

    [instrument.configuration]
      trans_monitor = "ROI"

    [transmission]

      [transmission.ROI]
        file = "filename.xml"

.. _trans_transpec-ref:

TRANS/TRANSPEC=s
-----------------

This command was used to specify the spectrum (not monitor) *number*
containing the transmission data. The spectrum number and the monitor*
number may, or may not, be the same depending on the instrument!

**Replacement**

..  code-block:: none

    [instrument.configuration]
      # Where Mn is arbitrary but must match the section label
      trans_monitor = "Mn"

    [transmission]
      [transmission.monitor.Mn]
        spectrum_number = s

**Existing Example:**

..  code-block:: none

    TRANS/TRANSPEC=3

**Replacement Example**

..  code-block:: none

    [instrument.configuration]
      trans_monitor = "M3"

    [transmission]
      [transmission.monitor.M3]
        spectrum_number = 3

TRANS/TRANSPEC=s/SHIFT=dz
-------------------------

This command was used to specify any correction to the default Z coordinate
in the Mantid Instrument Definition File defining the nominal position of the
transmission monitor represented by the specified spectrum number. The offset
is a relative value with positive offsets translating the sample position
*towards* the detector(s).

This command was typically used to fine tune the position of beamstop-mounted
transmission monitors.

**Replacement**

..  code-block:: none

    [instrument.configuration]
      # Where Mn is arbitrary but must match the section label
      trans_monitor = "Mn"

    [transmission]
      [transmission.monitor.Mn]
        spectrum_number = s
		    shift = dz

**Existing Example:**

..  code-block:: none

    TRANS/TRANSPEC=17788/SHIFT=-12

**Replacement Example**

..  code-block:: none

    [instrument.configuration]
      trans_monitor = "M4"

    [transmission]
      [transmission.monitor.M4]
        spectrum_number = 17788
        shift = -0.012

TUBECALIBFILE=filename
----------------------

This command was used to specify a spatial calibration file for tube
array detectors. Only one file could be specified, and so if an instrument
had more than one such detector the calibrations for each needed to
be amalgamated.

**Replacement**

..  code-block:: none

    [detector]

    [detector.correction.tube]
      file = "filename"

**Existing Example:**

..  code-block:: none

    TUBECALIBFILE=TUBE_SANS2D_BOTH_64393_15Mar20.nxs

**Replacement Example**

..  code-block:: none

    [detector]

    [detector.correction.tube]
      file = "TUBE_SANS2D_BOTH_64393_15Mar20.nxs"


.. categories:: Techniques
