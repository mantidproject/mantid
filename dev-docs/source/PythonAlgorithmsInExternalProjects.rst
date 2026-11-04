.. _PythonAlgorithmsInExternalProjects:

======================================
Python Algorithms in External Projects
======================================

It can be useful to create mantid algorithms that are kept in an external project.
This is especially the case for processing that is specific to a single instrument or novel technique.
To use algorithms distributed this way, one needs to register them with the :class:`mantid.api.AlgorithmFactory <mantid:mantid.api.AlgorithmFactoryImpl>` and create wrappers so they can be called as python functions.
There are two techniques to accomplish this

Modify Mantid.user.properties
-----------------------------

Inside the user's :ref:`properties file <mantid:Properties File>`, one can set a
:ref:`directory property <mantid:Directory Properties>` to add the location of the additional python algorithms to the ``framework.plugins.directory`` variable.
During initialization of the mantid framework, all files that are within this directory or a subdirectory will be parsed and algorithms will be added to the ``AlgorithmFactory``.
*This is the preferred method.*

Create function wrapper at runtime
----------------------------------

An alternative method is to create the function wrappers within the extension code itself after the mantid framework has been initialized.
The example below is modified from the `shiver project <https://github.com/neutrons/Shiver/blob/23b651e3e57965c32f545a4f8718fc80ead63663/src/shiver/models/makeslices.py#L264-L269>`_.

.. code-block:: python

   from mantid.api import AlgorithmFactory
   from mantid.simpleapi import _create_algorithm_function

   # ALGORITHM IMPLEMENTATION

   AlgorithmFactory.subscribe(MakeSFCorrectedSlices)
   # Puts function in simpleapi globals
   makeslices = MakeSFCorrectedSlices()
   makeslices.initialize()
   _create_algorithm_function("MakeSFCorrectedSlices", 1, makeslices)
   del makeslices, _create_algorithm_function


This creates the function wrapper in the ``mantid.simpleapi`` namespace when the file containing these lines is parsed.
Unlike the method of instructing mantid to parse the files, this requires actively importing the file containing the extra instructions before the algorithm is available.

Integrating other software with Mantid
--------------------------------------

Registering external Python algorithms, as described above, is one of several
ways to use another library or application together with Mantid.
If you are unsure which suits your project, ask on
`GitHub Discussions <https://github.com/mantidproject/mantid/discussions>`__.

Using a library or command line program
#######################################

- If the library can be imported as a Python module, use it directly from
  Python scripts or Python algorithms, see :ref:`mantid:pim_intro` and
  :ref:`mantid:emwp_intro`.
- A command line program can be run from Python with the
  :mod:`python:subprocess` module.
- Mantid can also be extended in C++, see :ref:`WritingAnAlgorithm`.
- If the library would be of general benefit and should be shipped with
  Mantid, discuss it with the team first.

Exchanging data with another application
########################################

- Add a load algorithm that reads the application's output files, as
  :ref:`LoadMcStas <mantid:algm-LoadMcStas>` does for McStas.
- Add a save algorithm that writes files the application can read, as
  :ref:`SaveGSS <mantid:algm-SaveGSS>` does for GSAS.
- Add a graphical interface that runs the application, following the
  :ref:`GuiStandards`.
