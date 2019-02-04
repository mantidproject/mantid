.. _McStas - A neutron ray-trace simulation package:

McStas - A neutron ray-trace simulation package
===============================================
Introduction to McStas
----------------------
The McStas neutron ray-tracing simulation package is a versatile tool for producing accurate
simulations of neutron scattering instruments at reactors and pulsed spallation sources. McStas is
extensively used for design and optimization of instruments, virtual experiments, and user training.
McStas is an abbreviation for Monte Carlo Simulation of triple axis spectrometers, but allows for
describing and simulating any type of neutron scattering instrument.

For more information see:

- `McStas Homepage <http://www.mcstas.org/>`_
- `McStas Manual <http://www.mcstas.org/documentation/manual/>`_
- `McStas and Mantid integration <https://arxiv.org/abs/1607.02498>`_

McStas and Mantid integration
-----------------------------
McStas works by assembling a C file from instrument information which is then compiled and can be
run to simulate neutron scattering experiments.

When compiled, McStas instrument code is not automatically configured to produce files in
:ref:`NeXus <Nexus file>` format. Mantid also requires the instrument information it reads to be saved as
an :ref:`Instrument Definition File <InstrumentDefinitionFile>`. Instructions on how to do this can be found on the `McStas and Mantid wiki page <https://github.com/McStasMcXtrace/McCode/wiki/McStas-and-Mantid>`_ in the McStas repository.

Once the NeXus data has been generated it can be loaded into Mantid using :ref:`algm-LoadMcstas`.

.. categories:: Concepts
