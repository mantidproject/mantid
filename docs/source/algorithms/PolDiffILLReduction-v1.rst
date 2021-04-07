.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm performs polarised diffraction reduction for the D7 instrument at the ILL.
With each call, this algorithm processes one type of data which is a part of the whole experiment.
The logic is resolved by the property **ProcessAs**, which governs the reduction steps based on the requested type.
It can be one of the 8: absorber, empty beam, beam-with-absorber, transmission, container, quartz, vanadium, and sample.
The full data treatment of the complete experiment should be build up as a chain with multiple calls of this algorithm over various types of acquisitions.
The sequence should be logical, typically as enumerated above, since the later processes need the outputs of earlier processes as input.
The common mandatory input is a run file (numor), or a list of them. In case a list is provided, coming for example from a scan over twoTheta angle,
the data is treated individually up to the point of background subtraction, and then can be either left as a list, each detector can averaged over the scan,
or all data from the scan can summed.

Most of the corrections, such as background subtraction or polarisation correction, are optional and their inclusion depends on the provided inputs and set flags.
However, it is mandatory to provide basic information about the vanadium and sample, such as mass, chemical formula, and either density or number density.

The common mandatory output is a workspace, but up to which step it is processed, depends on **ProcessAs**.

ProcessAs
---------
Different input properties can be specified depending on the value of **ProcessAs**, as summarized in the table:

+------------------+--------------------------------------+--------------------------------------------+
| ProcessAs        | Input Workspace Properties           | Other Input Properties                     |
+==================+======================================+============================================+
| BeamWithAbsorber |                                      |                                            |
+------------------+--------------------------------------+--------------------------------------------+
| EmptyBeam        | * AbsorberTransmissionInputWorkspace |                                            |
|                  |                                      |                                            |
+------------------+--------------------------------------+--------------------------------------------+
| Transmission     | * AbsorberTransmissionInputWorkspace |                                            |
|                  | * **BeamInputWorkspace**             |                                            |
+------------------+--------------------------------------+--------------------------------------------+
| Absorber         |                                      |                                            |
+------------------+--------------------------------------+--------------------------------------------+
| Container        |                                      |                                            |
+------------------+--------------------------------------+--------------------------------------------+
| Quartz           | * AbsorberInputWorkspace             | * OutputTreatment                          |
|                  | * ContainerInputWorkspace            |                                            |
|                  | * **TransmissionInputWorkspace**     |                                            |
+------------------+--------------------------------------+--------------------------------------------+
| Vanadium         | * AbsorberInputWorkspace             | * SampleGeometry                           |
|                  | * ContainerInputWorkspace            | * **SampleAndEnvironmentProperties**       |
|                  | * TransmissionInputWorkspace         | * OutputTreatment                          |
|                  | * QuartzInputWorkspace               |                                            |
+------------------+--------------------------------------+--------------------------------------------+
| Sample           | * AbsorberInputWorkspace             | * SampleGeometry                           |
|                  | * ContainerInputWorkspace            | * **SampleAndEnvironmentProperties**       |
|                  | * TransmissionInputWorkspace         | * OutputTreatment                          |
|                  | * QuartzInputWorkspace               |                                            |
+------------------+--------------------------------------+--------------------------------------------+

All the input workspace properties above are optional, unless bolded.
For example, if processing as sample, if a container and absorber inputs are specified, subtraction will be performed, if not, the step will be skipped.
The rare exceptions are when processing as transmission, when beam input workspace is mandatory, and to calculate polarising efficiencies,
where input from transmission is indispensable.

SampleAndEnvironmentProperties
##############################

This property is a dictionary containing all of the information about the sample and its environment. This information is used in self-attenuation
calculations and in normalisation.

The complete list of keys can is summarised below:

Sample-only keys:

- *SampleMass*
- *FormulaUnits*
- *FormulaUnitMass*
- *SampleChemicalFormula*
- *SampleDensity*
- *Height*

Container-only keys:

- *ContainerChemicalFormula*
- *ContainerDensity*

Beam-only keys:

