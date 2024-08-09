.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Calculate Z-score of a spectrum in a given workspace.

Definition of Z-score
#####################

The standard score of a raw score :math:`x` is:

.. math:: z = \frac{x-\mu}{\sigma}.

where (1) :math:`\mu` is the mean of the population and (2) :math:`\sigma` is the standard deviation of the population.


Usage
-----

**Example - calculate Z-score for a single spectrum workspace:**

.. testcode:: ExHistSimple

  # create histogram workspace
  dataX = [0,1,2,3,4,5,6,7,8,9]
  dataY = [1,1.5,-11,1,19,1,1,2,1]
  ws = CreateWorkspace(dataX, dataY)

  # rebin from min to max with size bin = 2
  ws2 = CalculateZscore(InputWorkspace=ws)

  print("The Z-scores are: {}".format(ws2.readY(0)))

.. testcleanup:: ExHistSimple

   DeleteWorkspace(ws)
   DeleteWorkspace(ws2)

Output:

.. testoutput:: ExHistSimple

  The Z-scores are: [0.11618485 0.04647394 1.78924674 0.11618485 2.39340797 0.11618485
   0.11618485 0.02323697 0.11618485]

.. categories::

.. sourcelink::
