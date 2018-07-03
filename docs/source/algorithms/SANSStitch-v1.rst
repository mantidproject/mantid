.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm is used to stitch together reduced data typically provided by :ref:`algm-Q1D`. Data from high-angle and low-angle banks in a SANS experiment can be stitched together using this algorithm. 

Merging of inputs is achieved using the following foruma, where *C* denotes counts, *N* denotes normalization and *f* and *r* relate to forward (high-angle) and rear (low-angle) respectively:

.. math:: 

   \frac{C_f(Q)+(shift\cdot N_f(Q))+C_r(Q)}{\frac{N_f(Q)}{scale} + N_r(Q)}

Fit Modes
##############

There are 4 available fit modes used to scale and shift the high angle bank data so that it can be stitched together with the low angle bank data. Where fitting is required :ref:`algm-Fit` is used with a composite function comprised of a *FlatBackground* and *TabulatedFunction*. In all cases the shift and scale are used to alter the counts and errors for the high angle bank. 

*None* is the mode for no fit determined scaling or shifting. In this case the *ScaleFactor* and *ShiftFactor* properties must both be provided. With *Both*, fitting is used to establish optimum parameters for both the scaling and shifting of the high angle bank data. *ScaleOnly* mode ties the shift, so *ShiftFactor* must be provided. *ShiftOnly* mode ties the scale so *ScaleFactor* must be provided.

Can Runs
############

When can runs are provided they are processed separately using the same merge forumla above, and then subtracted from the processed sample run. If can runs are provided as inputs then the scale and shift factors are determined as part of fitting by operating on a workspace calculated from: 

.. math:: 
   \frac{C_{sample}(Q)}{N_{sample}(Q)} - \frac{C_{can}(Q)}{N_{can}(Q)}


This is analogous to how :ref:`algm-Q1D` operates on input workspaces.

Merging of front and rear banks for the can is achieved using a different form from that above. 

.. math:: 

   \frac{C_f(Q)+C_r(Q)}{\frac{N_f(Q)}{scale} + N_r(Q)}

where *C* denotes counts, *N* denotes normalization and *f* and *r* relate to forward (high-angle) and rear (low-angle) respectively. The can workspace is subtracted from the merged sample workspace to generate the output.

Workflow
########

.. diagram:: SANSStitch-v1_wkflw.dot

Usage
-----


**Example - Simple shift:**

.. testcode:: ExSimpleShift

   hab_counts = CreateWorkspace(DataX=range(4,10), DataY=[1]*5, UnitX='MomentumTransfer')
   hab_norm = CreateWorkspace(DataX=range(4,10), DataY=[1]*5, UnitX='MomentumTransfer')
   lab_counts = CreateWorkspace(DataX=range(0,6), DataY=[6]*5, UnitX='MomentumTransfer')
   lab_norm = CreateWorkspace(DataX=range(0,6), DataY=[1]*5, UnitX='MomentumTransfer')
   
   uniform_binning = [0, 1, 10]
   hab_counts = Rebin(hab_counts, Params=uniform_binning)
   hab_norm = Rebin(hab_norm, Params=uniform_binning)
   lab_counts = Rebin(lab_counts, Params=uniform_binning)
   lab_norm = Rebin(lab_norm, Params=uniform_binning)

   stitched, scale, shift = SANSStitch(HABCountsSample=hab_counts, 
       HABNormSample=hab_norm, 
       LABCountsSample=lab_counts, 
       LABNormSample=lab_norm, 
       Mode='ShiftOnly', ScaleFactor=1.0 )

   print("{:.1f}".format(scale))
   print("{:.1f}".format(shift))

Output:
   
.. testoutput:: ExSimpleShift

   1.0
   6.0
   
.. categories::

.. sourcelink::
