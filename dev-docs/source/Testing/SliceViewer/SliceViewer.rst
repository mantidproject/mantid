.. _sliceviewer_testing:

SliceViewer Testing
===================

.. contents::
   :local:

Introduction
------------

The Sliceviewer in Workbench has the joint functionality of the SpectrumViewer and SliceViewer from MantidPlot. So while the advanced use cases for multi-dimensional diffraction data are important to test, it is also worth checking more basic uses, for example opening a Workspace2D and examining the subplots and dynamic cursor data.

See here for a brief overview of the :ref:`sliceviewer`.

.. _sliceviewer_testing_matrixws:
MatrixWorkspace
---------------
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

MD Workspaces
-------------
MD workspaces are hold multi-dimensional data (typically 2-4D) and come in two forms: :ref:`MDEventWorkspace <MDWorkspace>`, :ref:`MDHistoWorkspace <MDHistoWorkspace>`.
In terms of sliceviewer functionality, the key difference is that MDHistoWorkspace have binned the events onto a regular grid and cannot be dynamically rebinned unless the original MDWorkspace
(that holds the events) exists in the ADS (and the MDHistoWorkspace has not been altered by a binary operation).

MDWorkspace (with events)
#########################
- Create a 4D MDWorkspace with some data

.. code-block:: python

	from mantid.simpleapi import *
    md_4D = CreateMDWorkspace(Dimensions=4, Extents=[-0.5,0.5,-1,1,-1.5,1.5,-2,2], Names="H,K,L,E", Frames='HKL,HKL,HKL,General Frame',Units='r.l.u.,r.l.u.,r.l.u.,meV')
    FakeMDEventData(InputWorkspace=md_4D, UniformParams='1e6') # 4D data
    tmp = CreateMDWorkspace(Dimensions=4, Extents=[-0.25,0.25,-1,0.5,-1.5,1,-2,1], Names="H,K,L,E", Frames='HKL,HKL,HKL,General Frame',Units='r.l.u.,r.l.u.,r.l.u.,meV')
    FakeMDEventData(InputWorkspace=tmp, UniformParams='1e6') # 4D data
    md_4D += tmp
    DeleteWorkspace(tmp)

    expt_info = CreateSampleWorkspace()
    md_4D.addExperimentInfo(expt_info)

    # Add a non-orthogonal UB
    SetUB(Workspace='md_4D', c=2, gamma=120)
    # Creat a peaks workspace
    CreatePeaksWorkspace(InstrumentWorkspace='md_4D', NumberOfPeaks=0, OutputWorkspace='peaks')
    CopySample(InputWorkspace='md_4D', OutputWorkspace='peaks', CopyName=False, CopyMaterial=False, CopyEnvironment=False, CopyShape=False)
    AddPeakHKL(Workspace='peaks', HKL='0,0,1')

- Test the toolbar buttons pan, zoom, line plots, ROI as in :ref:`MatrixWorkspace<mantid:sliceviewer_testing_matrixws>`.
    - This workspace should be dynamically rebinned - i.e. the number of bins within the view limits along each axis should be preserved when zooming.
- Change the number of bins along one of the viewing axes (easier to pick a small number e.g. 2)
- Change the integration width along the non-viewed axes.
    - Increasing the width should improve the stats on the uniform background and the color limit should increase (event counts are summed not averaged).
- Change the slicepoint along one of the non-viewed axes
    - Confirm the slider moves when the spinbox value is updated.
    - Confirm moving the slider updates the spinbox.
- Test the :ref`Nonorthogonal view<mantid:sliceviewer_nonortho>`
    - Enable nonorthogonal view
        - This should disable ROI and lineplot buttons in the toolbar
        - This should automatically turn on grid-lines
        - When H and K are the viewing axes the grid-lines should not be perpendicular to each other
        - The features in the data should align with the grid lines
        - Zoom and pan
        - Confirm the autoscaling of the colorbar works in non-orthogonal view
        - Change one of the viewing axes to be L (e.g. click `X` button next to L in top left of window)
            - Gridlines should now appear to be orthogonal
        - Change one of the viewing axes to be 'E' (e.g. click `Y` button next to E in top left of window)
            - Nonorthogonal view should be disabled (only enabled for momentum axes)
            - Line plots and ROI should be enabled
        -Change the viewing axis presently selected as `E` to be a momentum axis (e.g. `H`)
            - The nonorthogonal view should be automatically re-enabled.
- Test the :ref`Peak Overlay<mantid:liceviewer_peaks_overlay>`
    - Click to peak overlay button in the toolbar
    - Check the `Overlay?` box next to peaks
    - This should open a table on the RHS of the window - it should have a single row corresponding to a peak at HKL = (0,0,1).
    - Double click the row
        - It should change the slicepoint along the integrated momentum axis and zoom into the peak - e.g. in (X,Y) = (H,K) then the slicepoint along L will be set to 1 and there will be a cross at (0,0).
        - The cross should be plotted at all E (obviously a Bragg peak will only be on the elastic line but the peak object has no elastic/inelastic logic and the sliceviewer only knows that `E` is not a momentum axis, it could be temperature etc.).










