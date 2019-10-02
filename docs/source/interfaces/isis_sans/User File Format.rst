.. _ISIS_SANS_User_File-ref:

================
User File Format
================

.. contents:: Table of Contents
  :local:

Instrument Selection
--------------------
This should be the first command in a user file for the following instruments:

- LOQ
- SANS2D

BACK
----
BACK is used to specify a time region in which to measure the
(time-independent) neutron background count level. The background is
calculated and subtracted from the monitor and transmission spectra before
the data are rebinned into wavelength. It is possible to set the TOF time
window for all monitors or just one specific monitor.

Syntax
^^^^^^

.. code-block:: none

    BACK/MON/TIMES t1 t2
    BACK/Mm/TIMES t1 t2
    BACK/Mm t1 t2
    BACK/Mm/OFF

Qualifiers
^^^^^^^^^^
**/MON/TIMES**
    Set the start and end of the time of flight window assumed for all monitors

**/M**
    Used to set the time of flight window that should be used for a single
    specified monitor

**/Mm/OFF**
    This command is only relevant to use after /MON/TIMES to turn off
    background for a specific monitor.


Parameters
^^^^^^^^^^
**t1 t2**
    Neutron times of flight (microseconds) specifying the time region
    to be considered as the background level


**m**
    The spectrum number of the spectrum produced by the monitor of interest
    (note spectrum numbers start at 1 as opposed to workspace indices which are
    normally offset by 1)

DET
---
DET specifies which detector data is to be corrected, or whether detector
position encoder values are to be corrected.

Note for now that the /CORR commands only have an effect on SANS2D.

Syntax
^^^^^^

.. code-block:: none

    DET/CORR/FRONT/qualifier [parameter]
    DET/CORR/REAR/qualifier [parameter]
    DET/FRONT
    DET/REAR
    DET/BOTH
    DET/MERGED
    DET/MERGE
    DET/MAIN
    DET/HAB

    DET/RESCALE rescale 
    DET/SHIFT shift  
    DET/RESCALE/FIT [Q1 Q2]
    DET/SHIFT/FIT [Q1 Q2]

Qualifiers
^^^^^^^^^^

**/X x1**
    Applies a correction to the detector X encoder value
**/Y y1**
    Applies a correction to the detector Y encoder value
**/Z z1**
    Applies a correction to the detector Z encoder value
**/ROT d1**
    Applies a correction to the detector Rotary encoder value
**/RADIUS r**
    Increase the apparent radius from the rotation axis of the detector to
    the active plane
**/SIDE s**
    Translate the rotation axis of the detector perpendicular to the plane
    of the detector.
**/FRONT**
    When used as DET/CORR/FRONT correct data from the Front detector
    (if SANS2D) or High-Angle Bank (if LOQ).
    When used as DET/FRONT set the analysis to apply to the FRONT detector.
**/HAB**
    This command applies only to LOQ. It is equivalent to DET/FRONT
**/REAR**
    When used as DET/CORR/REAR correct data from the Rear detector (if SANS2D)
    or Main detector (if LOQ). When used as DET/REAR set the analysis to apply
    to the REAR detector.
**/MAIN**
    This command applies only to LOQ. It is equivalent to DET/REAR
**/BOTH**
    When used as DET/BOTH set the analysis to apply both detectors.
    It is equivalent to set the SANS Dialog Analysis to Detector Bank -> both.
**/MERGED**
    When used as DET/MERGED set the analysis to apply both detectors and them
    merge them in one. It is equivalent to set the SANS Dialog Analysis to
    Detector Bank -> merged.
**/MERGE**
    Same as /MERGED
**/RESCALE rescale**
    Rescale front detector data, defaults to 1.0. Multiplying the front
    detector reduced data by this number
**/SHIFT shift**
    Shift const background of front detector data, defaults to 0.0
**/RESCALE/FIT [Q1 Q2]**
    If specified fit RESCALE so front and back data match. Optionally
    provide fitting range Q1 to Q2. If not specified will use entire
    overlapping Q region of FRONT and REAR detector data
