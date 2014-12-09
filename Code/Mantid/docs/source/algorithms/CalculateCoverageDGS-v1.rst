
.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

The algorithm calculates what part of the reciprocal space is covered by direct geometry spectrometers. It is supposed to be used
in planning tools. The input workspace should contain the incident energy log, a goniometer, and the UB matrix. The incident energy 
can be overwritten by setting the IncidentEnergy parameter. The output is an MDHisto workspace. You can visualize it with
SliceViewer.
 

The parameters Q1Basis, Q2Basis, Q3Basis can be used to look at different directions in the reciprocal space, such as [H,H,0]

DimensionI, where I is from 1 to 4, allows one to select in which order Q1, Q2, Q3, DeltaE are stored in the output workspace.
The algorithm will complain if you select the same dimension twice.
For each dimension one can define minimum, maximum, and step size.
If no minimum/maximum are set for DeltaE, these are chosen to be +/-Ei. 
If not specified, minimum/maximum for the Q dimensions are calculated based on the instrument geometry, incident energy, 
minimum/maximum for DeltaE, and lattice parameters. 

The algorithm calculates detector trajectories in the reciprocal space and sets to 1 the coresponding points in the output workspace.
All other points are 0. 

Usage
-----
.. include:: ../usagedata-note.txt

**Example - CalculateCoverageDGS**

.. testcode:: CalculateCoverageDGSExample
    
    w=Load(Filename="CNCS_7860")
    w=ConvertUnits(InputWorkspace=w,Target='DeltaE', EMode='Direct', EFixed='3')
    SetGoniometer(Workspace=w, Axis0='30,0,1,0,1')
    SetUB(Workspace=w, a='2', b='2', c='2')
    w=Rebin(InputWorkspace=w, Params='-3,0.1,3', PreserveEvents='0')
    coverage=CalculateCoverageDGS(InputWorkspace=w, IncidentEnergy=3, 
        Dimension1Min=-1, Dimension1Max=1, Dimension1Step=0.02, 
        Dimension2Min=-1, Dimension2Max=1, Dimension2Step=2, 
        Dimension3Min=-1, Dimension3Max=1, Dimension3Step=2, 
        Dimension4Min=0, Dimension4Max=3, Dimension4Step=0.1)

    # Print the result
    print "You cover %i MD bins out of %i" %(coverage.getSignalArray().sum(),coverage.getSignalArray().size)

Output:

.. testoutput:: CalculateCoverageDGSExample

    You cover 506 MD bins out of 3000
  
.. categories::

