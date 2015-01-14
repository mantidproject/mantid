.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Assuming *InputWorkspace* contains the real and imaginary parts of the asymmetry in the lab-fram, the algorithm 
returns the muon polarization in the Rotating Reference Frame (RRF), given the frequency of the oscillations and the phase of the 
detectors, as given by the formula:

.. math:: P_R^{RRF} (\nu_0, \phi, t)= + P_R\left(0,t\right) \cos\left(2\pi\nu_0 t + \phi\right) + P_I\left(0,t\right) \sin\left(2\pi\nu_0 t + \phi\right)
.. math:: P_I^{RRF} (\nu_0, \phi, t)= - P_R\left(0,t\right) \sin\left(2\pi\nu_0 t + \phi\right) + P_I\left(0,t\right) \cos\left(2\pi\nu_0 t + \phi\right)

where :math:`P_R\left(0,t\right)` and :math:`P_I\left(0,t\right)` are the real and imaginary part of the asymmetry in the lab-frame, 
:math:`\nu_0` is the input frequency, and :math:`\phi` the input phase.

Usage
-----

**Example - Computing polarization in RRF**

.. testcode:: ExRRF

   import math
   # Create an input workspace with two spectra
   datax = [(i-50)/100. for i in range(1,100)]
   datay1 = [ math.cos(2*3.14159*3*(50-i)/100.) for i in range(1,99) ]
   datay2 = [ math.sin(2*3.14159*3*(50-i)/100.) for i in range(1,99) ]
   datay = datay1 + datay2
   input = CreateWorkspace(dataX=datax, dataY=datay,Nspec=2)
   # Compute polarization in RRF
   output = RRFMuon(input,196,0)
   # Print some values
   print output.readY(0)[49]
   print output.readY(1)[49]

Output:

.. testoutput:: ExRRF

   1.0
   0.0


.. categories::