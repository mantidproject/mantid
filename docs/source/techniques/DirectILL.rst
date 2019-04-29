.. _DirectILL:

====================================================
Data reduction for ILL's direct geometry instruments
====================================================

.. contents:: Table of contents
    :local:

There are seven workflow algorithms supporting data reduction at ILL's time-of-flight instruments. These algorithms are:

:ref:`algm-DirectILLCollectData`
    Loads data from disk and applies some simple reduction steps. The starting point of all reductions, as most of the other DirectILL algorithms expect their input data to originate from here.

:ref:`algm-DirectILLReduction`
    Does the actual reduction and produces the final :math:`S(q,\omega)`.

:ref:`algm-DirectILLIntegrateVanadium`
    Integrates a calibration workspace.

:ref:`algm-DirectILLDiagnostics`
    Provides a masking workspace to remove detectors (faulty or otherwise) from the reduction.

:ref:`algm-DirectILLSelfShielding`
    Calculates absorption corrections for usage with :ref:`algm-DirectILLApplySelfShielding`.

:ref:`algm-DirectILLApplySelfShielding`
    Applies absorption corrections and subtracts an empty container measurement.

:ref:`algm-DirectILLTubeBackground`
    Calculates a per-tube backgrounds for position sensitive tubes such as those found in IN5.

The algorithms can be used as flexible building blocks in Python scripts. Not all of them need to be necessarily used in a reduction: the simplest script could call :ref:`algm-DirectILLCollectData` and :ref:`algm-DirectILLReduction` only.

Together with the other algorithms and services provided by the Mantid framework, the reduction algorithms can handle a great number of reduction scenarios. If this proves insufficient, however, the algorithms can be accessed using Python. Before making modifications it is recommended to copy the source files and rename the algorithms as not to break the original behavior.

This document tries to give an overview on how the algorithms work together via Python examples. Please refer to the algorithm documentation for details of each individual algorithm.

Instrument specific defaults and recommendations
################################################

Some algorithm properties have the word 'AUTO' in their default value. This means that the default will be chosen according to the instrument by reading the actual default from the instrument parameters. The documentation of the algorithms which have these types of properties includes tables showing the defaults for supported ILL instruments.

Reduction basics
================

.. include:: ../usagedata-note.txt

A very basic reduction would include a vanadium reference and a sample and follow the steps:

#. Load vanadium data.

#. Integrate vanadium.

#. Run diagnostics.

   * Generally, this step should not be skipped even if no actual diagnostics were performed, as :ref:`algm-DirectILLDiagnostics` may create a default mask for the instrument. This is the case of IN5, for example, where the pixels at the detector tube ends are masked.

#. Load sample data.

#. Reduce the data applying vanadium calibration coefficients and diagnostics mask.

On instruments like IN4 and IN6, these steps would translate to something like the following simple Python script:

.. testsetup:: BasicIN4Reduction

   config['default.facility'] = 'ILL'
   config['default.instrument'] = 'IN4'

.. testcode:: BasicIN4Reduction

    # Uncomment to add a temporary data search directory.
    #mantid.config.appendDataSearchDir('/data/')
    
    # Vanadium
    DirectILLCollectData(
        Run='ILL/IN4/085801-085802.nxs',
        OutputWorkspace='vanadium',
        OutputEPPWorkspace='vanadium-epps',  # Elastic peak positions.
        OutputRawWorkspace='vanadium-raw'    # 'Raw' data for diagnostics.
    )
    DirectILLIntegrateVanadium(
        InputWorkspace='vanadium',
        OutputWorkspace='integrated',
        EPPWorkspace='vanadium-epps'
    )
    DirectILLDiagnostics(
        InputWorkspace='vanadium-raw',
        OutputWorkspace='diagnostics',
        EPPWorkspace='vanadium-epps',
    )
    # Sample
    DirectILLCollectData(
        Run='ILL/IN4/087294+087295.nxs',
        OutputWorkspace='sample'
    )
    DirectILLReduction(
        InputWorkspace='sample',
        OutputWorkspace='SofQW',
        IntegratedVanadiumWorkspace='integrated',
        DiagnosticsWorkspace='diagnostics'
    )
    SofQW = mtd['SofQW']
    qAxis = SofQW.readX(0)  # Vertical axis
    eAxis = SofQW.getAxis(1)  # Horizontal axis
    print('S(Q,W): Q range: {:.3}...{:.3}A; W range {:.3}...{:.3}meV'.format(
        qAxis[0], qAxis[-1], eAxis.getMin(), eAxis.getMax()))

