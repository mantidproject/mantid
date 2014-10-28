.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Following A.J.Schultz's anvred, the weight factors should be:

:math:`sin^2(theta) / (lambda^4 * spec * eff * trans)`

where

-  theta = scattering_angle/2
-  lamda = wavelength (in angstroms?)
-  spec = incident spectrum correction
-  eff = pixel efficiency
-  trans = absorption correction

The quantity: :math:`sin^2(theta) / eff` depends only on the pixel and can be
pre-calculated for each pixel. It could be saved in array pix_weight[].

For now, pix_weight[] is calculated by the method ``BuildPixWeights()``
and just holds the :math:`sin^2(theta)` values. The wavelength dependent portion
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
pixel, Which is a fairly expensive calulation when done for each event.

Also see :ref:`algm-LorentzCorrection`

Usage
-----

**Example:**

.. testcode:: AnvredCorrection

    ws = CreateSampleWorkspace("Event",XMin=5000)
    wsOut = AnvredCorrection(ws,LinearScatteringCoef=1.302,
        LinearAbsorptionCoef=1.686,Radius=0.170,PowerLambda=3)

.. categories::

