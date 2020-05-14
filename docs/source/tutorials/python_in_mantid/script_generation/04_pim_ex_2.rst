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

Here you are going to perform a number of steps in the MantidPlot user interface to correct some LOQ data by the incident beam monitor. For more information in a specific algorithm, click on its name.

#. :ref:`algm-Load` - Use the LOQ48097.raw file, set the OutputWorkspace to small_angle and set LoadMonitors to Separate, being careful to pass in string arguments where necessary.
#. :ref:`algm-ConvertUnits` - Convert the units for the Monitor workspace to wavelength with EMode=Elastic.
#. :ref:`algm-ConvertUnits` - Do the same for the small_angle data with EMode=Elastic.
#. :ref:`algm-Rebin` - Rebin the monitor from 2.2 to 10 in log steps of 0.035. To achieve this, the rebin params string will be "2.2,-0.035,10.0".
#. :ref:`algm-Rebin` - Do the same for the small_angle data.
#. :ref:`algm-ExtractSingleSpectrum` - Use WorkspaceIndex=1 on the small_angle_Monitors workspace to pull out the 2nd monitor spectrum, putting the answer back in to the input workspace.
#. :ref:`algm-Divide` - Run divide with LHSWorkspace as the small_angle data workspace & RHSWorkspace as the monitor, into an OutputWorkspace called Corrected data.

Generating a script from a workspace
------------------------------------

Once you have performed an analysis you can easily save it out as a script

#. Right-Click on the final result workspace, then select Show Workspace History;
#. Click on the Script to Clipboard button;
#. Open the script window in MantidPlot and paste in the code.

Generalizing the Script
-----------------------

#. Make the Load command display a dialog so the user can input their desired file, making sure the IncludeMonitors,SpectrumMin,SpectrumMax,SpectrumList parameters are disabled
#. Define a variable to hold the target of the ConvertUnits call so it is only defined once, i.e. a python string variable so that the target is only defined once in the script
#. In the first rebin - Allow the user to alter the rebin parameters, but provide them the ones you use as a default.
#. Capture the rebin parameters the user selected in the first rebin command and use the same values in the second.


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

#. Right-Click on the final result workspace, then select Show Workspace History.
#. Click on the GenerateScript Button.
#. Save the script file to disk (remember where you put it.)
#. Open the script window in MantidPlot and load the file you have just saved.

Generalizing the Script
-----------------------

#. Make the first Load command display a dialog so the user can input their desired file. Hint: After fetching the algorithm property, calling .value will return the value held by that property.
#. Extract the selected filename from the LoadNexus command and print it to the screen with logger.information("message").
#. In the rebin - Allow the user to alter the rebin parameters, but provide them the ones you use as a default.
#. Since rebin is performed twice, run the second rebin using the same parameters provided in the first rebin.
#. Add messages to the dialog boxes displayed for Rebin to tell the user what you want them to do with the additional Message parameter.


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

#. Right-Click on the final result workspace, then select Show Workspace History;
#. Click on the Script to Clipbpoard button;
#. Open the script window in MantidPlot and paste in the code.
#. Alternately, click on the GenerateScript Button.
#. Save the script file to disk (remember where you put it.)
#. Open the script window in MantidPlot and load the file you have just saved.

Generalizing the Script
-----------------------

#. Make the Load command display a dialog so the user can input their desired file, making sure the FilenameVanadium and WorkspaceVanadium parameters are disabled
#. Extract the selected filename from the LoadILL command and print it to the screen with logger.information("message").
#. In the rebin - Allow the user to alter the rebin parameters, but provide them the ones you use as a default.
#. Add messages to the dialog boxes displayed for Rebin to tell the user what you want them to do with the additional Message parameter.
#. Print the Rebin binning to the screen with logger.information("message").
