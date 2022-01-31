.. _sliceviewer_testing:

SliceViewer Testing
===================

.. contents::
   :local:

Introduction
------------

The Sliceviewer in Workbench has the joint functionality of the SpectrumViewer and SliceViewer from MantidPlot. So while the advanced use cases for multi-dimensional diffraction data are important to test, it is also worth checking more basic uses, for example opening a Workspace2D and examining the subplots and dynamic cursor data.

See here for a brief overview of the :ref:`sliceviewer`.

Basic Usage
-----------

.. _sliceviewer_testing_matrixws:
MatrixWorkspace
###############
Do the following tests with an EventWorkspace (e.g. ``CNCS_7860_event.nxs``) and a Workspace2D (e.g. ``MAR11060.raw``) from the `TrainingCourseData <https://download.mantidproject.org/>`_.
- Load the workspace and open in sliceviewer.
- Check the toolbar buttons:
    - Pan and Zoom (use mouse scroll and magnifying glass tool) and Home
        - As you zoom check the color scale changes in autoscale enabled in checkbox below colorbar).
    - Toggle grid lines on/off
    - Enable line plots
        - Confirm the curves update with the cursor position correctly
        - Export cuts to workspaces in ADS using keys `x`,`y`,`c`
            - This should produce workspaces in the ADS with suffix `_cut_x` and `_cut_y`
            - Plot these in workjbench check they agree with sliceviewer plots
    - Disable line plots and Enable :ref:`ROI tool<mantid:sliceviewer_roi>`

.. figure:: ../../../../docs/source/images/wb-sliceviewer51-roibutton.png
   :class: screenshot
   :width: 75%
   :align: center

        - The line plot button should be automatically enabled
        - Draw, move and resize the rectangle
        - Move it off the axes (it should just clip itself to be contained within the axes).
        - Disabling the ROI tool should automatically disable the line plot tool and the axes will be removed.
        - Export the cuts with keys - in addition the ROI can be exported by pressing `r`
            - This should produce another workspace with suffix `_roi`
            - Open it in sliceviewer and check the data and limits agree with the ROI drawn.
    - :ref`Peak overlay<mantid:sliceviewer_roi>` and :ref`Nonorthogonal view<mantid:sliceviewer_nonortho>` buttons should be disabled (greyed out).
    - Try saving the figure (with and without ROI/lineplots).
- Test the colorbar and colorscale
    - Change normalisation
        - The color limits should only change if autoscale is enabled.
    - Change the scale type to e.g. Log
        - In Log scale pixels with 0 counts shoudl appear white
        - When you zoom to a region with no data it will set the color axis limits to (0,0) and force the scale ot be linear
    - Change colormap
    - Reverse colormap
- Test the :ref:`Cursor Information Widget<mantid:sliceviewer_cursor>` (table at top of sliceviewer window with TOF, spectrum, DetID etc.)
    - Confirm it tracks with the cursor when Track Cursor is unchecked
    - Uncheck the track cursor and confirm it updates when the cursor is clicked.
- Test transposing axes
    - Click the Y button to the right of the Time-of-flight label (top left corner) - the image should transposed and the axes labels updated.
    - Repeat the test for the cursor info table.
- Resize the sliceviewer window, check the widgets, buttons etc. are still visible and clear for reasonable aspect ratios.

MD Workspaces
#############
MD workspaces are hold multi-dimensional data (typically 2-4D) and come in two forms: :ref:`MDEventWorkspace <MDWorkspace>`, :ref:`MDHistoWorkspace <MDHistoWorkspace>`.
In terms of sliceviewer functionality, the key difference is that MDHistoWorkspace have binned the events onto a regular grid and cannot be dynamically rebinned unless the original MDWorkspace
(that holds the events) exists in the ADS (and the MDHistoWorkspace has not been altered by a binary operation).

MDWorkspace (with events)
~~~~~~~~~~~~~~~~~~~~~~~~~
- Create a 3D and 4D MDWorkspaces with some data - repeat the following tests with both `md_4D` and `md_3D`

