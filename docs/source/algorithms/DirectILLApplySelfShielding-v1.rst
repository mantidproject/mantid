.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm subtracts empty container data and applies self-shielding corrections to *InputWorkspace*. Both operations are optional: what is actually done depends on the input properties.

This algorithm is part of :ref:`ILL's direct geometry data reduction algorithms <DirectILL>`.

*SelfShieldingCorrectionWorkspace* can be obtained from the :ref:`DirectILLSelfShielding <algm-DirectILLSelfShielding>` algorithm.

Usage
-----

.. include:: ../usagedata-note.txt

**Example - Absorption corrections and empty container subtraction**

.. testsetup:: IN4Example

   config['default.facility'] = 'ILL'
   config['default.instrument'] = 'IN4'

.. testcode:: IN4Example

    import numpy
    
    DirectILLCollectData(Run='ILL/IN4/087294+087295.nxs',
        OutputWorkspace='sample',
        # Needed to ensure equal binning with empty container
        OutputIncidentEnergyWorkspace='E_i')  
    DirectILLCollectData(Run='ILL/IN4/087306-087309.nxs',
        OutputWorkspace='empty_can',
        IncidentEnergyWorkspace='E_i',  # From sample
    )
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
        InputWorkspace='sample',
        Geometry=geometry,
        Material=material
    )
    DirectILLSelfShielding(
        InputWorkspace='sample',
        OutputWorkspace='absorption_corrections',
        NumberOfSimulatedWavelengths=10
    )
    DirectILLApplySelfShielding(
        InputWorkspace='sample',
        OutputWorkspace='sample_corrected',
        EmptyContainerWorkspace='empty_can',
        SelfShieldingCorrectionWorkspace='absorption_corrections'
    )
    
    sample = mtd['sample']
    maxY = numpy.amax(sample.readY(0))
    print('Elastic peak maximum before corrections: {:.3}'.format(maxY))
    corrected = mtd['sample_corrected']
    maxY = numpy.amax(corrected.readY(0))
    print('After empty container subtraction and absorption corrections: {:.3}'.format(maxY))

Output:

.. testoutput:: IN4Example

    Elastic peak maximum before corrections: 12.1
    After empty container subtraction and absorption corrections: 16.0

.. categories::

.. sourcelink::
