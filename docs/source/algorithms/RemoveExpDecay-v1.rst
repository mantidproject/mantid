.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm removes the exponential time decay from the specified muon
spectra, leaving the rest unchanged. By default, all the spectra
in a workspace will be corrected. If the detector records a total of
zero counts (i.e. the detector is dead) then the output will be all zeros for that detector.

The formula for removing the exponential decay is given by:

.. math:: \textrm{NewData} = (\textrm{OldData}\times{e^\frac{t}{\tau}})/N_0 - 1.0

where :math:`\tau` is the muon lifetime (2.1969811e-6 seconds). :math:`N_0` is a
fitted normalisation constant.

Usage
-----

**Example - Removing exponential decay:**

.. testcode:: ExSimple

   y = [100, 150, 50, 10, 5]
   x = [1,2,3,4,5,6]
   input = CreateWorkspace(x,y)

   output = RemoveExpDecay(input)

   print("Exp. decay removed: {}".format(output.readY(0)))

Output:

.. testoutput:: ExSimple

   Exp. decay removed: [-0.24271431  0.79072482 -0.05900907 -0.70331658 -0.76614798]

**Example - Removing exponential decay with dead detector:**

.. testcode:: dead

  import numpy as np
  import random

  # create data
  x=np.linspace(start=0,stop=10,num=5)
  xData = np.append(x,x)
  xData = np.append(xData,xData)

  #make y data
  def genYData(x,phi):
    return np.sin(5.0*x+phi)+np.exp(-x/2.2)

  yData = np.append(genYData(x, 0.0), genYData(x, 1.2))
  yData = np.append(yData, np.zeros(len(x))) # dead detector
  yData = np.append(yData, genYData(x, 3.4))

  # create workspace
  ws = CreateWorkspace(xData,yData,NSpec=4, UnitX="Time")
  label=ws.getAxis(0).setUnit("Label")
  label.setLabel("Time","microsecond")

  output = RemoveExpDecay(ws)# Print the result
  for j in range(4):
      print ("Exp. decay removed: {}".format(output.readY(j)))

Output:

.. testoutput:: dead

  Exp. decay removed: [  0.04103434  -0.17277399  -1.29718867  -6.20972864 -25.84444803]
  Exp. decay removed: [-0.12586409  0.73213038  3.31216989 11.02723309 33.95108191]
  Exp. decay removed: [0. 0. 0. 0. 0.]
  Exp. decay removed: [ 0.05933294 -0.42223811 -1.30901512 -2.13214736  1.38620519]


.. categories::

.. sourcelink::