.. code-block:: python

	from mantid.simpleapi import *
    md_4D = CreateMDWorkspace(Dimensions=4, Extents=[-2,2,-1,1,-1.5,1.5,-0.25,0.25], Names="H,K,L,E", Frames='HKL,HKL,HKL,General Frame',Units='r.l.u.,r.l.u.,r.l.u.,meV')
    FakeMDEventData(InputWorkspace=md_4D, UniformParams='5e5') # 4D data
    tmp = CreateMDWorkspace(Dimensions=4, Extents=[-0.5,0.5,-1,-0.5,-1.5,-1, -0.25,0], Names="H,K,L,E", Frames='HKL,HKL,HKL,General Frame',Units='r.l.u.,r.l.u.,r.l.u.,meV')
    FakeMDEventData(InputWorkspace=tmp, UniformParams='1e5') # 4D data
    md_4D += tmp
    DeleteWorkspace(tmp)

    # Add a non-orthogonal UB
    expt_info = CreateSampleWorkspace()
    md_4D.addExperimentInfo(expt_info)
    SetUB(Workspace='md_4D', c=2, gamma=120)

    # make a 3D MDEvent workspace by integrating over all E
    md_3D = SliceMD(InputWorkspace='md_4D', AlignedDim0='H,-2,2,100', AlignedDim1='K,-1,1,100', AlignedDim2='L,-1.5,1.5,100')

    # Create a peaks workspace and fake data in 3D MD
    CreatePeaksWorkspace(InstrumentWorkspace='md_3D', NumberOfPeaks=0, OutputWorkspace='peaks')
    CopySample(InputWorkspace='md_3D', OutputWorkspace='peaks', CopyName=False, CopyMaterial=False, CopyEnvironment=False, CopyShape=False)
    AddPeakHKL(Workspace='peaks', HKL='1,0,1')
    AddPeakHKL(Workspace='peaks', HKL='1,0,0')

- Test the toolbar buttons pan, zoom, line plots, ROI as in :ref:`MatrixWorkspace<mantid:sliceviewer_testing_matrixws>`.
    - This workspace should be dynamically rebinned - i.e. the number of bins within the view limits along each axis should be preserved when zooming.
- Change the number of bins along one of the viewing axes (easier to pick a small number e.g. 2)
- Change the integration width along the non-viewed axes.
    - Increasing the width should improve the stats on the uniform background and the color limit should increase (event counts are summed not averaged).
- Change the slicepoint along one of the non-viewed axes
    - Confirm the slider moves when the spinbox value is updated.
    - Confirm moving the slider updates the spinbox.
- Test the :ref`Nonorthogonal view<mantid:sliceviewer_nonortho>`

.. figure:: ../../../../docs/source/images/wb-sliceviewer51-nonorthobutton.png
   :class: screenshot
   :align: center

    - Click the nonorthogonal view button in the toolbar
        - This should disable ROI and lineplot buttons in the toolbar
        - This should automatically turn on grid-lines
        - When H and K are the viewing axes the grid-lines should not be perpendicular to each other
        - The features in the data should align with the grid lines
        - Zoom and pan
        - Confirm the autoscaling of the colorbar works in non-orthogonal view
        - Change one of the viewing axes to be L (e.g. click `X` button next to L in top left of window)
            - Gridlines should now appear to be orthogonal
        - For `md_4D` only
            - Change one of the viewing axes to be 'E' (e.g. click `Y` button next to E in top left of window)
                - Nonorthogonal view should be disabled (only enabled for momentum axes)
                - Line plots and ROI should be enabled
            - Change the viewing axis presently selected as `E` to be a momentum axis (e.g. `H`)
                - The nonorthogonal view should be automatically re-enabled.
- Test the :ref`Peak Overlay<mantid:liceviewer_peaks_overlay>`

.. figure:: ../../../../docs/source/images/wb-sliceviewer51-peaksbutton.png
   :class: screenshot
   :align: center

    - Click to peak overlay button in the toolbar
    - Check the `Overlay?` box next to `peaks`
        - This should open a table on the RHS of the window - it should have a two rows corresponding to peaks at HKL = (1,0,1) and (1,0,0).
    - Double click a row
        - It should change the slicepoint along the integrated momentum axis and zoom into the peak - e.g. in (X,Y) = (H,K) then the slicepoint along L will be set to 1 and there will be a cross at (0,0).
        - Note for `md_4D` the cross should be plotted at all E (obviously a Bragg peak will only be on the elastic line but the peak object has no elastic/inelastic logic and the sliceviewer only knows that `E` is not a momentum axis, it could be temperature etc.).
    - Click Add Peaks in the Peak Actions section at the top of the peak viewer
    - Click somewhere in the colorfill plot
        - Confirm a peak has been added to the table at the position you clicked
    - Click Remove Peaks
    - Click on the cross coreesponding to the peak you just added
        - Confirm the correct row has been removed from the table
        - The cross should be removed from the plot
    - Repeat the above steps in non-orthogonal view.

MDHistoWorkspace
~~~~~~~~~~~~~~~~
- Make a 2D and 3D MDHistoWorkspaces
.. code-block:: python
    md_3D_histo = BinMD(InputWorkspace='md_4D', AlignedDim0='H,-2,2,100', AlignedDim1='K,-1,1,100', AlignedDim2='L,-1.5,1.5,100')
