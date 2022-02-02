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

.. _toolbar-checklist:

MatrixWorkspace
###############

Do the following tests with an EventWorkspace (e.g. ``CNCS_7860_event.nxs``) and a Workspace2D (e.g. ``MAR11060.raw``) from the `TrainingCourseData <https://download.mantidproject.org/>`_.

1. Load the workspace and open in sliceviewer (right-click on the workspace in the ADS > Show Slice Viewer).
2. Confirm that for MatrixWorkspaces the :ref:`Peak overlay<mantid:sliceviewer_roi>` and :ref:`Nonorthogonal view<mantid:sliceviewer_nonortho>` buttons should be disabled (greyed out).
3. Check the other toolbar buttons.

    a. Pan and Zoom (use mouse scroll and magnifying glass tool) and Home

        * As you zoom check the color scale changes if autoscale checked in checkbox below colorbar).

    b. Toggle grid lines on/off
    c. Enable line plots

        * Confirm the curves update with the cursor position correctly

    d. Export cuts to workspaces in ADS using keys `x`, `y`, `c`

        * This should produce workspaces in the ADS with suffix `_cut_x` and `_cut_y`
        * Plot these in workbench check they agree with sliceviewer plots

    e. Disable line plots and Enable :ref:`ROI tool<mantid:sliceviewer_roi>`

    .. figure:: ../../../../docs/source/images/wb-sliceviewer51-roibutton.png
       :class: screenshot
       :width: 50%
       :align: center

        * The line plot button should be automatically enabled

    f. Draw, move and resize the rectangle

        * Move it off the axes (it should just clip itself to be contained within the axes).
        * Export the cuts with keys `x`, `y`, `c`
        * In addition the ROI can be exported by pressing `r`

            - This should produce another workspace with suffix `_roi`
            - Open it in sliceviewer and check the data and limits agree with the ROI drawn.

    g.  Disable the ROI tool

        * The line plot tool should remain enabled.

4. Try saving the figure (with and without ROI/lineplots).
5. Test the colorbar and colorscale

    a. Change normalisation

        * The color limits should only change if autoscale is enabled.

    b. Change the scale type to e.g. Log

        * In Log scale bins with 0 counts should appear white
        * When you zoom in to a region comprising only of bins with 0 counts it will set the color axis limits to (0,0) and force the scale to be linear
        * Zoom in to a region outside the extent of the data, check the Log colorscale option is disabled.

    c. Change colormap
    d. Reverse colormap

6. Test the :ref:`Cursor Information Widget<mantid:sliceviewer_cursor>` (table at top of sliceviewer window with TOF, spectrum, DetID etc.)

    a. Confirm it tracks with the cursor when Track Cursor is unchecked
    b. Uncheck the track cursor and confirm it updates when the cursor is clicked.

7. Test transposing axes

    a. Click the Y button to the right of the Time-of-flight label (top left corner) - the image should be transposed and the axes labels updated.
    b. Repeat the test for the cursor info table.

8. Resize the sliceviewer window, check the widgets, buttons etc. are still visible and clear for reasonable aspect ratios.

MD Workspaces
#############

MD workspaces hold multi-dimensional data (typically 2-4D) and come in two forms: :ref:`MDEventWorkspace <MDWorkspace>`, :ref:`MDHistoWorkspace <MDHistoWorkspace>`.
In terms of sliceviewer functionality, the key difference is that MDHistoWorkspace have binned the events onto a regular grid and cannot be dynamically rebinned unless the original MDWorkspace
(that holds the events) exists in the ADS (and the MDHistoWorkspace has not been altered by a binary operation e.g. ``MinusMD``).

MDWorkspace (with events)
~~~~~~~~~~~~~~~~~~~~~~~~~
1. Create a 3D and 4D MDWorkspaces with some data - repeat the following tests with both ``md_4D`` and ``md_3D``

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

2. Test the toolbar buttons pan, zoom, line plots, ROI as in step 3 of the :ref:`toolbar-checklist` instructions.

    - This workspace should be dynamically rebinned - i.e. the number of bins within the view limits along each axis should be preserved when zooming.

3. Change the number of bins along one of the viewing axes (easier to pick a small number e.g. 2)
4. Change the integration-width/slice-thickness (spinbox to the left of the word `thick`) along the non-viewed axes.

    - Increasing the width should improve the stats on the uniform background and the color limit should increase (event counts are summed not averaged).

5. Change the slicepoint along one of the non-viewed axes

    a. Confirm the slider moves when the spinbox value is updated.
    b. Confirm moving the slider updates the spinbox.

