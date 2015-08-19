.. _MatrixWorkspace:

Matrix Workspace
================

What information is in a MatrixWorkspace
----------------------------------------

Mandatory:

-  Measured or derived data with associated errors

Optionally:

-  `Axes <http://www.mantidproject.org/Interacting_with_Workspaces#Axes>`__ with
   :ref:`Units <Unit Factory>`
-  Sample and sample environment data
-  Run logs
-  A full :ref:`instrument <instrument>` geometric definition, along with
   an instrument parameter map
-  A spectra - detector map
-  A distribution flag
-  A list of 'masked' bins

Documentation on the :ref:`CreateWorkspace <algm-CreateWorkspace>` 
algorithm may also be useful.

.. include:: WorkspaceNavigation.txt
   
More information on working with them: `Interacting with Matrix
Workspaces <http://www.mantidproject.org/Interacting_with_Workspaces>`__.



.. categories:: Concepts