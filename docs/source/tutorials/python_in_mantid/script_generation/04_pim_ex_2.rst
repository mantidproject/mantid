.. _04_pim_ex_2:

============================
Python in Mantid: Exercise 2
============================

The aim of this exercise is to create a general script that can accept user interaction to alter the parameters for future uses

Perform a set of algorithms by pointing and clicking within Mantid
Use the history window to generate a script based upon the executed algorithms
Use the dialog functions to make the script more general for future runs.

.. contents:: Table of contents
    :local:



A - Processing ISIS Data
========================

Running an analysis Manually
----------------------------

Here you are going to perform a number of steps in the Mantid user interface to correct a LOQ dataset using the incident beam monitor. For more information in a specific algorithm, click on its name.

#. :ref:`algm-Load` - Use the LOQ48097.raw file, set the OutputWorkspace to `Small_Angle` and set LoadMonitors to Separate.
#. :ref:`algm-ConvertUnits` - Convert the units for the Monitor workspace to wavelength with EMode=Elastic.
#. :ref:`algm-ConvertUnits` - Do the same for the small_angle data with EMode=Elastic.
#. :ref:`algm-Rebin` - Rebin the monitor from 2.2 to 10 in log steps of 0.035. To achieve this, the rebin params string will be "2.2,-0.035,10.0".
#. :ref:`algm-Rebin` - Do the same for the small_angle data.
#. :ref:`algm-ExtractSingleSpectrum` - Use WorkspaceIndex=1 on the Small_Angle_monitors workspace to pull out the 2nd monitor spectrum, putting the answer back into the input workspace.
#. :ref:`algm-Divide` - Run divide with LHSWorkspace as the small_angle data workspace & RHSWorkspace as the monitor, into an OutputWorkspace called Corrected_data.

Generating a script from a workspace
------------------------------------

Once you have performed an analysis you can easily save it out as a script

#. Right-Click on the final result workspace, then select **Show History**
#. Click on the `Script to Clipboard` button
#. Paste the code into the Script Editor
#. Delete all the entries in the Workspace Toolbox
#. Run the script to check it works!

One step further
----------------

#. Extract the binning params from the first Rebin, print them using the logger, AND use them as the input for the second Rebin
#. Add comments to your script to explain what it does!


B - Plotting ILL Data
=====================

Load and Plot
-------------

#. :ref:`algm-Load` the file `164198.nxs`
#. Double-click on the main data workspace *164198*
#. Produce a normal plot of the spectrum numbers: 50,100,200,300

Edit in Options Menu
--------------------

#. Click on the Gear icon to open the :ref:`Options menu <06_formatting_plots>`
#. In the Axes tab... Change the title to "My Beautiful Plot"
#. Set the **x-limits** to 460-600, the **y-scale** to `log` and the **y-limits** to 1-2000
#. In the Curves tab... Select spectrum 50 and give this curve "a funky label"
#. ... for the re-labelled curve, un-hide the error bars and set Capsize = 1.0
#. ... Select spectrum 300 and change the line color to black
#. Click Apply

Generating a script from a plot
-------------------------------

#. Click the generate a script button |GenerateAScript.png| and copy&paste the script into the script editor
#. Close your beautiful plot and run the script to re-generate it!


C - Processing and Plotting SNS Data
====================================

Running an analysis Manually
----------------------------

#. :ref:`algm-Load` - Load the file EQSANS_6071_event.nxs.
#. :ref:`algm-ConvertUnits` - Convert the units for both the monitor and detector workspaces to wavelength
#. :ref:`algm-Rebin` - Rebin both workspaces in wavelength from 2.5 to 5.5 in linear steps of 0.1
#. :ref:`algm-SumSpectra` - Sum up all the detectors to give the beam profile
#. :ref:`algm-Divide` - Normalize the rebinned and summed spectra in wavelength by the monitors

Generating a script from a workspace
------------------------------------
Once you have performed an analysis you can easily save it out as a script.

#. Right-Click on the final result workspace *normalized*, then select **Show History**
#. Click on the `Script to File` button and save the file somewhere useful!
#. Open the script by selecting `File > Open Script` or use the keyboard shortcut Ctrl+O (or Cmd+O)
#. Delete all workspaces and Run the script to check it works!

Plot and Edit
-------------

#. Double-click on the *normalized* spectrum to plot it
#. Right-click on *run_monitors_lambda_rebinned* and select Plot > Overplot Spectrum.
#. Also, overplot with *run_lambda_summed*
#. In the Options menu (Gear icon)
#. Set the **x-upper-limit** to 4.5
#. Set the **y-scale** to log
#. Click Apply

Generating a script from a plot
-------------------------------

#. Click the generate a script button |GenerateAScript.png| and select `Script to Clipboard`
#. Paste the code on the end of the script for processing the data

#. **Delete all workspaces and plots and run the script to reprocess and plot the data**


:ref:`Solutions <02_pim_sol>`

.. |GenerateAScript.png| image:: /images/GenerateAScript.png
   :width: 30px