Test the :ref:`Nonorthogonal view<mantid:sliceviewer_nonortho>`

.. figure:: ../../../../docs/source/images/wb-sliceviewer51-nonorthobutton.png
   :class: screenshot
   :align: center

1. Click the nonorthogonal view button in the toolbar

    - This should disable ROI and lineplot buttons in the toolbar
    - This should automatically turn on gridlines
    - When H and K are the viewing axes the gridlines should not be perpendicular to each other
    - The features in the data should align with the grid lines

2. Zoom and pan

    - Confirm the autoscaling of the colorbar works in non-orthogonal view

3. Change one of the viewing axes to be `L` (e.g. click `X` button next to L in top left of window)

    - Gridlines should now appear to be orthogonal

4. For ``md_4D`` only change one of the viewing axes to be `E` (e.g. click `Y` button next to `E` in top left of window)

    - Nonorthogonal view should be disabled (only enabled for momentum axes)
    - Line plots and ROI should be enabled
    - Change the viewing axis presently selected as `E` to be a momentum axis (e.g. `H`)

            - The nonorthogonal view should be automatically re-enabled.

Test the :ref:`Peak Overlay<mantid:sliceviewer_peaks_overlay>`

.. figure:: ../../../../docs/source/images/wb-sliceviewer51-peaksbutton.png
   :class: screenshot
   :align: center

1. Click to peak overlay button in the toolbar
2. Check the `Overlay?` box next to ``peaks``

    - This should open a table (peak viewer) on the RHS of the sliceviewer window - it should have two rows corresponding to peaks at HKL = (1,0,1) and (1,0,0).

3. Double click a row

    - It should change the slicepoint along the integrated momentum axis and zoom into the peak - e.g. in (X,Y) = (H,K) then the slicepoint along L will be set to 1 and there will be a cross at (0,0).
    - Note for ``md_4D`` the cross should be plotted at all E (obviously a Bragg peak will only be on the elastic line but the peak object has no elastic/inelastic logic and the sliceviewer only knows that `E` is not a momentum axis, it could be temperature etc.).

4. Click Add Peaks in the Peak Actions section at the top of the peak viewer
5. Click somewhere in the colorfill plot

    - Confirm a peak has been added to the table at the position you clicked

6. Click Remove Peaks
7. Click on the cross corresponding to the peak you just added

    - Confirm the correct row has been removed from the table
    - The cross should be removed from the plot

8. Repeat the above steps 1-7 in non-orthogonal view.

MDHistoWorkspace
~~~~~~~~~~~~~~~~

1. Make a 3D MDHistoWorkspace

.. code-block:: python

    md_3D_histo = BinMD(InputWorkspace='md_4D', AlignedDim0='H,-2,2,100', AlignedDim1='K,-1,1,100', AlignedDim2='L,-1.5,1.5,100')