Data
----

- Load 2D data (normal MatrixWorkspace), simply load ``CNCS_7860_event.nxs`` from the `TrainingCourseData <https://download.mantidproject.org/>`_. We will also load in some SXD data later on, it is good to test the Sliceviewer on both MatrixWorkspaces as the CNCS data was taken in event mode and the SXD data in histogram mode.

- Create fake 4D data and take a 3D and 2D cut:

.. code-block:: python

	from mantid.simpleapi import *

	md_4D = CreateMDWorkspace(Dimensions=4, Extents=[-1,1,-1,1,-1,1,-10,10], Names="H,K,L,E", Frames='HKL,HKL,HKL,General Frame',Units='r.l.u.,r.l.u.,r.l.u.,meV')
	FakeMDEventData(InputWorkspace=md_4D, PeakParams='500000,0,0,0,0,3') # 4D data
	# Create a histogrammed (binned) workspace with 100 bins in each of the H, K and L dimensions
	mdHisto_3D = BinMD(InputWorkspace=md_4D, AlignedDim0='H,-1,1,100', AlignedDim1='K,-1,1,100', AlignedDim2='L,-1,1,100') # 3D cut
	mdHisto_2D = BinMD(InputWorkspace=md_4D, AlignedDim0='H,-1,1,100', AlignedDim1='K,-1,1,100') # 2D cut

- Create an MD workspace with non-orthogonal axes:

.. code-block:: python

	from mantid.simpleapi import *

	# SXD23767.raw is available in the TrainingCourseData from the downloads page
	SXD23767 = Load(Filename='SXD23767.raw', LoadMonitors='Exclude')
	# Set some UB with angles we can play with
	SetUB(SXD23767, 1,1,2,90,90,120)
	SXD_MD_nonortho = ConvertToDiffractionMDWorkspace(InputWorkspace='SXD23767', OutputDimensions='HKL')

.. figure:: ../../../../docs/source/images/MBC_PickDemo.png
   :alt: MBC_PickDemo.png
   :align: center
   :width: 75%

- Create PeaksWorkspaces for the SXD data:

  - Open instrument viewer by right-clicking on the workspace``SXD23767``.
  - On the *Pick* tab, select the |PickTabAddPeakButton.png| "Add a single crystal peak" button.
  - Click on an intense bragg peak on the detectors, and then click on one or many of the intense peaks in the produced mini-plot. Repeat for a few different bragg peaks across the detectors.
  - Notice that this has produced a ``SingleCrystalPeakTable``.
  - Create another peak table which we will use to integrate (note in order to index these peaks we find the actual UB matrix which happens to be orthogonal - this is not a problem for test purposes)

.. code-block:: python

	FindSXPeaks(InputWorkspace='SXD23767', PeakFindingStrategy='AllPeaks', AbsoluteBackground=1500, ResolutionStrategy='AbsoluteResolution', XResolution=500, PhiResolution=5, TwoThetaResolution=5, OutputWorkspace='peaks')
	FindUBUsingLatticeParameters(PeaksWorkspace='peaks', a=5.65, b=5.65, c=5.65, alpha=90, beta=90, gamma=90, FixParameters=True)
	IndexPeaks(PeaksWorkspace='peaks')

- Create an Integrated PeaksWorkspace:

.. code-block:: python

	peaks = mtd['peaks']
	integrated_peaks = IntegratePeaksMD(InputWorkspace='SXD_MD_nonortho', PeaksWorkspace='peaks',\
	     PeakRadius=0.12, BackgroundOuterRadius=0.2, BackgroundInnerRadius=0.16)


Tests
-----

Remember to SliceView MatrixWorkspaces and 2D,3D,4D and non-orthogonal MD objects.

1. Viewing Data
###############

For the data types above:
	- Change the number of bins displayed
	- Move the sliders (this applies only to 3D and 4D MD workspaces - i.e. ``SXD_MD_nonortho``, ``md_4D``, ``mdHisto_3D``)
	- Edit color limits, colormap, scale(lin/log), etc.

2. Toggle gridlines on/off
##########################

- For normal and non-orthogonal axes data

3. Select Axes
##############

- Change the axes that are displayed by selecting the relevant ``X`` and ``Y`` axes in the top left. This is more interesting for higher dimension data.

4. Non-orthogonal view
######################

A. For the ``SXD_MD_nonortho`` workspace, the non-orthogonal view button (see below) should be enabled - clicking this should also turn on grid lines. When viewing the H and K axes you should see the gridlines are no longer perpendicular to each other.

.. figure:: ../../../../docs/source/images/wb-sliceviewer51-nonorthobutton.png
   :class: screenshot
   :align: center

B. This tests that the sliceviewer gets the correct basis vectors for an ``MDHisto`` object from a non-axis aligned cut.

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

