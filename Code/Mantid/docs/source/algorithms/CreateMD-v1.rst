.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This workflow algorithm creates MDWorkspaces in the Q3D, HKL frame using `ConvertToMD <http://www.mantidproject.org/ConvertToMD>`__. 

Setting the UB matrix
######################################

*u* and *v* are required. *u* and *v* are both 3-element vectors. These specify how the crystal's axes were oriented relative to the spectrometer in the setup for which you define psi to be zero. *u* specifies the lattice vector that is parallel to the incident neutron beam, whilst *v* is a vector perpendicular to this in the horizontal plane. In UB matrix notation, *u* and *v* provide the U matrix. See `SetUB <http://www.mantidproject.org/SetUB>`__. *Alatt* and *Angdeg* are the lattice parameters in Angstroms and lattice angles in degrees respectively. Both are 3-element vectors. These form the B-matrix.

Goniometer Settings
#####################

If goniometer settings have been provided then these will be applied to the input workspace(s). For multiple input workspaces, you will need to provide goniometer settings (*Psi*, *Gl*, *Gs*) as vectors where each element of the vector corresponds to the workspace in the order listed in *InputWorkspaces*. You do not need to provide the goniometer settings at all. If you run `SetGoniometer <http://www.mantidproject.org/SetGoniometer>`__ individually on the input workspace prior to running CreateMD, then those settings will not be overwritten by CreateMD.

Merging Individually Converted Runs
#####################################

If a sequence of input workspaces are provided then these are individually processed as above, and are merged together via `MergeMD <http://www.mantidproject.org/MergeMD>`__. Intermediate workspaces are not kept.

Additional Information
#######################
CreateMD uses `ConvertToMDMinMaxLocal <http://www.mantidproject.org/ConvertToMDMinMaxLocal>`__ to determine the min and max extents prior to running `ConvertToMD <http://www.mantidproject.org/ConvertToMD>`__ on each run.


.. figure:: /images/HoraceOrientation.png
   :alt: HoraceOrientation.png

   `Horace <http://horace.isis.rl.ac.uk/Generating_SQW_files>`__ style orientation used by CreateMD. DSPI and Omega in the image have no meaning in Mantid and are not required by this algorithm.


*Example*
##########################################

.. testcode:: CreateMDExample

  print 'Hello World'
  
Output
^^^^^^

.. testoutput:: CreateMDExample

  'Not implemented yet'


.. categories::
