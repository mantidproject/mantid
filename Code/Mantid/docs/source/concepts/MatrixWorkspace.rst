.. _MatrixWorkspace:

MatrixWorkspace
===============

What information is in a MatrixWorkspace
----------------------------------------

Mandatory:

-  Measured or derived data with associated errors

Optionally:

-  `Axes <Interacting_with_Workspaces#Axes>`__ with
   `Units <Unit Factory>`__
-  Sample and sample environment data
-  Run logs
-  A full `instrument <instrument>`__ geometric definition, along with
   an instrument parameter map
-  A spectra - detector map
-  A distribution flag
-  A list of 'masked' bins

Concrete Matrix Workspaces
--------------------------

-  WorkspaceSingleValue - Holds a single number (and X & error value, if
   desired). Mainly used for workspace algebra, e.g. to divide all bins
   in a 2D workspace by a single value.
-  `Workspace2D <Workspace2D>`__ - A workspace for holding two
   dimensional data in memory. This is the most commonly used workspace.
-  `EventWorkspace <EventWorkspace>`__ - A workspace that retains the
   individual neutron event data.

More information on working with them: `Interacting with Matrix
Workspaces <Interacting with Matrix Workspaces>`__.



.. categories:: Concepts