
.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This is a wrapper algorithm that calculates the spin leakage calibration and correction for a PA-SANS setup [#KRYCKA]_, compatible with the ISIS instruments ``ZOOM`` and ``LARMOR``.
To process the data, it combines several Mantid ``PolarizationCorrections`` algorithms.

The reduction is split in two separate steps, first; a calibration of transmission runs that generates the efficiency workspaces for the polarizing components of the setup, namely: Polarizer, Flippers and Helium Analyzer.
The second step corrects the spin leakage contribution on the scattering runs using the previously calculated efficiencies with the Wildes method [#WILDES]_.

The type of reduction; calibration or correction, can be set through  the ``ReductionType`` property, which selects from a list of values: ``Calibration``, ``Correction`` and ``CalibrationAndCorrection``.
If either ``Calibration`` or ``Correction`` are selected, only the respective reduction process will be processed. If ``CalibrationAndCorrection`` is selected, both the calibration and the correction
will be processed in sequence.

A path to a valid user file is mandatory and must be set through the ``UserFilePath`` property. The user file is expected to be a toml file in the :ref:`SANS Toml V2 Format <sans_toml_v1-ref>`, which  includes
fields for polarization settings. The user file will be used to extract information about the instrument: name, monitors, detector offsets and binning as well as the experiment flipper configuration and paths to the efficiency files.

A save path can be set with the ``SavePath`` property, which will save the efficiencies, fitting tables and processed transmission workspaces to nexus files.
If no output path is chosen, the produced workspaces will be kept on memory at Mantid's Analysis Data Service (ADS).

A suffix can be appended to the names of the efficiency workspaces and fitting tables by setting the ``OutputSuffix`` property.

The property ``KeepWSOnADS``, when set to :code:`False`, will delete the workspaces published to the ADS when the reduction is completed. It is only enabled when a valid path is chosen in ``SavePath``.
The wrapper can be called sequentially and the processed transmission and scattering workspaces, as well as the depolarized and direct runs won't be reloaded and reprocessed
if they are found on the ADS.


Calibration
-----------
For calibration, a list of transmission runs must be specified in the ``TransmissionRuns`` property, as well as a direct run in ``DirectBeamRun`` and a depolarized run in ``DepolarizedCellRun``.
The following steps are part of a calibration sequence [#KRYCKA]_:

- Load and normalize the direct (``DirectBeamRun``), depolarized (``DepolarizedCellRun``), empty cell (loaded from path in user file) and transmission runs (``TransmissionRuns``).
- Fit the :ref:`cell opacity <algm-DepolarizedAnalyserTransmission>` from the transmission between the depolarized and empty cell runs.
- Calculate the :ref:`Helium Analyser Efficiency <algm-HeliumAnalyserEfficiency>` for each transmission run and fit to an exponential decay to extract the initial polarization and decay rate of the analyzer cell.
- Calculate the :ref:`Flipper <algm-FlipperEfficiency>` and :ref:`Polarizer<algm-PolarizerEfficiency>` efficiencies for each transmission run. The flipper configuration is expected to be the same for all input transmission runs, and
  it is retrieved from the user file or calculated using :ref:`algm-DetermineSpinStateOrder` if the configuration is missing in the file. If the property ``AssertSpinState`` is set to :code:`False`, the spin state set on ``FlipperConfiguration`` field
  of the user file will be used in all runs without any further check.
- Average the calculated efficiencies for all transmission runs.
- Save the results in the path chosen in ``SavePath``. Additionally, the cell opacity and analyzer decay parameters are also stored in a table workspace at this location.

Correction
----------
For correction, a list of scattering runs must be specified in the ``ScatteringRuns`` property. Efficiency files are needed to calculate the corrections; If the reduction
type is ``CalibrationAndCorrection``, the efficiency workspaces on the ADS will be used. Otherwise if the reduction type is ``Correction``, efficiency workspaces will be loaded from file paths extracted from the user file.
The following steps are part of a correction sequence:

- Load scattering runs (``ScatteringRuns``) as well as efficiencies and polarization decay parameters if necessary.
- For each scattering run, calculate the Helium Analyzer Efficiency at the time of the experiment using the analyzer cell polarization decay parameters [#KRYCKA]_.
- Join the polarizer, flippers and analyzer efficiencies with the :ref:`algm-JoinISISPolarizationEfficiencies` algorithm.
- Correct the scattering data using the :ref:`algm-PolarizationEfficiencyCor` algorithm.
- Save the results in the path chosen in ``SavePath``.


References
----------

.. [#KRYCKA] K. Krycka et al., *J. Appl. Crystallogr.*, **45** (2012)
             `doi: 10.1107/S0021889812003445 <https://doi.org/10.1107/S0021889812003445>`_
.. [#WILDES] A. R. Wildes, *Neutron News*, **17** 17 (2006)
             `doi: 10.1080/10448630600668738 <https://doi.org/10.1080/10448630600668738>`_


.. categories::

.. sourcelink::
