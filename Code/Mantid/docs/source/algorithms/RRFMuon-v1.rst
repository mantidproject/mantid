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

**Example - Computing asymmetry in RRF**

.. testcode:: ExRRF

   import math
   # Create an input workspace with two spectra
   datax = [i/100. for i in range(1,300)]
   datay1 = [ math.cos(2*3.14159*i/100.) for i in range(1,299) ]
   datay2 = [ math.sin(2*3.14159*i/100.) for i in range(1,299) ]
   datay = datay1 + datay2
   input = CreateWorkspace(dataX=datax, dataY=datay,Nspec=2,UnitX="TOF")
   # Compute polarization in RRF
   output = RRFMuon(input,1.0,"MHz",0)
   print("%.1f" % output.readY(0)[0])
   print("%.1f" % output.readY(1)[0])

Output:

.. testoutput:: ExRRF

   1.0
   -0.0


.. categories::