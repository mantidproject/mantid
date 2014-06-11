.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm will correct detector efficiency according to the ILL INX
program for time-of-flight data reduction.

A formula named "formula\_eff" must be defined in the instrument
parameters file. The input workspace must be in DeltaE units.

The output data will be corrected as:

:math:`y = \frac{y}{eff}`

where :math:`eff` is

:math:`eff = \frac{f(Ei - \Delta E)}{f(E_i)}`

The function :math:`f` is defined as "formula\_eff" in the IDF. To date
this has been implemented at the ILL for ILL IN4, IN5 and IN6.

Usage
-----

**Example - A sample correction**  

.. testcode:: Ex1

  # a sample workspace with a sample instrument
  ws = CreateSampleWorkspace()
  # convert to Wavelength
  ws = ConvertUnits(ws,"DeltaE",EMode="Direct",EFixed=3.27)

  correction_formula = "exp(-0.0565/sqrt(e))*(1.0-exp(-3.284/sqrt(e)))"
  SetInstrumentParameter(ws,ParameterName="formula_eff",Value=correction_formula)


  #Now we are ready to run the correction
  wsCorrected = DetectorEfficiencyCorUser(ws)


  print ("The correction correct the data by the user defined function.")
  print ("In this case: " + correction_formula)
  for i in range(0,wsCorrected.blocksize(),10):
    print ("The correct value in bin %i is %.2f compared to %.2f" % (i,wsCorrected.readY(0)[i],ws.readY(0)[i]))

Output:

.. testoutput:: Ex1

    The correction correct the data by the user defined function.
    In this case: exp(-0.0565/sqrt(e))*(1.0-exp(-3.284/sqrt(e)))
    The correct value in bin 0 is 5.53 compared to 0.30
    The correct value in bin 10 is 0.53 compared to 0.30
    The correct value in bin 20 is 0.36 compared to 0.30
    The correct value in bin 30 is 0.30 compared to 0.30



.. categories::
