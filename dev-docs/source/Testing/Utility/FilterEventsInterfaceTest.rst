.. _filter_event_testing:

Filter Events Interface Testing
===============================

.. contents::
   :local:

*Prerequisites*

- Download the `Usage Examples <https://www.mantidproject.org/installation/index#sample-data>`


Filter Events
--------------


**Time required: 10-15 minutes**

--------------

#. Open ``Interfaces`` > ``Utility`` > ``Filter Events``
#. Browse for file 'CNCS_7860_Event.nxs' from the Usage Data set.
#. Click ``Load``. After a few seconds, plot should be updated with a graph of summed up Counts vs.Time.
#. ``_Summed_..`` workspace should be generated on the ADS, as well as ``CNCS_7860_Event`` workspace.
#. Clicking on the ``Refresh`` button should update the drop-down list with the name of the loaded workspace.
#. Check Vertical Range Sliders work properly. You can't cross the sliders.
#. Check Horizontal Range Sliders work properly. You can't cross the sliders.
#. Check that the Text Edits for ``Starting Time`` and ``Stopping Time`` work correctly:

   - Position of vertical bars on the plot are updated appropriately when changing the numerical value of the edits and pressing `Enter`.
   - Setting a value on ``Starting Time`` larger than the current value on ``Stopping Time`` is not allowed, and viceversa.
   - Data Validation is working, you can't set non-numeric characters on the edits.
   - A red label with information is displayed on top of the plot whenever the input validation is wrong.

#. Clicking anywhere on the plot raises a dialog with the `(x,y)` position clicked on the graph.
#. On ``Output Name`` write `FilteredTemp`.
#. On ``Sample Log`` drop-down menu select ``SampleTemp`` and hit on ``Plot`` button. Plot should update with the temperature log for the run vs. time.
#. Moving vertical range sliders updates ``Minimum Value`` and ``Maximum Value`` text edits.
#. Move the upper vertical range slider to be at approximately ``279.95`` degrees and the lower vertical range slider to be at approximately ``279.91`` degrees.
#. Click on ``Filter`` button. That should generate several workspaces in the ADS. Two table workspaces ending with ``_info``, ``_splitters`` and a group workspace named ``FilteredTemp`` containing one
   event workspace named ``FilteredTemp_0``, as well as a ``TOFCorrTable`` 2D workspace.
#. Right-click on ``FilteredTemp_0`` and select ``Show Sample Logs``. On Sample Log Window, check the ``SampleTemp`` entry and make sure the temperature range is approximately
   the same as selected with the sliders in the interface.
#. Back to the filter events interface, click on the ``Refresh`` button. The drop-down list should refresh with the available event workspaces on the ADS (``CNCS_7860_Event`` and ``FilteredTemp_0``).
#. Select ``FilteredTemp_0`` and click on ``Use`` button. Plot should update accordingly.
#. On output name write `FilteredTime`, on ``Starting Time`` text edit write ``80`` and then press ``Enter``. On ``Stopping Time`` text edit write ``100`` and press ``Enter``.
#. Select ``Filtered by Time`` tab and on ``Time Interval`` text edit write ``10``. Then click on ``Filter`` button. Two table workspaces ending with ``_info``, ``_splitters`` and a group workspace named ``FilteredTime`` containing two
   event workspaces named ``FilteredTime_0`` and ``FilteredTime_1`` appear on ADS.
#. Double-click on ``FilteredTemp_0_info`` table workspace. The table should contain two rows, indicating two time intervals of approximately 10 seconds each, and the corresponding workspace group index.
#. Back to the filter events interface, without changing anything else from previous interaction, go to ``Advanced Setup`` tab and tick the ``Fast Log`` checkbox. Go back to ``Filter by Time`` and click
   on ``Filter`` button again. Workspace ending in ``_splitters`` should be a 2D workspace, all other workspaces should remain the same.
#. Now on ``Algorithms`` select ``FilterEvents`` algorithm and click on ``Execute``. As ``InputWorkspace`` select ``FilteredTemp_0`` workspace. On Splitter workspace select ``FilteredTemp_0_splitters``,
   and on ``Information Workspace`` select ``FilteredTemp_0_info``. Tick on ``GroupWorkspaces`` checkbox. And name the ``OutputWorkspaceBaseName`` as ``FilteredTimeRedux``.
#. ``Run`` the algorithm  with ``Group Workspaces`` checked. On the ADS, a new group event workspace named ``FilteredTimeRedux`` should appear. The two event workspaces that it contains should be equal to the two event workspace previuosly generated
   through the interface, ``FilteredTime_0`` and ``FilteredTime_1``.
#. Back to the Filter Events interface. Click on ``Refresh`` button again, and make sure it updates with the list of all event workspaces on the ADS.
