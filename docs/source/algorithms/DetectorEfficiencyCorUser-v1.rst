.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm will correct detector efficiency according to the ILL INX
program for time-of-flight data reduction.

A formula named "formula\_eff" must be defined in the instrument
parameters file and linked to all detectors in the input workspace. Different
components can have different formulas. The formula for each detector is
searched recursively. Thus, there can be, for example, a global formula for the
entire instrument and a specific formula for a bank or tube overriding the
global one.

The input workspace must be in DeltaE units.

The output data will be corrected as:

:math:`y = \frac{y}{eff}`

where :math:`eff` is

:math:`eff = \frac{f(Ei - \Delta E)}{f(E_i)}`

The function :math:`f` is defined as "formula\_eff" in the IDF. To date
this has been implemented at the ILL for ILL IN4, IN5 and IN6, and at
the MLZ for TOFTOF.

Unlike :ref:`algm-DetectorEfficiencyCor` algorithm, which uses tabulated formula
accounting for neutron adsorption efficiency dependence on neutron energy, 
the formula used by this algorithm 
is provided by instrument scientist and is adjusted for the instrument, 
accounting for a number of additional instrument specific factors. 

As example, for `TOFTOF <http://www.mlz-garching.de/toftof>`_ instrument, the energy-dependent intensity 
loss factor accounts also for absorption of neutrons by the Ar gas in the flight chamber, Al windows 
of sample environment etc.
The formula used by DetectorEfficiencyCorUser algorithm is derived for TOFTOF in paper of 
T. Unruh, 2007, doi:10.1016/j.nima.2007.07.015 and is set up in the TOFTOF instrument parameters file. 


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


  # Now we are ready to run the correction
  wsCorrected = DetectorEfficiencyCorUser(ws)


  print("The correction for the data by the user defined function.")
  print("In this case: " + correction_formula)
  for i in range(0,wsCorrected.blocksize(),10):
    print("The correct value in bin {} is {:.2f} compared to {:.2f}".format(i,wsCorrected.readY(0)[i],ws.readY(0)[i]))

Output:

.. testoutput:: Ex1

    The correction for the data by the user defined function.
    In this case: exp(-0.0565/sqrt(e))*(1.0-exp(-3.284/sqrt(e)))
    The correct value in bin 0 is 4.30 compared to 0.30
    The correct value in bin 10 is 0.52 compared to 0.30
    The correct value in bin 20 is 0.35 compared to 0.30
    The correct value in bin 30 is 0.30 compared to 0.30



.. categories::

.. sourcelink::
