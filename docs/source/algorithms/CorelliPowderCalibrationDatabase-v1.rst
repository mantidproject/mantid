
.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

The algorithm `CORELLIPowderCalibrationGenerate` receives (among other inputs) an `InputWorkspace` and produce and output `CalibrationTable`, 
a `TableWorkspace` object. The contents of the calibration table patch are:


 | ComponentName | Xposition | Yposition| Zposition| XdirectionCosine | YdirectionCosine | ZdirectionCosine | RotationAngle |


This issue will create algorithm `CORELLIPowderCalibrationDatabase` that will receive as input `CalibrationTablePatch` and `InputWorkspace`. 
A more detailed list of inputs:

- `InputWorkspace` [Input]: `EventsWorkspace` or `MatrixWorkspace`, the run used for the calibration. The day the experiment ran will be extracted

- `CalibrationTablePatch` [Input] A `TableWorkspace` object, the output of `CORELLIPowderCalibrationGenerate`

- `DatabaseDirectory` [Optional Input]: absolute path to the database, with a default location to be determined by the instrument scientists.

- `DayStamp` [Optional Input]: to be used in place of the `InputWorkspace`

- `CalibrationTable` [Optional Output]: a `TableWorkspace`

`CORELLIPowderCalibrationDatabase` creates the following outputs:  

**Merged Calibration TableWorkspace**: The contents of *corelli_instrument_YYYYMMDD.csv* will be loaded into a `TableWorkspace` object, the `CalibrationTable` output. It will have the same structure as `CalibrationTablePatch` but should (ideally) contain a row for every bank in the instrument.  *YYYYMMDD* is the date stamp of calibration run.  For example, if the calibraton's run start time is on 2020.02.15, then its date stamp is *20200215*.

**Single Component Database Files**: extract each row of `CalibrationTable` and append to appropriate component file in the the database. 
For instance, if the contents of `CalibrationTablePatch` are:

 | ComponentName | Xposition | Yposition| Zposition| XdirectionCosine | YdirectionCosine | ZdirectionCosine | RotationAngle |
 | moderator | 0 | 0 | -15.560 | 0 | 0 | 0 | 0 |
 | sample-position | 0.0001 | -0.0002 | 0.003 | 0 | 0 | 0 | 0 |
 | bank1 | 0.9678 | 0.0056 | 0.0003 | 0.4563 | -0.9999 | 0.3424 | 5.67 |

And the day stamp for this calibration is *YYYYMMDD*, the following line will be *appended* to file *corelli_moderator.csv* :

```
20201025, 0, 0, -15.560, 0, 0, 0, 0
```

the following line will be *appended* to file *corelli_sample-position.csv*:

```
20201025, 0.0001, -0.0002, 0.003, 0, 0, 0, 0
```

and the following lines to *corelli_bank001.csv*:
20201025, 0.9678, 0.0056, 0.0003, 0.4563, -0.9999, 0.3424, 5.67

The header for files *corelli_moderator.csv*, *corelli_sample-position.csv*, and files *corelli_bankXXX.csv* should be:

```
# YYYMMDD, Xposition, Yposition, Zposition, XdirectionCosine, YdirectionCosine, ZdirectionCosine, RotationAngle
```

**Combined Calibration Database File**: The last line of *corelli_moderator.csv*, *corelli_sample-position.csv*, and each of existing files *corelli_bankXXX.csv* will be collected and gathered into file *corelli_instrument_20202015.csv*. The header for this file should be:

```
# ComponentName, Xposition, Yposition, Zposition, XdirectionCosine, YdirectionCosine, ZdirectionCosine, RotationAngle
```
The name of the file should be output in the workbench logger.


Usage
-----
..  Try not to use files in your examples, 
    but if you cannot avoid it then the (small) files must be added to 
    autotestdata\UsageData and the following tag unindented
    .. include:: ../usagedata-note.txt

**Example - CorelliPowderCalibrateDatabase**

.. testcode:: CorelliPowderCalibrationExample
    
    # Import modules
    import numpy as np
    import os
    
    # Enpty workspace
    input = LoadEmptyInstrument(InstrumentName='CORELLI')
    # add start timedelta64
    AddSampleLog(Workspace=input, LogName='start_time', LogText='2020-02-20T12:57:17', LogType='String')
    
    # generate simulated stored database files
    bank2_str = "# YYYMMDD , Xposition , Yposition , Zposition , XdirectionCosine , YdirectionCosine , ZdirectionCosine , RotationAngle\n" \
                "# str , double , double , double , double , double , double , double\n" \
                "20001117,0.0001,-0.0002,0.003,0,-23.3,98.02,0"
    
    # generate simulated stored database files
    bank12_str = "# YYYMMDD , Xposition , Yposition , Zposition , XdirectionCosine , YdirectionCosine , ZdirectionCosine , RotationAngle\n" \
                "# str , double , double , double , double , double , double , double\n" \
                "20011117,1.0001,-2.0002,3.003,4,-23.3,98.02,0"
    
    calib_dir = 'sim_corelli_cal'
    if not os.path.exists(calib_dir):
        os.mkdir(calib_dir)
    
    for bank, content in [('bank2', bank2_str), ('bank12', bank12_str)]:
        bankfile = open(os.path.join(calib_dir, bank + '.csv'), 'w')
        bankfile.write(content)
        bankfile.close()
    
    # Create table
    calib_table = CreateEmptyTableWorkspace("CorelliCalibrationTestTable");
    calib_table.addColumn("str", "ComponentName")
    for colname in ["Xposition", "Yposition", "Zposition","XdirectionCosine", "YdirectionCosine", "ZdirectionCosine", "RotationAngle"]:
        calib_table.addColumn("double", colname)
    
    # add entry
    calib_table.addRow(["moderator" , 0. , 0. , -15.560 , 0. , 0. , 0., 0.])
    calib_table.addRow(["sample-position" , 0.0001 , -0.0002 , 0.003 , 0. , 0.,  0., 0.])
    calib_table.addRow(["bank1" , 0.9678 , 0.0056 , 0.0003 , 0.4563 , -0.9999, 0.3424, 0.321])
    
    # save for powder calibration database
    CorelliPowderCalibrationDatabase(InputWorkspace='input', InputCalibrationPatchWorkspace='calib_table',
                                     DatabaseDirectory='sim_corelli_cal',
                                     OutputWorkspace='mergedcalibrationtable')
    
    # check
    print('Number of components = {}'.format(mtd['mergedcalibrationtable'].rowCount()))
    bank1_file = os.path.join('sim_corelli_cal', 'bank1.csv')
    print('bank1 file {} exist = {}'.format(bank1_file, os.path.exists(bank1_file)))
    calib_file = os.path.join('sim_corelli_cal', 'corelli_instrument_20200220.csv')
    print('calibration file {} exist = {}'.format(calib_file, os.path.exists(calib_file)))

Output:

.. testoutput:: CorelliPowderCalibrationExample

    Number of components = 5
    bank1 file sim_corelli_cal/bank1.csv exist = True
    calibration file sim_corelli_cal/corelli_instrument_20200220.csv exist = True

.. categories::

.. sourcelink::

