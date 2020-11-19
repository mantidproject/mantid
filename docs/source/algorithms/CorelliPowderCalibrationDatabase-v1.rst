
.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

The algorithm `CORELLIPowderCalibrationGenerate` receives (among other inputs) an `InputWorkspace` and produce and output `CalibrationTable`, 
a `TableWorkspace` object. The contents of the calibration table patch are:


 | ComponentName | Xposition | Yposition| Zposition| XdirectionCosine | YdirectionCosine | ZdirectionCosine | RotationAngle |
 |----------|-----------|----------|----------|------------------|------------------|------------------|---------------|


This issue will create algorithm `CORELLIPowderCalibrationDatabase` that will receive as input `CalibrationTablePatch` and `InputWorkspace`. 
A more detailed list of inputs:
- `InputWorkspace` [Input]: `EventsWorkspace` or `MatrixWorkspace`, the run used for the calibration. The day the experiment ran will be extracted
- `CalibrationTablePatch` [Input] A `TableWorkspace` object, the output of `CORELLIPowderCalibrationGenerate`
- `DatabaseDirectory` [Optional Input]: absolute path to the database, with a default location to be determined by the instrument scientists.
- `DayStamp` [Optional Input]: to be used in place of the `InputWorkspace`
- `CalibrationTable` [Optional Output]: a `TableWorkspace`

`CORELLIPowderCalibrationDatabase` creates the following outputs:  

**Merged Calibration TableWorkspace**: The contents of *corelli_instrument_20202015.csv* will be loaded into a `TableWorkspace` object, the `CalibrationTable` output. It will have the same structure as `CalibrationTablePatch` but should (ideally) contain a row for every bank in the instrument

**Single Component Database Files**: extract each row of `CalibrationTable` and append to appropriate component file in the the database. 
For instance, if the contents of `CalibrationTablePatch` are:

 | ComponentName | Xposition | Yposition| Zposition| XdirectionCosine | YdirectionCosine | ZdirectionCosine | RotationAngle |
 |----------|-----------|----------|----------|------------------|------------------|------------------|---------------|
 | source | 0 | 0 | -15.560 | 0 | 0 | 0 | 0 |
 | sample | 0.0001 | -0.0002 | 0.003 | 0 | 0 | 0 | 0 |
 | bank1 | 0.9678 | 0.0056 | 0.0003 | 0.4563 | -0.9999 | 0.3424 | 5.67 |

And the day stamp for this calibration is 20201025, the following line will be *appended* to file *corelli_source.csv* :
```
20201025, 0, 0, -15.560, 0, 0, 0, 0
```
the following line will be *appended* to file *corelli_sample.csv*:
```
20201025, 0.0001, -0.0002, 0.003, 0, 0, 0, 0
```

and the following lines to *corelli_bank001.csv*:
20201025, 0.9678, 0.0056, 0.0003, 0.4563, -0.9999, 0.3424, 5.67

The header for files *corelli_source.csv*, *corelli_sample.csv*, and files *corelli_bankXXX.csv* should be:
```
# YYYMMDD, Xposition, Yposition, Zposition, XdirectionCosine, YdirectionCosine, ZdirectionCosine, RotationAngle
```

**Combined Calibrtion Database File**: The last line of *corelli_source.csv*, *corelli_sample.csv*, and each of existing files *corelli_bankXXX.csv* will be collected and gathered into file *corelli_instrument_20202015.csv*. The header for this file should be:
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

.. testcode:: CorelliCrossCorrelateExample
    
    # Load a Corelli data file.
    ws = Load('CORELLI_2100')

    # You will need to load the instrument if the one in the NeXus file doesn't contain the chopper sequence.
    LoadInstrument(ws, InstrumentName='CORELLI', RewriteSpectraMap=False)

    # Run the cross-correlation. This is using a TDC timing offset of 56000ns.
    wsOut = CorelliCrossCorrelate(ws, 56000)

    print('The detector 172305 has {} events.'.format(ws.getSpectrum(172305).getNumberEvents()))
    print('The event weights before cross-correlation are {}'.format(ws.getSpectrum(172305).getWeights()))
    print('The event weights after  cross-correlation are {}'.format(wsOut.getSpectrum(172305).getWeights()))

Output:

.. testoutput:: CorelliCrossCorrelateExample 

    The detector 172305 has 3 events.
    The event weights before cross-correlation are [ 1.  1.  1.]
    The event weights after  cross-correlation are [-0.99391854  1.          1.        ]

.. categories::

.. sourcelink::

