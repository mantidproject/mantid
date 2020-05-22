.. _04_pim_ex_2:

============================
Python in Mantid: Exercise 2
============================

The aim of this exercise is to create a general script that can accept user interaction to alter the parameters for future uses

Perform a set of algorithms by pointing and clicking with the MantidPlot menus
Use the history window to generate a script based upon the executed algorithms
Use the dialog functions to make the script more general for future runs.

.. contents:: Table of contents
    :local:


Using ISIS Data
===============

Running an analysis Manually
----------------------------

Here you are going to perform a number of steps in the Mantid user interface to correct some LOQ data by the incident beam monitor. For more information in a specific algorithm, click on its name.

#. :ref:`algm-Load` - Use the LOQ48097.raw file, set the OutputWorkspace to `Small_Angle` and set LoadMonitors to Separate.
#. :ref:`algm-ConvertUnits` - Convert the units for the Monitor workspace to wavelength with EMode=Elastic.
#. :ref:`algm-ConvertUnits` - Do the same for the small_angle data with EMode=Elastic.
#. :ref:`algm-Rebin` - Rebin the monitor from 2.2 to 10 in log steps of 0.035. To achieve this, the rebin params string will be "2.2,-0.035,10.0".
#. :ref:`algm-Rebin` - Do the same for the small_angle data.
#. :ref:`algm-ExtractSingleSpectrum` - Use WorkspaceIndex=1 on the Small_Angle_monitors workspace to pull out the 2nd monitor spectrum, putting the answer back in to the input workspace.
#. :ref:`algm-Divide` - Run divide with LHSWorkspace as the small_angle data workspace & RHSWorkspace as the monitor, into an OutputWorkspace called Corrected_data.

Generating a script from a workspace
------------------------------------

Once you have performed an analysis you can easily save it out as a script

#. Right-Click on the final result workspace, then select **Show History**
#. Click on the `Script to Clipboard` button
#. Paste the code into the Script Editor
#. Delete all the entries in the Workspace Toolbox
#. Run the script to check it works!


Using ILL Data
==============

Running an analysis Manually
----------------------------

Here you are going to perform a number of steps in the MantidPlot user interface to correct some IN6 data by the incident beam monitor. For more information in a specific algorithm, click on its name.

#. :ref:`algm-Load` - Use the 164198.nxs file, set the OutputWorkspace to data.
#. :ref:`algm-Integration` - Use WorkspaceIndex=0 on the data workspace to pull out the 1st monitor spectrum and integrate it over time.
#. :ref:`algm-Divide` - Run divide with LHSWorkspace as the data workspace & RHSWorkspace as the monitor, into an OutputWorkspace called data_norm.
#. :ref:`algm-ConvertUnits` - Convert the units for both the monitor and detector workspaces to DeltaE.
#. :ref:`algm-Rebin` - Rebin the workspaces in wavelength from -50 to 3 in linear steps of 0.1.

Generating a script from a workspace
------------------------------------

Once you have performed an analysis you can easily save it out as a script

#. Right-Click on the final result workspace, then select **Show History**
#. Click on the `Script to Clipboard` button
#. Paste the code into the Script Editor

Generalizing the Script
-----------------------

#. Extract the selected filename from the LoadILL command and print it to the screen with logger.information("message").
#. Print the Rebin binning to the screen with logger.information("message").


Using SNS Data
==============

Running an analysis Manually
----------------------------

Here you are going to perform a number of steps in the MantidPlot user interface to display the beam profile of EQ SANS

#. :ref:`algm-Load` - Load the file EQSANS_6071_event.nxs.
#. :ref:`algm-ConvertUnits` - Convert the units for both the monitor and detector workspaces to wavelength
#. :ref:`algm-Rebin` - Rebin both workspaces in wavelength from 2.5 to 5.5 in linear steps of 0.1
#. :ref:`algm-SumSpectra` - Sum up all the detectors to give the beam profile
#. :ref:`algm-Divide` - Normalize the rebinned and summed spectra in wavelength by the monitors

Generating a script from a workspace
------------------------------------
Once you have performed an analysis you can easily save it out as a script.

#. Right-Click on the final result workspace, then select **Show History**
#. Click on the `Script to File` button and save the file somewhere useful!
#. Open the script by selecting `File > Open Script` or use the keyboard shortcut Ctrl+O (or Cmd+O)
#. Delete all workspaces and Run the script to check it works!

Extra
-----

#. Extract the selected filename from the LoadNexus command (Hint: `.value` will return the value held by that property.) and print it to the screen with `logger.information("message")`.
#. Since rebin is performed twice, extract the parameters from the first rebin and use them in the second rebin.


