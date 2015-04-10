.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This workflow algorithm creates MDWorkspaces in the Q3D, HKL frame using :ref:`algm-ConvertToMD`. 

Setting the UB matrix
######################################

*u* and *v* are required. *u* and *v* are both 3-element vectors. These specify how the crystal's axes were oriented relative to the spectrometer in the setup for which you define psi to be zero. *u* specifies the lattice vector that is parallel to the incident neutron beam, whilst *v* is a vector perpendicular to this in the horizontal plane. In UB matrix notation, *u* and *v* provide the U matrix. See :ref:`algm-SetUB`. *Alatt* and *Angdeg* are the lattice parameters in Angstroms and lattice angles in degrees respectively. Both are 3-element vectors. These form the B-matrix.

Goniometer Settings
#####################

If goniometer settings have been provided then these will be applied to the input workspace(s). For multiple input workspaces, you will need to provide goniometer settings (*Psi*, *Gl*, *Gs*) as vectors where each element of the vector corresponds to the workspace in the order listed in *InputWorkspaces*. You do not need to provide the goniometer settings at all. If you run :ref:`algm-SetGoniometer` individually on the input workspace prior to running CreateMD, then those settings will not be overwritten by CreateMD.

EFixed
#######
The EFixed value may be provided for input workspaces. Specifying a single value, will ensure that that value is applied to all input DataSources. EFixed can also be provided as an array provided that the array is the same length as the DataSources, in this case, the EFixed values will be matched to the DataSources based on index. EFixed has units of meV.

Merging Individually Converted Runs
#####################################

If a sequence of input workspaces are provided, and *InPlace* is False, then these are individually processed as above, and are merged together via :ref:`algm-MergeMD`. Intermediate workspaces are not kept. If *InPlace* is applied conversion and merging are done in the same step. This requires less memory overhead, but may be slower.

Additional Information
#######################
CreateMD steps use :ref:`algm-ConvertToMDMinMaxGlobal` to determine the min and max possible extents prior to running :ref:`algm-ConvertToMD` on each run.


.. figure:: /images/HoraceOrientation.png
   :alt: HoraceOrientation.png

   `Horace <http://horace.isis.rl.ac.uk/Generating_SQW_files>`__ style orientation used by CreateMD. DSPI and Omega in the image have no meaning in Mantid and are not required by this algorithm.


**Single Conversion Example**
##########################################

.. testcode:: SingleConversion

   # Create some input data.
   current_ws = CreateSimulationWorkspace(Instrument='MAR', BinParams=[-3,1,3], UnitX='DeltaE')
   AddSampleLog(Workspace=current_ws,LogName='Ei',LogText='3.0',LogType='Number')

   # Execute CreateMD
   new_mdew = CreateMD(current_ws, Emode='Direct', Alatt=[1.4165, 1.4165,1.4165], Angdeg=[ 90, 90, 90], u=[1, 0, 0,], v=[0,1,0], Psi=6, Gs=0, Gl=[0])

   # Show dimensionality and dimension names
   ndims = new_mdew.getNumDims()
   for i in range(ndims):
       dim = new_mdew.getDimension(i)
       print dim.getName()
  
Output
^^^^^^

.. testoutput:: SingleConversion

   [H,0,0]
   [0,K,0]
   [0,0,L]
   DeltaE

**Multi Conversion Example**
##########################################

.. testcode:: MultiConversion

   # Create multiple runs 
   input_runs = list()
   psi = list()
   gs = list()
   gl = list()
   for i in range(1, 5):
       current_ws = CreateSimulationWorkspace(Instrument='MAR', BinParams=[-3,1,3], UnitX='DeltaE', OutputWorkspace='input_ws_' + str(i))
       input_runs.append(current_ws.name())
       psi.append(float(5 * i))
       gl.append(0.0)
       gs.append(0.0)
    
   # Convert and merge
   new_merged = CreateMD(input_runs, Emode='Direct', Alatt=[1.4165, 1.4165,1.4165], Angdeg=[ 90, 90, 90], u=[1, 0, 0,], v=[0,1,0], Psi=psi, Gl=gl, Gs=gs, EFixed=3.0)

   # Show dimensionality and dimension names
   ndims = new_merged.getNumDims()
   for i in range(ndims):
       dim = new_merged.getDimension(i)
       print dim.getName()

Output
^^^^^^

.. testoutput:: MultiConversion

   [H,0,0]
   [0,K,0]
   [0,0,L]
   DeltaE

**Multi Conversion Example InPlace**
##########################################

.. testcode:: MultiConversionInPlace

   # Create multiple runs 
   input_runs = list()
   psi = list()
   gs = list()
   gl = list()
   for i in range(1, 5):
       current_ws = CreateSimulationWorkspace(Instrument='MAR', BinParams=[-3,1,3], UnitX='DeltaE', OutputWorkspace='input_ws_' + str(i))
       input_runs.append(current_ws.name())
       psi.append(float(5 * i))
       gl.append(0.0)
       gs.append(0.0)
    
   # Convert and merge. ConversionToMD done in place.
   new_merged = CreateMD(input_runs, Emode='Direct', Alatt=[1.4165, 1.4165,1.4165], Angdeg=[ 90, 90, 90], u=[1, 0, 0,], v=[0,1,0], Psi=psi, Gl=gl, Gs=gs, EFixed=3.0, InPlace=True)

   # Show dimensionality and dimension names
   ndims = new_merged.getNumDims()
   for i in range(ndims):
       dim = new_merged.getDimension(i)
       print dim.getName()

Output
^^^^^^

.. testoutput:: MultiConversionInPlace

   [H,0,0]
   [0,K,0]
   [0,0,L]
   DeltaE

.. categories::

.. categories::
