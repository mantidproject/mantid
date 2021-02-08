.. _sans_gui_testing:

SANS GUI Testing
================

.. contents::
   :local:

Data reduction
--------------

**Time required: about 1 hour**

--------------

Set up
######

#. Get the Training data from the downloads page on the Mantid Project website.
#. Open ``Interfaces`` > ``SANS`` > ``ISIS SANS``.
#. Click ``Manage Directories``. This opens Mantid's main dialog for managing
   search paths. Ensure the Training data directory is in the search
   directories.
#. If you haven't set one up yet, add a folder to save test data into later.
#. Click OK on the manage-directories dialog and ensure the ``Save Path`` on
   the SANS GUI displays the correct save path.
#. Click ``Load User File``; from the Training data in the ``loqdemo`` folder,
   choose ``MaskFile.toml``.
#. Click ``Load Batch File``; from the Training data in the ``loqdemo`` folder,
   choose ``batch_mode_reduction.csv``.

Runs table editing
##################

In the ``Runs`` tab:

#. Check that the ``Insert``, ``Delete``, ``Copy``, ``Paste``, ``Cut`` and
   ``Erase`` icons work as expected on table rows.
#. Tick the ``Sample Geometry`` button - some extra columns should appear.
#. Create multiple rows in the table with different data e.g. one row with
   an output name, one row without.
#. Try changing a random setting in the settings tab and remember what you set
   it to.
#. Make some more edits to the table. Check that the setting is still showing
   the value you changed it to, rather than the original.
#. Click the ``Export Table`` button and save the table as a csv file. Check
   the file in an editor or Excel and ensure it looks like a sensible
   representation of the table in the format ``key,value,key,value,...``. All
   columns except ``Options`` and ``Sample Shape`` should be included.
#. Try unticking ``Sample Geometry`` and ticking ``Multi-period``. The
   displayed columns change but the saved file should contain the same set of
   columns regardless of whether these are ticked.
#. Click ``Load Batch file`` and select the newly saved table. All columns
   that were saved should be loaded.
#. Try deleting and/or reordering some of the columns in the saved file and
   re-load it. All of the values in the file should be populated in the correct
   columns.
#. Check again that the setting you changed is still showing the value you
   changed it to, rather than the original.
#. Re-load the original batch file.

User files
##########

#. Change some values on the ``Settings`` and ``Beam Centre`` tabs.
#. Re-load the user file and check the values you changed - they should have
   reverted to their original values.
#. Ensure that you can load the old style ``MaskFile.txt`` user file from the
   sample data.
#. In the table on the ``Runs`` tab, under the ``User File`` column, enter
   ``MaskFile.toml`` in one row and ``MaskFile.txt`` in the other row. Click
   ``Process All``. After some seconds, the rows should turn green to indicate
   that they processed successfully.
#. Re-load the original user and batch files as per the set-up instructions.

Display mask
############

In the ``Settings`` tab:

#. Go to ``Mask``.
#. Click ``Display Mask``.
#. This should give an instrument view with a circle at the centre.
#. Go to ``Q, Wavelength, Detector Limits`` sub-tab.
#. Change the ``Phi Limit`` from 0 to 45 and uncheck ``use mirror sector``.
#. Go to ``Mask`` sub-tab.
#. Click ``Display Mask``.
#. This should give an instrument view where only angles 0-45 are unmasked.
#. Change the settings back to -90 - 90 and reselect ``use mirror sector``.

Processing
##########

*1D reduction*

#. In ``General, Scale, Event Slice, Sample`` sub-tab, ensure the ``Reduction
   Mode`` is ``All``.
#. In the ``Runs`` tab, under ``Save Options``, select ``Both``, and tick
   ``CanSAS (1D)`` and ``NXcanSAS (1D/2D)``.
#. Click ``Process All``.
#. After some seconds the rows should turn green.
#. In the workspaces list, there should be a series of new workspaces; four
   group workspaces and four 1D workspaces.
