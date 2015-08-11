:orphan:

.. testcode:: mwTest_Extracting_And_Manipulating_Data[1]

   ws = Load(Filename="HRP39182.RAW")
   for i in range(ws.getNumberHistograms()):
       y = ws.readY(i)
       x = ws.readX(i)
       e = ws.readE(i)


.. testcode:: mwTest_Extracting_And_Manipulating_Data[12]

   ws = Load(Filename="HRP39182.RAW")
   ws = Rebin(InputWorkspace=ws, Params=1e4) # Rebin to make the looping more manageable.
   # Outer loop. Loop over spectrum
   for i in range(ws.getNumberHistograms()):
       y = ws.readY(i)
       sum_counts = 0
       # Inner loop. Loop over bins.
       for j in range(ws.blocksize()):
           sum_counts += y[j] 
       # Display spectrum number against sum_counts
       print ws.getSpectrum(i).getSpectrumNo(), sum_counts

.. testoutput:: mwTest_Extracting_And_Manipulating_Data[12]
   :options: +ELLIPSIS, +NORMALIZE_WHITESPACE

   1 20795.0
   2 51334.0
   ...
   344 18535.0
   345 19340.0


.. testsetup:: mwTest_Extracting_And_Manipulating_Data[36]

   ws = Load(Filename="HRP39182.RAW")

.. testcode:: mwTest_Extracting_And_Manipulating_Data[36]

   y = ws.readY(0)
   x = ws.readX(0)
   e = ws.readE(0)
   out_ws = CreateWorkspace(DataX=x, DataY=y, DataE=e, NSpec=1)


.. testcode:: mwTest_Extracting_And_Manipulating_Data[48]

   ws = Load(Filename="HRP39182.RAW")	
   x = ws.readX(0)
   y = ws.readY(0)
   e = ws.readE(0)
   new_x = x * 1e-3
   new_ws = CreateWorkspace(DataX=new_x, DataY=y, DataE=e, NSpec=1,UnitX='Label')
   unit = new_ws.getAxis(0).getUnit()
   unit.setLabel("Time-of-flight", "Milliseconds")


.. testcode:: mwTest_Extracting_And_Manipulating_Data[61]

   ws = Load(Filename="HRP39182.RAW")
   x = ws.extractX()
   y = ws.extractY()
   e = ws.extractE()
   print x.shape
   print y.shape
   print e.shape

.. testoutput:: mwTest_Extracting_And_Manipulating_Data[61]
   :options: +ELLIPSIS, +NORMALIZE_WHITESPACE

   (345L, 23988L)
   (345L, 23987L)
   (345L, 23987L)


