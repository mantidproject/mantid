.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm performs polarised diffraction and spectroscopy reduction for the D7 instrument at the ILL.
With each call, this algorithm processes one type of data which is a part of the whole experiment.
The logic is resolved by the property **ProcessAs**, which governs the reduction steps based on the requested type.
It can be one of the 8: cadmium, empty beam, beam-with-cadmium, transmission, empty, quartz, vanadium, and sample.
The full data treatment of the complete experiment should be build up as a chain with multiple calls of this algorithm over various types of acquisitions.
The sequence should be logical, typically as enumerated above, since the later processes need the outputs of earlier processes as input.
The common mandatory input is a run file (numor), or a list of them. In case a list is provided, coming for example from a scan over twoTheta angle,
the data is treated individually up to the point of background subtraction, and then can be either left as a list, each detector can averaged over the scan,
or all data from the scan can summed.

The input data is renamed, and a suffix is added to each numor containing information about the polarisation direction ('Z', 'X', 'Y', etc.) and the flipper state ('ON', 'OFF').

Most of the corrections, such as background subtraction or polarisation correction, are optional and their inclusion depends on the provided inputs and set flags.
However, it is mandatory to provide basic information about the vanadium and sample, such as mass, chemical formula, and either density or number density.

The common mandatory output is a workspace, but up to which step it is processed, depends on **ProcessAs**.

ProcessAs
---------
Different input properties can be specified depending on the value of **ProcessAs**, as summarized in the table:

+------------------+---------------------------------+--------------------------------------------+
| ProcessAs        | Input Workspace Properties      | Other Input Properties                     |
+==================+=================================+============================================+
| BeamWithCadmium  |                                 | * NormaliseBy                              |
+------------------+---------------------------------+--------------------------------------------+
| EmptyBeam        | * CadmiumTransmissionWorkspace  | * NormaliseBy                              |
|                  |                                 |                                            |
+------------------+---------------------------------+--------------------------------------------+
| Transmission     | * CadmiumTransmissionWorkspace  | * NormaliseBy                              |
|                  | * **EmptyBeamWorkspace**        |                                            |
+------------------+---------------------------------+--------------------------------------------+
| Cadmium          |                                 | * NormaliseBy                              |
+------------------+---------------------------------+--------------------------------------------+
| Empty            |                                 | * NormaliseBy                              |
+------------------+---------------------------------+--------------------------------------------+
| Quartz           | * CadmiumWorkspace              | * NormaliseBy                              |
|                  | * EmptyContainerWorkspace       | * OutputTreatment                          |
|                  | * **Transmission**              |                                            |
+------------------+---------------------------------+--------------------------------------------+
| Vanadium         | * CadmiumWorkspace              | * NormaliseBy                              |
|                  | * EmptyContainerWorkspace       | * SampleGeometry                           |
|                  | * Transmission                  | * **SampleAndEnvironmentProperties**       |
|                  | * QuartzWorkspace               | * OutputTreatment                          |
|                  | * ElasticChannelWorkspace       | * ConvertToEnergy                          |
|                  |                                 | * EnergyBinning                            |
|                  |                                 | * DetectorEfficiencyCorrection             |
|                  |                                 | * FrameOverlapCorrection                   |
|                  |                                 | * MaskDetectors                            |
|                  |                                 | * MaxTOFChannel                            |
|                  |                                 | * SubtractTOFBackgroundMethod              |
|                  |                                 | * PerformAnalyserTrCorrection              |
+------------------+---------------------------------+--------------------------------------------+
| Sample           | * CadmiumWorkspace              | * NormaliseBy                              |
|                  | * EmptyContainerWorkspace       | * SampleGeometry                           |
|                  | * Transmission                  | * **SampleAndEnvironmentProperties**       |
|                  | * QuartzWorkspace               | * OutputTreatment                          |
|                  | * ElasticChannelsWorkspace      | * MeasurementTechnique                     |
|                  |                                 | * ConvertToEnergy                          |
|                  |                                 | * EnergyBinning                            |
|                  |                                 | * DetectorEfficiencyCorrection             |
|                  |                                 | * FrameOverlapCorrection                   |
|                  |                                 | * MaskDetectors                            |
|                  |                                 | * MaxTOFChannel                            |
|                  |                                 | * SubtractTOFBackgroundMethod              |
|                  |                                 | * PerformAnalyserTrCorrection              |
+------------------+---------------------------------+--------------------------------------------+

