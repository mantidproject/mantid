.. _02_extracting_alg_prop:

===============================
Extracting Algorithm Properties
===============================

After an algorithm has been executed, the properties remain accessible for as long as the output workspace exists.
Algorithms return their output properties as a tuple, which python can unpack automatically, for example:

.. code-block:: python

    sample = Load('MAR11015.raw')
    ei, mon_peak, mon_index, tzero = GetEi(sample, Monitor1Spec=2,Monitor2Spec=3,EnergyEstimate=12)
    sample = ConvertUnits(sample, Target='DeltaE', Emode='Direct',EFixed=ei)

To access input properties, use the :ref:`workspace history <Workspace-Workspace_History>`. Using this handle allows the properties to be extracted by their name using the `getPropertyValue()` function:

.. code-block:: python

    sample = Load('MAR11015.raw')
    algorithm = sample.getHistory().lastAlgorithm()
    filename = algorithm.getPropertyValue("Filename")

For some algorithms, such as :ref:`algm-Rebin`, you can get properties without accessing algorithm history.

.. code-block:: python

    rebin_alg = Rebin('input_ws',Params='1,5,100')
    params = rebin_alg.getProperty('Params').value
