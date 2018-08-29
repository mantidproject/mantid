.. _02_algorithms:

==========
Algorithms 
==========

Algorithms
==========

.. figure:: /images/DemoLocationAlgorithmsPanel_MBC.PNG
   :alt: DemoLocationAlgorithmsPanel_MBC.PNG

Algorithms are the verbs of Mantid - they are the "doing" objects. If
you want to manipulate your data in any way it will be done through an
algorithm. Algorithms operate primarily on data in workspaces. They will
normally take one or more workspaces as an input, perform some
processing on them and provide an output as another workspace (although
it is possible to have multiple outputs).

In MantidPlot, your primary source for Algorithms is the Algorithm panel
shown on the right: The panel should be visible by default on the bottom
right of the MantidPlot window. It can be shown/hidden by choosing
**View -> Algorithms** option from the MantidPlot top menu. The panel
may also be re-located and re-docked by dragging the top bar with the
left mouse button held down.

| The algorithms can either be navigated by category (see green
  highlighted section), or searched by name (red highlighted section).
  In the search box, start writing the algorithm name, and it will
  predict the name of the algorithm as you continue typing.
| |MBC_AlgorithmDialog.png|

Algorithm Help
--------------

Once you have an algorithm selected, you can get additional information
about what it does, what it takes as inputs, what it provides as
outputs, and how to use it. This is achieved via the help button on the
algorithm dialog box that appears once the algorithm is selected. The
help button shows up as a **?** and will take you to the documentation
page specific to that algorithm.

Validation of Inputs
--------------------

Many algorithms have mandatory input properties. Others have constraints
on the inputs, such as numbers that are bounded. The red asterisk next
to some of the input properties indicate that they must be provided
before the algorithm can run. Hovering over the asterisk will tell you
what is wrong with the input.

Input workspaces are also validated against. If the workspace type does
not match the validator of the input workspace for the algorithm it will
not appear as a possible workspace to use as an input.

Overwriting InputWorkspace
--------------------------

The button next to the OutputWorkspace will populate the OutputWorkspace
property with the same name as that of the InputWorkspace. This will
mean that the new OutputWorkspace will overwrite the original input
workspace.

Running Algorithms
------------------

On the Algorithm dialog, the run button will execute the algorithm. If
successful, new output workspaces will appear in the Workspace list
panel. Algorithm outputs are sent to the Results Log panel. If the
algorithm fails to complete, the reason should also appear in the same
Results Log panel.

.. raw:: mediawiki

   {{SlideNavigationLinks|MBC_Algorithms_History_EventWorkspace|Mantid_Basic_Course|MBC_Workspaces}}

.. |MBC_AlgorithmDialog.png| image:: /images/MBC_AlgorithmDialog.png
   :width: 300px
