.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Calculates the theoretical scattering & transmission for a given sample over a
given wavelength range.

The sample chemical formula is input for the :ref:`SetSampleMaterial
<algm-SetSampleMaterial>` algorithm to calculate the cross-sections. The sample
number density & thickness is input to then calculate the percentage scattering
& transmission.

Usage
-----

**Example - Running CalculateSampleTransmission.**

.. testcode:: ExCalculateSampleTransmissionSimple

    ws = CalculateSampleTransmission(WavelengthRange='2.0, 0.1, 10.0', ChemicalFormula='H2-O')

    print 'Transmission: %f, %f, %f ...' % tuple(ws.readY(0)[:3])
    print 'Scattering: %f, %f, %f ...' % tuple(ws.readY(1)[:3])


Output:

.. testoutput:: ExCalculateSampleTransmissionSimple

    Transmission: 0.568102, 0.567976, 0.567851 ...
    Scattering: 0.429309, 0.429309, 0.429309 ...


**Example - Running CalculateSampleTransmission with a specified number density and thickness.**

.. testcode:: ExCalculateSampleTransmissionParams

    ws = CalculateSampleTransmission(WavelengthRange='2.0, 0.1, 10.0', ChemicalFormula='H2-O',
                                    NumberDensity=0.2, Thickness=0.58)

    print 'Transmission: %f, %f, %f ...' % tuple(ws.readY(0)[:3])
    print 'Scattering: %f, %f, %f ...' % tuple(ws.readY(1)[:3])

Output:

.. testoutput:: ExCalculateSampleTransmissionParams

    Transmission: 0.001417, 0.001413, 0.001410 ...
    Scattering: 0.998506, 0.998506, 0.998506 ...

.. categories::
