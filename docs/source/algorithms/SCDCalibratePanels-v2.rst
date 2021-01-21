.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This calibration algorithm is developed for CORELLI type instrument (pack of tubes).
The calibration targets includes `L_1` (source to sample distance in meters), panel
position (in meters) and orientation (as angle-axis pairs, in degrees), as well as
initial TOF offset (not fully implemented yet).
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
of banks (panels), which can unfortunately leads to out-of-order console logs.
By default, the initial TOF (time of flight) is optimized if the corresponding option
is set to true, followed by the calibration of `L1` as well as all Panels if requested.
If the users are not familiar with the instrument, the options in the advanced control
group should be left untouched as tweaking these paramters can drastically affect the
constrained optimization used under-the-hood.

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

    # Generate simulated workspace for CORELLI
    CreateSimulationWorkspace(
        Instrument='CORELLI',
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
        'X'=0, 'Y'=0, 'Z'=dL1,
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
        'X'=0, 'Y'=0, 'Z'=-dL1,
        RelativePosition=true,
    )

    # run the calibration on pws
    # similar to actual calibration, where
    #   1. the peaks in side pws knows the correct L1, but info is embeded in Qsamples
    #   2. the recored L1 in instrument Info is the default engineering value
    SCDCalibratePanels(
        PeakWorkspace="pws",
        a=silicon.a,
        b=silicon.b,
        c=silicon.c,
        alpha=silicon.alpha,
        beta=silicon.beta,
        gamma=silicon.gamma,
        CalibrateT0=False,
        CalibrateL1=True,
        CalibrateBanks=False,
        OutputWorkspace="testCaliTable",
        XmlFilename="test.xml",
    )

This calibration should be able to correct the `L1` recorded in the instrument info using
the information embeded in all peaks.



Future Development
------------------

This algorithm is a work-in-progress as the development team as well as the instrument
scientists are working on the following targets:

1) Fix the current T0 (initial TOF offset) calibration issue for synthetic data where a
   constant zero offset was found regardless of the actual prescirbed T0.

2) The original (version 1) SCDCalibratePanels can also support TOPAZ, which consists of
   multi-panels banks.
   It is imporant for the new version of SCDCalibratePanels to provide similar support.

3) In the current implementation, the calibration results are recorded as the absolute
   position and orientation of each component, which does not provide an intuitive
   representation of the calibration outcome.
   Per instrument scientists' request, a debug-type output where additional information
   will be provided in a CSV file, including but not limited to

   a. Relative translation and rotation with respect to the engineering position of each
      component.

   b. The optimization benchmark and ChiSquare for each component.

   c. The original ISAW app also provide some plots that assist the visualization of calibration
      results, which could be useful as part of the debug output.


.. categories::

.. sourcelink::
