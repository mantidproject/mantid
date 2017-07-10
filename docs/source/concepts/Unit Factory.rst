.. _Unit Factory:

=====
Units
=====

.. contents::
  :local:

What are units?
---------------

Units are a set of small classes in Mantid that define a unit of measure, and the conversions between various units.

The Unit Factory is a :ref:`Dynamic Factory <Dynamic Factory>` that creates
and hands out instances of Mantid Unit objects.

Available units
---------------

The following units are available in the default Mantid distribution.

+-------------------------------------------+---------------------------------+-----------------------------+------------------------------------------------------------------------------------------------------------------+
| Name                                      | ID (as known by Unit Factory)   | Unit                        | Relevant equation                                                                                                |
+===========================================+=================================+=============================+==================================================================================================================+
| Time of flight                            | TOF                             | :math:`\mu s`               | TOF                                                                                                              |
+-------------------------------------------+---------------------------------+-----------------------------+------------------------------------------------------------------------------------------------------------------+
| Wavelength                                | Wavelength                      | :math:`\mathrm{\AA}`        | :math:`\lambda = \frac{h}{p} = \frac{h \times \mathrm{tof}}{m_N \times L_{tot}}` (see below)                     |
+-------------------------------------------+---------------------------------+-----------------------------+------------------------------------------------------------------------------------------------------------------+
| Energy                                    | Energy                          | :math:`meV`                 | :math:`E = \frac{1}{2} mv^2 = \frac{m_N}{2} \left ( \frac{L_{tot}}{\mathrm{tof}} \right )^2`                     |
+-------------------------------------------+---------------------------------+-----------------------------+------------------------------------------------------------------------------------------------------------------+
| Energy in wavenumber                      | Energy\_inWavenumber            | :math:`cm^{-1}`             | :math:`8.06554465 \times E`                                                                                      |
+-------------------------------------------+---------------------------------+-----------------------------+------------------------------------------------------------------------------------------------------------------+
| Momentum (k)                              | Momentum                        | :math:`\mathrm{\AA}^{-1}`   | :math:`k = \frac{2 \pi }{\lambda}=\frac{2 \pi \times m_N \times L_{tot}}{h \times \mathrm{tof}}`                 |
+-------------------------------------------+---------------------------------+-----------------------------+------------------------------------------------------------------------------------------------------------------+
| d-spacing                                 | dSpacing                        | :math:`\mathrm{\AA}`        | :math:`d = \frac{n \, \lambda}{2 \, sin \, \theta}`                                                              |
+-------------------------------------------+---------------------------------+-----------------------------+------------------------------------------------------------------------------------------------------------------+
| Momentum transfer (Q)                     | MomentumTransfer                | :math:`\mathrm{\AA}^{-1}`   | :math:`Q = 2 \, k \, sin \, \theta = \frac{4 \pi sin \theta}{\lambda}`                                           |
+-------------------------------------------+---------------------------------+-----------------------------+------------------------------------------------------------------------------------------------------------------+
| Momentum transfer squared (:math:`Q^2`)   | QSquared                        | :math:`\mathrm{\AA}^{-2}`   | :math:`Q^2 \frac{}{}`                                                                                            |
+-------------------------------------------+---------------------------------+-----------------------------+------------------------------------------------------------------------------------------------------------------+
| Energy transfer                           | DeltaE                          | :math:`meV`                 | :math:`\Delta E = E_{i}-\frac{1}{2}m_N \left ( \frac{L_2}{\mathrm{tof}-L_1\sqrt{\frac{m_N}{2E_i}}} \right )^2`   |
+-------------------------------------------+---------------------------------+-----------------------------+------------------------------------------------------------------------------------------------------------------+
| Energy transfer in wavenumber             | DeltaE\_inWavenumber            | :math:`cm^{-1}`             | :math:`8.06554465 \times \Delta E`                                                                               |
+-------------------------------------------+---------------------------------+-----------------------------+------------------------------------------------------------------------------------------------------------------+
| Spin Echo Length                          | SpinEchoLength                  | :math:`nm`                  | | :math:`constant \times \lambda^2`                                                                              |
|                                           |                                 |                             | |  The constant is supplied in eFixed                                                                            |
+-------------------------------------------+---------------------------------+-----------------------------+------------------------------------------------------------------------------------------------------------------+
| Spin Echo Time                            | SpinEchoTime                    | :math:`ns`                  | | :math:`constant \times \lambda^3`                                                                              |
|                                           |                                 |                             | |  The constant is supplied in eFixed                                                                            |
+-------------------------------------------+---------------------------------+-----------------------------+------------------------------------------------------------------------------------------------------------------+
| d-spacingPerpendicular                    | dSpacingPerpendicular           | :math:`\mathrm{\AA}`        | :math:`d_{\perp} = \sqrt{\lambda^2 - 2\log\cos\theta}`                                                           |
+-------------------------------------------+---------------------------------+-----------------------------+------------------------------------------------------------------------------------------------------------------+


