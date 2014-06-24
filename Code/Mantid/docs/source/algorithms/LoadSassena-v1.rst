.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

The Sassena application `1 <http://sassena.org>`__ generates
intermediate scattering factors from molecular dynamics trajectories.
This algorithm reads Sassena output and stores all data in workspaces of
type `Workspace2D <http://www.mantidproject.org/Workspace2D>`_, grouped under a single
`WorkspaceGroup <http://www.mantidproject.org/WorkspaceGroup>`_.

Sassena ouput files are in HDF5 format
`2 <http://www.hdfgroup.org/HDF5>`__, and can be made up of the
following datasets: *qvectors*, *fq*, *fq0*, *fq2*, and *fqt*

Time units: Current Sassena version does not specify the time unit, thus
the user is required to enter the time in between consecutive data
points. Enter the number of picoseconds separating consecutive
datapoints.

The workspace for **qvectors**:

-  X-values for the origin of the vector, default: (0,0,0)
-  Y-values for the tip of the vector
-  one spectra with three bins for each q-vector, one bin per vector
   component. If orientational average was performed by Sassena, then
   only the first component is non-zero.

The workspaces for **fq**, **fq0**, and **fq2** contains two spectra:

-  First spectrum is the real part, second spectrum is the imaginary
   part
-  X-values contain the moduli of the q vector
-  Y-values contain the structure factors

Dataset **fqt** is split into two workspaces, one for the real part and
the other for the imaginary part. The structure of these two workspaces
is the same:

-  X-values contain the time variable
-  Y-values contain the structure factors
-  one spectra for each q-vector

.. categories::