- Open `md_3D_histo` in sliceviewer it should not support dynamic rebinning (can't change number of bins).
- Test the toolbar buttons pan, zoom, line plots, ROI as in :ref:`MatrixWorkspace<mantid:sliceviewer_testing_matrixws>`.
- Test changing/swapping viewing axes
- Test the :ref`Nonorthogonal view<mantid:sliceviewer_nonortho>` as above
- Open `md_4D_svrebinned` in sliceviewer (should be in the ADS after preceding tests).
    - It should support dynamic rebinning (i.e. will be able to change number of bins along each axis).
- With `md_4D_svrebinned` open in the sliceviewer, delete `md_4D` in the ADS.
    - It should close sliceviewer because the support for dynamic rebinning has changed
- Open `md_4D_svrebinned` in sliceviewer again
    - It should no longer support dynamic rebinning.
    - Confirm transposing axes works


Specific Tests
--------------

1. Representation of integrated peaks
#####################################

- Run the code below to generate fake data and integrate peaks in the 3D MDWorkspace ``md_3D``
.. code-block:: python
    # Fake data in 3D MD and integrate
    FakeMDEventData(md_3D, EllipsoidParams='1e4,1,0,1,1,0,0,0,1,0,0,0,1,0.005,0.005,0.015,0', RandomSeed='3873875') # ellipsoid
    FakeMDEventData(md_3D, EllipsoidParams='1e4,1,0,0,1,0,0,0,1,0,0,0,1,0.005,0.005,0.005,0', RandomSeed='3873875')  # spherical
    IntegratePeaksMD(InputWorkspace='md_3D', PeakRadius='0.25', BackgroundInnerRadius='0.25', BackgroundOuterRadius='0.32', PeaksWorkspace='peaks', OutputWorkspace='peaks_int_ellip', IntegrateIfOnEdge=False, Ellipsoid=True, UseOnePercentBackgroundCorrection=False)
    IntegratePeaksMD(InputWorkspace='md_3D', PeakRadius='0.25', BackgroundInnerRadius='0.25', BackgroundOuterRadius='0.32', PeaksWorkspace='peaks', OutputWorkspace='peaks_int_sphere', IntegrateIfOnEdge=False, Ellipsoid=False, UseOnePercentBackgroundCorrection=False)
    IntegratePeaksMD(InputWorkspace='md_3D', PeakRadius='0.25', BackgroundInnerRadius='0', BackgroundOuterRadius='0', PeaksWorkspace='peaks', OutputWorkspace='peaks_int_no_bg', IntegrateIfOnEdge=False, Ellipsoid=False, UseOnePercentBackgroundCorrection=False)
    # IntegratePeaksMD will throw an error
    #   Error in execution of algorithm MaskBTP:...
    # This is because the simulated ws don't have a real instrument but the integration will be executed
- Open ``md_3D`` in sliceviewer
- Click the peak overlay button in the toolbar
- Overlay ``peaks_int_ellip`` and ``peaks_int_sphere``
- Click the first row in the first table
    - It should zoom to a peak.
    - There should be an ellipse and a circle drawn with dashed lines with different colors (the color should match the color of the workspace name in the peak viewer table).
    - There should be a transparent shell indicating the background for each peak.
    - The ellipse should be smaller than the circle.
- Alter the slice point by moving the slider along the integrated dimension
    - The circle and ellipse should shrink
    - There should be no gap between the background shell and the dashed line.
- Click on the second row on the second table.
    - It should zoom in on a different peak.
    - The ellipse and circle should be very similar (not quite same as the covariance matrix was evaluated numerically for randomly generated data).
- Click the nonorthogonal view button
- The ellipse and circle should still agree with each other and the shape of the generated data.
- Click the Peak overlay button in the toolbar
- Overlay the ``peaks_int_no_bg`` workspace and remove ``peaks_int_sphere``
- Zoom in on a peak (click a row in the table)
    - There should be a dashed line but no background shell for peaks in ``peaks_int_no_bg``

Keep the three peak workspaces overlain for the next test.

2. ADS observer for peak overlay
################################

- Rename ``peaks_int_ellip`` in the ADS to e.g. ``peaks_int_ellipse``
    - Confirm the name changes in the peak viewer table
    - Click on a peak, the ellipse should still be drawn on the colofill plot
- Remove a row from ``peaks_int_no_bg`` table (open table from ADS > Right-click on a row > Delete)
    - Confirm the correct row is removed from the corresponding row in the peak viewer table
    - Click on the peak in the ``peaks_int_ellipse`` table that has been removed from ``peaks_int_no_bg``
        - Only the ellipse should be plotted.
- Delete ``peaks_int_no_bg`` from the ADS
    - The table should be removed from the peaks viewer
    - Confirm the Peak actions combo box is updated to only contain ``peaks_int_ellipse``
- Delete ``peaks_int_ellipse``  from the ADS
    - The peak overlay should be turned off and the table hidden

3. ADS observer for workspace
#############################
With ``MD_3D`` open in sliceviewer
- Rename ``MD_3D`` to e.g. ``MD_3Dim``
    - The workspace name in the  title of the sliceviewer window should have updated
    - Zoom to check dynamic rebinning still works
- Take a note of the colorbar limits and execute this command in the ipython terminal ``mtd['md_3Dim'] *= 2``
    - The colorbar max should be doubled.
    - Zoom to check dynamic rebinning still works
- Clone the workspace for future tests - ``CloneWorkspace(InputWorkspace='MD_3Dim', OutputWorkspace='MD_3D')``
- Delete ``MD_3Dim`` in the ADS
    - The sliceviewer window should have closed

4. ADS observer for support for nonorthogonal view
##################################################
- Open ``MD_3D`` in sliceviewer
- Run ``ClearUB`` algorithm on ``MD_3D``
    - Sliceviewer window should close with message
``Closing Sliceviewer as the underlying workspace was changed: The property supports_nonorthogonal_axes is different on the new workspace.``


5. Check BinMD called with NormalizeBasisVectors=False for HKL data
###################################################################

- Create a workspace with peaks at integer HKL and take a non axis-aligned cut
.. code-block:: python
    ws = CreateMDWorkspace(Dimensions='3', Extents='-3,3,-3,3,-3,3',
                           Names='H,K,L', Units='r.l.u.,r.l.u.,r.l.u.',
                           Frames='HKL,HKL,HKL',
                           SplitInto='2', SplitThreshold='10')

    # add fake Bragg peaks for primitive lattice in data
    for h in range(-3,4):
        for k in range(-3,4):
            for l in range(-3,4):
                hkl = ",".join([str(x) for x in [h,k,l]])
                FakeMDEventData(ws, PeakParams='1e+02,' + hkl + ',0.02', RandomSeed='3873875')

    BinMD(InputWorkspace=ws, AxisAligned=False,
        BasisVector0='[00L],U,0,0,1',
        BasisVector1='[HH0],U,1,1,0',
        BasisVector2='[-HH0],U,-1,1,0',
        OutputExtents='-4,4,-4,4,-0.25,0.25',
        OutputBins='101,101,1', OutputWorkspace='BinMD_out', NormalizeBasisVectors=False)

- Open ``BinMD_out`` in sliceviewer.
    - There should be peaks at integer HKL
- Zoom in (so that the dat are rebinned)
    - The peaks should still be at integer HKL (rather than multiples of sqrt(2))

6. Check gets the correct basis vectors for MDHisto workspaces
##############################################################

This tests that the sliceviewer gets the correct basis vectors for an ``MDHisto`` object from a non-axis aligned cut.

- Create the workspace
.. code-block:: python

    ws = CreateMDWorkspace(Dimensions='3', Extents='-3,3,-3,3,-3,3',
                       Names='H,K,L', Units='r.l.u.,r.l.u.,r.l.u.',
                       Frames='HKL,HKL,HKL',
                       SplitInto='2', SplitThreshold='10')
    expt_info = CreateSampleWorkspace()
    ws.addExperimentInfo(expt_info)
    SetUB(ws, 1,1,2,90,90,120)
    BinMD(InputWorkspace=ws, AxisAligned=False,
        BasisVector0='[00L],r.l.u.,0,0,1',
        BasisVector1='[HH0],r.l.u.,1,1,0',
        BasisVector2='[-HH0],r.l.u.,-1,1,0',
        OutputExtents='-4,4,-4,4,-0.25,0.25',
        OutputBins='101,101,1', OutputWorkspace='ws_slice', NormalizeBasisVectors=False)

- Open ``ws_slice`` in the sliceviewer.
    - The non-orthogonal view should be enabled (not greyed out).
- Click the non-orthogonal view button
    - Rectangular gridlines should appear (in this case 110 is orthogonal to 001).

6. Check non-orthogonal view is disabled for non-Q axes
#######################################################
Check that the non-orthogonal view is disabled for non-Q axes such as energy

- Create a workspace with energy as the first axis.
.. code-block:: python

    ws_4D = CreateMDWorkspace(Dimensions=4, Extents=[-1, 1, -1, 1, -1, 1, -1, 1], Names="E,H,K,L",
                                  Frames='General Frame,HKL,HKL,HKL', Units='meV,r.l.u.,r.l.u.,r.l.u.')
    expt_info_4D = CreateSampleWorkspace()
    ws_4D.addExperimentInfo(expt_info_4D)
    SetUB(ws_4D, 1, 1, 2, 90, 90, 120)

- Confirm that when the Energy axis is viewed (as X or Y) the non-orthogonal view is disabled.
- The button should be re-enabled when you view two Q-axes e.g. H and K.
