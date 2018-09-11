.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm is used to fit data from the high-angle bank to data from the low-angle bank in a SANS experiment. The degrees of freedom are a shift and a scale factor which determine by how much the high-angle bank data needs to be scaled and then shifted to match the low-angle bank data.

Fit Modes
##############

There are 4 available fit modes used to scale and shift the high angle bank data so that it matches up with the low angle bank data. Where fitting is required :ref:`algm-Fit` is used with a composite function comprised of a *FlatBackground* and *TabulatedFunction*. In all cases the shift and scale are used to alter the counts and errors for the high angle bank. 

*None* is the mode for no fit determined scaling or shifting. In this case the *ScaleFactor* and *ShiftFactor* properties must both be provided. With *Both*, fitting is used to establish optimum parameters for both the scaling and shifting of the high angle bank data. *ScaleOnly* mode ties the shift, so *ShiftFactor* must be provided. *ShiftOnly* mode ties the scale so *ScaleFactor* must be provided.

Usage
-----


**Example - Simple shift:**

.. testcode:: ExSimpleShift

	hab_ws= CreateWorkspace(DataX=range(4,10), DataY=[1]*5, UnitX='MomentumTransfer')
	lab_ws = CreateWorkspace(DataX=range(0,6), DataY=[6]*5, UnitX='MomentumTransfer')

	uniform_binning = [0, 1, 10]
	hab_ws= Rebin(hab_ws, Params=uniform_binning)
	lab_ws = Rebin(lab_ws, Params=uniform_binning)

	scale, shift = SANSFitShiftScale(HABWorkspace = hab_ws, LABWorkspace =lab_ws, Mode='ShiftOnly', ScaleFactor=1.0)

	print("{:.1f}".format(scale))
	print("{:.1f}".format(shift))

Output:
   
.. testoutput:: ExSimpleShift

	1.0
	2.5
   
.. categories::

.. sourcelink::