Output:

.. testoutput:: BasicIN4Reduction

    S(Q,W): Q range: 0.0...9.18A; W range -96.3...7.62meV

The basic reduction for IN5 and PANTHER differs slightly with regards to the diagnostics step. In this case, the "raw" workspace is not needed, and it is not necessary to pass the EPP workspace to :ref:`algm-DirectILLDiagnostics`:

.. code-block:: python

    # Uncomment to add a temporary data search directory.
    #mantid.config.appendDataSearchDir('/data/')
    
    # Vanadium
    DirectILLCollectData(
        Run='085801-085802',
        OutputWorkspace='vanadium',
        OutputEPPWorkspace='vanadium-epps',  # Elastic peak positions.
    )
    DirectILLIntegrateVanadium(
        InputWorkspace='vanadium',
        OutputWorkspace='integrated',
        EPPWorkspace='vanadium-epps'
    )
    DirectILLDiagnostics(
        InputWorkspace='vanadium',
        OutputWorkspace='diagnostics',
    )
    # Sample
    DirectILLCollectData(
        Run='087294+087295',
        OutputWorkspace='sample'
    )
    DirectILLReduction(
        InputWorkspace='sample',
        OutputWorkspace='SofQW',
        IntegratedVanadiumWorkspace='integrated',
        DiagnosticsWorkspace='diagnostics'
    )

Connecting inputs and outputs
=============================

Every ``DirectILL`` algorithm has an *OutputWorkspace* property which provides the main output workspace. Additionally, the algorithms may provide optional output workspaces to be used with other algorithms or for reporting/debugging purposes. The linking of outputs to inputs is an important feature of the ``DirectILL`` algorithms and allows for great flexibility in the reduction. An example of the usage of these optional output workspaces is the *OutputEPPWorkspace* which in the vanadium case above is needed in the integration and diagnostics steps:

.. code-block:: python

    ...
    DirectILLCollectData(
        ...
        OutputEPPWorkspace='epps'  # This workspace...
    )
    DirectILLIntegrateVanadium(
        ...
        EPPWorkspace='epps'        # ...is needed here...
    )
    DirectILLDiagnostics(
        ...
        EPPWorkspace='epps'        # ...and here.
    )
    ...

As shown above, these optional outputs are sometimes named similarly the corresponding inputs giving a hint were they are supposed to be used.

Time-independent background for position sensitive tubes
========================================================

The flat background subtraction in :ref:`algm-DirectILLCollectData` does not work properly for instruments such as IN5. Another algorithm, :ref:`algm-DirectILLTubeBackground` should be used instead. For this algorithm, one should run :ref:`algm-DirectILLDiagnostics` to utilize the default hard mask and beam stop masking in the background determination.

.. code-block:: python

    # Add a temporary data search directory.
    mantid.config.appendDataSearchDir('/data/')

    # Vanadium
    DirectILLCollectData(
        Run='0100-0109',
        OutputWorkspace='vanadium',
        OutputEPPWorkspace='vanadium-epps',  # Elastic peak positions.
    )

    DirectILLDiagnostics(
        InputWorkspace='vanadium-raw',
        OutputWorkspace='diagnostics',
    )

    # Determine time-independent background
    DirectILLTubeBackground(
        InputWorkspace='vanadium',
        OutputWorkspace='vanadium-background'
        EPPWorkspace='vanadiums-epps',
        DiagnosticsWorkspace='diagnostics'
    )
    # Subtract the background
    Subtract(
        LHSWorkspace='vanadium'
        RHSWorkspace='vanadium-background',
        OutputWorkspace='vanadium-bkgsubtr'
    )

    DirectILLIntegrateVanadium(
        InputWorkspace='vanadium-bkgsubtr',  # Integrate background subtracted data
        OutputWorkspace='integrated',
        EPPWorkspace='vanadium-epps'
    )

    # Sample
    DirectILLCollectData(
        Run='0201+0205+0209-0210',
        OutputWorkspace='sample',
        OutputEPPWorkspace='sample-epps'
    )

    # Determine time-independent background
    DirectILLTubeBackground(
        InputWorkspace='sample',
        OutputWorkspace='sample-background',
        EPPWorkspace='sample-epps',
        DiagnosticsWorkspace='diagnostics'
    )
    Subtract(
        LHSWorkspace='sample',
        RHSWorkspace='sample-background',
        OutputWorkspace='sample-bkgsubtr'
    )

    DirectILLReduction(
        InputWorkspace='sample-bkgsubtr',
        OutputWorkspace='SofQW',
        IntegratedVanadiumWorkspace='integrated'
        DiagnosticsWorkspace='diagnostics'
    )


