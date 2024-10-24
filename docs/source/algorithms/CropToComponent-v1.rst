
.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm takes a list of component names and an input workspaces and produces an output workspace which only contains the detectors which are part of the specified components. If no components are specified then the full workspace is returned.

The workspace allows users to select a specific bank for exclusive investigation in subsequent operations.

Usage
-----

**Example - CropToComponent**

.. testcode:: CropToComponentExample

    # Create sample workspace with four banks where each bank has 3x3 detectors
    sample_workspace = CreateSampleWorkspace(NumBanks=4, BankPixelWidth=3)

    # Crop to a component, we select bank2 here
    cropped_workspace = CropToComponent(InputWorkspace=sample_workspace, ComponentNames="bank2")

    # Check the number of histograms
    sample_number_of_histograms = sample_workspace.getNumberHistograms()
    cropped_number_of_histograms = cropped_workspace.getNumberHistograms()

    print("The original workspace has {0} histograms and the cropped workspace has {1} histograms.".format(sample_number_of_histograms, cropped_number_of_histograms))

Output:

.. testoutput:: CropToComponentExample

  The original workspace has 36 histograms and the cropped workspace has 9 histograms.

.. categories::

.. sourcelink::
