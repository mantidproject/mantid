.. _Workspace2D:

===========
Workspace2D
===========

.. contents::
  :local:

The Workspace2D is a Mantid data type for a
:ref:`MatrixWorkspace <MatrixWorkspace>`.

It consists of a workspace with 1 or more spectra. Typically, each
spectrum will be a histogram. For example, you might have 10 bins, and
so have 11 X-value, 10 Y-values and 10 E-values in a workspace.

In contrast to an :ref:`EventWorkspace <EventWorkspace>`, a Workspace2D
only contains bin information and does not contain the underlying event
data. The :ref:`EventWorkspace <EventWorkspace>` presents itself as a
histogram (with X,Y,E values) but preserves the underlying event data.

For more information on what a Workspace2D contains, see 
:ref:`MatrixWorkspace <MatrixWorkspace>`.

Working with Workspace2Ds in Python
-----------------------------------

Workspace2D is a :ref:`MatrixWorkspace <MatrixWorkspace>` and does not offer any functionality above that of a Matrix Workspace.

Accessing Workspaces
####################

The methods for getting a variable to an EventWorkspace is the same as shown in the :ref:`Workspace <Workspace-Accessing_Workspaces>` help page.

If you want to check if a variable points to something that is a Workspace2D you can use this:

.. testcode:: CheckWorkspace2D

    histoWS = CreateSampleWorkspace()

    if histoWS.id() == "Workspace2D":
        print histoWS.name() + " is an " + histoWS.id()

Output:

.. testoutput:: CheckWorkspace2D
    :options: +NORMALIZE_WHITESPACE

    histoWS is an Workspace2D

.. include:: WorkspaceNavigation.txt



.. categories:: Concepts
