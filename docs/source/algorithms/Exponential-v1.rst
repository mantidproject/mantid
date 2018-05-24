.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

The algorithm will apply the exponential function (i.e. :math:`e^y`) to
the data from a workspaces.
The corresponding error values will be updated using :math:`E_{new}=E_{old}e^y`, assuming errors are Gaussian and small compared to the signal.
The units of the
workspace are not updated, so the user must take care in the use of such
output workspaces. When acting on an event workspace, the output will be
a Workspace2D, with the default binning from the original workspace.

Usage
-----

.. testcode::

  import numpy as np

  # Create a workspace
  ws = CreateSampleWorkspace()

  # Apply the natural exponential function to the data in the workspace
  res = Exponential( ws )

  # Check the result
  y = ws.readY(0)
  yres = res.readY(0)

  # Use numpy array calculation to apply an exponential to all elements of array y
  yexp = np.exp(y)
  # Use numpy to check that all elements in two arrays are equal
  print(np.all( yexp == yres ))

Output
######

.. testoutput::

  True

.. categories::

.. sourcelink::
