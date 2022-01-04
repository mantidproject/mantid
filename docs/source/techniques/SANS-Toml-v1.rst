.. _sans_toml_v1-ref:

===============
SANS TOML Files
===============

.. contents:: Table of Contents
    :local:

General Notes
=============

- Lengths are given in meters within TOML files, unlike previous legacy formats.


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

Conversion From Legacy User Files
=================================

Layout
------

This section is designed like a reference that users can paste straight into
existing TOML files.

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

For converting existing files I recommend the following process:

- Copy your existing user file
- Remove any commented out lines (starting with ``!``)
- Go line by line with this guide adding to a **blank** TOML file
- Delete each line from the copied user file as it's converted

BACK/MON/TIMES
--------------

Note: This command subtracts the *same* background level from *all* monitors.
The continued use of this method of monitor correction is now deprecated.
See BACK/M[n]/TIMES.

..  code-block:: none

    [normalisation]
      [normalisation.all_monitors]
        background = [x, y]
        enabled = true

**Existing Example**

..  code-block:: none

    BACK/MON/TIMES 30000 40000

**Existing Replacement**

..  code-block:: none

    [normalisation]
      [normalisation.all_monitors]
        background = [30000, 40000]
        enabled = true


BACK/M[n]/TIMES x y
-------------------

Note: This command subtracts the specified background level from the specified monitor.

..  code-block:: none

    # Note: both "normalisation" and "normalisation" are both accepted
    [normalisation]
      [normalisation.monitor.Mn]
        spectrum_number = n
        background = [x, y]

*OR*

..  code-block:: none

    [transmission]
      [transmission.monitor.Mn]
        spectrum_number = n
  	    use_own_background = true
        background = [x, y]


**Existing Example**

..  code-block:: none

    BACK/M1/TIMES 30000 40000

**Existing Replacement**

..  code-block:: none

    [normalisation]
      [normalisation.monitor.M1]
        spectrum_number = 1
        background = [30000.0, 40000.0]

COMPATIBILITY ON
----------------

**Replacement**
Unsupported


DET/CORR [FRONT][REAR] [X][Y][Z][ROT] a
---------------------------------------

..  code-block:: none

    [detector]
      [detector.calibration.position]
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

    DET/CORR REAR X 0.0
    DET/CORR REAR Z 58
    DET/CORR FRONT X -33
    DET/CORR FRONT Y -20
    DET/CORR FRONT Z -47
    DET/CORR FRONT ROT 0.0

**Existing Replacement**

..  code-block:: none

    [detector]
      [detector.calibration.position]
        front_x = -0.033
        front_y = -0.020
        front_z = -0.047
        front_rot = 0.0
        rear_x = 0.0
        rear_z = 0.058

DET/[LAB][HAB][MERGED][ALL]
---------------------------

Note: The group now refers to *LAB* and *HAB* as *rear* and *front* respectively.
All reduction types are lower case.

..  code-block:: none

    [detector.configuration]
      selected_detector = "rear"

**Existing Example**

..  code-block:: none

    DET/HAB

**Existing Replacement**

..  code-block:: none

    [detector.configuration]
      # Accepts "front", "rear", "merged", or "all".
      selected_detector = "front"

DET/RESCALE[/FIT] x y
---------------------

..  code-block:: none

  [reduction]
    [reduction.merged.rescale]
        min = x
        max = y
        use_fit = true  # or false

**Existing Example**

..  code-block:: none

    DET/RESCALE/FIT 0.14 0.24


**Existing Replacement**

..  code-block:: none

  [reduction]
    [reduction.merged.rescale]
      min = 0.14
      max = 0.24
      use_fit = true

DET/SCALE x
-----------

..  code-block:: none

  [reduction]
    [reduction.merged.rescale]
        factor = x
        use_fit = false  # Must be false for single value

**Existing Example**

..  code-block:: none

    DET/SCALE 0.123


**Existing Replacement**

..  code-block:: none

  [reduction]
    [reduction.merged.rescale]
        factor = 0.123
        use_fit = false

