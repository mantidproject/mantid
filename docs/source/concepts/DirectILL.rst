.. _DirectILL:

====================================================
Data reduction for ILL's direct geometry instruments
====================================================

.. contents:: Table of contents
    :local:

There are six workflow algorithms supporting data reduction at ILL's time-of-flight instruments. These algorithms are:

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

The algorithms can be used as flexible building blocks in Python scripts. Not all of them need to be necessarily used in a reduction: the simplest script could call :ref:`algm-DirectILLCollectData` and :ref:`algm-DirectILLReduction` only.

Together with the other algorithms and services provided by the Mantid framework, the reduction algorithms can handle a great number of reduction scenarios. If this proves insufficient, however, the algorithms can be accessed using Python. Before making modifications it is recommended to copy the source files and rename the algorithms as not to break the original behavior.

This document tries to give an overview on how the algorithms work together via Python examples. Please refer to the algorithm documentation for details of each individual algorithm.

Reduction basics
================

The following simple script reduces a sample run using a vanadium reference.

.. code-block:: python

    # Add a temporary data search directory.
    mantid.config.appendDataSearchDir('/data/')

    # Vanadium
    DirectILLCollectData(
        Run='0100:0109',
        OutputWorkspace='vanadium',
        OutputEPPWorkspace='vanadium-epps'   # Elastic peak positions.
    )

    DirectILLIntegrateVanadium(
        InputWorkspace='vanadium',
        OutputWorkspace='integrated',
        EPPWorkspace='vanadium-epps'
    )

    # Sample
    DirectILLCollectData(
        Run='0201, 0205, 0209-0210',
        OutputWorkspace='sample'
    )

    DirectILLReduction(
        InputWorkspace='sample',
        OutputWorkspace='SofQW',
        IntegratedVanadiumWorkspace='integrated'
    )

All data is loaded into Mantid using :ref:`algm-DirectILLCollectData`. After setting the data search directory, Mantid can find the data files by the numors only. It is possible to specify ranges of numors, as for the vanadium above:

