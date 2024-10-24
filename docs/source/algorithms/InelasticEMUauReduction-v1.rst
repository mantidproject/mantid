.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------
The purpose of data reduction is to convert the measured spectra into neutron scattering double differential cross sections.
That is, the scattering probability given a certain momentum and energy exchange between incident neutron and sample.
Together with knowledge of instrument configuration (distances, angles, and backscattering monochromator velocity), observed
times of arrival at the detectors allow assigning momentum and energy transfers to the scattered neutrons, within the
instrumental resolution. Normalisation to the incident neutron velocity spectrum is further required to obtain a probability.

The Reduce routine thus performs the above necessary algebra on the specified run numbers.  At a minimum, the sample runs
are needed. Additional runs for background subtraction and joint detector efficiency / solid angle correction are normally
provided as well, with Reduce applied to each group separately, and the groups combined as shown in the diagram.

Neutron data collection times imply that inelastic scattering data typically consist in fixed-temperature spectra where the
energy transfer is scanned, such that Reduce produces cross section and its estimated error vs energy and momentum transfers.

Workflow
--------

.. diagram:: EMUauInelasticReduction-v1_wkflw.dot

Usage
-----

.. testcode:: EMUauInelasticReductionExample

    test = InelasticEMUauReduction('20985', SpectrumAxis='Q', OutputWorkspace='test', ConfigurationFile='emu_doctest.ini')
    print('Workspaces in group = {}'.format(test.getNumberOfEntries()))
    gp = test.getNames()
    print('First workspace: {}'.format(gp[0]))

.. testoutput::  EMUauInelasticReductionExample

    Workspaces in group = 2
    First workspace: test_Q_1D

References
----------

.. sourcelink::

.. categories::
