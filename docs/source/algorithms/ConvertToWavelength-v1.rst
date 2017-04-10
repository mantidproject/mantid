.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm performs a combination of unit conversion and rebinning.


Usage
-----

.. include:: ../usagedata-note.txt

**Example - Convert sample workspace to wavelength:**

.. testcode:: ExConvertToWavelength

    ws = CreateSampleWorkspace(NumBanks=1, BankPixelWidth=1)
    converted = ConvertToWavelength(InputWorkspace=ws, WavelengthLow=1, WavelengthHigh=4, WavelengthStep=1, WavelengthStepType="Lin", RebinMode="Rebin")

    print("There should be 4 values and got {}.".format(len(converted.dataX(0))))

.. testcleanup:: ExRelativeChangeTimeZero

   DeleteWorkspace('ws')
   DeleteWorkspace('converted')


Output:

.. testoutput:: ExConvertToWavelength

  There should be 4 values and got 4.

.. categories::

.. sourcelink::
