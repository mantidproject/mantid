.. _WorkbenchAlgorithmToolbox:

=================
Algorithm Toolbox
=================

.. image:: ../images/Workbench/AlgorithmListWidget/AlgorithmWidgetDiagram.png

The algorithm toolbox offers access to all of Mantid's algorithms.
If you are familiar with MantidPlot it functions similarly. The algorithms are
listed in categories and there is also a search bar.

Execute Button
--------------

The execute button will bring up the execution dialog for the selected
algorithm.

.. image:: ../images/Workbench/AlgorithmListWidget/AlgorithmExecuteDialog.png

From this window you can enter your input workspace, the parameters for your
selected algorithm and your target output workspace.


Algorithm Categories
--------------------

The algorithms are sorted into categories to make navigating easier. Below is a
the list of categories to help you find what you're looking for. To ensure the
algorithms are found in sensible places they can be in more than one category.

General Algorithms
^^^^^^^^^^^^^^^^^^
Arithmetic
        Generic mathematical operations on workspaces, including fast fourier transforms.
CorrectionFunctions
        Various correction functions including absorption and multiple scattering corrections.
DataHandling
        For loading and saving data from a wide variety of formats.
Deprecated
        Algorithms in this category are planned for removal in future releases, alternatives are in the algorithm pages.
Diagnostics
        Algorithms for performing diagnostics on workspaces.
Events
        Algorithms for manipulating event based data, including filtering, compressing and sorting event workspaces.
MDAlgorithms
        Algorithms for creating and manipulating multi-dimensional workspaces.
Optimization
        Support for fitting splines, peaks and other models to data. Including peak finding, as stripping peaks from data.
Remote
        Support for launching job on remote computing resources.
Sample
        Sample shape and material definition, manipulation and sample transmission calculation.
Simulation
        Algorithms to load and operate on data from external simulation packages.
Transforms
        Algorithms for transforming data and other workspace axes such as axes, grouping, masking rebinning, units etc.
Utility
        Various algorithms for operations like cloning, deleting, renaming and comparing workspaces, extracting monitors and generating python from a workspace.
Workflow
        Internal algorithms decribing complete, and partial workflows for data. Most of these are not planned for direct use by users.

Technique Specific Algorithms
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Crystal
        Algorithms specifically for single crystal experiments.
Diffraction
        Algorithms to support single crystal, powder and total diffraction.
Inelastic
        Algorithms to support direct and indirect neutron spectroscopy.
Muon
        Algorithms specifically for muon experiments.
Reflectometry
        Algorithms for reflectometry.
SANS
        Algorithms for SANS experiments.

Facility Specific Algorithms
^^^^^^^^^^^^^^^^^^^^^^^^^^^^
ILL
        Algorithms specific to the ILL facility
ISIS
        Algorithms specific to the ISIS facility
SINQ
        Algorithms specific to the SINQ facility