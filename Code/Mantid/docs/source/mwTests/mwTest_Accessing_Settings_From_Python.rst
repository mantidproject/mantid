:orphan:

.. testcode:: mwTest_Accessing_Settings_From_Python[8]

   from mantid import config
    
   default_inst_name = config['default.instrument']


.. testcode:: mwTest_Accessing_Settings_From_Python[28]

   from mantid import config
   
   config['default.instrument'] = 'INST_NAME'


.. Skipping Test  mwTest_Accessing_Settings_From_Python[40]


.. testcode:: mwTest_Accessing_Settings_From_Python[62]

   from mantid import config
   
   facility = config.getFacility('SNS')  # or config.getFacility() returns the current default
   inst_info = config.getInstrument('CNCS') # or config.Instrument() returns the current default


