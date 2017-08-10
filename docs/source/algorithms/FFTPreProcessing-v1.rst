.. algorithm::

.. summary::

Allows the user to generate data that has padding (extra measurements of zeros) and/or apply an apodization function.

.. alias::

.. properties::


Description
-----------

This algorithm can prepare data for 
  :ref:`FFT <algm-FFT>` by applying padding and/or applying an apodization function. 

Padding is when the input data is extended by adding extra measurments of zero at regular intervals after the ned of the input data. 

`Apodization functions <http://mathworld.wolfram.com/ApodizationFunction.html>`_ can be used to remove data with large errors. These are usualy
found at large time scales. The current implementation includes the following functions:

- None.
- Bartlett.
- Lorentz.
- Connes.
- Cosine.
- Gaussian.
- Welch.
 
Usage
-----

**Example - Apply apodization function:**

.. testcode:: ExSimple

   import math
   import numpy as np
   y = [100, 150, 50, 10, 5]
   x = [1,2,3,4,5,6]
   input = CreateWorkspace(x,y)
   output=FFTPreProcessing(InputWorkspace=input,spectra=0,ApodizationFunction="Gaussian",DecayConstant=2.44,Padding=0,)
   print  "output: ",['{0:.2f}'.format(value)  for value in output.readY(0)]
   
Output:

.. testoutput:: ExSimple

   output:  ['91.94', '107.20', '23.48', '2.61', '0.61']

**Example - Add padding:**

.. testcode:: ExPadding

   import math
   import numpy as np
   y = [100, 150, 50, 10, 5]
   x = [1,2,3,4,5,6]
   input = CreateWorkspace(x,y)
   output=FFTPreProcessing(InputWorkspace=input,spectra=0,Padding=2,)
   print  "output: ",['{0:.2f}'.format(value)  for value in output.readY(0)]
   
Output:

.. testoutput:: ExPadding

   output:  ['100.00', '150.00', '50.00', '10.00', '5.00', '0.00', '0.00', '0.00', '0.00', '0.00', '0.00', '0.00', '0.00', '0.00', '0.00']

**Example - Add padding and apodization function:**

.. testcode:: ExBoth

   import math
   import numpy as np
   y = [100, 150, 50, 10, 5]
   x = [1,2,3,4,5,6]
   input = CreateWorkspace(x,y)
   output=FFTPreProcessing(InputWorkspace=input,spectra=0,ApodizationFunction="Gaussian",DecayConstant=2.44,Padding=2,)
   print  "output: ",['{0:.2f}'.format(value)  for value in output.readY(0)]
   
Output:

.. testoutput:: ExBoth

   output:  ['91.94', '107.20', '23.48', '2.61', '0.61', '0.00', '0.00', '0.00', '0.00', '0.00', '0.00', '0.00', '0.00', '0.00', '0.00']
   
.. categories::

.. sourcelink::