- *BeamHeight*
- *BeamWidth*

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
Letters denote beam with absorber (AT), beam (B), transmission (T), absorber (A), container (C), quartz (Q), vanadium (V), sample (S).
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

    vanadium_dictionary = {'SampleMass':8.54,'SampleDensity':6.0,'FormulaUnits':1,'FormulaUnitMass':50.94}

    sample_dictionary = {'SampleMass':2.932,'SampleDensity':2.0,'FormulaUnits':1, 'FormulaUnitMass':182.56}

    # Beam with cadmium absorber, used for transmission
    PolDiffILLReduction(
        Run='396991',
        OutputWorkspace='cadmium_ws',
        ProcessAs='BeamWithAbsorber'
    )
    # Beam measurement for transmisison
    PolDiffILLReduction(
        Run='396983',
        OutputWorkspace='beam_ws',
        AbsorberTransmissionInputWorkspace='cadmium_ws_1',
        ProcessAs='EmptyBeam'
    )
    print('Cadmium absorber transmission is {0:.3f}'.format(mtd['cadmium_ws_1'].readY(0)[0] / mtd['beam_ws_1'].readY(0)[0]))

    # Quartz transmission
    PolDiffILLReduction(
        Run='396985',
        OutputWorkspace='quartz_transmission',
        AbsorberTransmissionInputWorkspace='cadmium_ws_1',
        BeamInputWorkspace='beam_ws_1',
       ProcessAs='Transmission'
    )
    print('Quartz transmission is {0:.3f}'.format(mtd['quartz_transmission_1'].readY(0)[0]))

    # Empty container
    PolDiffILLReduction(
        Run='396917',
        OutputWorkspace='container_ws',
        ProcessAs='Container'
    )

    # Absorber
    PolDiffILLReduction(
        Run='396928',
        OutputWorkspace='absorber_ws',
        ProcessAs='Absorber'
    )

    # Polarisation correction
    PolDiffILLReduction(
        Run='396939',
        OutputWorkspace='pol_corrections',
        AbsorberInputWorkspace='absorber_ws',
        ContainerInputWorkspace='container_ws',
        TransmissionInputWorkspace='quartz_transmission_1',
        OutputTreatment='Average',
        ProcessAs='Quartz'
    )

    # Vanadium transmission
    PolDiffILLReduction(
        Run='396990',
        OutputWorkspace='vanadium_transmission',
        AbsorberTransmissionInputWorkspace='cadmium_ws_1',
        BeamInputWorkspace='beam_ws_1',
        ProcessAs='Transmission'
    )
    print('Vanadium transmission is {0:.3f}'.format(mtd['vanadium_transmission_1'].readY(0)[0]))

    # Vanadium reduction
    PolDiffILLReduction(
        Run='396993',
        OutputWorkspace='vanadium_ws',
        AbsorberInputWorkspace='absorber_ws',
        ContainerInputWorkspace='container_ws',
        TransmissionInputWorkspace='vanadium_transmission_1',
        QuartzInputWorkspace='pol_corrections',
        OutputTreatment='Sum',
        SampleGeometry='None',
        SampleAndEnvironmentProperties=vanadium_dictionary,
        ProcessAs='Vanadium'
    )

    # Sample transmission
    PolDiffILLReduction(
       Run='396986',
       OutputWorkspace='sample_transmission',
       AbsorberTransmissionInputWorkspace='cadmium_ws_1',
       BeamInputWorkspace='beam_ws_1',
       ProcessAs='Transmission'
    )
    print('Sample transmission is {0:.3f}'.format(mtd['sample_transmission_1'].readY(0)[0]))

    # Sample reduction
    PolDiffILLReduction(
        Run='397004',
        OutputWorkspace='sample_ws',
        AbsorberInputWorkspace='absorber_ws',
        ContainerInputWorkspace='container_ws',
        TransmissionInputWorkspace='sample_transmission_1',
        QuartzInputWorkspace='pol_corrections',
        OutputTreatment='Individual',
        SampleGeometry='None',
        SampleAndEnvironmentProperties=sample_dictionary,
	ProcessAs='Sample'
    )

Output:

.. testoutput:: ExPolDiffILLReduction

    Cadmium absorber transmission is 0.011
    Quartz transmission is 0.700
    Vanadium transmission is 0.886
    Sample transmission is 0.962

.. testcleanup:: ExPolDiffILLReduction

    mtd.clear()

.. categories::

.. sourcelink::
