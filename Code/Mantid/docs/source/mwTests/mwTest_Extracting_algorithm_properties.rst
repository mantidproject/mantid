:orphan:

.. testcode:: mwTest_Extracting_algorithm_properties[4]

   sample = Load('MAR11015.raw')
   ei, mon_peak, mon_index, tzero = GetEi(sample, Monitor1Spec=2,Monitor2Spec=3,EnergyEstimate=12)
   sample = ConvertUnits(sample, Target='DeltaE', Emode='Direct',EFixed=ei)


.. testcode:: mwTest_Extracting_algorithm_properties[11]

   sample = Load('MAR11015.raw')
   algorithm = sample.getHistory().lastAlgorithm()
   filename = algorithm.getPropertyValue("Filename")


.. Skipping Test  mwTest_Extracting_algorithm_properties[20]


