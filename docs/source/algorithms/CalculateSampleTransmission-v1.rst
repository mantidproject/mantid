.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Calculates the theoretical scattering & transmission for a given sample over a
given wavelength range.

The sample chemical formula is input for the :ref:`SetSampleMaterial
<algm-SetSampleMaterial>` algorithm to calculate the cross-sections. The sample
mass density/number density & thickness is input to then calculate the percentage
scattering & transmission.

A flat plate sample which is perpendicular to the beam is assumed.

Usage
-----

**Example - Running CalculateSampleTransmission.**

.. testcode:: ExCalculateSampleTransmissionSimple

    ws = CalculateSampleTransmission(WavelengthRange='2.0, 0.1, 10.0',
                                     ChemicalFormula='H2-O')

    print('Transmission: {:.6f}, {:.6f}, {:.6f} ...'.format(*ws.readY(0)[:3]))
    print('Scattering: {:.6f}, {:.6f}, {:.6f} ...'.format(*ws.readY(1)[:3]))


Output:

.. testoutput:: ExCalculateSampleTransmissionSimple

    Transmission: 0.945063, 0.945051, 0.945040 ...
    Scattering: 0.054697, 0.054697, 0.054697 ...


**Example - Running CalculateSampleTransmission with a specified number density and thickness.**

.. testcode:: ExCalculateSampleTransmissionParams

    ws = CalculateSampleTransmission(WavelengthRange='2.0, 0.1, 10.0',
                                     ChemicalFormula='H2-O',
                                     DensityType='Number Density',
                                     Density=0.2,
                                     Thickness=0.58)

    print('Transmission: {:.6f}, {:.6f}, {:.6f} ...'.format(*ws.readY(0)[:3]))
    print('Scattering: {:.6f}, {:.6f}, {:.6f} ...'.format(*ws.readY(1)[:3]))

Output:

.. testoutput:: ExCalculateSampleTransmissionParams

    Transmission: 0.001450, 0.001448, 0.001446 ...
    Scattering: 0.998506, 0.998506, 0.998506 ...

.. categories::

.. sourcelink::
