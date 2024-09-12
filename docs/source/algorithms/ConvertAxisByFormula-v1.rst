.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm allows users to adjust the axes of a workspace by a user
defined math formula. It will adjust the data values
(other than in one case the X values) of a workspace, although it will
reverse the spectra if needed to keep the X values increasing across the workspace.
This only works for MatrixWorkspaces, so will not work on
Multi Dimensional Workspaces or Table Workspaces. If you specify one of the
known :ref:`Units <Unit Factory>` then that will be the resulting unit,
otherwise like the
:ref:`algm-ConvertSpectrumAxis` algorithm the result of
this algorithm will have custom units defined for the axis you have
altered, and as such may not work in all other algorithms.

The algorithm can operate on the X or Y axis, but cannot alter the
values of a spectrum axis (the axis used as the Y axis on newly loaded
Raw data). If you wish to alter this axis use the
:ref:`algm-ConvertSpectrumAxis` algorithm first.

The formula is defined in a simple math syntax. For example:

* Squaring - *x^2*
* Square root - *sqrt(x)*
* Cubing - *x^3*
* Basic addition - *y + 3*
* Brackets - *(y+1)/20*
* Natural Log - *ln(x)*
* Log 10 - *log(x)*
* Exponent - *exp(y)*
* Round to nearest integer - *rint(y/10)*
* Absolute value - *abs(y-100)*
* Sine function (angle in radians) - *sin(x)*

*x* and *y* can be used interchangeably to refer to the current axis value.

Using Constants
###############

The following constants are predeined for use in your equations:

* pi - The ratio of a circle's circumference to its diameter
* h - Planck constant in J*s
* h_bar - Planck constant in J*s, divided by 2 PI
* g - Standard acceleration due to gravity. Precise value in ms\ :sup:`-2`
* mN - Mass of the neutron in kg
* mNAMU -  Mass of the neutron in AMU

Using geometry variables
########################

If the axis you are not transforming is a spectrum axis (the axis used as the Y axis on newly loaded
Raw data). Then you may also include geometry variables in your formula and they will be correctly interpreted by the algorithm.

The algorithm supports the following geometry variables:
* l1 - the distance between the source and sample
* l2 - the distance between the sample and detector
* signedtwotheta - the exit angle from the sample when it is determined with respect to the extended incoming beam
* twotheta - abs(signedtwotheta)

Refer to the
`muparser page <http://muparser.beltoforion.de/mup_features.html#idDef2>`_
for a full list of the functions available.

Usage
-----

**Example - Squaring the X axis:**

.. testcode:: ExSquareXAxis

   # Create sample input workspace
   dataX = [1,2,3,4,5]
   dataY = [1,1,1,1,1]
   input = CreateWorkspace(dataX, dataY)

   output = ConvertAxisByFormula(InputWorkspace=input,
                                 Axis="X",
                                 Formula="x^2",
                                 AxisTitle="Squared X",
                                 AxisUnits="x^2")

   print("New X values: {}".format(output.getAxis(0).extractValues()))
   print("New X units: {}".format(output.getAxis(0).getUnit().symbol()))
   print("New X title: {}".format(output.getAxis(0).getUnit().caption()))

Output:

.. testoutput:: ExSquareXAxis

   New X values: [ 1.  4.  9. 16. 25.]
   New X units: x^2
   New X title: Squared X

**Example - Doubling the Y axis:**

.. testcode:: ExDoubleYAxis

   from mantid.api import NumericAxis

   # Create sample input workspace (with 5 spectra)
   dataX = [1,2,3,4,5]
   dataY = [1,1,1,1,1]
   input = CreateWorkspace(dataX, dataY, NSpec=5)

   # Create numeric Y axis with values [1..5]
   yAxis = NumericAxis.create(5)
   for i in range(0,5):
     yAxis.setValue(i, i+1)

   # Replace Y axis in the input workspace. This is necessary because CreateWorkspace
   # uses TextAxis by default, which are not suitable for conversion.
   input.replaceAxis(1, yAxis)

   output = ConvertAxisByFormula(InputWorkspace=input,
                                 Axis="Y",
                                 Formula="y*2",
                                 AxisTitle="Doubled Y",
                                 AxisUnits="y*2")

   print("New Y values: {}".format(output.getAxis(1).extractValues()))
   print("New Y units: {}".format(output.getAxis(1).getUnit().symbol()))
   print("New Y title: {}".format(output.getAxis(1).getUnit().caption()))

Output:

.. testoutput:: ExDoubleYAxis

   New Y values: [ 2.  4.  6.  8. 10.]
   New Y units: y*2
   New Y title: Doubled Y

**Example - Converting from Wavelength to Momentum Transfer:**

.. testcode:: ExWv2MT

   wsWavelength = CreateSampleWorkspace(XUnit='Wavelength', XMin=2, XMax=6, BinWidth=0.05)
   # Convert to momentum transfer
   # directly using a formula
   wsMTbyFormula = ConvertAxisByFormula(InputWorkspace=wsWavelength,  Formula='(4*pi*sin(twotheta/2))/x', AxisUnits='MomentumTransfer')
   # using convert units (this will convert via time of flight)
   wsMTbyConvertUnits = ConvertUnits(InputWorkspace=wsWavelength, Target='MomentumTransfer')

   #check they are the same
   isMatched, messageTable = CompareWorkspaces(wsMTbyFormula,wsMTbyConvertUnits,0.00001,checkAxes=True, CheckType=True)
   if isMatched:
       print("Both methods create matching workspaces.")

Output:

.. testoutput:: ExWv2MT

   Both methods create matching workspaces.

.. categories::

.. sourcelink::
