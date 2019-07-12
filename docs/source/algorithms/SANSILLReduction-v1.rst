.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm performs SANS reduction for the instruments at the ILL.
With each call, this algorithm processes one type of data which is a part of the whole experiment.
The logic is resolved by the property **ProcessAs**, which governs the reduction steps based on the requested type.
It can be one of the 6: absorber, beam, transmission, container, reference and sample.
The full data treatment of the complete experiment should be build up as a chain with multiple calls of this algorithm over various types of acquisitions.
The sequence should be logical, typically as enumerated above, since the later processes need the outputs of earlier processes as input.
The common mandatory input is a run file (numor), or a list of them, in which case they will be summed at raw level, so right after loading.
The other common input is the normalisation type (time or monitor) that must be the same for all the runs in full reduction.
The common mandatory output is a workspace, but up to which step it is processed, depends on **ProcessAs**.

ProcessAs
---------
Different input properties can be specified depending on the value of **ProcessAs**, as summarized in the table:

+--------------+-------------------------------+------------------------+
| ProcessAs    | Input Workspace Properties    | Other Input Properties |
+==============+===============================+========================+
| Absorber     |                               |                        |
+--------------+-------------------------------+------------------------+
| Beam         | * AbsorberInputWorkspace      | * BeamRadius           |
|              |                               | * BeamFinderMethod     |
+--------------+-------------------------------+------------------------+
| Transmission | * AbsorberInputWorkspace      | * BeamRadius           |
|              | * **BeamInputWorkspace**      |                        |
+--------------+-------------------------------+------------------------+
| Container    | * AbsorberInputWorkspace      |                        |
|              | * BeamInputWorkspace          |                        |
|              | * TransmissionInputWorkspace  |                        |
+--------------+-------------------------------+------------------------+
| Reference    | * AbsorberInputWorkspace      | * SampleThickness      |
|              | * BeamInputWorkspace          |                        |
|              | * TransmissionInputWorkspace  |                        |
|              | * ContainerInputWorkspace     |                        |
|              | * MaskedInputWorkspace        |                        |
+--------------+-------------------------------+------------------------+
| Sample       | * AbsorberInputWorkspace      | * SampleThickness      |
|              | * BeamInputWorkspace          |                        |
|              | * TransmissionInputWorkspace  |                        |
|              | * ContainerInputWorkspace     |                        |
|              | * MaskedInputWorkspace        |                        |
|              | * ReferenceInputWorkspace     |                        |
|              | * SensitivityInputWorkspace   |                        |
+--------------+-------------------------------+------------------------+

All the input workspace properties above are optional.
For example, if processing as sample, if a container input is specified, subtraction will be performed, if not, the step will be skipped.
The only exception is when processing as transmission, when beam input workspace is mandatory.
When processing as reference there is an additional optional output workspace for sensitivity.

In the flowcharts below the yellow ovals represent the inputs, the grey parallelograms are the outputs for each process type.

Absorber
########

.. diagram:: ILLSANS-v1_absorber_wkflw.dot

Beam
####

.. diagram:: ILLSANS-v1_beam_wkflw.dot

Transmission
############

.. diagram:: ILLSANS-v1_transmission_wkflw.dot

Container
#########

.. diagram:: ILLSANS-v1_container_wkflw.dot

Reference
#########

.. diagram:: ILLSANS-v1_reference_wkflw.dot

Sample
######

.. diagram:: ILLSANS-v1_sample_wkflw.dot

Full Treatment
##############

Full treatment is built by stacking up unary reductions with corresponding **ProcessAs**. The diagram below illustrates the flow of processing.
Letters denote absorber (A), beam (B), transmission (T), container (C), reference (R), sample (S).
A is processed first, and passed to all the other processes.
B takes only A as input, and the output of B is needed by the rest.
T takes A and B as inputs, and the calculated transmission is used by C, R and S respectively.
C takes A, B and the container T as input, and the output is supplied to R and S respectively.
R takes A, B, its T, and C, and the output is seeded to S.
S takes A, B, its T and C, as well as R as inputs.
The output of S is passed to final step integration.

.. diagram:: ILLSANS-v1_all_wkflw.dot

This example below performs a complete reduction for D11 data.

.. note::

  For transmission calculation, the beam run and the transmission run have to be recorded at the same instrument configuration.
  For beam flux normalisation and beam center movement, the beam run and the sample run have to be recorded at the same configuration.
  For container subtraction, the container and the sample run have to be recorded at the same configuration.
  Otherwise a warning is logged, but the execution does not stop.