#. Check your default save directory - there should be two saved output files
   (an ``.xml`` and ``.h5`` file) for each main output as well as the
   unsubtracted can and sample workspaces, i.e. six output files for each row.
#. Double-click the 1D workspaces and you should get a single line plot.
#. Change the contents of the first cell in the first row to ``74045`` and click
   ``Process Selected``.
#. The row should turn blue; hovering over the row should give an error message.
#. Change the first column of the first row back to ``74044``.

*2D reduction*

#. On the ``Runs`` tab, under ``Reduction`` select ``2D``.
#. Under ``Save Options``, check that ``NXcanSAS (1D/2D)`` is still ticked, and
   that ``CanSAS (1D)`` is greyed out.
#. Tick the ``Plot Results`` box.
#. Click ``Process All``.
#. A plot window will open; initially empty, then with a line.
#. You should get four 2D workspaces instead of the previous 1D workspaces
   (they will have 100 spectra instead of 1). Double-click them and check you
   can do a colourfill plot.
#. Check your save directory. There should now only be a ``.h5`` file for each
   output.
#. Change ``Reduction`` back to 1D.
#. Click ``Process All``.
#. A new plot window should open and you should end up with multiple lines plotted.
#. Check the ``Multi-period`` box - six additional columns should appear in the table.
#. Delete all rows and re-load the batch file.

*Merged reduction*

#. In the ``Settings`` tab, ``General, Scale, Event Slice, Sample`` sub-tab,
   set ``Reduction Mode`` to ``Merged``.
#. Return to the ``Runs`` tab.
#. Ensure ``Plot results`` is ticked and that save outputs ``CanSAS (1D)`` and
   ``NXcanSAS (1D/2D)`` are ticked.
#. Click ``Process All``.
#. This should result in a plot with six lines.
#. The workspaces list should now contain a group named
   ``LAB_and_HAB_workspaces_from_merged_reduction`` that contains the ``main``
   and ``HAB`` workspaces, which were previously ungrouped for a non-merged
   reduction.
#. Check your save directory. As well as the previous 1D outputs, there should
   now be an additional ``.xml`` and ``.h5`` output file for the merged output
   for each row.
#. In the ``Settings`` tab, ``General, Scale, Event Slice, Sample`` sub-tab,
   change the ``Reduction Mode`` back to ``All``.

Beam centre finder
##################

In the ``Beam centre`` tab:

#. Both the ``Update main-detector`` and ``Update Hab`` checkboxes should be
   checked and enabled.
#. Click Run.
#. A plot should appear after some seconds, with four lines.
#. The four lines should gradually get closer together.
#. This will run for some time, probably minutes.

Sum runs
########

In the ``Sum Runs`` tab:

#. Enter ``74044, 74019`` in the top line.
#. Click ``Add`` at the side.
#. Enter ``LOQ74044-add`` as Output file.
#. Click ``Select Save Directory`` and select a directory in your managed paths.
#. Click ``Sum`` at the bottom.
#. Go back to the ``Runs`` tab.
#. Remove all rows.
#. Reload the batch file as before.
#. Change the first column of both rows to ``LOQ74044-add``.
#. Click ``Process All``.
#. This should now process as before.

Diagnostics
###########

In the ``Diagnostic Page`` tab:

#. For run choose ``Browse`` and load the ``LOQ74044.nxs`` file.
#. Click each of the ``Integral`` buttons.
#. They should produce plots.
#. Check the ``Apply Mask`` boxes and click the buttons again.
#. They should produce new, slightly different plots.

Display
#######

#. In the ``Runs`` tab, check that all table, process, and load buttons have
   clear tooltips by hovering over them.
#. Check that ``Zero Error Free`, ``Use Optimizations``, and ``Plot Results``
   have clear tooltips.
#. In all tabs, check the tooltips are still there by hovering over each box.
   Users rely on the tooltips a lot and really do notice each missing one.