Self-shielding corrections
==========================

A more complete reduction example would include corrections for self-shielding:

#. Load vanadium data.

#. Integrate vanadium.

#. Run diagnostics.

#. Load sample data.

#. Calculate absorption corrections for the sample.

   * This may be a time consuming step. Fortunately, the resulting correction factors can be reused as many times as needed.

   * Sample and beam parameters can be set using :ref:`algm-SetSample` and :ref:`algm-SetBeam`.

#. Apply the corrections.

#. Reduce the data applying vanadium calibration coefficients and diagnostics mask.

The above workflow would translate to this kind of Python script for IN4 and IN6:

.. testsetup:: SelfShieldingReduction

   config['default.facility'] = 'ILL'
   config['default.instrument'] = 'IN4'

.. testcode:: SelfShieldingReduction

    # Uncomment to add a temporary data search directory.
    #mantid.config.appendDataSearchDir('/data/')
    
    # Vanadium
    DirectILLCollectData(
        Run='ILL/IN4/085801-085801.nxs',
        OutputWorkspace='vanadium',
        OutputEPPWorkspace='vanadium-epps',  # Elastic peak positions.
        OutputRawWorkspace='vanadium-raw'    # 'Raw' data for diagnostics.
    )
    DirectILLIntegrateVanadium(
        InputWorkspace='vanadium',
        OutputWorkspace='integrated',
        EPPWorkspace='vanadium-epps'
    )
    DirectILLDiagnostics(
        InputWorkspace='vanadium-raw',  # For IN5/PANTHER, 'vanadium' could be used
        OutputWorkspace='diagnostics',
        EPPWorkspace='vanadium-epps',  # Can be omitted for IN5/PANTHER
    )
    # Sample
    DirectILLCollectData(
        Run='ILL/IN4/087294+087295.nxs',
        OutputWorkspace='sample',
    )
    geometry = {
        'Shape': 'FlatPlate',
        'Width': 4.0,
        'Height': 5.0,
        'Thick': 0.1,
        'Center': [0.0, 0.0, 0.0],
        'Angle': 45.0
    }
    material = {
        'ChemicalFormula': 'Cd S',
        'SampleNumberDensity': 0.01
    }
    SetSample(
        InputWorkspace='sample',
        Geometry=geometry,
        Material=material
    )
    DirectILLSelfShielding(
        InputWorkspace='sample',
        OutputWorkspace='corrections',
        NumberOfSimulatedWavelengths=10
    )
    DirectILLApplySelfShielding(
        InputWorkspace='sample',
        OutputWorkspace='sample-corrected',
        SelfShieldingCorrectionWorkspace='corrections',
    )
    DirectILLReduction(
        InputWorkspace='sample-corrected',
        OutputWorkspace='SofQW',
        IntegratedVanadiumWorkspace='integrated',
        DiagnosticsWorkspace='diagnostics'
    )
    SofQW = mtd['SofQW']
    qAxis = SofQW.readX(0)  # Vertical axis
    eAxis = SofQW.getAxis(1)  # Horizontal axis
    print('S(Q,W): Q range: {:.3}...{:.3}A; W range {:.3}...{:.3}meV'.format(
        qAxis[0], qAxis[-1], eAxis.getMin(), eAxis.getMax()))

Output:

.. testoutput:: SelfShieldingReduction

    S(Q,W): Q range: 0.0...9.18A; W range -96.3...7.62meV

Workspace compatibility
=======================

Mantid can be picky with binning when doing arithmetics between workspaces. This is an issue for the time-of-flight instruments at ILL as the time axis needs to be corrected to correspond to a physical flight distance. Even thought data is recorded with the same nominal wavelength, the actual value written in the NeXus files may differ between runs. Incident energy calibration further complicates matters. As the correction to the time-of-flight axis depends on the wavelength, two datasets loaded into Mantid with :ref:`algm-DirectILLCollectData` may have slightly different time-of-flight axis. This prevents arithmetics between the workspaces. The situation is most often encountered between a sample and the corresponding empty container.

To alleviate the situation, the output workspaces of :ref:`algm-DirectILLCollectData` can be forced to use the same wavelength. The following Python script shows how to propagate the calibrated incident energy from the first loaded workspace into the rest:

.. testsetup:: SampleContainerCompatibility

   config['default.facility'] = 'ILL'
   config['default.instrument'] = 'IN4'

.. testcode:: SampleContainerCompatibility

    # Sample
    DirectILLCollectData(
        Run='ILL/IN4/087294-087295.nxs',
        OutputWorkspace='sample',
        OutputIncidentEnergyWorkspace='Ei'  # Get a common incident energy.
    )
    # Empty container.
    DirectILLCollectData(
        Run='ILL/IN4/087306-087309.nxs',
        OutputWorkspace='container',
        IncidentEnergyWorkspace='Ei'  # Ensure same TOF binning.
    )
    x_sample = mtd['sample'].readX(0)
    x_container = mtd['container'].readX(0)
    print("Sample's TOF axis starts at {:.4}mus, container's at {:.4}mus".format(
        x_sample[0], x_container[0]))

Output:

.. testoutput:: SampleContainerCompatibility

    Sample's TOF axis starts at 966.8mus, container's at 966.8mus

Container subtraction
=====================

The container subtraction is done perhaps a bit counterintuitively in :ref:`algm-DirectILLApplySelfShielding`. At the moment the self-shielding corrections and the empty container data do not have much to do with each other but this may change in the future if the so called Paalman-Pings corrections are used.

With empty container data, the steps to reduce the experimental data might look like this:

#. Load vanadium data.

#. Integrate vanadium.

#. Run diagnostics.

#. Load sample data.

#. Load container data.

   * Propagate the incident energy from the sample, see `Workspace compatibility`_.

#. Calculate and apply absorption corrections for the container.

#. Calculate the absorption corrections for the sample.

#. Apply the absoprtion corrections and subtract the container.

#. Reduce the data applying vanadium calibration coefficients and diagnostics mask.

A corresponding Python script follows.

.. testsetup:: ContainerSubtraction

   config['default.facility'] = 'ILL'
   config['default.instrument'] = 'IN4'

.. testcode:: ContainerSubtraction

    # Uncomment to add a temporary data search directory.
    #mantid.config.appendDataSearchDir('/data/')
    
    # Vanadium
    DirectILLCollectData(
        Run='ILL/IN4/085801-085802.nxs',
        OutputWorkspace='vanadium',
        OutputEPPWorkspace='vanadium-epps',
        OutputRawWorkspace='vanadium-raw'  # Can be omitted for IN5/PANTHER
    )
    DirectILLIntegrateVanadium(
        InputWorkspace='vanadium',
        OutputWorkspace='integrated',
        EPPWorkspace='vanadium-epps'
    )
    DirectILLDiagnostics(
        InputWorkspace='vanadium-raw',  # IN5/PANTHER can use 'vanadium'
        OutputWorkspace='diagnostics',
        EPPWorkspace='vanadium-epps',  # Can be omitted for IN5/PANTHER
    )
    # Sample
    DirectILLCollectData(
        Run='ILL/IN4/087294+087295.nxs',
        OutputWorkspace='sample',
        OutputIncidentEnergyWorkspace='Ei'  # For empty container
    )
    # Container
    DirectILLCollectData(
        Run='ILL/IN4/087306-087309.nxs',
        OutputWorkspace='container',
        IncidentEnergyWorkspace='Ei'  # Ensure common TOF axis
    )
    # Sample self-shielding and container subtraction.
    geometry = {
        'Shape': 'HollowCylinder',
        'Height': 4.0,
        'InnerRadius': 1.9,
        'OuterRadius': 2.0,
        'Center': [0.0, 0.0, 0.0]
    }
    material = {
        'ChemicalFormula': 'Cd S',
        'SampleNumberDensity': 0.01
    }
    SetSample('sample', geometry, material)
    DirectILLSelfShielding(
        InputWorkspace='sample',
        OutputWorkspace='sample-corrections',
        NumberOfSimulatedWavelengths=10
    )
    DirectILLApplySelfShielding(
        InputWorkspace='sample',
        OutputWorkspace='sample-corrected',
        SelfShieldingCorrectionWorkspace='sample-corrections',
        EmptyContainerWorkspace='container'  # Also subtract container
    )
    DirectILLReduction(
        InputWorkspace='sample-corrected',
        OutputWorkspace='SofQW',
        IntegratedVanadiumWorkspace='integrated',
        DiagnosticsWorkspace='diagnostics'
    )
    SofQW = mtd['SofQW']
    qAxis = SofQW.readX(0)  # Vertical axis
    eAxis = SofQW.getAxis(1)  # Horizontal axis
    print('S(Q,W): Q range: {:.3}...{:.3}A; W range {:.3}...{:.3}meV'.format(
        qAxis[0], qAxis[-1], eAxis.getMin(), eAxis.getMax()))