.. code-block:: python

    DirectILLCollectData(
        Run='0100:0109',
    ...

The syntax for the ranges supports characters `,`, `-`, `:`, `+`, allowing complex sets of numors to be loaded. The following merges numors 0201, 0205, 0209 and 0210:

.. code-block:: python

    DirectILLCollectData(
        Run='0201, 0205, 0209-0210',
        OutputWorkspace='sample'
    )

Every ``DirectILL`` algorithm has an *OutputWorkspace* property which provides the main output workspace. Additionally, the algorithms may provide optional output workspaces to be used with other algorithms or for reporting/debugging purposes. Such an optional output is the *OutputEPPWorkspace* which in the vanadium case above is needed in integration:

.. code-block:: python

    # Vanadium
    DirectILLCollectData(
        ...
        OutputEPPWorkspace='vanadium-epps'    # This workspace...
    )

    DirectILLIntegrateVanadium(
        ...
        EPPWorkspace='vanadium-epps'          # ...is needed here.
    )

As shown above, these optional outputs are sometimes named similarly the corresponding inputs giving a hint were they are supposed to be used.

Debugging
=========

The reduction algorithms do not produce much output to Mantid logs by default. Also, none of the intermediate workspaces will show up in the analysis data service. Both behaviors can be controlled by the *SubalgorithmLogging* and *Cleanup* properties. Enabling *SubalgorithmLogging* will log all messages from child algorithms to Mantid's logs. Disabling *Cleanup* will unhide the intermediate workspaces created during the algorithm run and disable their deletion.

Note, that disabling *Cleanup* might produce a large number of workspaces and cause the computer to run out of memory.

Empty containers
================

The container background subtraction is done perhaps a bit counterintuitively in :ref:`algm-DirectILLApplySelfShielding`. At the moment the self-shielding corrections and the empty container data do not have much to do with each other but this may change in the future if the so called Paalman-Pings corrections are used.

There is a quirk with regards to binning of the container data shown in the example below.

.. code-block:: python

    mantid.config.appendDataSearchDir('/data/')

    # Sample.
    # Has to be loaded before the container because of incident energy.
    DirectILLCollectData(
        Run='0540+0545',
        OutputWorkspace='sample',
        OutputIncidentEnergyWorkspace='Ei' # Needed for container.
    )

    # Container.
    DirectILLCollectData(
        Run='0561, 0564',
        OutputWorkspace='container,
        IncidentEnergyWorkspace='Ei' # Ensure same binning in TOF.
    )

    # Subtraction can be done without the self-shielding corrections.
    DirectILLApplySelfShielding(
        InputWorkspace='sample',
        EmptyContainerWorkspace='container'
    )

Mantid is picky about the compatibility of workspaces when it comes to arithmetics. Thus, the container and the sample workspaces must have the same TOF binning. Since the binning depends on the incident energy, the value has to be propagated from the sample to the container data:

.. code-block:: python

    # Sample.
    DirectILLCollectData(
        ...
        OutputIncidentEnergyWorkspace='Ei' # This output...
    )

    # Container.
    DirectILLCollectData(
        ...
        IncidentEnergyWorkspace='Ei'       # ...is needed here.
    )

Further, this procedure is recommended for all sample and container data measured with the same wavelength:

.. code-block:: python

    # Sample at 3K.
    DirectILLCollectData(
        ...
        OutputWorkspace='sample_3K',
        OutputIncidentEnergyWorkspace='Ei' # This output...
    )

    # Container.
    DirectILLCollectData(
        ...
        OutputWorkspace='container',
        IncidentEnergyWorkspace='Ei'       # ...is needed here...
    )

    # Sample at 50K.
    DirectILLCollectData(
        ...
        OutputWorkspace='sample_230K',
        IncidentEnergyWorkspace='Ei'       # ...as well as here.
    )

Interpolation to different temperatures
---------------------------------------

One can use Mantid's workspace arithmetics to perform simple linear interpolation:

.. code-block:: python

    # Container measurement temperatures.
    T0 = 3.0
    T1 = 250.0
    DT = T1 - T0
    # Target sample temperature.
    Ts = 190.0
    # Linear interpolation.
    container_190 = (T1 - Ts) / DT * mtd['container_3'] + (Ts - T0) / DT * mtd['container_250']

    DirectILLApplySelfShielding(
        InputWorkspace='sample',
        EmptyContainerWorkspace=container_190
    )

As usual, care should be taken when extrapolating the container data outside the measured range.

Vanadium
========

Vanadium (or equivalent reference sample) does not only offer detector calibration data but is very usable for detector diagnostics as well. Extending the example script in the `Reduction basics`_ section above:

.. code-block:: python

    # Add a temporary data search directory.
    mantid.config.appendDataSearchDir('/data/')

    # Vanadium
    DirectILLCollectData(
        Run='0100:0109',
        OutputWorkspace='vanadium',
        OutputEPPWorkspace='vanadium-epps', # Elastic peak positions.
        OutputRawWorkspace='vanadium-raw'   # Unnormalized 'raw' data, no bkg subtracted.
    )

    DirectILLIntegrateVanadium(
        InputWorkspace='vanadium',
        OutputWorkspace='integrated',
        EPPWorkspace='vanadium-epps'
    )

    DirectILLDiagnostics(
        InputWorkspace='vanadium-raw', # 'Raw' data needed here...
        OutputWorkspace='diagnostics',
        EPPWorkspace='vanadium-epps'   # ...and the elastic peak positions.
    )

    # Sample
    DirectILLCollectData(
        Run='0201, 0205, 0209-0210',
        OutputWorkspace='sample'
    )

    DirectILLReduction(
        InputWorkspace='sample',
        OutputWorkspace='SofQW',
        IntegratedVanadiumWorkspace='integrated',
        DiagnosticsWorkspace='diagnostics'        # Mask 'bad' detectors.
    )

Detector diagnostics and masking
================================

:ref:`algm-DirectILLDiagnostics` not only performs detector diagnostics, it also handles masking in general. The output workspace can be further fed to :ref:`algm-DirectILLReduction` where the mask is actually applied.

It is recommended to use vanadium or similar reference workspace for the diagnostics.

.. code-block:: python

    # Add a temporary data search directory.
    mantid.config.appendDataSearchDir('/data/')

    # Vanadium
    DirectILLCollectData(
        Run='0100:0109',
        OutputWorkspace='vanadium',
        OutputEPPWorkspace='vanadium-epps', # Elastic peak positions.
        OutputRawWorkspace='vanadium-raw'   # Unnormalized 'raw' data, no bkg subtracted.
    )

    diagResult = DirectILLDiagnostics(
        InputWorkspace='vanadium-raw', # 'Raw' data needed here...
        OutputWorkspace='diagnostics',
        EPPWorkspace='vanadium-epps',  # ...and the elastic peak positions.
        MaskComponents='rosace, bottom_bank, top_bank' # Interested only on middle_bank.
        OutputReportWorkspace='diagnostics-report'
    )

    # Print a formatted string of what was masked.
    print(diagResult.OutputReport)

    # Prepare for the reduction.
    DirectILLIntegrateVanadium(
        InputWorkspace='vanadium',
        OutputWorkspace='vanadium-integrated'
    )

    DirectILLCollectData(
        Run='0151, 0153, 0155',
        OutputWorkspace='sample',
    )

    DirectILLReduction(
        InputWorkspace='sample',
        OutputWorkspace='SofQW',
        IntegratedVanadiumWorkspace='vanadium-integrated',
        DiagnosticsWorkspace='diagnostics' # This applies the mask.
    )

Absorption corrections
======================

Due to the time consuming nature of simulating the absorption corrections, there are two algorithms dealing with absorption corrections. :ref:`algm-DirectILLSelfShielding` calculates the corrections and needs to be run only once. The result can then be applied by :ref:`algm-DirectILLApplySelfShielding` to any number of workspaces.

:ref:`algm-DirectILLApplySelfShielding` is also used for empty container subtraction, see `Empty containers`_.

.. code-block:: python

    # Add a temporary data search directory.
    mantid.config.appendDataSearchDir('/data/')

    DirectILLCollectData(
        Run='0151+0155',
        OutputWorkspace='sample',
    )

    # Set sample shape, material and beam profile.
    ws = mtd['sample']
    sampleGeometry = {
        'Shape': 'Cylinder',
        'Height': 8.0,
        'Radius': 2.0,
        'Center': [0.0, 0.0, 0.0]
    }
    sampleMaterial = {
        'ChemicalFormula': 'Yb 2 O 3.2 Fe',
        'SampleNumberDensity': 0.23
    }
    SetSample(
        InputWorkspace=ws,
        Geometry=sampleGeometry,
        Material=sampleMaterial
    )
    beamGeometry = {
        'Shape': 'Slit',
        'Width': 2.0,
        'Height': 4.0
    }
    SetBeam(
        InputWorkspace=ws,
        Geometry=beamGeometry
    }

    DirectILLSelfShielding(
        InputWorkspace=ws,
        OutputWorkspace='absorption-corrections'
    )

    DirectILLApplySelfShielding(
        InputWorkspace=ws,
        OutputWorkspace='sample-absorption-corrected',
        SelfShieldingCorrectionWorkspace='absorption-corrections'
    )

    # Apply corrections to other measurements as well.
    DirectILLCollectData(
        Run='0158+0162',
        OutputWorkspace='sample2',
    )

    DirectILLApplySelfShielding(
        InputWorkspace='sample2',
        OutputWorkspace='sample2-absorption-corrected',
        SelfShieldingCorrectionWorkspace='absorption-corrections'
    )

IN5 specifics
=============

Elastic peak positions
----------------------

The intensities of individual pixels on IN5 are very low. This makes the fitting procedure employed by :ref:`algm-FindEPP` to work unreliably or fail altogether. Because of this, :ref:`algm-DirectILLCollectData` will use :ref:`algm-CreateEPP` instead by default for IN5. :ref:`algm-CreateEPP` produces an artificial EPP workspace based on the instrument geometry. This should be accurate enough for vanadium integration and diagnostics.

Diagnostics
-----------

The elastic peak diagnostics might be usable to mask the beam stop of IN5. The background diagnostics, on the other hand, are turned off by default as it makes no sense to mask individual pixels based on them.

Memory management
-----------------

Certain instruments with a large number of detectors/pixels may create workspaces which consume a lot of memory. When working on memory constrained systems, it is recommended to manually delete the workspaces which are not needed anymore in the reduction script. The :ref:`algm-SaveNexus` can be used to save the data to disk.

Full example
============

Lets put it all together into a complex Python script. The script below reduces the following dataset:

* Vanadium reference.

* An empty vanadium container.

  * Same shape as the sample container.
  * Complex shape: has to be given as XML.

* Sample measured at wavelength 1 at 50, 100 and 150K.

  * Share time-independent backgrounds from the measurement at 50K.

* Empty container measured at wavelength 1 at 50 and 150K.

  * Need to interpolate to 150K.

* Sample measured at wavelength 2 at 50, 100 and 150K.

  * Share time-independent backgrounds from the measurement at 50K.

* Empty container measured at wavelength 2.



.. code-block:: python

    mantid.config.appendDataSearchDir('/data/')

    # Gather dataset information.
    containerRuns = '96,97'
    vanadiumRuns = '100-103'
    # Samples at 50K, 100K and 150K.
    # Wavelength 1
    containerRuns1 = {
        50: '131-137',
        150: '138-143'
    }
    runs1 = {
        50: '105, 107-110',
        100: '112-117',
        150: '119-123, 125'
    }
    # Wavelength 2
    containerRun2 = '166-170'
    runs2 = {
        50: '146, 148, 150',
        100: '151-156',
        150: '160-165'
    }

    # Vanadium & vanadium container.
    DirectILLCollectData(
        Run=vanadiumRuns,
        OutputWorkspace='vanadium',
        OutputEPPWorkspace='vanadium-epp',
        OutputRawWorkspace='vanadium-raw',
        OutputIncidentEnergyWorkspace='vanadium-Ei' # Use for container
    )

    DirectILLCollectData(
        Run=containerRuns,
        OutputWorkspace='vanadium-container',
        IncidentEnergyWorkspace='vanadium-Ei'
    )

    containerShape = """
        <hollow-cylinder id="inner-ring">
          <centre-of-bottom-base x="0.0" y="-0.04" z="0.0" />
          <axis x="0.0" y="1.0" z="0.0" />
          <inner-radius val="0.017" />
          <outer-radius val="0.018" />
          <height val="0.08" />
        </hollow-cylinder>
        <hollow-cylinder id="outer-ring">
          <centre-of-bottom-base x="0.0" y="-0.04" z="0.0" />
          <axis x="0.0" y="1.0" z="0.0" />
          <inner-radius val="0.02" />
          <outer-radius val="0.021" />
          <height val="0.08" />
        </hollow-cylinder>
        <algebra val="inner-ring : outer-ring" />
    """
    containerGeometry = {
        'CSG': containerShape
    }
    containerMaterial = {
        'ChemicalFormula': 'Al',
        'SampleNumberDensity': 0.1
    }
    SetSample('vanadium-container', containerGeometry, containerMaterial)
    DirectILLSelfShielding(
        InputWorkspace='vanadium-container',
        OutputWorkspace='vanadium-container-self-shielding'
    )
    DirectILLApplySelfShielding(
        InputWorkspace='vanadium-container',
        OutputWorkspace='vanadium-container-corrected'
        SelfShieldingCorrectionWorkspace='vanadium-container-self-shielding'
    )

    sampleGeometry = {
        'Shape': 'HollowCylinder',
        'Height': 8.0,
        'InnerRadius': 1.8,
        'OuterRadium': 2.0,
        'Center': [0.0, 0.0, 0.0]
    }
    vanadiumMaterial = {
        'ChemicalFormula': 'V',
        'SampleNumberDensity': 0.15
    }
    SetSample('vanadium', sampleGeometry, vanadiumMaterial)
    DirectILLSelfShielding(
        InputWorkspace='vanadium',
        OutputWorkspace='vanadium-self-shielding'
    )
    DirectILLApplySelfShielding(
        InputWorkspace='vanadium',
        OutputWorkspace='vanadium-corrected',
        SelfShieldingCorrectionWorkspace='vanadium-self-shielding',
        EmptyContainerWorkspace='vanadium-container-corrected'
    )

    DirectILLIntegrateVanadium(
        InputWorkspace='vanadium-corrected',
        OutputWorkspace='vanadium-calibration',
        EPPWorkspace='vanadium-epp'
    )

    diagnosticsResult = DirectILLDiagnoseDetectors(
        InputWorkspace='vanadium-raw',
        OutputWorkspace='mask',
        EPPWorkspace='vanadium-epp',
        OutputReportWorkspace='diagnostics-report'
    )

    DirectILLCollectData(
        Run=runs1[50],
        OutputWorkspace='run1-50K',
        OutputIncidentEnergyWorkspace='Ei1',
        OutputFlatBkgWorkspace='bkg1-50K'
    )

    DirectILLCollectData(
        Run=containerRuns1[50],
        OutputWorkspace='container1-50K',
        IncidentEnergyWorkspace='Ei1'
    )

    SetSample('container1-50K', containerGeometry, containerMaterial)

    DirectILLSelfShielding(
        InputWorkspace='container1-50K',
        OutputWorkspace='container1-self-shielding'
    )

    DirectILLCollectData(
        Run=containerRuns1[150],
        OutputWorkspace='container1-150K',
        IncidentEnergyWorkspace='Ei1'
    )

    interpolated = 0.5 * (mtd['container1-50K'] + mtd['container1-150K'])
    RenameWorkspace(interpolated, 'container1-100K')

    for T in [50, 100, 150]:
        DirectILLApplySelfShielding(
            InputWorkspace='container1-{}K'.format(T),
            OutputWorkspace='container1-{}K-corrected'.format(T),
            SelfShieldingCorrectionWorkspace='container1-self-shielding'
        )

    sampleMaterial = {
        'ChemicalFormula': 'Fe 2 O 3',
        'SampleNumberDensity': 0.23
    }
    SetSample('run1-50K', sampleGeometry, sampleMaterial)
    DirectILLSelfShielding(
        InputWorkspace='run1-50K',
        OutputWorkspace='run1-self-shielding',
    )

    for T in runs1:
        if T != 50:
            # 50K data has been loaded already.
            DirectILLCollectData(
                Run=runs1[T],
                OutputWorkspace='run1-{}K'.format(T),
                IncidentEnergyWorkspace='Ei1',
                FlatBkgWorkspace='bkg1-50K'
            )
        DirectILLApplySelfShielding(
            InputWorkspace='run1-{}K'.format(T),
            OutputWorkspace='run1-{}K-corrected'.format(T),
            SelfShieldingCorrectionWorkspace='run1-self-shielding',
            EmptyContainerWorkspace='container1-{}K-corrected'.format(T)
        )
        DirectILLReduction(
            InputWorkspace='run1-{}K-corrected'.format(T),
            OutputWorkspace='SofQW1-{}K'.format(T),
            IntegratedVanadiumWorkspace='vanadium-calibration',
            DiagnosticsWorkspace='mask'
        )
        SaveNexus('SofQW1-{}K'.format(T), '/data/output2-{}.nxs'.format(T))

    DirectILLCollectData(
        Run=runs2[50],
        OutputWorkspace='run2-50K',
        OutputIncidentEnergyWorkspace=Ei2',
        OutputFlatBkgWorkspace='bgk2-50K'
    )

    DirectILLCollectData(
        Run=containerRun2,
        OutputWorkspace='container2',
        IncidentEnergyWorkspace='Ei2'
    )

    SetSample('container2', containerGeometry, containerMaterial)
    DirectILLSelfShielding(
        InputWorkspace='container2',
        OutputWorkspace='container2-self-shielding'
    )
    DirectILLApplySelfShielding(
        InputWorkspace='container2',
        OutputWorkspace='container2-corrected',
        SelfShieldingCorrectionWorkspace='container2-self-shielding'
    )

    SetSample('run2-50K', sampleGeometry, sampleMaterial)
    DirectILLSelfShielding(
        InputWorkspace='run2-50K',
        OutputWorkspace='run2-self-shielding'
    )

    for T in runs2:
        if T != 50:
            # 50K data has been loaded already.
            DirectILLCollectData(
                Run=runs2[T]
                OuputWorkspace='run2-{}K'.format(T),
                IncidentEnergyWorkspace='Ei2',
                FlatBkgWorkspace='bkg2-50K
            )
        DirectILLApplySelfShielding(
            InputWorkspace='run2-{}K'.format(T),
            OutputWorkspace='run2-{}K-corrected'.format(T),
            SelfShieldingCorrectionWorkspace='run2-self-shielding',
            EmptyContainerWorkspace='container2'
        )
        DirectILLReduction(
            InputWorkspace='run2-{}K-corrected'.format(T),
            OutputWorkspace='SofQW2-{}K'.format(T),
            IntegratedVanadiumWorkspace='vanadium-calibration',
            DiagnosticsWorkspace='mask'
        )
        SaveNexus('SofQW2-{}K'.format(T), '/data/output2-{}.nxs'.format(T))

.. categories: Concepts