All the input workspace properties above are optional, unless bolded.
For example, if processing as sample, if a empty container and cadmium absorber inputs are specified, subtraction will be performed, if not, the step will be skipped.
The rare exceptions are when processing as transmission, when beam input workspace is mandatory, and to calculate polarising efficiencies,
where input from transmission is indispensable. Transmission however can be provided also as a string containing floating point value of desired tranmission, that needs
to be in the range (0, 1].

NormaliseBy
-----------

This property allows to choose how the data is going to be normalised. The choices are: `Monitor` (Monitor 1) and `Time` (experiment duration saved in the NeXus file).

MeasurementTechnique
--------------------

This property allows to distinguish between reducing powder data from single crystal measurement. The options are: `Powder`, `SingleCrystal`, and `TOF`. In the case of single crystal
data, only one bank position can be processed at a time, and all input files for that bank are concatenated into a single workspace with vertical axis being 2theta positions of detectors,
and the horizontal axis containing omega scan steps.

In the `TOF` case, the X-axis is binned according to uncalibrated time, with binning information (start of the range, time bin width, number of bins) coming from NeXus files. Setting
the `MeasurementTechnique` property to `TOF` enables also elastic peak calibration of data, with peak information coming from :ref:`FindEPP <algm-FindEPP>` which is run on vanadium data
during reducing data as `Vanadium`. The output of `Vanadium` reduction containing information with fitted elastic peak positions provided through `ElasticChannelsWorkspace` property,
or a mock TableWorkspace containing at least two columns: `PeakCentre` and `Sigmas`, or an `EPCentre` (optionally also `EPWidth`) key defined via `SampleAndEnvironmentProperties` property,
is required when the `processAs` is set to `Sample` and `MeasurementTechnique` is set to `TOF`. The output workspace processed in this mode will have its X-axis units set to calibrated energy
exchange (`DeltaE`), where the energy exchange equal to 0 is set to fall at the position of the elastic peak for each detector.

SubtractTOFBackgroundMethod
---------------------------

This property is relevant only when `MeasurementTechnique` is `TOF`. The property introduces three options for subtracting the background from the sample in the time-of-flight mode.
In all cases, the background is coming from either a measurement of the empty container or vanadium sample, if container measurement is not possible. The background is split in two
contributions, time-independent and time-dependent, respectively. The time-independent contribution is calculated as an average of counts outside of the elastic peak region. The time-dependent
contribution is the background present in the elastic peak region. Different options of the `SubtractTOFBackgroundMethod` allow the user to choose how the time-dependent background
estimate is going to be subtracted from the current sample (vanadium, sample) data. The full mathematical description is placed in the technique document for the Polarised Diffraction
at D7.

When `SubtractTOFBackgroundMethod` is equal to `Data`, the counts coming from empty container or vanadium measurement are used without modification and directly subtracted from current
sample counts. When this property is set to `Rectangular`, an average of the measured background source counts is used instead of direct counts. This allows to smooth our fluctuations in
the background source data, which in principle may be quite noisy. The last option is `Gaussian`, which estimates the background source counts as a gaussian distribution, with a centre
at the elastic peak, the width of the elastic peak, and the integrated counts equal to that of the background source.

Detector and analyser energy correction
---------------------------------------

The detector and analyser energy corrections are performed only when the `ConvertToEnergy` property is checked.

The detector energy correction is performed using :ref:`DetectorEfficiencyCorUser <algm-DetectorEfficiencyCorUser>` algorithm, with the following function:

.. math:: f(E_{i}) = 1.0 - \mathrm{exp} \left( \frac{-13.153}{\sqrt{E_{i}}} \right),

where the constant value of -13.153 is derived from multiplying the pressure of detector tubes (10 Pa), their diameter (2.54 cm),
and a factor of −0.51784, obtained by D7 responsible scientists using Monte Carlo simulations.

