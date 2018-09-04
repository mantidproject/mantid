.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm calculates self shielding correction factors for the input workspace. It is part of :ref:`ILL's direct geometry reduction suite <DirectILL>`. *InputWorkspace* should have a sample defined using :ref:`SetSample <algm-SetSample>`. Beam profile can be optionally set using :ref:`SetBeam <algm-SetBeam>`. The algorithm uses :ref:`MonteCarloAbsorption <algm-MonteCarloAbsorption>` as its backend.

To speed up the simulation, the sparse instrument option of :ref:`MonteCarloAbsorption <algm-MonteCarloAbsorption>` is used by default. The number of detectors to simulate can be given by *SparseInstrumentRows* and *SparseInstrumentColumns*.

By default the correction factors are calculated for each bin. By specifying *NumberOfSimulatedWavelengths*, one can restrict the number of points at which the calculation is done thus reducing the execution time. In this case CSplines are used to interpolate the correction factor over all bins.

The correction factor contained within the *OutputWorkspace* can be further fed to :ref:`DirectILLApplySelfShielding <algm-DirectILLApplySelfShielding>`.

Usage
-----

.. include:: ../usagedata-note.txt

**Example - Calculating absorption corrections**

.. testsetup:: IN4Example

   config['default.facility'] = 'ILL'
   config['default.instrument'] = 'IN4'

.. testcode:: IN4Example

    preprocessed = DirectILLCollectData(Run='ILL/IN4/087294-087295')
    geometry = {
        'Shape': 'FlatPlate',
        'Width': 4.0,
        'Height': 3.0,
        'Thick': 0.01,
        'Angle': 45.0,
        'Center': [0.0, 0.0, 0.0]
    }
    material = {
        'ChemicalFormula': 'Cd S',
        'SampleNumberDensity': 0.01
    }
    SetSample(
        InputWorkspace=preprocessed,
        Geometry=geometry,
        Material=material
    )
    DirectILLSelfShielding(
        InputWorkspace=preprocessed,
        OutputWorkspace='absorption_corrections',
        NumberOfSimulatedWavelengths=10
    )
    corrections = mtd['absorption_corrections']
    f_short = corrections.readY(0)[0]
    f_long = corrections.readY(0)[-1]
    print('Absoprtion correction factors for detector 1')
    print('Short wavelengths: {:.2f}'.format(f_short))
    print('Long wavelengths:  {:.2f}'.format(f_long))

Output:

.. testoutput:: IN4Example

    Absoprtion correction factors for detector 1
    Short wavelengths: 0.82
    Long wavelengths:  0.55

.. categories::

.. sourcelink::
