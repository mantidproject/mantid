.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm performs polarised diffraction reduction for the D7 instrument at the ILL.
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
| BeamWithCadmium  |                                 |                                            |
+------------------+---------------------------------+--------------------------------------------+
| EmptyBeam        | * CadmiumTransmissionWorkspace  |                                            |
|                  |                                 |                                            |
+------------------+---------------------------------+--------------------------------------------+
| Transmission     | * CadmiumTransmissionWorkspace  |                                            |
|                  | * **EmptyBeamWorkspace**        |                                            |
+------------------+---------------------------------+--------------------------------------------+
| Cadmium          |                                 |                                            |
+------------------+---------------------------------+--------------------------------------------+
| Empty            |                                 |                                            |
+------------------+---------------------------------+--------------------------------------------+
| Quartz           | * CadmiumWorkspace              | * OutputTreatment                          |
|                  | * EmptyContainerWorkspace       |                                            |
|                  | * **Transmission**              |                                            |
+------------------+---------------------------------+--------------------------------------------+
| Vanadium         | * CadmiumWorkspace              | * SampleGeometry                           |
|                  | * EmptyContainerWorkspace       | * **SampleAndEnvironmentProperties**       |
|                  | * Transmission                  | * OutputTreatment                          |
|                  | * QuartzWorkspace               |                                            |
+------------------+---------------------------------+--------------------------------------------+
| Sample           | * CadmiumWorkspace              | * SampleGeometry                           |
|                  | * EmptyContainerWorkspace       | * **SampleAndEnvironmentProperties**       |
|                  | * Transmission                  | * OutputTreatment                          |
|                  | * QuartzWorkspace               |                                            |
+------------------+---------------------------------+--------------------------------------------+

All the input workspace properties above are optional, unless bolded.
For example, if processing as sample, if a empty container and cadmium absorber inputs are specified, subtraction will be performed, if not, the step will be skipped.
The rare exceptions are when processing as transmission, when beam input workspace is mandatory, and to calculate polarising efficiencies,
where input from transmission is indispensable. Transmission however can be provided also as a string containing floating point value of desired tranmission, that needs
to be in the range (0, 1].

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

`IndividualXY` allows to display all measured point on a single plot as a function of a twotheta. This option is indended as a convenient diagnostics, and the output
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

Optional keys:

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

**Example - full treatment of a sample**

.. testsetup:: ExPolDiffILLReduction

    config['default.facility'] = 'ILL'
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

.. categories::

.. sourcelink::
