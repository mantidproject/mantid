.. _sans_gui_testing:

SANS GUI Testing
================

.. contents::
   :local:

Data reduction
--------------

*Preparation*

-  Get the ISIS sample data from the website
-  Ensure that the ISIS sample data directory is in the search directories


**Time required 20 - 30 minutes**

--------------

#. Open ``Interfaces`` > ``SANS`` > ``SANS v2``
#. Set ``Instrument`` to ``LOQ``
#. Choose ``Load User File``; from the ISIS sample data in the ``loqdemo`` folder, choose ``Maskfile.txt``
#. Choose ``Load Batch File``; from the ISIS sample data in the `loqdemo` folder, choose ``batch_mode_reduction.csv``
#. In the ``Settings`` tab:
    - Go to ``Mask``
    - Click ``Display Mask``
    - This should give an instrument view with a circle at the centre
#. In the ``Runs`` tab
	- Click ``Process``
	- After some seconds the rows should turn green
	- In the Main window there should be a series of new workspaces; 3 group workspaces and 4 2D workspaces
	- Change the first column of the first row to 74045; click process
	- The row should turn blue; hovering over the row should give an error message
	- Change the ``Reduction`` button to 2D
	- Check the ``Plot Results`` box
	- Click ``Process``
	- A plot window will open; initially empty, then with a red line
	- Change ``Reduction`` back to 1D
	- Click ``Process``
	- In the plot you should end up with a red and a black line plotted
	- Check the ``Multi-period`` box
	- 6 additional rows should appear in the table
	- Check that the ``Add``, ``Delete``, ``Copy``, ``Paste``, ``Cut`` and ``Erase`` icons work as expected
	- Delete all rows and re-load the batch file as previously
#. In the ``Settings`` tab
	- Set ``Reduction Type`` to ``Merged``
	- Return to the ``Runs`` tab
	- Ensure ``Plot results`` is checked
	- Click ``Process``
	- This should result in a plot with three lines
#. In the ``Beam centre`` tab
	- Click 
	- A plot should appear after some seconds, with 4 lines
	- The 4 lines should gradually get closer together
	- This will run for some time, probably minutes
#. In the ``Sum Runs`` tab
		- Enter ``74044, 74019`` in the top line
		- Enter ``LOQ74044-add`` as Output file
		- Ensure that the ``Output directory`` is in you r managed paths
		- Click ``Sum`` at the bottom
		- Go back to the ``Runs`` tab
		- Remove all rows
		- Reload the batch file as before
		- Change the first column of both rows to ``LOQ74044-add``
		- Click ``Process``
		- This should now process as before
#. In the ``Diagnostic Page`` tab
	- For run choose ``Browse`` and load the ``LOQ74044.nxs`` file
	- Click each of the ``Integral`` buttons
	- They should produce plots
	- Check the ``Apply Mask`` boxes and click the buttons again
	- They should produce new, slightly different plots
#. In the ``Runs`` tab
	- Check all variables and inputs for tool tips by hovering over them