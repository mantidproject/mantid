.. _McStas - A neutron ray-trace simulation package:

McStas - A neutron ray-trace simulation package
===============================================

This page gives a brief explanation of the McStas package as well as a more detailed explanation of the 
interoperability between Mantid and McStas.

Introduction to McStas
----------------------
The McStas neutron ray-tracing simulation package is a versatile tool for producing accurate
simulations of neutron scattering instruments at reactors and pulsed spallation sources. McStas is
extensively used for design and optimization of instruments, virtual experiments, and user training.
McStas is an abbreviation for Monte Carlo Simulation of triple axis spectrometers, but allows for
describing and simulating any type of neutron scattering instrument.[1]

For more information see:
- `McStas Homepage <http://www.mcstas.org/>`__
- `McStas Manual <http://www.mcstas.org/documentation/manual/>`__

McStas works by assembling a C file from instrument information which is then compiled and can be 
run to simulate neutron scattering experiments.


McStas and Mantid integration
-----------------------------
When compiled, McStas instrument code is not automatically configured to produce files in 
:ref:`NeXus <NexusFile>` format. Mantid also requires the instrument information it reads to be saved as 
an :ref:`Instrument Definition File <InstrumentDefinitionFile>`. Details of how to get McStas to 
do both of these tasks may be found `here <https://github.com/McStasMcXtrace/McCode/wiki/McStas-and-Mantid>`__.

Once the data from McStas is in the correct format it can be loaded into Mantid using 
:ref:`LoadMcStas <algm-LoadMcStas>`__.


References
----------
	[1] `McStas and Mantid integration, arXiv:1607.02498 [physics.ins-det] <https://arxiv.org/abs/1607.02498>`__

.. categories:: Concepts
