.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

InputWorskpace
##############

It can be either a MaskWorkspace, containing the masking information, or
a Data workspace (EventWorkspace or Workspace2D), having detectors
masked.

Optional MaskTableWorkspace
###########################

If the optional input 'MaskTableWorkspace' is given, it must be a table
workspace having the same format as output TableWorkspace such that it
contains 3 columns, XMin, XMax and DetectorIDsList.

The contents in this mask table workspace will be copied to output
workspace.

If a detector is masked in this input 'MaskTableWorkspace', and it is
also masked in input MaskWorkspace or data workspace, the setup (Xmin
and Xmax) in MaskTableWorkspace has higher priority, i.e., in the output
mask table workspace, the masked detector will be recorded in the row
copied from input MaskTableWrokspace.

.. categories::