Output:

.. testoutput:: ContainerSubtraction

    S(Q,W): Q range: 0.0...9.18A; W range -96.3...7.62meV

Interpolation of container data to different temperatures
---------------------------------------------------------

Sometimes the empty container is not measured at all the experiment's temperature points. One can use Mantid's workspace arithmetics to perform simple linear interpolation in temperature:

.. testsetup:: ContainerInterpolation

   config['default.facility'] = 'ILL'
   config['default.instrument'] = 'IN4'

.. testcode:: ContainerInterpolation

    import numpy
    DirectILLCollectData(
        Run='ILL/IN4/087283-087290.nxs',
        OutputWorkspace='sample_50K',
        OutputIncidentEnergyWorkspace='E_i'
    )
    DirectILLCollectData(
        Run='ILL/IN4/087306-087309.nxs',
        OutputWorkspace='container_1.5K',
        IncidentEnergyWorkspace='E_i'
    )
    DirectILLCollectData(
        Run='ILL/IN4/087311-087314.nxs',
        OutputWorkspace='container_100K',
        IncidentEnergyWorkspace='E_i'
    )
    # Container measurement temperatures.
    T0 = 1.5
    T1 = 100.0
    DT = T1 - T0
    # Target sample temperature.
    Ts = 50.0
    # Linear interpolation.
    container_50K = (T1 - Ts) / DT * mtd['container_1.5K'] + (Ts - T0) / DT * mtd['container_100K']
    T_sample_logs = container_50K.run().getProperty('sample.temperature').value
    mean_T = numpy.mean(T_sample_logs)
    print('Note, that the mean temperature from the sample logs is {:.4}K, a bit off.'.format(mean_T))

Output:

.. testoutput:: ContainerInterpolation

    Note, that the mean temperature from the sample logs is 51.0K, a bit off.

As usual, care should be taken when extrapolating the container data outside the measured range.

Finding out what went wrong
===========================

The reduction algorithms do not produce much output to Mantid logs by default. Also, none of the intermediate workspaces generated during the run of the ``DirectILL`` algorithms will show up in the analysis data service. Both behaviors can be controlled by the *SubalgorithmLogging* and *Cleanup* properties. Enabling *SubalgorithmLogging* will log all messages from child algorithms to Mantid's logs. Disabling *Cleanup* will unhide the intermediate workspaces created during the algorithm run and disable their deletion.

Note, that disabling *Cleanup* might produce a large number of workspaces and cause the computer to run out of memory.

Instrument specific defaults and recommendations
================================================

Elastic peak positions
----------------------

The intensities of individual pixels on position sensitive detectors are very low. This makes the fitting procedure employed by :ref:`algm-FindEPP` to work unreliably or fail altogether. Because of this, :ref:`algm-DirectILLCollectData` will use :ref:`algm-CreateEPP` instead by default for IN5 and PANTHER. :ref:`algm-CreateEPP` produces an artificial EPP workspace based on the instrument geometry. This should be accurate enough for vanadium integration and diagnostics.

Diagnostics
-----------

The elastic peak and background diagnostics are turned off by default for IN5 and PANTHER as it makes no sense to mask individual pixels based on them.

Memory management
-----------------

When working on memory constrained systems, it is recommended to manually delete the workspaces which are not needed anymore in the reduction script. The :ref:`algm-SaveNexus` can be used to save the data to disk.

Full example
============

Lets put it all together into a complex Python script. The script below reduces the following dataset:

* Vanadium reference.

* Sample measured at 1.5 and 50K.

  * Share time-independent backgrounds from the measurement at 1.5K.

* Empty container measured at 1.5 and 100K.

  * Need to interpolate to 50K.

.. testsetup:: FullExample

   config['default.facility'] = 'ILL'
   config['default.instrument'] = 'IN4'

