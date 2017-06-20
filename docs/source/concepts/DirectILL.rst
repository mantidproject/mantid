.. _DirectILL:

====================================================
Data reduction for ILL's direct geometry instruments
====================================================

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



- IN5 specifics.

Detector diagnostics and masking
================================

- How and where masking is used.

- Prerequisites of input workspace (OutputRawWorkspace).

- User masking, default masks.

- IN5 specifics.

Diagnosing the diagnostics
--------------------------

- Tips on how to plot columns from the diagnostics report.

Absorption corrections
======================

- How and where absorption corrections are used.

- How to set sample and beam information.

Recommendations for IN5
=======================

There are a few things to point out with regards to IN5. Some of these may be valid for IN4 and IN6 data as well.

Elastic peak positions
----------------------

The intensities measured by individual pixels on IN5 are very low. This makes the fitting procedure employed by :ref:`algm-FindEPP` to work unreliably or fail altogether. There is an option in :ref:`algm-DirectILLCollectData` to use :ref:`algm-CreateEPP` instead. This algorithm will create an artificial EPP workspace based on the instrument geometry. This should be accurate enough for vanadium integration, though.

.. code-block:: python

    # Add a temporary data search directory.
    mantid.config.appendDataSearchDir('/data/')

    # Vanadium
    DirectILLCollectData(
        Run='0100:0109',
        OutputWorkspace='sample',
        EPPCreationMethod='CalculateEPP'
    )

Diagnostics
-----------

The elastic peak diagnostics might be usable to mask the beam stop of IN5. The background diagnostics, on the other hand, should not be used as it makes no sense to mask individual pixels based on them.

Memory management
-----------------

Certain instruments with a large number of detectors/pixels may create workspaces which consume a lot of memory. When working on memory constrained systems, it is recommended to manually delete the workspaces which are not needed anymore in the reduction script. The :ref:`algm-SaveNexus` can be used to save the data to disk.


Full example
============

- A well documented example script involving everyting:
- Empty containers and absorption corrections for vanadium.
- Empty container interpolation for sample.
- Absorption corrections.
- Samples

.. categories: Concepts
