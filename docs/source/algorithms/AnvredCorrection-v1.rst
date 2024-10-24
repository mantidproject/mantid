.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Following A.J.Schultz's anvred, the weight factors should be:

:math:`\text{sin}^2(theta) / (lambda^4 * spec * eff * trans)`

where

-  theta = scattering_angle/2
-  lamda = wavelength (in angstroms?)
-  spec = incident spectrum correction
-  eff = pixel efficiency
-  trans = absorption correction

The quantity: :math:`\text{sin}^2(theta) / eff` depends only on the pixel and can be
pre-calculated for each pixel. It could be saved in array pix_weight[].

For now, pix_weight[] is calculated by the method ``BuildPixWeights()``
and just holds the :math:`\text{sin}^2(theta)` values. The wavelength dependent portion
of the correction is saved in the array lamda_weight[].

The time-of-flight is converted to wave length by multiplying by
tof_to_lamda[id], then (int)STEPS_PER_ANGSTROM \* lamda gives an
index into the table lamda_weight[]. The lamda_weight[] array contains
values like: 1/(lamda^power \* spec(lamda)) which are pre-calculated for
each lamda. These values are saved in the array lamda_weight[]. The
optimal value to use for the power should be determined when a good
incident spectrum has been determined. Currently, power=3 when used with
an incident spectrum and power=2.4 when used without an incident
spectrum.

The pixel efficiency and incident spectrum correction are NOT CURRENTLY
USED. The absorption correction, trans, depends on both lamda and the
pixel, Which is a fairly expensive calculation when done for each event.
The transmission is calculated for a spherical sample using the fits to
the tabulated values of :math:`A^* = 1/\text{transmission}` in [#WEB]_
using the functional form set out in [#DWI]_.

Also see :ref:`algm-LorentzCorrection`

Usage
-----

**Example:**

.. testcode:: AnvredCorrection

    ws = CreateSampleWorkspace("Event",XMin=5000)
    wsOut = AnvredCorrection(ws,LinearScatteringCoef=1.302,
        LinearAbsorptionCoef=1.686,Radius=0.170,PowerLambda=3)

References
----------

.. [#WEB] Weber, K. *Acta Crystallographica Section B*, 25.6 (1969): 1174-1178.
          `doi: 10.1107/S0567740869003682 <https://doi.org/10.1107/S0567740869003682>`_
.. [#DWI] Dwiggins, C. W. *Acta Crystallographica Section A* 31.3 (1975): 395-396.
          `doi: 10.1107/S0567739475000873 <https://doi.org/10.1107/S0567739475000873>`_

.. categories::

.. sourcelink::