Where :math:`L_1` and :math:`L_2` are sample to the source and sample to
detector distances respectively, :math:`L_{tot} = L_1+L_2` and
:math:`E_i` is the energy of neutrons leaving the source. :math:`\theta`
here is the Bragg scattering angle (e.g. half of the
:math:`\theta`-angle used in spherical coordinate system directed along
Mantid z-axis)

**Note on Wavelength**: If the emode property in
:ref: `ConvertUnits <algm-ConvertUnits>`
is specified as inelastic Direct/Indirect (inelastic) then the
conversion to wavelength will take into account the fixed initial/final
energy respectively. Units conversion into elastic momentum transfer
(MomentumTransfer) will throw in elastic mode (emode=0) on inelastic
workspace (when energy transfer is specified along x-axis)

**d-spacingPerpendicular** is a unit invented in `J. Appl. Cryst. (2015) 48, pp. 1627--1636 <https://doi.org/10.1107/S1600576715016520>`_ for 2D Rietveld refinement
of angular and wavelength-dispersive neutron time-of-flight powder diffraction data. Together with the d-Spacing :math:`d`,
d-SpacingPerpendicular :math:`d_{\perp}` forms a new orthogonal coordinate system.


Working with Units in Python
----------------------------

Accessing units on workspaces
#############################

Units on MatrixWorkspaces are accessed via the Axis.

.. testcode:: UnitWorkspaceAxes


  ws = CreateSampleWorkspace()
  for i in range(ws.axes()):
      axis = ws.getAxis(i)
      print "Axis {0} is a {1}{2}{3}".format(i,
                                             "Spectrum Axis" if axis.isSpectra() else "",
                                             "Text Axis" if axis.isText() else "",
                                             "Numeric Axis" if axis.isNumeric() else "")

      unit = axis.getUnit()
      print "\t caption:{0}".format(unit.caption())
      print "\t symbol:{0}".format(unit.symbol())

Output:

.. testoutput:: UnitWorkspaceAxes
  :options: +NORMALIZE_WHITESPACE

  Axis 0 is a Numeric Axis
     caption:Time-of-flight
     symbol:microsecond
  Axis 1 is a Spectrum Axis
     caption:Spectrum
     symbol:


Setting the axisLabel to a Label of your choice
###############################################


.. testcode:: UnitAxesLabel

  ws = CreateSampleWorkspace()
  axis = ws.getAxis(1)
  # Create a new axis
  axis.setUnit("Label").setLabel('Temperature', 'K')

  unit = axis.getUnit()
  print "New caption:{0}".format(unit.caption())
  print "New symbol:{0}".format(unit.symbol())

Output:

.. testoutput:: UnitAxesLabel
  :options: +ELLIPSIS,+NORMALIZE_WHITESPACE

  New caption:Temperature
  New symbol:K


Adding new units
----------------

Writing and adding a new unit is relatively straightforward.
Instructions will appear here in due course. In the meantime if a unit
that you require is missing, then please contact the development team
and we will add it to the default Mantid library.



.. categories:: Concepts
