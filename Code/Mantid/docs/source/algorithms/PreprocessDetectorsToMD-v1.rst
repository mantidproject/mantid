.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

The algorithm calculates expensive part of the transformation from real to reciprocal space, calculating namely, 
detector positions, sample-detector distances, angular detectors positions, etc. These values can be slow to calculate 
for composite detectors and other algorithms can substantially benifit from using preprocessed values. 

The algorithm places processed values into a table workspace with the following colums:

+---------------+--------+------------------------------------------------------+
| Column Name : |  Type  |  Description:                                        |
+===============+========+======================================================+
| DetDirections |  V3D   | unit 3D vector directed from sample to detector      |
+---------------+--------+------------------------------------------------------+
| L2            | double | sample-detector distance                             |
+---------------+--------+------------------------------------------------------+ 
| TwoTheta      | double | Composite detectors Polar angle                      |
+---------------+--------+------------------------------------------------------+ 
| Azimuthal     | double | Composite detectors Azimutal angle                   |
+---------------+--------+------------------------------------------------------+ 
| DetectorID    | int    | Unique detector number                               |
+---------------+--------+------------------------------------------------------+ 
| detIDMap      | size_t | index of the detector in the detector's array        |
+---------------+--------+------------------------------------------------------+ 
| detMask [#f1]_| int    |  1 if masked and 0 if not                            |
+---------------+--------+------------------------------------------------------+ 
| efixed [#f2]_ | double |  energy of the detector (intirect only)              |
+---------------+--------+------------------------------------------------------+

In addition to the preprocessed detectors intofmation, the following log entries are added to the table workspace:

-  **Ei** -- Incident neurtorns energy if such log value is attached to input workspace.
-  **L1** -- Source-sample distance.
-  **ActualDetectorsNum** -- total number of existing preprocessed detectors (number of rows in the table above).
-  **InstrumentName** -- the name of the source instrument.
-  **FakeDetectors** -- if detectors posisions were not actually preprocessed but fake detectros were used instread 
    (InstrumentName==*FakeInstrument* in this case).

    
.. rubric:: Notes

.. [#f1] Present if property **GetMaskState** is set to True.
.. [#f2] Present if **Indirect** mode selected.


Usage
-----

**Example - Find min-max values for |Q| transformation :**

.. testcode:: ExPreprocessDetectoresToMD

    # Simulates Load of a workspace with all necessary parameters #################
    detWS = CreateSimulationWorkspace(Instrument='MAR',BinParams=[-50,5,50],UnitX='DeltaE')
    AddSampleLog(detWS,LogName='Ei',LogText='52.',LogType='Number');
    # 
    preprDetWS = PreprocessDetectorsToMD(InputWorkspace=detWS,GetMaskState=1,GetEFixed=1)
    # Look at sample results:       
    print "The resulting table has the following columns:"
    print preprDetWS.keys()
    print "The number of rows in the workspace is : ",len(preprDetWS.column('L2'))
    polar = preprDetWS.column('TwoTheta')
    print "The polar angle for detector N {0} is {1:5f} rad".format(10,polar[10])
    print "The table workspace logs (properties) are currently not availible from python"
    
    
.. testcleanup:: ExPreprocessDetectoresToMD

   DeleteWorkspace(detWS)
   DeleteWorkspace(preprDetWS)   

**Output:**

.. testoutput:: ExPreprocessDetectoresToMD

   The resulting table has the following columns:
   ['DetDirections', 'L2', 'TwoTheta', 'Azimuthal', 'DetectorID', 'detIDMap', 'spec2detMap', 'detMask', 'eFixed']
   The number of rows in the workspace is :  918
   The polar angle for detector N 10 is 0.314159 rad
   The table workspace logs (properties) are currently not availible from python



.. categories::
