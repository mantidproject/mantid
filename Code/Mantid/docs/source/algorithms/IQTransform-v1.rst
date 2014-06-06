.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm is intended to take the output of a SANS reduction and
apply a transformation to the data in an attempt to linearise the curve.
Optionally, a background can be subtracted from the input data prior to
transformation. This can be either a constant value, another workspace
or both. Note that this expects a single spectrum input; if the input
workspace contains multiple spectra, only the first will be transformed
and appear in the output workspace.

A SANS reduction results in data in the form I(Q) vs Q, where Q is
Momentum Transfer and I denotes intensity (the actual unit on the Y axis
is 1/cm). These abbreviations are used in the descriptions of the
transformations which follow. If the input is a histogram, the mid-point
of the X (i.e. Q) bins will be taken. The output of this algorithm is
always point data.

+-----------------------+-----------------------------------------------------------------------------------------------+--------------------------------------------------------------------------------------------------+
| Transformation Name   | Y                                                                                             | X                                                                                                |
+=======================+===============================================================================================+==================================================================================================+
| Guinier (spheres)     | :math:`\ln (I)`                                                                               | :math:`Q^2`                                                                                      |
+-----------------------+-----------------------------------------------------------------------------------------------+--------------------------------------------------------------------------------------------------+
| Guinier (rods)        | :math:`\ln (IQ)`                                                                              | :math:`Q^2`                                                                                      |
+-----------------------+-----------------------------------------------------------------------------------------------+--------------------------------------------------------------------------------------------------+
| Guinier (sheets)      | :math:`\ln (IQ^2)`                                                                            | :math:`Q^2`                                                                                      |
+-----------------------+-----------------------------------------------------------------------------------------------+--------------------------------------------------------------------------------------------------+
| Zimm                  | :math:`\frac{1}{I}`                                                                           | :math:`Q^2`                                                                                      |
+-----------------------+-----------------------------------------------------------------------------------------------+--------------------------------------------------------------------------------------------------+
| Debye-Bueche          | :math:`\frac{1}{\sqrt{I}}`                                                                    | :math:`Q^2`                                                                                      |
+-----------------------+-----------------------------------------------------------------------------------------------+--------------------------------------------------------------------------------------------------+
| Holtzer               | :math:`I \times Q`                                                                            | :math:`Q`                                                                                        |
+-----------------------+-----------------------------------------------------------------------------------------------+--------------------------------------------------------------------------------------------------+
| Kratky                | :math:`I \times Q^2`                                                                          | :math:`Q`                                                                                        |
+-----------------------+-----------------------------------------------------------------------------------------------+--------------------------------------------------------------------------------------------------+
| Porod                 | :math:`I \times Q^4`                                                                          | :math:`Q`                                                                                        |
+-----------------------+-----------------------------------------------------------------------------------------------+--------------------------------------------------------------------------------------------------+
| Log-Log               | :math:`\ln(I)`                                                                                | :math:`\ln(Q)`                                                                                   |
+-----------------------+-----------------------------------------------------------------------------------------------+--------------------------------------------------------------------------------------------------+
| General [*]_          | :math:`Q^{C_1} \times I^{C_2} \times \ln{\left( Q^{C_3} \times I^{C_4} \times C_5 \right)}`   | :math:`Q^{C_6} \times I^{C_7} \times \ln{\left( Q^{C_8} \times I^{C_9} \times C_{10} \right)}`   |
+-----------------------+-----------------------------------------------------------------------------------------------+--------------------------------------------------------------------------------------------------+

.. [*] The constants :math:`C_1 - C_{10}` are, in subscript order, the ten constants passed to the GeneralFunctionConstants property.

Usage
-----

**Example - Zimm transformation:**

.. testcode:: ExZimm

   x = [1,2,3]
   y = [1,2,3]
   input = CreateWorkspace(x,y)
   input.getAxis(0).setUnit("MomentumTransfer")
   input.setDistribution(True)

   output = IQTransform(input, 'Zimm')

   print 'Output Y:', ', '.join(['{:.3f}'.format(y) for y in output.readY(0)])
   print 'Output X:', ', '.join(['{:.3f}'.format(x) for x in output.readX(0)])
Output:

.. testoutput:: ExZimm

   Output Y: 1.000, 0.500, 0.333
   Output X: 1.000, 4.000, 9.000

**Example - Zimm transformation and background:**

.. testcode:: ExZimmBg

   x = [1,2,3]
   y = [1,2,3]
   input = CreateWorkspace(x,y)
   input.getAxis(0).setUnit("MomentumTransfer")
   input.setDistribution(True)

   output = IQTransform(input, 'Zimm', BackgroundValue=0.5)

   print 'Output Y:', ', '.join(['{:.3f}'.format(y) for y in output.readY(0)])
   print 'Output X:', ', '.join(['{:.3f}'.format(x) for x in output.readX(0)])
Output:

.. testoutput:: ExZimmBg

   Output Y: 2.000, 0.667, 0.400
   Output X: 1.000, 4.000, 9.000

**Example - General transformation:**

.. testcode:: ExGeneral

   import math

   x = [1,2,3]
   y = [1,2,3]
   input = CreateWorkspace(x,y)
   input.getAxis(0).setUnit("MomentumTransfer")
   input.setDistribution(True)

   constants = [2,2,0,0,math.e,3,0,0,0,math.e]
   output = IQTransform(input, 'General', GeneralFunctionConstants=constants)

   print 'Output Y:', ', '.join(['{:.3f}'.format(y) for y in output.readY(0)])
   print 'Output X:', ', '.join(['{:.3f}'.format(x) for x in output.readX(0)])
Output:

.. testoutput:: ExGeneral

   Output Y: 1.000, 16.000, 81.000
   Output X: 1.000, 8.000, 27.000

.. categories::
