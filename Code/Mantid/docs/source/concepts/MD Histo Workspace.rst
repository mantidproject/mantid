.. _MDHistoWorkspace:

MDHistoWorkspace
================

The MDHistoWorkspace is a simple multi-dimensional workspace. In
contrast to the :ref:`MDWorkspace <MDWorkspace>`, which contains
points in space, the MDHistoWorkspace consists of a signal and error
spread around space on a regular grid.

In a way, the MDHistoWorkspace is to a
:ref:`MDWorkspace <MDWorkspace>` is what the
:ref:`Workspace2D <Workspace2D>` is to the
:ref:`EventWorkspace <EventWorkspace>`.

Creating a MDHistoWorkspace
---------------------------

MDHistoWorkspaces typically have 3 or 4 dimensions, although they can be
created in up to 9 dimensions.

-  You can bin a :ref:`MDWorkspace <MDWorkspace>` to a
   MDHistoWorkspace using the :ref:`BinMD <algm-BinMD>` algorithm.

   -  You can use :ref:`CreateMDWorkspace <algm-CreateMDWorkspace>` to create a
      blank MDWorkspace first, if you do not have data to bin.

-  Paraview and the `Vates Simple
   Interface <http://www.mantidproject.org/VatesSimpleInterface>`__ will create a MDHistoWorkspace
   from a :ref:`MDWorkspace <MDWorkspace>` when rebinning on a regular
   grid.

Viewing a MDHistoWorkspace
--------------------------

-  MDHistoWorkspaces can be created and visualized directly within
   Paraview and the `Vates Simple
   Interface <http://www.mantidproject.org/VatesSimpleInterface>`__ when rebinning along a regular
   grid.
-  You can right-click on the workspace and select:

   -  **Plot MD**: to perform a 1D plot of the signal in the workspace
      (only works on 1D MDHistoWorkspaces).
   -  **Show Slice Viewer**: to open the `Slice
      Viewer <http://www.mantidproject.org/MantidPlot:_SliceViewer>`__, which shows 2D slices of the
      multiple-dimension workspace.

Arithmetic Operations
---------------------

The following algorithms allow you to perform simple arithmetic on the
values:

-  :ref:`MinusMD <algm-MinusMD>`, :ref:`PlusMD <algm-PlusMD>`, :ref:`DivideMD <algm-DivideMD>`,
   :ref:`MultiplyMD <algm-MultiplyMD>`
-  :ref:`ExponentialMD <algm-ExponentialMD>`, :ref:`PowerMD <algm-PowerMD>`,
   :ref:`LogarithmMD <algm-LogarithmMD>`

These arithmetic operations propagate errors as described
`here <http://en.wikipedia.org/wiki/Propagation_of_uncertainty#Example_formulas>`__.
The formulas used are described in each algorithm's wiki page.

The basic arithmetic operators are available from python. For example:

| ``# Get two workspaces``
| ``A = mtd['workspaceA']``
| ``B = mtd['workspaceB']``
| ``# Creating a new workspace``
| ``C = A + B``
| ``C = A - B``
| ``C = A * B``
| ``C = A / B``
| ``# Modifying a workspace in-place``
| ``C += A``
| ``C -= A``
| ``C *= A``
| ``C /= A``
| ``# Operators with doubles``
| ``C = A * 12.3``
| ``C *= 3.45``

Compound arithmetic expressions can be made, e.g:

``E = (A - B) / (C - D)``

Boolean Operations
~~~~~~~~~~~~~~~~~~

The MDHistoWorkspace can be treated as a boolean workspace. In this
case, 0.0 is "false" and 1.0 is "true".

The following operations can create a boolean MDHistoWorkspace:

-  :ref:`LessThanMD <algm-LessThanMD>`, :ref:`GreaterThanMD <algm-GreaterThanMD>`,
   :ref:`EqualToMD <algm-EqualToMD>`

These operations can combine/modify boolean MDHistoWorkspaces:

-  :ref:`NotMD <algm-NotMD>`, :ref:`AndMD <algm-AndMD>`, :ref:`OrMD <algm-OrMD>`,
   :ref:`XorMD <algm-XorMD>`

These boolean operators are available from python. Make sure you use the
bitwise operators: & \| ^ ~ , not the "word" operators (and, or, not).
For example:

| ``# Create boolean workspaces by comparisons``
| ``C = A > B``
| ``D = B < 12.34``
| ``# Combine boolean workspaces using not, or, and, xor:``
| ``not_C = ~C``
| ``C_or_D = C | D``
| ``C_and_D = C & D``
| ``C_xor_D = C ^ D``
| ``C |= D``
| ``C &= D``
| ``C ^= D``

| ``# Compound expressions can be used:``
| ``D = (A > 123) & (A > B) & (A < 456)``

Using Boolean Masks
^^^^^^^^^^^^^^^^^^^

The :ref:`SetMDUsingMask <algm-SetMDUsingMask>` algorithm allows you to modify
the values in a MDHistoWorkspace using a mask created using the boolean
operations above. See the `algorithm wiki page <algm-SetMDUsingMask>`__ for
more details.



.. categories:: Concepts