DET/SCALE x y /FIT
------------------

..  code-block:: none

  [reduction]
    [reduction.merged.rescale]
        min = x
        max = y
        use_fit = true  # Must be true for fitting

**Existing Example**

..  code-block:: none

    DET/SCALE 0.1 0.2 /FIT


**Existing Replacement**

..  code-block:: none

  [reduction]
    [reduction.merged.rescale]
        min = 0.1
        max = 0.2
        use_fit = true  # Must be true for fitting


DET/SHIFT x
-----------

..  code-block:: none

  [reduction]
    [reduction.merged.shift]
        distance = x
        use_fit = false  # Must be false for single value

**Existing Example**

..  code-block:: none

    DET/SHIFT 0.123


**Existing Replacement**

..  code-block:: none

  [reduction]
    [reduction.merged.shift]
        distance = 0.123
        use_fit = false

DET/SHIFT x y /FIT
------------------

..  code-block:: none

  [reduction]
    [reduction.merged.shift]
        min = x
        max = y
        use_fit = true  # Must be true for fitting

**Existing Example**

..  code-block:: none

    DET/SHIFT 0.1 0.2 /FIT

**Existing Replacement**

..  code-block:: none

  [reduction]
    [reduction.merged.shift]
        min = 0.1
        max = 0.2
        use_fit = true


DET/OVERLAP x y
---------------

..  code-block:: none

  [reduction]
    [reduction.merged.merge_range]
      min = x
      max = y
      use_fit = true

**Existing Example**

..  code-block:: none

    DET/OVERLAP 0.14 0.24


**Existing Replacement**

..  code-block:: none

  [merged]
    [reduction.merged.merge_range]
        min = 0.14
        max = 0.24
        use_fit = true


FIT/CENTRE x y
---------------

**Replacement**
Unsupported

FIT/MID
-------

**Replacement**
Unsupported

FIT/MONITOR x y
---------------

*Note:* This was only enabled for LOQ in source code, so
if you are not converting a LOQ file this should not be copied
as it will produce different results

**Replacement**

..  code-block:: none

  [mask]
    prompt_peak = {start = x, stop = y}

**Existing Example**

..  code-block:: none

    FIT/MONITOR 19900 20500

**Existing Replacement**

..  code-block:: none

  [mask]
    prompt_peak = {start = 19900.0, stop = 20500.0}


FIT/TRANS/LIN x y
-----------------

**Replacement**

..  code-block:: none

    [transmission]
      [transmission.fitting]
        enabled = true
        parameters = {lambda_min = x, lambda_max = x}
        # Can be: Linear / Logarithmic / Polynomial
        function = "Linear"
        # Only used when set to Polynomial
        polynomial_order = 2

**Existing Example**

..  code-block:: none

    FIT/TRANS/LIN 3.0 11.0

**Existing Replacement**

..  code-block:: none

    [transmission]
      [transmission.fitting]
        enabled = true
        parameters = {lambda_min = 3.0, lambda_max = 11.0}
        function = "Linear"

GRAVITY/ON
----------

**Replacement**

..  code-block:: none

    [instrument.configuration]
      gravity_enabled = true

**Existing Example**

..  code-block:: none

    GRAVITY/ON

**Existing Replacement**

..  code-block:: none

    [instrument.configuration]
      gravity_enabled = true


GRAVITY/LEXTRA x
----------------

**Replacement**

..  code-block:: none

    [instrument.configuration]
      gravity_extra_length = x

**Existing Example**

..  code-block:: none

    GRAVITY/LEXTRA 2.0

**Existing Replacement**

..  code-block:: none

    [instrument.configuration]
      gravity_extra_length = 2.0


L/EVENTSTIME str
----------------

**Replacement**

..  code-block:: none

  [reduction.events]
    binning = "str"

**Existing Example**

..  code-block:: none

    L/EVENTSTIME 7000.0,500.0,60000.0

**Existing Replacement**

..  code-block:: none

  [reduction.events]
    # A negative step (middle val) indicates Log
    binning = "7000.0,500.0,60000.0"


