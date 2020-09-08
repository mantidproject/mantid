.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm can be used to remove negative intensity values from workspaces created by the :ref:`Bin2DPowderDiffraction
<algm-Bin2DPowderDiffraction>` algorithm. The algorithm looks for the lowest negative intensity value and adds this value 
to all other intensities, leading to a workspace with 0 as the lowest intensity. The existing :ref:`ResetNegatives
<algm-ResetNegatives>` algorithm does not produce the same results, as it looks for the lowest negative intensity in each 
spectrum and adds this value for the corresponding spectrum only. 

Usage
-----

**Example: Reset negative values from a 2D Workspace. Remember to change the Filepath for the OutputFile!**

.. testcode::PrintP2D

    # create a 2D Workspace
    # repeat this block for each spectrum
    xData = [1.0,2.0,3.0,4.0,5.0,6.0,7.0,8.0,9.0]            # d values for one spectrum (one dPerpendicular value)
    yData = ['1','2','3','4']                                # dPerpendicular binedges
    zData = [1.0,-1.0,1.0,1.0,1.0,1.0,-2.0,1.0,1.0]          # intensity values
    eData = [1,1,1,1,1,1,1,1,1]                              # error values

    # used to join all spectra
    xDataTotal = []                                          # d Values for all spectra
    zDataTotal = []                                          # intensity values for all spectra
    eDataTotal = []                                          # error values for all spectra
    nSpec = len(yData)-1                                     # number of spectra

    # Create d and intensity lists for workspace
    for i in range(0,nSpec):
        xDataTotal.extend(xData)       # extends the list of x values in accordance to the number of spectra used
        zDataTotal.extend(zData)       # extends the list of intensity values in accordance to the number of spectra used
        eDataTotal.extend(eData)       # extends the list of error values in accordance to the number of spectra used

    # Create a 2D Workspace containing d and dPerpendicular values with intensities
    CreateWorkspace(OutputWorkspace = 'Usage_Example', DataX = xDataTotal, DataY = zDataTotal, DataE = eDataTotal, WorkspaceTitle = 'test', NSpec = nSpec, UnitX = 'dSpacing', VerticalAxisUnit = 'dSpacingPerpendicular', VerticalAxisValues = yData)
    # Reset the negative values
    ResetNegatives2D(Workspace = "Usage_Example")

Output:

.. testoutput:: PrintP2D

.. categories::

.. sourcelink::