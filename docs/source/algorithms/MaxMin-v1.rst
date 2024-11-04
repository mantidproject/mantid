.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

The algorithm creates a new 2D workspace containing the first maxima
(minima) for each spectrum, as well as their X boundaries and error.
This is used in particular for single crystal as a quick way to find
strong peaks. By default, the algorithm returns the maxima.

The :ref:`algm-Max` and :ref:`algm-Min` algorithms are just calls to the
:ref:`algm-MaxMin` algorithm, with the ShowMin flag set to true/false
respectively.

Usage
-----
**Example - Get maximum values for 9 spectra**

.. testcode:: ExMaxMinForMax

   # Create a workspace with 9 spectra each with 10 values of with the maximum is 10.3 for each spectrum
   ws = CreateSampleWorkspace("Histogram",  NumBanks=1, BankPixelWidth=3, BinWidth=10, Xmax=100)

   # Run algorithm
   wsMax = MaxMin (ws)

   print("Maximum of each spectrum")
   for i in range(9):
      print(wsMax.readY(i))

Output:

.. testoutput:: ExMaxMinForMax

   Maximum of each spectrum
   [10.3]
   [10.3]
   [10.3]
   [10.3]
   [10.3]
   [10.3]
   [10.3]
   [10.3]
   [10.3]


**Example - Get minimum values for 9 spectra**

.. testcode:: ExMaxMinForMin

   # Create a workspace with 9 spectra each with 10 values of with the minimum is 0.3 for each spectrum
   ws = CreateSampleWorkspace("Histogram",  NumBanks=1, BankPixelWidth=3, BinWidth=10, Xmax=100)

   # Run algorithm
   wsMax = MaxMin (ws, ShowMin=True)

   print("Minimum of each spectrum")
   for i in range(9):
      print(wsMax.readY(i))

Output:

.. testoutput:: ExMaxMinForMin

   Minimum of each spectrum
   [0.3]
   [0.3]
   [0.3]
   [0.3]
   [0.3]
   [0.3]
   [0.3]
   [0.3]
   [0.3]

.. categories::

.. sourcelink::
