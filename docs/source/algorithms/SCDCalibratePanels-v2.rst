.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This calibration algorithm is developed for TOPAZ type instrument (flat panels).
The calibration targets includes:

1) `L_1`, also known as the source to sample distance in meters

2) Panel position (in meters) and orientation (as angle-axis pairs, in degrees)

3) `T0`, also known as initial TOF offset (in micro seconds).

4) sample position, also known as the initial sample position offset, which are
   three offsets (in meters) of the sample stage along x, y, z in lab reference frame.

The underlining mechanism of this calibration is to match the measured Q vectors
(`Q_{sample}`) with the those generated from tweaked instrument position and orientation,
i.e.

.. math::

   \left\vert 2\pi \rm U \rm B \left(
                               \begin{array}{c}
                                 NINT(h_i) \\
                                 NINT(k_i) \\
                                 NINT(l_i) \\
                               \end{array}
                             \right) - \rm Q_{sample,i}(p) \right\vert ^2

where NINT is the nearest integer function.
To improve the speed of calibration of the whole instrument, openMP was used for calibration
of banks (panels).
Users are recommended to use the mantid.usersettings to restrict number of threads if the
calibration is run on a system with limited computing resources.

Calibrating Instrument
----------------------

The calibration of an instrument can vary greatly from one instrument to another.
However, here are some key points to keep in mind while using this calibration algorithm to calibrate an
instrument:

1) Always double-check the attached UB matrix or make sure correct lattice constant is provided to the calibration
   algorithm with `RecalculateUB` enabled.
   This calibration algorithm is not smart enough to recognize incorrect UB matrix or lattice constant on-the-fly.

2) Try to calibrate `L1`, `T0` and `sample position` within the same calibration process by checking all of them
   as the optimizer will have a better chance to find the global minimum.
   Otherwise, a manual or semi-manual iterative call of this calibration algorithm is needed to locate the actual
   minimum, which can be time consuming and error prone.

3) Please provide reasonable search radius for the calibration target as a too-narrow search radius will force the
   optimizer to settle at the boundary while a too-wide search radius might yield some physically unrealistic
   results (e.g. it is highly unlikely that a detector is off by a few meters from its engineering position).

4) This calibration algorithm does **NOT** modify the instrument attached to the input workspace.
   But the UB matrix attached to the input workspace will be modified if the `RecalculateUB` is elected as part of
   the calibration process.

5) This calibration algorithm can generate ISAW detector calibration file (.detcal file), mantid instrument parameter
   file (.xml) as well as an ASCII table format of CORELLI calibration table (.csv).
   The XML file contains all calibration results whereas the detcal file will miss the information on sample position.
   The CORELLI calibration does not support neither T0 or sample position, therefore the users might need to use the other
   two formats to supplement it if this ASCII table is the primary calibration media.

6) The advanced option section provides several profiling options, which are mostly intended for experienced beamline scientists
   and Mantid developers.
   Regular users are recommended to stay away from this section as running parameter space profiling is very time consuming, and
   the results are often irrelevant to the data reduction.

Usage
-----

.. code-block:: python

    # necessary import
    import numpy as np
    from collections import namedtuple
    from mantid.simpleapi import *
    from mantid.geometry import CrystalStructure

    # generate synthetic testing data
    def convert(dictionary):
        return namedtuple('GenericDict', dictionary.keys())(**dictionary)

    # lattice constant for Si
    # data from Mantid web documentation
    lc_silicon = {
        "a": 5.431,  # A
        "b": 5.431,  # A
        "c": 5.431,  # A
        "alpha": 90,  # deg
        "beta": 90,  # deg
        "gamma": 90,  # deg
    }
    silicon = convert(lc_silicon)
    cs_silicon = CrystalStructure(
        f"{silicon.a} {silicon.b} {silicon.c}",
        "F d -3 m",
        "Si 0 0 0 1.0 0.05",
    )

    # Generate simulated workspace for TOPAZ
    CreateSimulationWorkspace(
        Instrument='TOPAZ',
        BinParams='1,100,10000',
        UnitX='TOF',
        OutputWorkspace='cws',
    )
    cws = mtd['cws']

    # Set the UB matrix for the sample
    # u, v is the critical part, we can start with the
    # ideal position
    SetUB(
        Workspace="cws",
        u='1,0,0',  # vector along k_i, when goniometer is at 0
        v='0,1,0',  # in-plane vector normal to k_i, when goniometer is at 0
        **lc_silicon,
    )

    # set the crystal structure for virtual workspace
    cws.sample().setCrystalStructure(cs_silicon)

    # tweak L1
    dL1 = 1.414e-2  # 1.414cm
    MoveInstrumentComponent(
        Workspace='cws',
        ComponentName='moderator',
        X=0, Y=0, Z=dL1,
        RelativePosition=true,
    )

    # Generate predicted peak workspace
    dspacings = convert({'min': 1.0, 'max': 10.0})
    wavelengths = convert({'min': 0.8, 'max': 2.9})

    # Collect peaks over a range of omegas
    CreatePeaksWorkspace(OutputWorkspace='pws')
    omegas = range(0, 180, 3)

    for omega in tqdm(omegas):
        SetGoniometer(
            Workspace="cws",
            Axis0=f"{omega},0,1,0,1",
        )

        PredictPeaks(
            InputWorkspace='cws',
            WavelengthMin=wavelengths.min,
            wavelengthMax=wavelengths.max,
            MinDSpacing=dspacings.min,
            MaxDSpacing=dspacings.max,
            ReflectionCondition='All-face centred',
            OutputWorkspace='_pws',
        )

        CombinePeaksWorkspaces(
            LHSWorkspace="_pws",
            RHSWorkspace="pws",
            OutputWorkspace="pws",
        )

    pws = mtd['pws']

    # move the source back to make PWS forget the answer
    MoveInstrumentComponent(
        Workspace='pws',
        ComponentName='moderator',
        X=0, Y=0, Z=-dL1,
        RelativePosition=true,
    )

    # run the calibration on pws
    # similar to actual calibration, where
    #   1. the peaks in side pws knows the correct L1, but info is embedded in Qsamples
    #   2. the recorded L1 in instrument Info is the default engineering value
    SCDCalibratePanels(
        PeakWorkspace="pws",
        a=silicon.a,
        b=silicon.b,
        c=silicon.c,
        alpha=silicon.alpha,
        beta=silicon.beta,
        gamma=silicon.gamma,
        CalibrateL1=True,
        CalibrateBanks=False,
        CalibrateT0=True,
        TuneSamplePosition=True,
        OutputWorkspace="testCaliTable",
        XmlFilename="test.xml",
    )

This calibration should be able to correct the `L1` recorded in the instrument info using
the information embedded in all peaks.



Future Development
------------------

This algorithm is a work-in-progress as the development team as well as the instrument
scientists are working on the following targets:

1) The current data structure (detector representation) is not suitable for calibrating
   instrument with tube-type detectors (such as CORELLI).
   Additional work on an improved internal detector and scattering vector representation
   are needed in order to make this toolkit useful for CORELLI like instrument.

2) In the current implementation, the calibration results are recorded as the absolute
   position and orientation of each component, which does not provide an intuitive
   representation of the calibration outcome.
   Per instrument scientists' request, a debug-type output where additional information
   should be provided in a CSV file, including but not limited to

   a. Relative translation and rotation with respect to the engineering position of each
      component.

   b. The optimization benchmark and ChiSquare for each component.

   c. The original ISAW app also provide some plots that assist the visualization of calibration
      results, which could be useful as part of the debug output.


.. categories::

.. sourcelink::
