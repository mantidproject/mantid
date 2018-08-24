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
The common mandatory output is a workspace, but up to which step it is processed, depends on **ProcessAs**.

ProcessAs
---------
In the flowcharts below the yellow ovals represent the inputs, the grey parallelograms are the outputs for each process type.

Absorber
~~~~~~~~

.. diagram:: ILLSANS-v1_absorber_wkflw.dot

Beam
~~~~

.. diagram:: ILLSANS-v1_beam_wkflw.dot

Transmission
~~~~~~~~~~~~

.. diagram:: ILLSANS-v1_transmission_wkflw.dot

Container
~~~~~~~~~

.. diagram:: ILLSANS-v1_container_wkflw.dot

Reference
~~~~~~~~~

.. diagram:: ILLSANS-v1_reference_wkflw.dot

Sample
~~~~~~

.. diagram:: ILLSANS-v1_sample_wkflw.dot

Full Treatment
~~~~~~~~~~~~~~

This example performs the complete reduction. :ref:`Q1DWeighted <algm-Q1DWeighted>` can be called over the final result to produce the I of Q.

**Example - full treatment of a sample**

.. testsetup:: ExILLSANSReduction

    config['default.facility'] = 'ILL'

.. testcode:: ExILLSANSReduction

  # Process the dark current Cd/B4C for water
  ILLSANSReduction(Run='010455.nxs', ProcessAs='Absorber', OutputWorkspace='Cdw')

  # Process the empty beam for water
  ILLSANSReduction(Run='010414.nxs', ProcessAs='Beam', AbsorberInputWorkspace='Cdw', OutputWorkspace='Dbw')

  # Water container transmission
  ILLSANSReduction(Run='010446.nxs', ProcessAs='Transmission',
                   AbsorberInputWorkspace='Cdw', BeamInputWorkspace='Dbw',
                   OutputWorkspace='wc_tr')
  print('Water container transmission is {0:.3f}'.format(mtd['wc_tr'].readY(0)[0]))

  # Water container
  ILLSANSReduction(Run='010454.nxs', ProcessAs='Container',
                   AbsorberInputWorkspace='Cdw', BeamInputWorkspace='Dbw',
                   TransmissionInputWorkspace='wc_tr', OutputWorkspace='wc')

  # Water transmission
  ILLSANSReduction(Run='010445.nxs', ProcessAs='Transmission',
                   AbsorberInputWorkspace='Cdw', BeamInputWorkspace='Dbw', OutputWorkspace='w_tr')
  print('Water transmission is {0:.3f}'.format(mtd['w_tr'].readY(0)[0]))

  # Water
  ILLSANSReduction(Run='010453.nxs', ProcessAs='Reference',
                   AbsorberInputWorkspace='Cdw', ContainerInputWorkspace='wc',
                   BeamInputWorkspace='Dbw', TransmissionInputWorkspace='wc_tr',
                   SensitivityOutputWorkspace='sens', OutputWorkspace='water')

  # Process the dark current Cd/B4C for sample
  ILLSANSReduction(Run='010462.nxs', ProcessAs='Absorber', OutputWorkspace='Cd')

  # Process the empty beam for sample
  ILLSANSReduction(Run='010413.nxs', ProcessAs='Beam', AbsorberInputWorkspace='Cd', OutputWorkspace='Db')

  # Sample container transmission
  ILLSANSReduction(Run='010444.nxs', ProcessAs='Transmission',
                   AbsorberInputWorkspace='Cd', BeamInputWorkspace='Dbw', OutputWorkspace='sc_tr')
  print('Sample container transmission is {0:.3f}'.format(mtd['sc_tr'].readY(0)[0]))

  # Sample container
  ILLSANSReduction(Run='010460.nxs', ProcessAs='Container',
                   AbsorberInputWorkspace='Cd', BeamInputWorkspace='Db',
                   TransmissionInputWorkspace='sc_tr', OutputWorkspace='sc')

  # Sample transmission
  ILLSANSReduction(Run='010585.nxs', ProcessAs='Transmission',
                   AbsorberInputWorkspace='Cd', BeamInputWorkspace='Dbw', OutputWorkspace='s_tr')
  print('Sample transmission is {0:.3f}'.format(mtd['s_tr'].readY(0)[0]))

  # Sample
  ILLSANSReduction(Run='010569.nxs', ProcessAs='Sample',
                   AbsorberInputWorkspace='Cd', ContainerInputWorkspace='sc',
                   BeamInputWorkspace='Db', SensitivityInputWorkspace='sens',
                   TransmissionInputWorkspace='s_tr', OutputWorkspace='sample_flux')

Output:

.. testoutput:: ExILLSANSReduction

    Water container transmission is 0.945
    Water transmission is 0.501
    Sample container transmission is 0.665
    Sample transmission is 0.640

.. testcleanup:: ExILLSANSReduction

    mtd.clear()

.. note::

  For transmission calculation, the beam run and the transmission run have to be recorded at the same instrument configuration.
  For beam flux normalisation and beam center movement, the beam run and the sample run have to be recorded at the same configuration.
  For container subtraction, the container and the sample run have to be recorded at the same configuration.
  Otherwise a warning is logged, but the execution does not stop.

.. categories::

.. sourcelink::
