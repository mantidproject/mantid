.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm makes it easier to load POLDI data. Besides importing the raw data (:ref:`algm-LoadSINQ`), it performs the otherwise manually performed steps of instrument loading (:ref:`algm-LoadInstrument`), truncation (:ref:`algm-PoldiTruncateData`). To make the algorithm more useful, it is possible to load data from multiple runs by specifying a range. In many cases, data files need to be merged in a systematic manner, which is also covered by this algorithm. For this purpose there is a parameter that specifies how the files of the specified range should be merged. The data files are named following the scheme `group_data_run`, where `group` is the name specified in `OutputWorkspace` and `run` is the run number and placed into a WorkspaceGroup with the name given in `OutputWorkspace`.

The data loaded in this way can be used directly for further processing with :ref:`algm-PoldiAutoCorrelation`.

Usage
-----

.. include:: ../usagedata-note.txt

To load only one POLDI data file (in this case a run from a calibration measurement with silicon standard), it's enough to specify the year and the run number. Nevertheless it will be placed in a WorkspaceGroup. 

.. testcode:: ExLoadSingleFile

    calibration = PoldiLoadRuns(2013, 6903)
    
    # calibration is a WorkspaceGroup, so we can use getNames() to query what's inside.
    workspaceNames = calibration.getNames()
    
    print "Number of data files loaded:", len(workspaceNames)
    print "Name of data workspace:", workspaceNames[0]

Since only one run number was supplied, only one workspace is loaded. The name corresponds to the scheme described above:

.. testoutput:: ExLoadSingleFile
    
    Number of data files loaded: 1
    Name of data workspace: calibration_data_6903

Actually, the silicon calibration measurement consists of more than one run, so in fact it would be better to load all the files at once, so the start and end of the range to be loaded is provided:

.. testcode:: ExLoadMultipleFiles

    # Load two calibration data files (6903 and 6904)
    calibration = PoldiLoadRuns(2013, 6903, 6904)
    
    workspaceNames = calibration.getNames()
    
    print "Number of data files loaded:", len(workspaceNames)
    print "Names of data workspaces:", workspaceNames
    
Now all files from the specified range are in the `calibration` WorkspaceGroup:
    
.. testoutput:: ExLoadMultipleFiles

    Number of data files loaded: 2
    Names of data workspaces: ['calibration_data_6903','calibration_data_6904']
    
But in fact, these data files should not be processed separately, they belong to the same measurement and should be merged together. Instead of using :ref:`algm-PoldiMerge` directly to merge the data files, it's possible to tell the algorithm to merge the files directly after loading, by specifying how many of the files should be merged together. Setting the parameter to the value `2` means that the whole range will be iterated and files will be merged together in pairs:

.. testcode:: ExLoadMultipleFilesMerge

    # Load two calibration data files (6903 and 6904) and merge them
    calibration = PoldiLoadRuns(2013, 6903, 6904, 2)
    
    workspaceNames = calibration.getNames()
    
    print "Number of data files loaded:", len(workspaceNames)
    print "Names of data workspaces:", workspaceNames
    
The merged files will receive the name of the last file in the merged range:
    
.. testoutput:: ExLoadMultipleFilesMerge

    Number of data files loaded: 1
    Names of data workspaces: ['calibration_data_6904']
    
When the merge parameter and the number of runs in the specified range are not compatible (for example specifying `3` in the above code), the algorithm will merge files in the range as long as it can and leave out the rest. In the above example that would result in no data files being loaded at all.

A situation that occurs often is that one sample consists of multiple ranges of runs, which can not be expressed as one range. It's nevertheless possible to collect them all in one WorkspaceGroup. In fact, that is the default behavior of the algorithm if the supplied `OutputWorkspace` already exists:

.. testcode:: ExLoadMultipleRanges

    # Load calibration data
    calibration = PoldiLoadRuns(2013, 6903)
    
    # Add another file to the calibration WorkspaceGroup
    calibration = PoldiLoadRuns(2013, 6904)
    
    workspaceNames = calibration.getNames()
    
    print "Number of data files loaded:", len(workspaceNames)
    print "Names of data workspaces:", workspaceNames
    
The result is the same as in the example above, two files are in the WorkspaceGroup:
    
.. testoutput:: ExLoadMultipleRanges

    Number of data files loaded: 2
    Names of data workspaces: ['calibration_data_6903','calibration_data_6904']
    
On the other hand it is also possible to overwrite an existing WorkspaceGroup, for example if there was a mistake with the previous data loading. The parameter needs to be specified explicitly.

.. testcode:: ExLoadMultipleRangesOverwrite

    # Load calibration data, forget merging
    calibration = PoldiLoadRuns(2013, 6903, 6904)

    # Load data again, this time with correct merging    
    calibration = PoldiLoadRuns(2013, 6903, 6904, 2, OverwriteExistingWorkspace=True)
    
    workspaceNames = calibration.getNames()
    
    print "Number of data files loaded:", len(workspaceNames)
    print "Names of data workspaces:", workspaceNames
    
The data loaded in the first call to the algorithm have been overwritten with the merged data set:
    
.. testoutput:: ExLoadMultipleRangesOverwrite

    Number of data files loaded: 1
    Names of data workspaces: ['calibration_data_6904']

.. categories::
