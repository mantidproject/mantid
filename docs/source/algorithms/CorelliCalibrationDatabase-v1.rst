
.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

The goal of algorithm `CORELLIPowderCalibrationGenerate` is twofold. The first goal is to store a recent calibration
for a *subset* of all banks (property `InputCalibrationPatchWorkspace`) within a database directory. The second
goal is to combine this recent calibration with previous calibrations in order to produce a calibration for
*all* banks. The assumption is that older calibrations will contain calibrations for the banks that are missing
in the recent calibration.

The recent calibration is the output of algorithm :ref:`algm-CORELLIPowderCalibrationCreate`,
a :ref:`TableWorkspace <Table Workspaces>` with the following columns:

 | ComponentName | Xposition | Yposition| Zposition| XdirectionCosine | YdirectionCosine | ZdirectionCosine | RotationAngle |

A typical calibration will contain a row for the moderator, possibly a row for the sample, and one row for each
calibrated bank. For instance:

+--------------------+------------+-----------+-----------+------------------+------------------+------------------+---------------+
| ComponentName      | Xposition  | Yposition | Zposition | XdirectionCosine | YdirectionCosine | ZdirectionCosine | RotationAngle |
+====================+============+===========+===========+==================+==================+==================+===============+
| moderator          | 0.0        | 0.0       | -19.9944  |      0.0         |      0.0         |      0.0         |      0.0      |
+--------------------+------------+-----------+-----------+------------------+------------------+------------------+---------------+
| sample-position    | 0.0001     | -0.0002   | 0.003     | 0.0              | 0.0              | 0.0              | 0.0           |
+--------------------+------------+-----------+-----------+------------------+------------------+------------------+---------------+
| bank7/sixteenpack  | 0.9678     | 0.0056    | 0.0003    | 0.4563           | -0.9999          | 0.3424           | 5.67          |
+--------------------+------------+-----------+-----------+------------------+------------------+------------------+---------------+
| bank8/sixteenpack  | 0.9650     | 0.0050    | 0.0002    | 0.4513           | -0.9999          | 0.3921           | 9.03          |
+--------------------+------------+-----------+-----------+------------------+------------------+------------------+---------------+

For more details on this output, see the documentation for :ref:`algm-CORELLIPowderCalibrationCreate`.

The rows of the recent calibration table are read one by one, and the corresponding *single component* database files are updated
accordingly. Thus in our previous example the line for the moderator is extracted and appended to CSV file
*corelli_moderator.csv*. Before appending, the line is amended with the day-stamp of the recent calibration. The
day-stamp is retrieved from the input workspace of property `InputWorkspace`. A typical *corelli_moderator.csv*
file may look like this:

```
20190601, 0, 0, -15.550, 0, 0, 0, 0
20191013, 0, 0, -15.552, 0, 0, 0, 0
20201025, 0, 0, -15.560, 0, 0, 0, 0
```

The last line is the moderator row from table `InputCalibrationPatchWorkspace` amended with the day stamp
contained in `InputWorkspace` (20201025).

Files for the moderator and the other components (*corelli_sample-position.csv*, *corelli_bank7.csv*,..,*corelli_bank91.csv*)
are all located under the directory specified in property `DatabaseDirectory`

After the first goal is accomplished (store the recent calibration), the algorithm proceeds to produce a
calibration file and :ref:`TableWorkspace <Table Workspaces>` for the whole instrument. The last line of
*corelli_moderator.csv*, *corelli_sample-position.csv*, and each of *corelli_bankXXX.csv* is
collected, the day-stamp stripped, and lines are gathered into file *corelli_instrument_20201025.csv*,
bearing in mind that 20201025 is the day-stamp of our recent calibration. The first few lines of the file:

```
# ComponentName, Xposition, Yposition, Zposition, XdirectionCosine, YdirectionCosine, ZdirectionCosine, RotationAngle
moderator, 0.0, 0.0, -15.560, 0.0, 0.0, 0.0, 0.0
sample-position, 0.0001, -0.0002, 0.003, 0.0, 0.0, 0.0, 0.0
bank7/sixteenpack, 0.9678, 0.0056, 0.0003, 0.4563, -0.9999, 0.3424, 5.67
bank8/sixteenpack, 0.9650, 0.0050, 0.0002, 0.4513, -0.9998, 0.3921, 9.03
```

In addition, a :ref:`TableWorkspace <Table Workspaces>` similar to the table of `InputCalibrationPatchWorkspace` is
produced with (hopefully) a row for each bank.


Usage
-----
..  Try not to use files in your examples,
    but if you cannot avoid it then the (small) files must be added to
    autotestdata\UsageData and the following tag unindented
    .. include:: ../usagedata-note.txt

**Example - CorelliCalibrateDatabase**

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
    calib_table.addRow(["bank1/sixteenpack" , 0.9678 , 0.0056 , 0.0003 , 0.4563 , -0.9999, 0.3424, 0.321])

    # save for powder calibration database
    CorelliCalibrationDatabase(InputWorkspace='input', InputCalibrationPatchWorkspace='calib_table',
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
    bank1 file ....csv exist = True
    calibration file ....csv exist = True

.. testcleanup:: CorelliPowderCalibrationExample

    import shutil
    shutil.rmtree(calib_dir)

.. categories::

.. sourcelink::