2. Open ``md_3D_histo`` in sliceviewer it should not support dynamic rebinning (can't change number of bins).
3. Test the toolbar buttons pan, zoom, line plots, ROI as in step 3 of the :ref:`toolbar-checklist` instructions.
4. Test changing/swapping viewing axes
5. Test the :ref:`Nonorthogonal view<mantid:sliceviewer_nonortho>` as above
6. Open ``md_4D_svrebinned`` in sliceviewer (should be in the ADS after preceding tests).

    - It should support dynamic rebinning (i.e. will be able to change number of bins along each axis).

7. With ``md_4D_svrebinned`` open in the sliceviewer, delete ``md_4D`` in the ADS.

    - It should close sliceviewer because the support for dynamic rebinning has changed

8. Open ``md_4D_svrebinned`` in sliceviewer again

    - It should no longer support dynamic rebinning
    - Confirm transposing axes works


Specific Tests
--------------

1. Representation of integrated peaks
#####################################

1. Run the code below to generate fake data and integrate peaks in the 3D MDWorkspace ``md_3D``

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

2. Open ``md_3D`` in sliceviewer
3. Click the peak overlay button in the toolbar
4. Overlay ``peaks_int_ellip`` and ``peaks_int_sphere``
5. Click the first row in the first table

    - It should zoom to a peak.
    - There should be an ellipse and a circle drawn with dashed lines with different colors (the color should match the color of the workspace name in the peak viewer table).
    - There should be a transparent shell indicating the background for each peak.
    - The ellipse should be smaller than the circle.

6. Alter the slice point by moving the slider along the integrated dimension

    - The circle and ellipse should shrink
    - There should be no gap between the background shell and the dashed line.

7. Click on the second row on the second table.

    - It should zoom in on a different peak.
    - The ellipse and circle should be very similar (not quite same as the covariance matrix was evaluated numerically for randomly generated data).

8. Click the nonorthogonal view button
9. The ellipse and circle should still agree with each other and the shape of the generated data.
10. Click the Peak overlay button in the toolbar
11. Overlay the ``peaks_int_no_bg`` workspace and remove ``peaks_int_sphere``
12. Zoom in on a peak (click a row in the table)

    - There should be a dashed line but no background shell for peaks in ``peaks_int_no_bg``

Keep the three peak workspaces overlain for the next test.

2. ADS observer for peak overlay
################################

1. Rename ``peaks_int_ellip`` in the ADS to e.g. ``peaks_int_ellipse``

    a. Confirm the name changes in the peak viewer table
    b. Click on a peak, the ellipse should still be drawn on the colofill plot

2. Remove a row from ``peaks_int_no_bg`` table (open table from ADS > Right-click on a row > Delete)

    a. Confirm the correct row is removed from the corresponding row in the peak viewer table
    b. Click on the peak in the ``peaks_int_ellipse`` table that has been removed from ``peaks_int_no_bg``

        - Only the ellipse should be plotted.

3. Delete ``peaks_int_no_bg`` from the ADS

    - The table should be removed from the peaks viewer
    - Confirm the Peak actions combo box is updated to only contain ``peaks_int_ellipse``

4. Delete ``peaks_int_ellipse`` from the ADS

    - The peak overlay should be turned off and the table hidden

3. ADS observer for workspace
#############################

With ``md_3D`` open in sliceviewer

1. Rename ``md_3D`` to e.g. ``md_3Dim``

    - The workspace name in the  title of the sliceviewer window should have updated
    - Zoom to check dynamic rebinning still works

2. Take a note of the colorbar limits and execute this command in the ipython terminal

    .. code-block:: python

        mtd['md_3Dim'] *= 2

    - The colorbar max should be doubled.
    - Zoom to check dynamic rebinning still works

3. Clone the workspace for future tests

.. code-block:: python

    CloneWorkspace(InputWorkspace='md_3Dim', OutputWorkspace='md_3D')

4. Delete ``md_3Dim`` in the ADS

    - The sliceviewer window should close

4. ADS observer for support for nonorthogonal view
##################################################

1. Open ``md_3D`` in sliceviewer
2. Run ``ClearUB`` algorithm on ``md_3D``

    - Sliceviewer window should close with message
    ``Closing Sliceviewer as the underlying workspace was changed: The property supports_nonorthogonal_axes is different on the new workspace.``


5. Check BinMD called with NormalizeBasisVectors=False for HKL data
###################################################################

1. Create a workspace with peaks at integer HKL and take a non axis-aligned cut

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

2. Open ``BinMD_out`` in sliceviewer.

    - There should be peaks at integer HKL

3. Zoom in (so that the data are rebinned)

    - The peaks should still be at integer HKL (rather than multiples of :math:`\sqrt{2}`)

6. Check gets the correct basis vectors for MDHisto workspaces
##############################################################

This tests that the sliceviewer gets the correct basis vectors for an ``MDHisto`` object from a non-axis aligned cut.

1. Create the workspace

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

2. Open ``ws_slice`` in the sliceviewer.

    - The non-orthogonal view should be enabled (not greyed out).

3. Click the non-orthogonal view button

    - Rectangular gridlines should appear (as in this case 110 is orthogonal to 001).

7. Check non-orthogonal view is disabled for non-Q axes
#######################################################
Check that the non-orthogonal view is disabled for non-Q axes such as energy

1. Create a workspace with energy as the first axis.

.. code-block:: python

    ws_4D = CreateMDWorkspace(Dimensions=4, Extents=[-1, 1, -1, 1, -1, 1, -1, 1], Names="E,H,K,L",
                                  Frames='General Frame,HKL,HKL,HKL', Units='meV,r.l.u.,r.l.u.,r.l.u.')
    expt_info_4D = CreateSampleWorkspace()
    ws_4D.addExperimentInfo(expt_info_4D)
    SetUB(ws_4D, 1, 1, 2, 90, 90, 120)

2. Open `` ws_4D`` in sliceviewer.
3. Confirm that when the Energy axis is viewed (as X or Y) the non-orthogonal view is disabled.
4. The button should be re-enabled when you view two Q-axes e.g. H and K.
