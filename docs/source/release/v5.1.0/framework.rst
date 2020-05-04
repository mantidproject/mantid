=================
Framework Changes
=================

.. contents:: Table of Contents
   :local:

.. warning:: **Developers:** Sort changes under appropriate heading
    putting new features at the top of the section, followed by
    improvements, followed by bug fixes.

Concepts
--------

Algorithms
----------

- Add specialization to :ref:`SetUncertainties <algm-SetUncertainties>` for the
   case where InputWorkspace == OutputWorkspace. Where possible, avoid the
   cost of cloning the inputWorkspace.
- Adjusted :ref:`AddPeak <algm-AddPeak>` to only allow peaks from the same instrument as the peaks worksapce to be added to that workspace. 

Data Handling
-------------

- The material definition has been extended to include an optional filename containing a profile of attenuation factor versus wavelength. This new filename has been added as a parameter to these algorithms:

  - :ref:`SetSampleMaterial <algm-SetSampleMaterial>`
  - :ref:`LoadSampleEnvironment <algm-LoadSampleEnvironment>`

- The attenuation profile filename can also be specified in the materials section of the sample environment xml file
- Fixed a long standing bug where log filtering was not being applied after loading a Mantid processed NeXus file.  This now works correctly so
  run status and period filtering will now work as expected, as it did when you first load the file from a raw or NeXus file.
- The sample environment xml file now supports the geometry being supplied in the form of a .3mf format file (so far on the Windows platform only). Previously it only supported .stl files. The .3mf format is a 3D printing format that allows multiple mesh objects to be stored in a single file that can be generated from many popular CAD applications. As part of this change the algorithms :ref:`LoadSampleEnvironment <algm-LoadSampleEnvironment>` and :ref:`SaveSampleEnvironmentAndShape <algm-SaveSampleEnvironmentAndShape>` have been updated to also support the .3mf format


Data Objects
------------

- Added MatrixWorkspace::findY to find the histogram and bin with a given value 

Python
------
- A list of spectrum numbers can be got by calling getSpectrumNumbers on a 
  workspace. For example: spec_nums = ws.getSpectrumNumbers()

:ref:`Release 5.1.0 <v5.1.0>`
