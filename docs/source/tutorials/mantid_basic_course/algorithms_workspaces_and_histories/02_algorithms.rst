.. _02_algorithms:


==========
Algorithms
==========

.. raw:: html

    <style> .red {color:#FF0000; font-weight:bold} </style>
    <style> .green {color:#008000; font-weight:bold} </style>
    <style> .blue {color:#0000FF; font-weight:bold} </style>
    <style> .orange {color:#FF8C00; font-weight:bold} </style>

.. role:: red
.. role:: blue
.. role:: green
.. role:: orange

.. figure:: /images/DemoLocationAlgorithmsPanel_MBC.png
   :alt: DemoLocationAlgorithmsPanel_MBC.png#
   :align: right
   :width: 500px


Algorithms are the verbs of Mantid - they are the "doing" objects. If
you want to manipulate your data in any way it will be done through an
algorithm. Algorithms operate primarily on data in workspaces. They will
normally take one or more workspaces as an input, perform some
processing on them and provide an output as another workspace (although
it is possible to have multiple outputs).

In Mantid Workbench, your primary source for Algorithms is the Algorithm Toolbox
shown on the right. The panel should be visible by default on the bottom
left of the Workbench window. You may have to click the Algorithm Tab on the very bottom corner of the main window.

Find an Algorithm by looking through the :red:`Categories available` or :green:`Searching for its name`.

In the :green:`search box`, start writing the algorithm name, and it will
filter algorithms by name as you continue typing.

Bring up the relevant Algorithm Dialog by either clicking :green:`"Execute" on a searched Algorithm` or
:red:`double-clicking on its name in the category box`.

.. figure:: /images/MBC_AlgorithmDialog.png
   :alt: MBC_AlgorithmDialog.png
   :align: right
   :width: 500px

Algorithm Help
--------------

Once you have an algorithm selected, you can get additional information
such as what it does, its inputs, outputs, and how to use it.
Click the :blue:`Help button [?]` on the algorithm dialog box for information
specific to this algorithm in our documentation.

Validation of Inputs
--------------------

Many algorithms have mandatory input properties. Others have constraints
on the inputs, such as numbers that are bounded. The red asterisk `*` indicates
required or incorrect inputs.  Hover over the asterisk to see
what is wrong with the input.

Input workspaces are also validated against. If the workspace type does
not match the validator of the input workspace for the algorithm it will
not appear as a possible workspace to use as an input.

Overwriting InputWorkspace
--------------------------

The :orange:`Match button` next to the OutputWorkspace entry will name
the OutputWorkspace the same as the InputWorkspace, overwriting it in the process.
Be careful that this is what you want to do. It can be a very useful tool!

Running Algorithms
------------------

On the Algorithm dialog, the run button will execute the algorithm. If
successful, new output workspaces will appear in the Workspace Toolbox.
Algorithm outputs are sent to the Messages Box, on the right of the main
Mantid Workbench window. If the
algorithm fails to complete, the error should appear in the Messages Box.
