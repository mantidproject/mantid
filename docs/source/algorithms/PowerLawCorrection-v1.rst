.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm corrects the data and error values on a workspace by the
value of an function of the form :math:`C0 \times x^{C1}`. This formula
is calculated for each data point, with the value of *x* being the
mid-point of the bin in the case of histogram data. The data and error
values are multiplied by the value of this function.

Usage
-----

**Example - A sample correction**  

.. testcode:: Ex1

  # a sample workspace with a sample instrument
  ws = CreateSampleWorkspace(Function="Flat background")
 
  #Now we are ready to run the correction
  wsCorrected = PowerLawCorrection(ws,C0=3,C1=2)

  print("The correction counts and errors are multiplied by function 3*x^2")
  for i in range(0,wsCorrected.blocksize(),10):
    print ("The correct value in bin {} is {:.2f} compared to {:.2f}".format(i, wsCorrected.readY(0)[i], ws.readY(0)[i]))

Output:

.. testoutput:: Ex1

    The correction counts and errors are multiplied by function 3*x^2
    The correct value in bin 0 is 30000.00 compared to 1.00
    The correct value in bin 10 is 13230000.00 compared to 1.00
    The correct value in bin 20 is 50430000.00 compared to 1.00
    The correct value in bin 30 is 111630000.00 compared to 1.00
    The correct value in bin 40 is 196830000.00 compared to 1.00
    The correct value in bin 50 is 306030000.00 compared to 1.00
    The correct value in bin 60 is 439230000.00 compared to 1.00
    The correct value in bin 70 is 596430000.00 compared to 1.00
    The correct value in bin 80 is 777630000.00 compared to 1.00
    The correct value in bin 90 is 982830000.00 compared to 1.00


.. categories::

.. sourcelink::