.. testcode:: FullExample

    # Uncomment to add a temporary data search directory.
    #mantid.config.appendDataSearchDir('/data/')
    
    # Vanadium
    DirectILLCollectData(
        Run='ILL/IN4/085801-085802.nxs',
        OutputWorkspace='vanadium',
        OutputEPPWorkspace='vanadium-epps',
        OutputRawWorkspace='vanadium-raw'  # Can be omitted for IN5/PANTHER
    )
    DirectILLIntegrateVanadium(
        InputWorkspace='vanadium',
        OutputWorkspace='integrated',
        EPPWorkspace='vanadium-epps'
    )
    DirectILLDiagnostics(
        InputWorkspace='vanadium-raw',  # IN5/PANTHER can use 'vanadium'
        OutputWorkspace='diagnostics',
        EPPWorkspace='vanadium-epps',  # Can be omitted for IN5/PANTHER
    )
    # Samples
    DirectILLCollectData(
        Run='ILL/IN4/087294+087295.nxs',
        OutputWorkspace='sample_1.5K',
        OutputIncidentEnergyWorkspace='Ei',  # For other datasets
        OutputFlatBkgWorkspace='bkgs'  # For sample at 50K
    )
    DirectILLCollectData(
        Run='ILL/IN4/087283-087290.nxs',
        OutputWorkspace='sample_50K',
        IncidentEnergyWorkspace='Ei',  # Ensure common TOF axis
        FlatBkgWorkspace='bkgs'  # Use flat backgrounds from 1.5K
    )
    # Containers
    DirectILLCollectData(
        Run='ILL/IN4/087306-087309.nxs',
        OutputWorkspace='container_1.5K',
        IncidentEnergyWorkspace='Ei'  # Ensure common TOF axis
    )
    DirectILLCollectData(
        Run='ILL/IN4/087311-087314.nxs',
        OutputWorkspace='container_100K',
        IncidentEnergyWorkspace='Ei'  # Ensure common TOF axis
    )    
    # Sample self-shielding and container subtraction.
    geometry = {
        'Shape': 'HollowCylinder',
        'Height': 4.0,
        'InnerRadius': 1.9,
        'OuterRadius': 2.0,
        'Center': [0.0, 0.0, 0.0]
    }
    material = {
        'ChemicalFormula': 'Cd S',
        'SampleNumberDensity': 0.01
    }
    SetSample('sample_1.5K', geometry, material)
    SetSample('sample_50K', geometry, material)
    # Self-shielding corrections need to be calculated only once.
    DirectILLSelfShielding(
        InputWorkspace='sample_1.5K',
        OutputWorkspace='corrections',
        NumberOfSimulatedWavelengths=10
    )
    DirectILLApplySelfShielding(
        InputWorkspace='sample_1.5K',
        OutputWorkspace='corrected_1.5K',
        SelfShieldingCorrectionWorkspace='corrections',
        EmptyContainerWorkspace='container_1.5K'
    )
    # Need to interpolate container to 50K
    T0 = 1.5
    T1 = 100.0
    DT = T1 - T0
    Ts = 50.0 # Target T
    container_50K = (T1 - Ts) / DT * mtd['container_1.5K'] + (Ts - T0) / DT * mtd['container_100K']
    DirectILLApplySelfShielding(
        InputWorkspace='sample_50K',
        OutputWorkspace='corrected_50K',
        SelfShieldingCorrectionWorkspace='corrections',
        EmptyContainerWorkspace=container_50K
    )
    DirectILLReduction(
        InputWorkspace='corrected_1.5K',
        OutputWorkspace='SofQW_1.5K',
        IntegratedVanadiumWorkspace='integrated',
        DiagnosticsWorkspace='diagnostics'
    )
    DirectILLReduction(
        InputWorkspace='corrected_50K',
        OutputWorkspace='SofQW_50K',
        IntegratedVanadiumWorkspace='integrated',
        DiagnosticsWorkspace='diagnostics'
    )
    outputs = ['SofQW_1.5K', 'SofQW_50K']
    for output in outputs:
        SofQW = mtd[output]
        qAxis = SofQW.readX(0)  # Vertical axis
        eAxis = SofQW.getAxis(1)  # Horizontal axis
        print('{}: Q range: {:.3}...{:.3}A; W range {:.3}...{:.3}meV'.format(
            output, qAxis[0], qAxis[-1], eAxis.getMin(), eAxis.getMax()))

Output:

.. testoutput:: FullExample

    SofQW_1.5K: Q range: 0.0...9.18A; W range -96.3...7.62meV
    SofQW_50K: Q range: 0.0...9.18A; W range -96.3...7.62meV

.. categories:: Techniques
