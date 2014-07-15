.. _MDHistoWorkspace:

MDHistoWorkspace
================

The MDHistoWorkspace is a simple multi-dimensional workspace. In
contrast to the `MDEventWorkspace <MDEventWorkspace>`__, which contains
points in space, the MDHistoWorkspace consists of a signal and error
spread around space on a regular grid.

In a way, the MDHistoWorkspace is to a
`MDEventWorkspace <MDEventWorkspace>`__ is what the
`Workspace2D <Workspace2D>`__ is to the
`EventWorkspace <EventWorkspace>`__.

Creating a MDHistoWorkspace
---------------------------

MDHistoWorkspaces typically have 3 or 4 dimensions, although they can be
created in up to 9 dimensions.

-  You can bin a `MDEventWorkspace <MDEventWorkspace>`__ to a
   MDHistoWorkspace using the `BinMD <BinMD>`__ algorithm.

   -  You can use `CreateMDWorkspace <CreateMDWorkspace>`__ to create a
      blank MDEventWorkspace first, if you do not have data to bin.

-  `Paraview <Paraview>`__ and the `Vates Simple
   Interface <VatesSimpleInterface>`__ will create a MDHistoWorkspace
   from a `MDWorkspace <MDWorkspace>`__ when rebinning on a regular
   grid.

Viewing a MDHistoWorkspace
--------------------------

-  MDHistoWorkspaces can be created and visualized directly within
   `Paraview <Paraview>`__ and the `Vates Simple
   Interface <VatesSimpleInterface>`__ when rebinning along a regular
   grid.
-  You can right-click on the workspace and select:

   -  **Plot MD**: to perform a 1D plot of the signal in the workspace
      (only works on 1D MDHistoWorkspaces).
   -  **Show Slice Viewer**: to open the `Slice
      Viewer <MantidPlot:_SliceViewer>`__, which shows 2D slices of the
      multiple-dimension workspace.

Arithmetic Operations
---------------------

The following algorithms allow you to perform simple arithmetic on the
values:

-  `MinusMD <MinusMD>`__, `PlusMD <PlusMD>`__, `DivideMD <DivideMD>`__,
   `MultiplyMD <MultiplyMD>`__
-  `ExponentialMD <ExponentialMD>`__, `PowerMD <PowerMD>`__,
   `LogarithmMD <LogarithmMD>`__

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

-  `LessThanMD <LessThanMD>`__, `GreaterThanMD <GreaterThanMD>`__,
   `EqualToMD <EqualToMD>`__

These operations can combine/modify boolean MDHistoWorkspaces:

-  `NotMD <NotMD>`__, `AndMD <AndMD>`__, `OrMD <OrMD>`__,
   `XorMD <XorMD>`__

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

The `SetMDUsingMask <SetMDUsingMask>`__ algorithm allows you to modify
the values in a MDHistoWorkspace using a mask created using the boolean
operations above. See the `algorithm wiki page <SetMDUsingMask>`__ for
more details.



.. categories:: Concepts