- Run the above code and open ``ws_slice`` in the sliceviewer.
- Check that non-orthogonal view is enabled on opening - however when clicked the gridlines are orthogonal  (in this case 110 is orthogonal to 001).

C. Check that changing the properties of the workspace that governs the support for the non-orthogonal transform closes the sliceviewer window.

- Open ``ws`` from the above test script in the sliceviewer

- Run ``ClearUB(ws)`` (it should close sliceviewer with warning ``property supports_nonorthogonal_axes is different...``)

- Instead of clearing the UB you can also replace the workspace with a workspace of a different frame that doesn't support the non-orthogonal view (e.g. QLab)

.. code-block:: python

	ws = CreateMDWorkspace(Dimensions='3', Extents='-6,6,-4,4,-0.5,0.5',
                Names='Q1,Q2,Q3', Units='Ang-1,Ang-1,Ang-1',
                Frames='QLab,QLab,QLab',
                SplitInto='2', SplitThreshold='50')

D. Check that the non-orthogonal view is disabled for non-Q axes such as energy

.. code-block:: python

    ws_4D = CreateMDWorkspace(Dimensions=4, Extents=[-1, 1, -1, 1, -1, 1, -1, 1], Names="E,H,K,L",
                                  Frames='General Frame,HKL,HKL,HKL', Units='meV,r.l.u.,r.l.u.,r.l.u.')
    expt_info_4D = CreateSampleWorkspace()
    ws_4D.addExperimentInfo(expt_info_4D)
    SetUB(ws_4D, 1, 1, 2, 90, 90, 120)

- When the Energy axis is viewed (as X or Y) the non-orthogonal view is disabled. The button should be re-enabled when you view two Q-axes e.g. H and K.


5. Cursor Tracking
##################

- Toggle "Track Cursor" on/off and check the cursor data makes sense
- For a MatrixWorkspace (e.g. ``SXD23767``) there is much more information than for an MD object. See :ref:`Cursor Information Widget<mantid:sliceviewer_cursor>` for more details.

6. Peak Overlay
###############

This functionality only applies only to 3D MD workspaces - specifically you should test this on the ``SXD_MD_nonortho`` workspace.

.. figure:: ../../../../docs/source/images/wb-sliceviewer51-peaksbutton.png
   :class: screenshot
   :align: center

- Select the peak overlay button to choose which PeakWorkspace/s to overlay.
- Click on different peaks in the peak sorting table to zoom in on a peak.
- Try overlaying multiple peaks workspaces
- Overlay Integrated peaks and observe the peak radius and background shell
  (see *Calculations* section of :ref:`algm-IntegratePeaksMD`) as displayed in the image below.
- Click on a column title in the peak table to sort by that value, such as ``DetID``
- Zooming in on peaks, and check that the data and peak move together
- Repeat step 2. (Select Axes) with peaks shown
- Repeat these instructions with non-orthogonal view enabled.
- Peak overlay should not be shown for 2D data

.. figure:: ../../../../docs/source/images/wb-sliceviewer51-peaksoverlay.png
   :class: screenshot
   :width: 75%
   :align: center

7. Toolbar buttons for changing axis limits
###########################################

- Home
- Pan + Stretch
- Zoom (dynamic rebinning to ``_svrebinned`` workspace for MD workspace) - both by selecting region with mouse and scrolling


8. Line subplots and Region of Interest integration
###################################################

**(this functionality is disabled when non-orthogonal view is enabled)**

.. figure:: ../../../../docs/source/images/wb-sliceviewer51-roibutton.png
   :class: screenshot
   :align: center

- Select the toolbar button for region of interest integration
- Draw a shape to integrate over on the image and notice the line subplots change
- Test the keyboard options in the bottom status bar message to output to workspaces
	- Output the cuts displayed on line-subplots, for axis: x = X , y = Y, c = Both
	- Output slice over the region of interest box: r = roi

- Also, test that the basic Line-subplots toolbar button (to the left of ROI integration button) works.
	- Check keyboard options for cuts displayed on line-subplots, for axis: x = X , y = Y, c = Both

.. figure:: ../../../../docs/source/images/wb-sliceviewer51-roi.png
   :class: screenshot
   :width: 75%
   :align: center

9. Save image
#############

- Use the Save image toolbar button, in many instances, such as with peaks overlaid
- In future there will also be a toolbar button to copy the image to clipboard

10. Resizing
###########

- Play around with resizing the window and adjusting the size of the peak table**

11. Alter the underlying workspace
##################################

- Delete the workspace and Sliceviewer should close
- Rename the workspace and Sliceviewer should stay open and continue to work
- Change the data in the workspace by cropping or running some algorithm (e.g. double the data ``SXD_MD_nonortho *= 2``)
- Delete rows or re-integrate a PeaksWorkspace that is overlaid.


.. |PickTabAddPeakButton.png| image:: ../../../../docs/source/images/PickTabAddPeakButton.png

