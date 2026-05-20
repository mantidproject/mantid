.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

PEARL high-pressure, high-temperature measurements rely on the use neutron transmission data to determine the sample
temperature. Typically Hf foil is included with the sample, though others can be used, provided they have well
characterised resonances. Transfit is an algorithm which reads high-energy neutron resonances from the downstream
monitor data on the PEARL instrument. It fits a Voigt function to them to determine the sample temperature. It is
broadly similar to the Fortran77 version originally implemented on PEARL through OpenGenie. As currently coded – this is
only appropriate for use on the PEARL instrument.

Full details of how the temperature information is extracted from the peak shape parameters, can be found in the
references below. Essentially, there are two components contributing to the peak shape, the Gaussian and Lorentzian.
First, the calibration run must be performed to fit the instrument contributions to the line-shape, and account for any
other effects to the line-shape at a given sample pressure. After this, only the Gaussian component is fitted to extract
the temperature. Note that a recalibration is required for each given sample pressure.

Version 2
---------
For version 1 of this algorithm, see :ref:`PEARLTransfit-v1 <algm-PEARLTransfit-v1>`.

In version 2 of this algorithm:
    - The Output Workspace and Parameters Table are created as output properties and not directly extracted from the fit, this preserves workspace history.
    - The `Output` property is a string that sets the basename for outputs in the ADS.
    - There are no inputs for background parameter estimation, these can be provided in the `InputCalibrationParameters`.
    - There is no rebin in constant energy.
    - In non-calibration fits, a debug table with information about sample temperature can be output if `CreateDebugTable` is set to `True`. The output table also contains the computed sample temperature.


If no `InputCalibrationParameters` is provided on a calibration run, the background parameters will be estimated from the data.
This generates an output with the suffix `_Parameters` on the ADS containing, among others, background estimation parameters.
This table can be used on successive calls to input background parameters. If custom background parameters need to be provided in a calibration run,
a table workspace with the appropriate parameters can be created from the following script.

.. code:: python

    def create_input_calibration_table(bg0, bg1, bg2, name="InputCalibrationParameters"):
        table = CreateEmptyTableWorkspace(OutputWorkspace=name)

        table.addColumn("str","Name")
        table.addColumn("float","Value")

        for idx, bg in enumerate([bg0, bg1, bg2]):
            table.addRow({"Name":f"Bg{idx}", "Value": bg})
        return table

    table = create_input_calibration_table(10, 0.01, 0.000004)
    cal_ws, params_table = PEARLTransfit(Files='PEARL00112777', FoilType="Hf01", Calibration=True, InputCalibrationParameters=table, Output='pearl_cal')

See script below for usage.

.. testcode:: PEARLTransfitV2Example

    from mantid.simpleapi import *

    # Calibration
    cal_ws, params_table = PEARLTransfit(Files='PEARL00112777', FoilType="Hf01", Calibration=True, EstimateBackground=True, Output='pearl_cal')

    # Using results from calibration
    non_cal_ws, params_table, debug_table= PEARLTransfit(Files='PEARL00112777', FoilType="Hf01", Calibration=False, InputCalibrationParameters=params_table, CreateDebugTable=True)

    # Debug table contains temperature and fitting information.
    row_temp_debug = debug_table.row(debug_table.rowCount()-1)
    print(f"Sample Temperature (K) {row_temp_debug['Value']:.3f} +- {row_temp_debug['Error']:.3f}")
    # Fit Parameters table also includes calculated temperature
    row_temp_fit_table = params_table.row(params_table.rowCount()-1)
    print(f"Sample Temperature (K) {row_temp_fit_table['Value']:.3f} +- {row_temp_fit_table['Error']:.3f}")

Output:

.. testoutput:: PEARLTransfitV2Example

   Sample Temperature (K) 279.760 +- 14.717
   Sample Temperature (K) 279.760 +- 14.717

References
----------

‘Temperature measurement in a Paris-Edinburgh cell by neutron resonance spectroscopy' - Journal Of Applied Physics 98, 064905 (2005)
'Remote determination of sample temperature by neutron resonance spectroscopy' - Nuclear Instruments and Methods in Physics Research A 547 (2005) 601-615

.. categories::

.. sourcelink::