L/PHI [/NOMIRROR] x y
---------------------


**Replacement**

..  code-block:: none

    [mask]
      [mask.phi]
        mirror = bool
        start = x
        stop = y

**Existing Example**

..  code-block:: none

    L/PHI/NOMIRROR -45 45

**Existing Replacement**

..  code-block:: none

    [mask]
      [mask.phi]
        mirror = false
        start = x
        stop = y


L/Q rebin_string
----------------

**Replacement**

..  code-block:: none

    [binning.1d_reduction]
        # Negative indicates log
        binning = "rebin_string"

**Existing Example**

..  code-block:: none

    L/Q .02,0.05,0.5,-0.1,10

**Existing Replacement**

..  code-block:: none

    [binning]
      [binning.1d_reduction]
        # Negative indicates log
        binning = "0.02,0.05,0.5,-0.1,10.0"

L/Q/RCUT x
----------

**Replacement**

..  code-block:: none

    [binning.1d_reduction]
        radius_cut = x

**Existing Example**

..  code-block:: none

    L/Q/RCUT 100

**Existing Replacement**

..  code-block:: none

    [binning]
      [binning.1d_reduction]
        radius_cut = 0.1


L/Q/WCUT x
----------

**Replacement**

..  code-block:: none

    [binning.1d_reduction]
        wavelength_cut = x

**Existing Example**

..  code-block:: none

    L/Q/WCUT 8

**Existing Replacement**

..  code-block:: none

    [binning]
      [binning.1d_reduction]
        wavelength_cut = 8.0

L/QXY start stop step [/LIN]
----------------------------

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

**Existing Replacement**

..  code-block:: none

    [binning]
      [binning.2d_reduction]
        step = 0.002
        stop = 0.1
        type = "Lin"

L/R x y [step]
--------------

Note step was ignored previously.

..  code-block:: none

    [detector]
      radius_limit = {min = 0.038, max = -0.001}

**Existing Example**

..  code-block:: none

    L/R 38 -1

**Existing Replacement**

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

**Existing Replacement**

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

**Existing Replacement**

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

**Existing Replacement**

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

**Existing Replacement**

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

**Existing Replacement**

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

**Existing Replacement**

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

**Existing Replacement**

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

**Existing Replacement**

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

**Existing Replacement**

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

**Existing Replacement**

..  code-block:: none

    [mask]
      beamstop_shadow = {width = 0.03, angle = 170.0, x_pos=0.3, y_pos=0.1}

MON/DIRECT="filename"
---------------------

**Replacement**

..  code-block:: none

    [detector]
      [detector.calibration.direct]
        rear_file = "filename"
        front_file = "filename"


**Existing Example:**

..  code-block:: none

    MON/DIRECT=DIRECT_RUN524.dat

**Existing Replacement**

..  code-block:: none

    [detector]
      [detector.calibration.direct]
        rear_file = "DIRECT_RUN524.dat"
        front_file = "DIRECT_RUN524.dat"

MON/FLAT=str
------------

**Replacement**

..  code-block:: none

    [detector]
      [detector.calibration.flat]
        rear_file = "str"


**Existing Example:**

..  code-block:: none

    MON/FLAT="flat_file.091"

**Existing Replacement**

..  code-block:: none

    [detector]
      [detector.calibration.flat]
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

**Existing Replacement**

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

**Existing Replacement**

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

**Existing Replacement**

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

**Existing Replacement**

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

**Existing Replacement**

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

**Existing Replacement**

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

**Existing Replacement**

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

**Existing Replacement**

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

**Existing Replacement**

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

**Existing Replacement**

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

**Existing Replacement**

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

**Existing Replacement**

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

**Existing Replacement**

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

  [detector.calibration.tube]
    file = "str"

**Existing Example:**

..  code-block:: none

  TUBECALIBFILE=Tube.nxs

**Existing Replacement**

..  code-block:: none

  [detector]

  [detector.calibration.tube]
    file = "Tube.nxs"

.. categories:: Techniques