.. include:: ../usagedata-note.txt

**Example - full treatment of a sample**

.. testsetup:: ExSANSILLReduction

    config['default.facility'] = 'ILL'
    config.appendDataSearchSubDir('ILL/D11/')

.. testcode:: ExSANSILLReduction

    # Load the mask (or load the sample, and mask graphically using the Instrument View)
    LoadNexusProcessed(Filename='D11_mask.nxs', OutputWorkspace='mask')

    # Process the dark current Cd/B4C for water
    SANSILLReduction(Run='010455.nxs', ProcessAs='Absorber', OutputWorkspace='Cdw')

    # Process the empty beam for water
    SANSILLReduction(Run='010414.nxs', ProcessAs='Beam', AbsorberInputWorkspace='Cdw', OutputWorkspace='Dbw')

    # Water container transmission
    SANSILLReduction(Run='010446.nxs', ProcessAs='Transmission',
                     AbsorberInputWorkspace='Cdw', BeamInputWorkspace='Dbw',
                     OutputWorkspace='wc_tr')
    print('Water container transmission is {0:.3f}'.format(mtd['wc_tr'].readY(0)[0]))

    # Water container
    SANSILLReduction(Run='010454.nxs', ProcessAs='Container',
                     AbsorberInputWorkspace='Cdw', BeamInputWorkspace='Dbw',
                     TransmissionInputWorkspace='wc_tr', OutputWorkspace='wc')

    # Water transmission
    SANSILLReduction(Run='010445.nxs', ProcessAs='Transmission',
                     AbsorberInputWorkspace='Cdw', BeamInputWorkspace='Dbw', OutputWorkspace='w_tr')
    print('Water transmission is {0:.3f}'.format(mtd['w_tr'].readY(0)[0]))

    # Water
    SANSILLReduction(Run='010453.nxs', ProcessAs='Reference', MaskedInputWorkspace='mask',
                     AbsorberInputWorkspace='Cdw', ContainerInputWorkspace='wc',
                     BeamInputWorkspace='Dbw', TransmissionInputWorkspace='wc_tr',
                     SensitivityOutputWorkspace='sens', OutputWorkspace='water')

    # Process the dark current Cd/B4C for sample
    SANSILLReduction(Run='010462.nxs', ProcessAs='Absorber', OutputWorkspace='Cd')

    # Process the empty beam for sample
    SANSILLReduction(Run='010413.nxs', ProcessAs='Beam', AbsorberInputWorkspace='Cd', OutputWorkspace='Db')

    # Sample container transmission
    SANSILLReduction(Run='010444.nxs', ProcessAs='Transmission',
                     AbsorberInputWorkspace='Cd', BeamInputWorkspace='Dbw', OutputWorkspace='sc_tr')
    print('Sample container transmission is {0:.3f}'.format(mtd['sc_tr'].readY(0)[0]))

    # Sample container
    SANSILLReduction(Run='010460.nxs', ProcessAs='Container',
                     AbsorberInputWorkspace='Cd', BeamInputWorkspace='Db',
                     TransmissionInputWorkspace='sc_tr', OutputWorkspace='sc')

    # Sample transmission
    SANSILLReduction(Run='010585.nxs', ProcessAs='Transmission',
                     AbsorberInputWorkspace='Cd', BeamInputWorkspace='Dbw', OutputWorkspace='s_tr')
    print('Sample transmission is {0:.3f}'.format(mtd['s_tr'].readY(0)[0]))

    # Sample
    SANSILLReduction(Run='010569.nxs', ProcessAs='Sample', MaskedInputWorkspace='mask',
                     AbsorberInputWorkspace='Cd', ContainerInputWorkspace='sc',
                     BeamInputWorkspace='Db', SensitivityInputWorkspace='sens',
                     TransmissionInputWorkspace='s_tr', OutputWorkspace='sample_flux')

    # Convert to I(Q)
    SANSILLIntegration(InputWorkspace='sample_flux', OutputWorkspace='iq')

Output:

.. testoutput:: ExSANSILLReduction

    Water container transmission is 0.946
    Water transmission is 0.501
    Sample container transmission is 0.667
    Sample transmission is 0.642

.. testcleanup:: ExSANSILLReduction

    mtd.clear()

.. categories::

.. sourcelink::