**/SHIFT/FIT [Q1 Q2]**
    If specified fit SHIFT so front and back data match
**/OVERLAP Q1 Q2**
    If specified is the region between which the merged data will be used

Parameters
^^^^^^^^^^

**x1**
    Amount (mm) by which to correct the detector X encoder value
    from the run log file
**y1**
    Amount (mm) by which to correct the detector Y encoder value
    from the run log file
**z1**
    Amount (mm) by which to correct the detector Z encoder value
    from the run log file
**d1**
    Amount (degrees) by which to correct the detector Rotary encoder
    value from the run log file
**r**
    Amount (mm) by which to correct the detector RADIUS encoder value
**s**
    Amount (mm) by which to correct the detector SIDE encoder value

FIT
---

FIT specifies whether the calculated transmission data should be fitted to a
simple function. Subsequent data treatment then uses values interpolated
from the function. Fitting can improve the statistical quality of transmission
data and is implemented by CalculateTransmission.

Syntax
^^^^^^

.. code-block:: none

    FIT/TRANS/CLEAR  or  FIT/TRANS/OFF

    FIT/TRANS/LIN [w1 w2]  or  FIT/TRANS/LINEAR [w1 w2]  or  FIT/TRANS/STRAIGHT [w1 w2]
    FIT/TRANS/LOG [w1 w2]  or  FIT/TRANS/YLOG [w1 w2]

    FIT/MONITOR time1 time2

    FIT/TRANS/[CAN/|SAMPLE/][LIN|LOG|POLYNOMIAL[2|3|4|5]]

Qualifiers
^^^^^^^^^^
**/TRANS/CLEAR or TRANS/OFF**
    Disable fitting of transmissions
**/TRANS/LIN** or **/TRANS/LINEAR** or **/TRANS/STRAIGHT**
    Use a linear fit of the form Y=mX+C
**/TRANS/LOG** or **/TRANS/YLOG**
    Use a fit of the form Y=exp(aX+C)
**/TIME time1 time2**
    time1 and time2 will be the XMin and XMax passed to the algorithm RemoveBins. RemoveBins is called the all monitors, for transmission and normalisation
**/TRANS/POLYNOMIAL** # for # bigger than 2
    Use a Polynomial of order # of the form Y = c0 + c1X + c2X^2 + ...

Optional Qualifiers
^^^^^^^^^^^^^^^^^^^

**/TRANS/SAMPLE/...**
    Configure the settings for FIT just for the sample workspace. Ex: FIT/TRANS/SAMPLE/POLYNOMIAL3 - applies a third order polynomial to fit sample workspace for transmission. 
**/TRANS/CAN/...**
    Configure the settings for FIT just for the can workspace. Ex: FIT/TRANS/SAMPLE/LIN - fit the transmission of the can using a linear function. 

If SAMPLE or CAN is not provided, it is assumed that the FIT/TRANS option applies to both.

Parameters
^^^^^^^^^^

**w1 w2**
    [optional] Neutron wavelengths (Angstroms) specifying the fitting range
    Default if omitted is to use the full wavelength range

GRAVITY
-------

GRAVITY specifies whether detector data should be corrected for the ballistic effects of gravity on the neutrons. This correction is particularly important at long sample-detector distances and long wavelengths.

Syntax
^^^^^^

.. code-block:: none

    GRAVITY ON
    GRAVITY OFF
    GRAVITY/LEXTRA=l1

Qualifiers
^^^^^^^^^^

**/LEXTRA**
    Specifies the extra length in m that can be added to the gravity correction. The extra length is only taken into account when GRAVITY is explicitly set to ON. The LEXTRA qualifier needs be currently placed into a separate line. The default value is set to 0.0m which is used when the qualifier is not explicitly specified.

Parameters
^^^^^^^^^^

**ON**
    Use gravity correction
**OFF**
    Do not use gravity correction
**l1**
    The extra length in m.

COMPATIBILITY
-------------

COMPATIBILITY specifies whether the new backend should be run in compatibility mode such that it produces identical results to the old backend.

