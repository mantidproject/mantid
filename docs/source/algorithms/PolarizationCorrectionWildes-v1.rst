
.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------


This algorithm corrects for non-ideal instrument component efficiencies in a polarization analysis experiment by following the procedure and conventions introduced by Wildes [#WILDES]_. In the full polarization analysis case it solves the corrected count rates :math:`\Sigma^{++}`, :math:`\Sigma^{+-}`, :math:`\Sigma^{-+}` and :math:`\Sigma^{--}` from the equation

.. math::
   \begin{bmatrix}
   \Sigma^{++} \\
   \Sigma^{+-} \\
   \Sigma^{-+} \\
   \Sigma^{--}
   \end{bmatrix}
   = \bm{M}
   \begin{bmatrix}
   I^{00} \\
   I^{01} \\
   I^{10} \\
   I^{11}
   \end{bmatrix},

where :math:`I^{jk}` are the experimental count rates for flipper configuration :math:`jk` and :math:`\bm{M}` is the four-by-four correction matrix as defined by equations (4) in [#WILDES]_.

Flipper configurations
######################

*InputWorkspaces* is a list containing one to four workspace names (X unit: wavelength) corresponding to the instrument configurations given as *Flippers*. Supported configurations are:

:literal:`'00, 01, 10, 11'`
   Full polarization corrections: both flippers off, analyzer flipper on, polarizer flipper on, both flippers on. Four input workspaces are required. The flipper configuration can be provided in any order and should match the order of the workspaces in the input group.

:literal:`'00, 01, 11'` and :literal:`'00, 10, 11'`
   Polarization corrections with the assumption that the corrected count rates :math:`\Sigma^{+-} = \Sigma^{-+}`. In this case the intensity of the missing flipper configuration (01 or 10) can be solved from the other intensities. The flipper configuration can be provided in any order and should match the order of the workspaces in the input group.

:literal:`'00, 11'`
   Polarization corrections with the assumption that the corrected count rates :math:`\Sigma^{+-} = \Sigma^{-+} = 0`. In this case the intensities of the missing flipper configurations (01 and 10) can be solved from the other intensities. The flipper configuration can be provided in any order and should match the order of the workspaces in the input group.

:literal:`'0, 1'`
   Polarization corrections when no analyzer has been used: polarizer flipper off, polarizer flipper on. The flipper configuration can be provided in any order and should match the order of the workspaces in the input group.

:literal:`'0'`
   Polarization corrections for a direct beam measurement in a reflectometry experiment.

Output
######

The algorithm's output is a group workspace containing the corrected workspaces. The names of each corrected workspace is prefixed by :literal:`_++`, :literal:`_+-`, :literal:`_-+` or :literal:`_--` depending on which :math:`\Sigma^{mn}` they correspond to.

Efficiency factors
##################

The *Efficiencies* input property expects to get a workspace with the following properties:

* Contains four histograms, each labeled by their vertical axis as :literal:`P1`, :literal:`P2`, :literal:`F1`, :literal:`F2`. Other histograms (if present) are ignored.
* The Y values of each histogram should be the corresponding efficiencies as functions of wavelength as defined in [#WILDES]_.
* The wavelength values (X values) should be the same is in the input workspaces.

.. note::
   Users at ILL can load a conforming efficiency workspace from disk by :ref:`algm-LoadILLPolarizationFactors`.

Error propagation
#################

.. note::
   Errors are calculated as per Wildes [#WILDES]_, except for the numerically solved intensity in :literal:`'00, 01, 11'` and :literal:`'00, 10, 11'` flipper configurations in which case the uncertainties of :math:`\Sigma^{+-}` or :math:`\Sigma^{-+}` are set to zero.

Usage
-----

.. include:: ../usagedata-note.txt

**Example - PolarizationEfficiencyCor**

.. testcode:: PolarizationEfficiencyCorExample

   LoadILLReflectometry(
       Filename='ILL/D17/317370.nxs',
       OutputWorkspace='direct_beam',
       XUnit='TimeOfFlight')
   LoadILLReflectometry(
       Filename='ILL/D17/317370.nxs',
       OutputWorkspace='reflected_beam',
       Measurement='ReflectedBeam',
       BraggAngle=0.8,
       XUnit='TimeOfFlight')
   # Sum pixels containing the reflected intensity
   GroupDetectors(
       InputWorkspace='reflected_beam',
       OutputWorkspace='reflected_beam',
       WorkspaceIndexList=[199, 200, 201, 202, 203, 204, 205])
   ConvertUnits(
       InputWorkspace='reflected_beam',
       OutputWorkspace='reflected_beam',
       Target='Wavelength',
       EMode='Elastic')
   # There are some unphysical wavelengths
   CropWorkspace(
       InputWorkspace='reflected_beam',
       OutputWorkspace='reflected_beam',
       XMin=0.)
   # Fake two flipper configurations
   RenameWorkspace(
       InputWorkspace='reflected_beam',
       OutputWorkspace='up'
   )
   CloneWorkspace(
       InputWorkspace='up',
       OutputWorkspace='down'
   )
   Scale(
       InputWorkspace='down',
       OutputWorkspace='down',
       Factor=0.1
   )
   LoadILLPolarizationFactors(
       Filename='ILL/D17/PolarizationFactors.txt',
       OutputWorkspace='efficiencies',
       WavelengthReference='up')
   PolarizationEfficiencyCor(
       InputWorkspaces='up, down',
       OutputWorkspace='corrected',
       Efficiencies='efficiencies',
       Flippers='00, 11')

   orig = mtd['up']
   corr = mtd['corrected_++']
   index = orig.yIndexOfX(15.)
   ratio_up = corr.readY(0)[index] / orig.readY(0)[index]
   print("Ratio of corrected and original 'up' intensity at 15A: {:.4}".format(ratio_up))
   orig = mtd['down']
   corr = mtd['corrected_--']
   index = orig.yIndexOfX(15.)
   ratio_down = corr.readY(0)[index] / orig.readY(0)[index]
   print("Ratio of corrected and original 'down' intensity at 15A: {:.4}".format(ratio_down))

Output:

.. testoutput:: PolarizationEfficiencyCorExample

   Ratio of corrected and original 'up' intensity at 15A: 0.1062
   Ratio of corrected and original 'down' intensity at 15A: 10.38

References
----------

.. [#WILDES] A. R. Wildes, *Neutron News*, **17** 17 (2006)
             `doi: 10.1080/10448630600668738 <https://doi.org/10.1080/10448630600668738>`_

.. categories::

.. sourcelink::