The analyser energy correction is a multiplicative correction, applied after the detector efficiency is taken into account, if the `PerformAnalyserTrCorrection` property is checked.
The correction factor is a ratio of the analyser transmission for the elastic energy and the final energy corresponding to each bin (after conversion from time channels to energy exchange).
The distribution of analyser transmission values as a function of wavelength (or, equivalently, energy) are coming from Monte Carlo simulations performed by D7 scientists, and are
shown in Fig. 9 of Ref. [#Steward]_.

OutputTreatment
---------------

This property of the algorithm allows to decide the treatment and shape of the output of the reduction workflow. There are several options available:

- *Individual*
- *IndividualXY*
- *AveragePol*
- *AverageTwoTheta*
- *Sum*.

The `Individual` setting will preserve the number of workspaces of the input, allowing to check workspace by workspace how the relevant process reduced the data.
This is the recommended setting for the sample data processing for the use as input to :ref:`D7AbsoluteCrossSections <algm-D7AbsoluteCrossSections>` algorithm.

`IndividualXY` allows to display all measured point on a single plot as a function of a twotheta. This option is intended as a convenient diagnostics, and the output
obtained with this selection is not a suitable input for further processing in :ref:`D7AbsoluteCrossSections <algm-D7AbsoluteCrossSections>`.

`AveragePol` will average the workspaces according to their polarization orientation and the flipper state. The output will contain as many workspaces as there are
relevant combinations of the polarization and the flipper state, so two workspaces in the case of the uniaxial measurement, six for XYZ, etc. This is the recommended
setting for processing Quartz.

`AverageTwoTheta` will average the workspaces with the same `2theta.requested` metadata entry. The output will contain as many workspaces as there were different
requested twotheta positions. This setting is intended only as a convenient diagnostics of the reduction processing, and the output is not suitable for further processing.

`Sum` behaviour depends on the process. For processing Vanadium, it will first average input workspaces according to their polarisation orientation, like in `AveragePol`,
and then the averaged workspaced will be summed. For different processes, this selection calls :ref:`SumOverlappingTubes <algm-SumOverlappingTubes>` algorithm and will
display data as a function of twotheta. This is the recommended setting for processing Vanadium; for other process types, the output is not suitable for further processing.


SampleAndEnvironmentProperties
##############################

This property is a dictionary containing all of the information about the sample and its environment. This information is used in self-attenuation
calculations and in normalisation.

The complete list of keys can is summarised below:

Sample-only keys:

- *SampleMass*
- *FormulaUnitMass*
- *SampleChemicalFormula*
- *SampleDensity*
- *Height*

The SampleMass needs to be defined, as well as the FormulaUnitMass, even when the self-attenuation is not taken into account. The other
parameters are required when the self-attenuation coefficients are calculated.

Container-only keys:

- *ContainerChemicalFormula*
- *ContainerDensity*

Beam-only keys:

- *BeamHeight*
- *BeamWidth*

These do not have to be defined, and by default will be set to be larger than the sample size.

Then, depending on the chosen sample geometry, additional parameters need to be defined:

* For FlatPlate:

  - *SampleThickness*
  - *SampleWidth*
  - *SampleCenter*
  - *SampleAngle*
  - *ContainerFrontThickness*
  - *ContainerBackThickness*

* For Cylinder:

  - *SampleRadius*
  - *ContainerRadius*

* For Annulus:

  - *SampleInnerRadius*
  - *SampleOuterRadius*
  - *ContainerInnerRadius*
  - *ContainerOuterRadius*

Time-of-flight specific keys:

- *EPCentre*
- *EPWidth*
- *EPNSigmasBckg*
- *EPNSigmasVana*

The *EPCentre* and *EPWidth* keys are the user-defined centre and width, respectively, of elastic peaks to be used
for integrating elastic peaks, in units of time-of-flight. If any of those keys is provided along with the ElasticChannelWorkspace,
it will be used instead of information provided in that table.

*EPNSigmasBckg* and *EPNSigmasVana* control the integration range around the position of elastic peak for each detector by expanding
the width of the peak by the provided factor, used for calculations of the time-dependent background and the vanadium normalisation, respectively.

Optional general-use keys:

- *InitialEnergy* - if not provided, the value will be calculated from the wavelength in the SampleLogs
- *NMoles* - if not provided, the value will be calculated based on the *SampleMass* and *FormulaUnitMass*

Workflows
#########

In the flowcharts below the yellow ovals represent the inputs, the grey parallelograms are the outputs for each process type.

Absorber transmission
#####################

.. diagram:: PolDiffILLReduction-v1_absorber_wkflw.dot

Beam
####

.. diagram:: PolDiffILLReduction-v1_beam_wkflw.dot

Transmission
############

.. diagram:: PolDiffILLReduction-v1_transmission_wkflw.dot

Container/Absorber
##################

.. diagram:: PolDiffILLReduction-v1_container_wkflw.dot

Quartz
######

.. diagram:: PolDiffILLReduction-v1_quartz_wkflw.dot


Reference
#########

.. diagram:: PolDiffILLReduction-v1_vanadium_wkflw.dot

Sample
######

.. diagram:: PolDiffILLReduction-v1_sample_wkflw.dot

Full Treatment
##############

Full treatment is built by stacking up unary reductions with corresponding **ProcessAs**. The diagram below illustrates the flow of processing.
Letters denote beam with absorber (AT), beam (B), transmission (T), cadmium (A), empty (C), quartz (Q), vanadium (V), sample (S).
AT is processed first, and passed to all the other processes.
B takes only AT as optional input, and the output of B is needed by all transmisison calculations.
T takes AT and B as inputs, and the calculated transmission is used by Q, V, and S respectively.
C and A are supplied to Q, V, and S respectively.
Q takes A, C, its T, and the output is provided to V and S.
V takes A, C, its T, and Q as inputs and the output can used to normalise S
S takes A, C, its T, as well as Q as inputs.
The output of S is reduced sample in desired units.

.. diagram:: PolDiffILLReduction-v1_all_wkflw.dot

This example below performs a complete reduction for D7 data.

.. note::

  For transmission calculation, the beam run and the transmission run have to be recorded at the same instrument configuration.
  For container subtraction, the container and the sample run have to be recorded at the same configuration.

.. include:: ../usagedata-note.txt

**Example - full treatment of a powder sample**

.. testsetup:: ExPolDiffILLReduction

    config['default.facility'] = 'ILL'
    config['default.instrument'] = 'D7'
    config.appendDataSearchSubDir('ILL/D7/')

.. testcode:: ExPolDiffILLReduction

    vanadium_dictionary = {'SampleMass':8.54,'SampleDensity':0.2,'FormulaUnitMass':50.94}

    sample_dictionary = {'SampleMass':2.932,'SampleDensity':0.1,'FormulaUnitMass':182.56}

    # Beam with cadmium absorber, used for transmission
    PolDiffILLReduction(
        Run='396991',
        OutputWorkspace='cadmium_transmission_ws',
        ProcessAs='BeamWithCadmium'
    )
    # Beam measurement for transmisison
    PolDiffILLReduction(
        Run='396983',
        OutputWorkspace='beam_ws',
        CadmiumTransmissionWorkspace='cadmium_transmission_ws',
        ProcessAs='EmptyBeam'
    )
    print('Cadmium absorber monitor 2 rate as a ratio of empty beam is {0:.3f}'.format(mtd['cadmium_transmission_ws_1'].readY(0)[0] / mtd['beam_ws_1'].readY(0)[0]))

    # Quartz transmission
    PolDiffILLReduction(
        Run='396985',
        OutputWorkspace='quartz_transmission',
        CadmiumTransmissionWorkspace='cadmium_transmission_ws',
        EmptyBeamWorkspace='beam_ws',
        ProcessAs='Transmission'
    )
    print('Quartz transmission is {0:.3f}'.format(mtd['quartz_transmission_1'].readY(0)[0]))

    # Empty container
    PolDiffILLReduction(
        Run='396917',
        OutputWorkspace='empty_ws',
        ProcessAs='Empty'
    )

    # Cadmium absorber
    PolDiffILLReduction(
        Run='396928',
        OutputWorkspace='cadmium_ws',
        ProcessAs='Cadmium'
    )

    # Polarisation correction
    PolDiffILLReduction(
        Run='396939',
        OutputWorkspace='pol_corrections',
        CadmiumWorkspace='cadmium_ws',
        EmptyContainerWorkspace='empty_ws',
        # Transmission='0.95', # transmission can be also provided as a string with desired value
        Transmission='quartz_transmission',
        OutputTreatment='AveragePol',
        ProcessAs='Quartz'
    )

    # Vanadium transmission
    PolDiffILLReduction(
        Run='396990',
        OutputWorkspace='vanadium_transmission',
        CadmiumTransmissionWorkspace='cadmium_transmission_ws',
        EmptyBeamWorkspace='beam_ws',
        ProcessAs='Transmission'
    )
    print('Vanadium transmission is {0:.3f}'.format(mtd['vanadium_transmission_1'].readY(0)[0]))

    # Vanadium reduction
    PolDiffILLReduction(
        Run='396993',
        OutputWorkspace='vanadium_ws',
        CadmiumWorkspace='cadmium_ws',
        EmptyContainerWorkspace='empty_ws',
        Transmission='vanadium_transmission',
        QuartzWorkspace='pol_corrections',
        OutputTreatment='Sum',
        SampleGeometry='None',
        SampleAndEnvironmentProperties=vanadium_dictionary,
        ProcessAs='Vanadium'
    )

    # Sample transmission
    PolDiffILLReduction(
       Run='396986',
       OutputWorkspace='sample_transmission',
       CadmiumTransmissionWorkspace='cadmium_transmission_ws',
       EmptyBeamWorkspace='beam_ws',
       ProcessAs='Transmission'
    )
    print('Sample transmission is {0:.3f}'.format(mtd['sample_transmission_1'].readY(0)[0]))

    # Sample reduction
    PolDiffILLReduction(
        Run='397004',
        OutputWorkspace='sample_ws',
        CadmiumWorkspace='cadmium_ws',
        EmptyContainerWorkspace='empty_ws',
        Transmission='sample_transmission',
        QuartzWorkspace='pol_corrections',
        OutputTreatment='Individual',
        SampleGeometry='None',
        SampleAndEnvironmentProperties=sample_dictionary,
	ProcessAs='Sample'
    )

Output:

.. testoutput:: ExPolDiffILLReduction

    Cadmium absorber monitor 2 rate as a ratio of empty beam is 0.011
    Quartz transmission is 0.700
    Vanadium transmission is 0.886
    Sample transmission is 0.963

.. testcleanup:: ExPolDiffILLReduction

    mtd.clear()

**Example - full treatment of a single crystal sample**

.. code-block:: python

   vanadium_mass = 8.535
   sample_formula_mass = 137.33 * 2.0 + 54.93 + 127.6 + 15.999 * 6.0
   sample_mass = 7.83
   vanadium_dictionary = {'SampleMass': vanadium_mass, 'FormulaUnits': 1, 'FormulaUnitMass': 50.942}
   sample_dictionary = {'SampleMass': sample_mass, 'FormulaUnits': 1, 'FormulaUnitMass': sample_formula_mass,
                        'KiXAngle': 45.0, 'OmegaShift': 52.5}
   calibration_file = "D7_YIG_calibration.xml"

   # Empty container for quartz and vanadium
   PolDiffILLReduction(Run='450747:450748',
                       OutputWorkspace='container_ws',
		       ProcessAs='Empty')

   # Empty container for bank position 1 (bt1), tth=79.5
   PolDiffILLReduction(
       Run='397406:397407',
       OutputTreatment='AveragePol',
       OutputWorkspace='container_bt1_ws',
       ProcessAs='Empty')
   # empty container for bt2, tth=75
   PolDiffILLReduction(
       Run='397397:397398',
       OutputTreatment='AveragePol',
       OutputWorkspace='container_bt2_ws',
       ProcessAs='Empty')

   PolDiffILLReduction(
       Run='450769:450770',
       OutputWorkspace='pol_corrections',
       EmptyContainerWorkspace='container_ws',
       Transmission='0.9',
       OutputTreatment='AveragePol',
       ProcessAs='Quartz')

   PolDiffILLReduction(
       Run='450835:450836',
       OutputWorkspace='vanadium_ws',
       EmptyContainerWorkspace='container_ws',
       Transmission='0.89',
       QuartzWorkspace='pol_corrections',
       OutputTreatment='Sum',
       SampleGeometry='None',
       SelfAttenuationMethod='Transmission',
       SampleAndEnvironmentProperties=vanadium_dictionary,
       AbsoluteNormalisation=True,
       InstrumentCalibration=calibration_file,
       ProcessAs='Vanadium')

   # bank position 1, tth=79.5
   PolDiffILLReduction(
       Run='399451:399452',
       OutputWorkspace='bt1',
       EmptyContainerWorkspace='container_bt1_ws',
       Transmission='0.95',
       QuartzWorkspace='pol_corrections',
       OutputTreatment='Individual',
       SampleGeometry='None',
       SampleAndEnvironmentProperties=sample_dictionary,
       MeasurementTechnique='SingleCrystal',
       InstrumentCalibration=calibration_file,
       ProcessAs='Sample')
   # bank position 2, tth=75
   PolDiffILLReduction(
       Run='400287:400288',
       OutputWorkspace='bt2',
       EmptyContainerWorkspace='container_bt2_ws',
       Transmission='0.95',
       QuartzWorkspace='pol_corrections',
       OutputTreatment='Individual',
       SampleGeometry='None',
       SampleAndEnvironmentProperties=sample_dictionary,
       MeasurementTechnique='SingleCrystal',
       InstrumentCalibration=calibration_file,
       ProcessAs='Sample')
   appended_ws = 'appended_ws'
   AppendSpectra(InputWorkspace1='bt1', InputWorkspace2='bt2',
   OutputWorkspace=appended_ws)
   # names need to be re-set, AppendSpectra just concatenates them
   possible_polarisations = ['ZPO_ON', 'ZPO_OFF', 'XPO_ON', 'XPO_OFF', 'YPO_ON', 'YPO_OFF']
   polarisation = ""
   for entry in mtd[appended_ws]:
       entry_name = entry.name()
       for polarisation in possible_polarisations:
           if polarisation in entry_name:
	       break
       RenameWorkspace(InputWorkspace=entry, OutputWorkspace="{}_{}".format(appended_ws, polarisation))


**Example - full treatment of a water sample measured in Time-of-flight mode**

.. code-block:: python

   # based on logbook from exp_6-02-594, cycle 193

   vanadium_mass = 6.11 * 4.0 * np.pi * (0.6**2 - 0.4**2)
   formula_weight_H2O = 1.008 * 2 + 15.999 # NIST
   sample_mass_H2O = 0.874
   sample_formula_H2O = 'H2O'
   sample_thickness_H2O = 1.95
   max_tof_channel = 511
   vanadium_dictionary = {'SampleMass':vanadium_mass, 'FormulaUnitMass':50.942, 'EPCentre':1645.0, 'EPWidth':54.0, 'EPNSigmasBckg':3.0, 'EPNSigmasVana': 3.0}
   sample_dictionary_H2O = {'SampleMass':sample_mass_H2O, 'FormulaUnitMass':formula_weight_H2O, 'SampleChemicalFormula':sample_formula_H2O}
   yig_calibration_file = "D7_YIG_calibration_TOF.xml"

   # Beam measurement for transmission
   PolDiffILLReduction(
		Run='395557',
		OutputWorkspace='beam_ws',
		ProcessAs='EmptyBeam'
   )

   # empty container
   PolDiffILLReduction(
		Run='396036:396155',
		OutputTreatment='AveragePol',
		OutputWorkspace='container_ws',
		ProcessAs='Empty'
   )

   # Vanadium transmission
   PolDiffILLReduction(
		Run='395564',
		OutputWorkspace='vanadium_tr',
		EmptyBeamWorkspace='beam_ws',
		ProcessAs='Transmission'
   )

   # Vanadium reduction
   PolDiffILLReduction(
		Run='396016:396034',
		OutputWorkspace='vanadium_ws',
		EmptyContainerWorkspace='container_ws',
		Transmission='vanadium_tr',
		OutputTreatment='Sum',
		SelfAttenuationMethod='None',
		SampleGeometry='None',
		AbsoluteNormalisation=False,
		SampleAndEnvironmentProperties=vanadium_dictionary,
		MeasurementTechnique='TOF',
		ProcessAs='Vanadium',
		InstrumentCalibration=yig_calibration_file,
		FrameOverlapCorrection=True,
		DetectorEnergyEfficiencyCorrection=True,
		ConvertToEnergy=True,
		ClearCache=True
   )

   # H2O transmission
   PolDiffILLReduction(
		Run='395560,395561', # 0 and 90 degrees
		OutputWorkspace='h2O_1cm_tr',
		EmptyBeamWorkspace='beam_ws',
		ProcessAs='Transmission'
   )
   # Sample reduction

   # water reduction
   PolDiffILLReduction(
		Run="395639:395798",
		OutputWorkspace='h2O_ws',
		EmptyContainerWorkspace='container_ws',
		Transmission='h2O_1cm_tr',
		OutputTreatment='AveragePol',
		SampleGeometry='None',
		SampleAndEnvironmentProperties=sample_dictionary_H2O,
		MeasurementTechnique='TOF',
		InstrumentCalibration=yig_calibration_file,
		ElasticChannelsWorkspace='vanadium_ws_elastic',
		ProcessAs='Sample',
		FrameOverlapCorrection=True,
		DetectorEnergyEfficiencyCorrection=True,
		ConvertToEnergy=True,
		ClearCache=True
   )

References
----------

.. [#Steward] Stewart, J. R. and Deen, P. P. and Andersen, K. H. and Schober, H. and Barthelemy, J.-F. and Hillier, J. M. and Murani, A. P. and Hayes, T. and Lindenau, B.
   *Disordered materials studied using neutron polarization analysis on the multi-detector spectrometer, D7*
   Journal of Applied Crystallography **42** (2009) 69-84
   `doi: 10.1107/S0021889808039162 <https://doi.org/10.1107/S0021889808039162>`_

.. categories::

.. sourcelink::
