
.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

CalculateDIFC calculates the value of :math:`DIFC` for every pixel in
an instrument. 

.. math:: DIFC = 252.816 \frac{(L_1 + L_2) 2 \sin(\theta)}{1 + {offset}}

This is used in the equation

.. math:: TOF = DIFC \times d

This algorithm uses the same underlying calculation as :ref:`algm-ConvertUnits`
and :ref:`algm-AlignDetectors`.

Assumptions: There are no assumptions and this algorithm works on the results 
of :ref:`algm-LoadEmptyInstrument`.

Usage
-----

**Example - CalculateDIFC**

.. testcode:: CalculateDIFCExample

   ws = LoadEmptyInstrument(Filename="NOMAD_Definition.xml", OutputWorkspace="ws")
   ws = CalculateDIFC(ws, OutputWorkspace="ws")

   # Print the result
   print "The output workspace has %i spectra" % ws.getNumberHistograms()
   print "DIFC of pixel %i is %1.f" % (100, ws.readY(100)[0])

Output:

.. testoutput:: CalculateDIFCExample

  The output workspace has 101378 spectra
  DIFC of pixel 100 is 1220

.. categories::

