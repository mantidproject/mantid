.. _Unit Factory:

Unit_Factory
============

What is it?
-----------

The Unit Factory is a :ref:`Dynamic Factory <Dynamic Factory>` that creates
and hands out instances of Mantid Unit objects.

Available units
~~~~~~~~~~~~~~~

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

Where :math:`L_1` and :math:`L_2` are sample to the source and sample to
detector distances respectively, :math:`L_{tot} = L_1+L_2` and
:math:`E_i` is the energy of neutrons leaving the source. :math:`\theta`
here is the Bragg scattering angle (e.g. half of the
:math:`\theta`-angle used in spherical coordinate system directed along
Mantid z-axis)

**Note on Wavelength**: If the emode property in
`ConvertUnits <http://docs.mantidproject.org/nightly/algorithms/ConvertUnits.html>`__
is specified as inelastic Direct/Indirect (inelastic) then the
conversion to wavelength will take into account the fixed initial/final
energy respectively. Units conversion into elastic momentum transfer
(MomentumTransfer) will throw in elastic mode (emode=0) on inelastic
workspace (when energy transfer is specified along x-axis)

Adding new units
~~~~~~~~~~~~~~~~

Writing and adding a new unit is relatively straightforward.
Instructions will appear here in due course. In the meantime if a unit
that you require is missing, then please contact the development team
and we will add it to the default Mantid library.



.. categories:: Concepts