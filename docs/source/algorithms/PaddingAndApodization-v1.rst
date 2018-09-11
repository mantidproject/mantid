.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm can prepare data for 
  :ref:`FFT <algm-FFT>` by applying padding and/or applying an apodization function. 

Padding is when the input data is extended by adding extra measurments of zero at regular intervals. For real data this is only done after the end of the input data. However, for complex data the padding should be shared between the start and end of the input data.

`Apodization functions <http://mathworld.wolfram.com/ApodizationFunction.html>`_ can be used to remove data with large errors. These are usualy
found at large time scales. These take a decay constant 
(:math:`\tau` ) that determines the rate at which the data goes to zero. 
The time the function is evaluated at is denoted by :math:`t`.
The current implementation includes the following functions:

- None.
- Lorentz :math:`\exp\left(-\frac{t}{\tau}\right)`.
- Gaussian :math:`\exp\left(-\frac{t^2}{2\tau^2}\right)`.
 
Usage
-----

**Example - Apply apodization function:**

.. testcode:: ExSimple

   import math
   import numpy as np
   y = [100, 150, 50, 10, 5]
   x = [1,2,3,4,5,6]
   input = CreateWorkspace(x,y)
   output=PaddingAndApodization(InputWorkspace=input,ApodizationFunction="Gaussian",DecayConstant=2.44,Padding=0,)
   print("output:  {}".format(['{0:.2f}'.format(value) for value in output.readY(0)]))
   
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
   output=PaddingAndApodization(InputWorkspace=input,Padding=2,)
   print("output:  {}".format(['{0:.2f}'.format(value)  for value in output.readY(0)]))
   
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
   output=PaddingAndApodization(InputWorkspace=input,ApodizationFunction="Gaussian",DecayConstant=2.44,Padding=2,)
   print("output:  {}".format(['{0:.2f}'.format(value)  for value in output.readY(0)]))
   
Output:

.. testoutput:: ExBoth

   output:  ['91.94', '107.20', '23.48', '2.61', '0.61', '0.00', '0.00', '0.00', '0.00', '0.00', '0.00', '0.00', '0.00', '0.00', '0.00']
   
.. categories::

.. sourcelink::