Syntax
^^^^^^

.. code-block:: none

    COMPATIBILITY ON
    COMPATIBILITY OFF

Parameters
^^^^^^^^^^

**ON**
    Use compatibility mode
**OFF**
    Don't use compatibility mode

L
---

L specifies various limits that configure or constrain the data reduction
process. Note that the command is L and not LIMIT!

Syntax
^^^^^^

.. code-block:: none

    L/PHI[/NOMIRROR] d1 d2**

    L/Q/ q1 q2 [dq[/LIN]]  or  L/Q q1 q2 [dq[/LOG]]
    L/Q q1 dq1 q3 dq2 q2  or  L/Q q1 -dq1 q2 -dq2 q3 (for logarithmic steps)

    L/Q/RCut c
    L/Q/WCut c

    L/QXY qxy1 qxy2 [dqxy[/LIN]]  or  L/QXY qxy1 qxy2 [dqxy[/LOG]]
    L/QXY qxy1 dqxy1 qxy2 dqxy2 qxy3 [/LIN]]  or  L/QXY qxy1 dqxy1 qxy2 dqxy2 qxy3 [/LOG]]

    L/R r1 r2

    L/WAV l1 l2 [dl[/LIN]  or  L/WAV l1 l2 [dl[/LOG]
    L/WAV l1 dl1 l3 dl2 l2 [/LIN]  or  L/WAV l1 dl1 l3 dl2 l2 [/LOG]

    L/EVENTSTIME rebin_str

Qualifiers
^^^^^^^^^^

**/PHI[/NOMIRROR]**
    Specifies the azimuthal sector of the detector to be included in a 2D data reduction, useful for processing anisotropic data
    The optional /NOMIRROR qualifier determines whether the mirror image sector with should be excluded (i.e. if say phi is specified 85 to 95 should -95 to -85 also be included)
**/Q**
    Specifies the Q range for a 1D reduction
**/Q/RCut**
    Specifies the RadiusCut property that is passed to Q1D in mm.
**/Q/WCut**
    The number following this will be passed to Q1D as the WaveCut property, in angstrom.
**/QXY**
    Specifies the Q range for a 2D reduction
**/R**
    Specifies the radial limits on the detector within which the radial integration of the data should be performed
**/WAV**
    Specifies the wavelength limits to be used in the data reduction
**/EVENTSTIME**
    Used to specifies a rebinning string to be applied to event mode data
**/LIN**
    [optional, default] Specifies that the step size or increment given is a constant value; ie, the bins are to be equally spaced
    The step size or increment should be given in the same units as the data X axis
**/LOG**
    [optional] Specifies that the step size or increment given is a geometric progression; ie, the bins will get further apart
    The step size or increment should be given in %/100; a typical value to match the wavelength resolution might be 5%
    Default is to use /LIN if omitted

Parameters
^^^^^^^^^^

**d1 d2**
    Minimum and maximum azimuthal angles (degrees) specifying the sector of interest; 0 degrees (or 360 degrees) is at 3 O'clock, 180 degrees (or -180 degrees) is at 9 O'clock
    To correct about the Equator use, say, L/PHI -30 30
    To correct about the Meridian use, say, L/PHI 60 120
**q1 q2**
    Minimum and maximum Q values (/Angstrom) over which a 1D reduction is to be performed; this will ultimately be constrained by the detector position and available wavelengths
    As a guide, try q1~0.002 & q2~0.3 (for SANS2D) or q1~0.006 & q2~0.3 (for LOQ)
**dq**
    [optional] Q step size
    The magnitude of this value will depend on whether a /LIN or /LOG qualifier is specified
    For variable step sizes in logarithmic form, use -dq, and remove the /LOG qualifier
**dq1,q3,dq2**
    Where dq1, q3 & dq2 are specified the data will be binned from q1->q3 in steps of dq1, and from q3->q2 in steps of dq2
**qxy1 qxy2**
    Minimum and maximum Qx (and Qy) values (/Angstrom) over which a 2D reduction is to be performed; this will ultimately be constrained by the detector position and available wavelengths
    For simplicity the same limits are applied to both axes
    NB qxy1 should always be set to 0!
**dqxy**
    [optional] Qxy step size
    The magnitude of this value will depend on whether a /LIN or /LOG qualifier is specified
    It is recommended that only linear bins are used for 2D data reductions
**dqxy1,qxy3,dqxy2**
    Where dqxy1, qxy3 & dqxy2 are specified the data will be binned from qxy1->qxy3 in steps of dqxy1, and from qxy3->qxy2 in steps of dqxy2
**r1 r2**
    Radial limits (mm) between which the radial integration of the data should be performed
    Generally r1 will be slightly larger than the shadow of the beamstop on the detector, and r2 will be the distance from the centre of the beamstop to the furthest corner of the detector
**l1 l2**
    Minimum and maximum wavelength values (Angstroms) to be used during data reduction
    As a guide, try l1~1.5 & l2~14 (for SANS2D) or l1~2.2 & l2~10.0 (for LOQ)
**dl**
    [optional] Wavelength step size
    The magnitude of this value will depend on whether a /LIN or /LOG qualifier is specified
**dl1,l3,dl2**
    Where dl1, l3 & dl2 are specified the data will be binned from l1->l3 in steps of dl1, and from l3->l2 in steps of dl2

MASK
----

MASK commands clear or implement detector spatial or time masks. Masked regions of the detector or time-of-flight spectra are not included in any data reduction.

Note that the shadow of the beam stop, and the corners of the detector, are masked automatically.

Also note that there is no equivalent in Mantid of the COLETTE box mask MASK x1 x2 y1 y2 where X & Y were specified in mm.

Syntax
^^^^^^

.. code-block:: none

    MASK/CLEAR[/TIME]

    MASK[/REAR/FRONT/HAB] Hn[>Hm]  or  MASK Vn[>Vm]  - to mask single wires or 'strips'
    MASK[/REAR/FRONT/HAB] Hn>Hm+Vn>Vm                - to mask a rectangular 'box'

    MASK Ssp1[>Ssp2]

    MASK/TIME t1 t2 or  MASK/T t1 t2

    MASK/LINE width angle [x y]

Qualifiers
^^^^^^^^^^

**/CLEAR**
    Clears any detector masks in operation
    Without any /TIME qualifier only spatial masks are cleared; with a /TIME qualifier only time masks are cleared
**/TIME** or **/T**
    [optional] Specifies that the command applies to time masks
**/LINE**
    Masking arm only has effect for SANS2D
**/REAR**
    Specifies that the mask applies to the Rear detector (if SANS2D) and the Main detector (if LOQ). This is default if not specified 
**/FRONT**
    Specifies that the mask applies to the Front detector (if SANS2D) and the High-Angle Bank (if LOQ)
**/HAB**
    This command is equivalent to /FRONT

Parameters
^^^^^^^^^^

**Hm Hn**
    Specify VERTICAL wires (columns) on the detector
    For SANS2D, 0<H<191; for LOQ 0<H<127
**Ssp1 Ssp2**
    Specifies a specific spectrum number; eg, S16641
**t1 t2**
    Neutron times of flight (microseconds) specifying the time region to be masked
**Vm Vn**
    Specify HORIZONTAL wires (rows) on the detector
    For SANS2D, 0<V<191; for LOQ 0<V<127
**width angle [x y]**
    width in units of mm and angle in degrees; x and y are in meters from the beam center and are optionals, their default values are 0.

MASKFILE
--------

The MASKFILE command enables users to pull in Mask Files created using the Instrument View. For more details on the format of the files, please see the LoadMask algorithm.

Multiple mask files can be specified with a comma-separated list.

Syntax
^^^^^^

.. code-block:: none

    MASKFILE=mask1.xml,mask2.xml,...

Qualifiers
^^^^^^^^^^
None

Parameters
^^^^^^^^^^

**mask1.xml,mask2.xml,...**
    Comma-separated list of mask files to include.

MON
---

MON commands control aspects of the data normalisation, such as detector efficiencies, flood source calibrations, and the identity and Z locations of beam monitors.

Syntax
^^^^^^

.. code-block:: none

    MON/DIRECT[/FRONT]=drive:\folder\det_effic_file.ext  or  MON/DIRECT[/REAR]=drive:\folder\det_effic_file.ext
    MON/FLAT[/FRONT]=drive:\folder\flat_cell_file.ext  or  MON/FLAT[/REAR]=drive:\folder\flat_cell_file.ext
    MON/HAB=drive:\folder\hab_effic_file.ext

    MON/LENGTH=z sp [/INTERPOLATE]  or  MON/LENGTH=z sp [/INTERPOLATE]
    MON[/TRANS]/SPECTRUM=sp [/INTERPOLATE]  or  MON[/TRANS]/SPECTRUM=sp [/INTERPOLATE]

Qualifiers
^^^^^^^^^^

**/DIRECT**
    Specifies the main (rear) detector efficiency ratio vs wavelength file
**/DIRECT/FRONT**
    Specifies the high-angle (front) detector efficiency ratio vs wavelength file. These files must be in RKH format.
    Also see **/HAB**
**/FLAT**
    Specifies that the file to be loaded is a 'flat cell' (flood source) calibration file containing the relative efficiency of individual detector pixels. Note that the numbers in this file include solid angle corrections for the sample-detector distance at which the flood field was measured. On SANS2D this flood field data is then rescaled for whatever sample-detector distance the experimental data was collected at. This file must be in RKH format and the 1st column spectrum number.
**/HAB**
    Specifies that the file to be loaded is a detector efficiency ratio file for the LOQ High-Angle Bank
    This command is equivalent to MON/DIRECT/FRONT but is retained for backwards compatibility with COLETTE
**/INTERPOLATE**
    [optional] Specifies that when the monitor spectrum is rebinned an interpolating rebin algorithm should be used; useful as a means of 'smoothing' noisy monitor spectra where the normal rebin command generates 'stepped' histograms
**/LENGTH**
    Not yet implemented
    Specifies the Z position (distance from the moderator) of a monitor (needed to calculate the time-of-flight accurately); defaults to the value in the Mantid Instrument Definition File.
    Note that the monitor specification given here is over-ridden if a MON/SPECTRUM command also exists in the file
**/SPECTRUM**
    Specifies the spectrum number of the monitor to be used for the normalisation of data
    Assumed to be an incident beam monitor unless /TRANS is specified
**/TRANS**
    [optional] Can only be used with /SPECTRUM and is used to specify the incident monitor for the transmission calculation. It is passed to the algorithm CalculateTransmission as IncidentBeamMonitor.
**/FRONT**
    Specifies that the following file applies to the Front detector (if SANS2D) or the High-Angle Bank (if LOQ)
**/REAR**
    Specifies that the preceding qualifier applies to the Rear detector (if SANS2D) or the Main detector (if LOQ)

Parameters
^^^^^^^^^^
None

Values
^^^^^^

**det_effic_file.ext**
    A detector efficiency ratio file; eg, DIRECT.yyn where yy is the year and n is the cycle number
**flat_cell_file.ext**
    A 'flat cell' (flood source) calibration file; eg, FLAT_CELL.yyn where yy is the year and n is the cycle number
**hab_effic_file.ext**
    A detector efficiency ratio file for the LOQ High-Angle Bank; eg, DIRECTHAB.yyn where yy is the year and n is the cycle number
**z**
    Moderator-monitor distance (m) of the specified monitor
**sp**
    Spectrum number of the specified monitor

SAMPLE
------

SAMPLE allows for a correction to the default sample position Z coordinate; ie, along the beam axis. It is used to enable or disable the correction for wide angle.

Syntax
^^^^^^

.. code-block:: none

    SAMPLE/OFFSET z1
    SAMPLE/PATH/ON
    SAMPLE/PATH/OFF

Qualifiers
^^^^^^^^^^

**/OFFSET**
**/PATH**

Parameters
^^^^^^^^^^

**z1**
    Amount (mm) by which the sample position is moved in the z-direction relative to its position specified in the IDF. For example for LOQ the IDF may place the sample position at z=11.0m. Setting z1=100mm then means the sample is translation to the position z1=11.1m, which means in this example that L1 goes from 11.0m to 11.1m.
**ON** | **OFF**
    Enable/Disable the wide-angle transmission correction. See SANSWideAngleCorrection

SET
---

SET defines the values of parameters critical to the data reduction process.

Syntax
^^^^^^

.. code-block:: none

    SET CENTRE[/MAIN] x y
    SET CENTRE/HAB x y
    SET SCALES s a b c d

Note x and y are in units of mm. For LOQ instrument, the origin is at te bottom left corner of the main detector.

Qualifiers
^^^^^^^^^^

**/MAIN**
    Define the Beam Centre position for the low-angle detector bank. This is an optional Qualifier and is set by default. (Valid only for CENTRE)
**/HAB**
    Define the Beam Centre position for the high-angle detector bank. (Valid only for CENTRE)

Command Verbs
^^^^^^^^^^^^^

**CENTRE**
    Defines the coordinates of the centre of the pattern on the detector and the detector pixel sizes 
**SCALES**
    Defines the absolute intensity scaling factors to be applied to the reduced data

Parameters
^^^^^^^^^^

**x y**
    X & Y coordinates (mm) of the centre of the scattering pattern on the detector
    On LOQ the centre will always be around {320,320}, but on SANS2D the values vary depending on the sample-detector distance and the X offset of the detector (though usually y~-200). Because of the coordinate system employed on SANS2D the centre coordinates are always negative


**s a b c d**
    Dimensionless scaling factors used to rescale the reduced data into absolute values
    s is the overall scale factor determined by comparing I(0) for the standard sample with its expected value
    a, b, c & d only apply to the LOQ High-Angle Bank and must be set to 1.0 in SANS2D user files (mask files)!

TRANS
-----

TRANS is used to control how the proportion of neutrons transmitted through the sample is calculated. This can be calculated using CalculateTransmission or from a specified workspace.

Syntax
^^^^^^

.. code-block:: none

    TRANS/TRANSPEC=n
    TRANS/SHIFT=d m
    TRANS/SAMPLEWS=ws1
    TRANS/CANWS=ws2
    TRANS/TRANSPEC=m/SHIFT=d
    TRANS/RADIUS=r
    TRANS/ROI=roi_mask.xml
    TRANS/MASK=mask.xml

Qualifiers
^^^^^^^^^^

**/TRANSPEC**
    The spectrum number to use to measure the number of neutrons transmitted through the sample
**/SAMPLEWS**
    Assume the transmission fraction through the sample is given by this named workspace
**/CANWS**
    If a can workspace and a TRANS/SAMPLEWS are both given this must be specified
**/TRANSPEC/SHIFT**
    Is used to set the z coordinate of monitor 4 or 5 to a given offset from the rear-detector and select it as the spectrum number to measure the number of neutrons through the sample. The offset is given in mm and is negative when monitor 4 is in front of the detector
**/SHIFT**
    Is used to set the z coordinate of monitor 4 and/or 5 to a given offset from the rear-detector without selecting it as the spectrum number to measure the number of neutrons. This is useful when the offsets of both monitors are given, as using /TRANSPEC/SHIFT will use the last monitor in the file as the selected monitor. The offset is given in mm and is negative when monitor 4 is in front of the detector
**/RADIUS**
    When using the detector as a beam stop, one can specify a circular region with the specified radius on this detector to measure the number of neutrons transmitted through the sample. Note that if you specified a TRANSPEC, it will be used instead.
**/ROI**
    Similar to the RADIUS we can specify a region of interest via a mask file which is used to measure the number of neutrons transmitted through the sample. Both ROI and RADIUS can be used together. Note that if you specified a TRANSPEC, it will be used instead.
**/MASK**
    This command excludes detector regions which were possibly specified by the the ROI or RADIUS command. Note that if you specified a TRANSPEC, it will be ignored.


Parameters
^^^^^^^^^^

**n**
    An integer specifying the detector ID of the monitor the beam interacts with after the sample. This will be passed to CalculateTransmission as the TransmissionMonitor property.
**ws1**
    The name of a workspace with a single workspace that contains transmission fractions for the sample
**ws2**
    This single spectrum workspace contains transmission fractions for the can run
**z**
    The offset of monitor 4 from the rear-detector in mm, should be a negative number so that the monitor is in front of the detector
**r**
    The radius of a circle on the detector. The unit is mm.
**"d"**
    The distance from the rear detector to the specified monitor. The unit is mm.
**"m"**
    The monitor to apply a shift to.
**roi_mask.xml**
    A mask file which is used to define a region of interst.
**mask.xml**
    A mask file which is used to exclude detectors from the region of interest.

TUBECALIBFILE
-------------

The TUBECALIBFILE command enables users to pull in a tube calibration file during the data reduction process.

A current limitation of this command is that it only accepts a single tube calibration file.

Syntax
^^^^^^

.. code-block:: none

    TUBECALIBFILE=calib_file.nxs

Qualifiers
^^^^^^^^^^
None

Parameters
^^^^^^^^^^

**calib_file.nxs**
    The tube calibration file to use.

QRESOLUTION
-----------

The QRESOLUTION command enables users to calculate the QResolution when the Q1D algorithm is run. Most settings are directly used by the TOFSANSByResolution algorithm

Syntax
^^^^^^

.. code-block:: none

    QRESOL/ON 
    QRESOL/OFF
    QRESOL/DELTAR=dr
    QRESOL/LCOLLIM="lcollim"
    QRESOL/MODERATOR=moderator_rkh_file.txt
    QRESOL/A1="a1"
    QRESOL/A2="a2"
    QRESOL/H1="h1"
    QRESOL/H2="h2"
    QRESOL/W1="w1"
    QRESOL/W2="w2"

Qualifiers
^^^^^^^^^^

**/ON**
    Turns the QResolution calcuation on for the reduction. Note that if not specified OFF is assumed.
**/OFF**
    Turns the QResolution calcuation explicitly off for the reduction.
**/DELTAR**
    Specifies the virtual ring width on the detector 
**/LCOLLIM**
    Specifies the collimation length
**/MODERATOR**
    Specifies the moderator file which contains the moderator time spread (microseconds) as afunction of wavelength.
**/A1**
    Specifies the source aperture diameter.
**/A2**
    Specifies the sample aperture diameter.
**/H1**
    Specifies the height of a rectangular collimation aperture of the source. Note that if H1, H2, W1 and W2 are all specified then this takes precedence over A1 and A2.
**/H2**
    Specifies the height of a rectangular collimation aperture of the sample. Note that if H1, H2, W1 and W2 are all specified then this takes precedence over A1 and A2.
**/W1**
    Specifies the width of a rectangular collimation aperture of the source. Note that if H1, H2, W1 and W2 are all specified then this takes precedence over A1 and A2.
**/W2**
    Specifies the width of a rectangular collimation aperture of the sample. Note that if H1, H2, W1 and W2 are all specified then this takes precedence over A1 and A2.

Parameters
^^^^^^^^^^

**dr**
    The virtual ring width on the detector in mm.
**"lcollim"**
    The collimation length in m.
**moderator_rkh_file.txt**
    The name of a saved RKH file which contains the moderator time spread (microseconds) as afunction of wavelength (Angstroms).
**"a1"**
    The source aperture diameter in mm.
**"a2"**
    The sample aperture diameter in mm.
**"h1"**
    The source aperture height in mm when using a rectangular aperture.
**"h2"**
    The sample aperture height in mm when using a rectangular aperture.
**"w1"**
    The source aperture width in mm when using a rectangular aperture.
**"w2"**
    The sample aperture width in mm when using a rectangular aperture.
