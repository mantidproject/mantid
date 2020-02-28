.. _04_Exercises_Plot:

=========
Exercises
=========


#. Open MantidPlot script Editor, File > "Open" and browse to the Python script "TempInGroup.py" found in the TrainingCourseData.
#. Run this by clicking Execute > "Execute All"
#. Note you have produced a WorkspaceGroup *TestGroup* that contains 10 workspaces with sample log values. Advanced plotting is used to compare multiple workspaces.


Exercise 1
==========

#. Right click on the workspace *TestGroup*
#. Select "Plot Advanced ..."
#. Enter Spectrum Number 6
#. Select "Plot Type" of "Waterfall"
#. In "Log value to plot against" select "Temp".
#. Press "OK"

and the Temperature log appears in the legend:

.. figure:: /images/PlotplotWaterfall.png
   :align: center
   :width: 500px


Exercise 2
==========

.. figure:: /images/ArtRightGUISurfaceCustomSq1.png
   :align: center
   :width: 250px

#. Again right-click on *TestGroup*, select Plot advanced and fill in as shown above
#. Press "OK"

Then one gets the following with custom log "Square" shown on an axis:

.. figure:: /images/ArtSurfacePlotCSq1.png
   :align: center
   :width: 500px

Note a Custom Log is like giving a number value to each Workspace within a Group



