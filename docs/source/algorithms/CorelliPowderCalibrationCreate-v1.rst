.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::


Description
-----------

Standard powder samples with Bragg scattering rings of accurately known lattice spacings can be used to adjust the
components of an instrument such that the observed Bragg scattering yield lattice spacings as close as
possible to the standard reference.

This algorithm adjusts the position of the moderator as well as the position and orientation of each bank
of detector pixels. The goal of these adjustments is to produce neutron paths and scattering angles that lead
to optimal comparison between observed and reference Bragg scattering.

The execution workflow is as follows:

.. diagram:: CorelliPowderCalibrationCreate-v1_wkflw.dot


Usage
-----

* Example -  adjust banks 42 and 87

.. testcode::

    from mantid.simpleapi import *
    LoadNexus(Filename='/tmp/CORELLI_124036_banks42_87.nxs', OutputWorkspace='LaB6')
    CorelliPowderCalibrationCreate(InputWorkspace='LaB6',
                                   OutputWorkspacesPrefix='LaB6_',
                                   TubeDatabaseDir='/tmp',
                                   TofBinning=[3000,-0.001,16660],
                                   PeakFunction='Gaussian',
                                   PeakPositions=[1.3143, 1.3854,1.6967, 1.8587, 2.0781, 2.3995, 2.9388, 4.1561],
                                   SourceMaxTranslation=0.1,
                                   ComponentList='bank42/sixteenpack,bank87/sixteenpack',
                                   ComponentMaxTranslation=0.02,
                                   ComponentMaxRotation=3.0)

A set of output workspaces are created, including the adjustment diagnostics

.. image:: ../images/CorelliPowderCalibrationCreate_1.png
    :align: center
    :width: 635
    :alt: original layout of CORELLI instrument

A description of the output workspaces follows:

Workspace ``LaB6_adjustments`` is the main result, a table containing adjustments for different instrument components.
In our example, we have adjusted the moderator and banks 42 and 87.

+--------------------+------------+-----------+-----------+------------------+------------------+------------------+---------------+
| Component          | Xposition  | Yposition | Zposition | XdirectionCosine | YdirectionCosine | ZdirectionCosine | RotationAngle |
+====================+============+===========+===========+==================+==================+==================+===============+
| moderator          | 0.0        | 0.0       | -19.9944  |      0.0         |      0.0         |      0.0         |      0.0      |
+--------------------+------------+-----------+-----------+------------------+------------------+------------------+---------------+
| bank42/sixteenpack | 2.5941     | 0.0625    | 0.0870    | 0.0009           | -0.9997          | 0.0210           |       92.3187 |
+--------------------+------------+-----------+-----------+------------------+------------------+------------------+---------------+

- ``Xposition``, ``Yposition``, ``Zposition``: coordinates for the center of ``Component`` in the lab's frame of reference. Units are Angstroms.
- ``XdirectionCosine``, ``YdirectionCosine``, ``ZdirectionCosine``: direction cosines (in the) lab's frame of references defining a rotation axis to set the orientation of ``Component``.
- ``RotationAngle``: rotate this many degrees around the previous rotation axis to set the orientation of ``Component``.


.. categories::

.. sourcelink::