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

Quick McStas Tutorial
----------------------
This tutorial is just to generate a simple data file with McStas that Mantid is able to load. It is recommendeed to install McStas with conda by following the instructions on the `McStas conda installation page <https://github.com/mccode-dev/McCode/blob/main/INSTALL-McStas/conda/README.md>`_.
Once you clone the McStas repository, build with conda, and activate the McStas environment, you can run the command ``mcgui`` on the command line to open the McStas GUI. On the GUI, select File -> New From Template and you will see a selection for Mantid.
Pick a template from Mantid and you will see a save window pop up. Rename the file if you wish and save it.
If you would like more info on how to make an instrument file for McStas, see `this section in the wiki <https://github.com/mccode-dev/McCode/wiki/McStas-and-Mantid#setup-the-mcstas-instrument-to-create-a-mantid-instrument>`_.
On the McStas GUI, click Run and a window will pop up asking for parameters. The main thing to change here is the output format to NeXus --IDF.
If it does not have --IDF when you generate the data, Mantid will not be able to load it properly. Click start to generate the data. Once the data has been generated, open Mantid and click Load -> File in the workspaces box.
Alternatively, you can use the :ref:`algm-LoadMcstas` algorithm to load the data. Browse to the McStas generated file and load it. It will be called something like mccode.h5.
You should now see the McStas data in the workspace browser.


.. categories:: Concepts
