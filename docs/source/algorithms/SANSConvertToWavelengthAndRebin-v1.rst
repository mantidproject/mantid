.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm converts the input workspace into a workspace with wavelength units. Subsequently it rebins the
wavelength-valued workspace. Either :ref:`algm-Rebin` or :ref:`algm-InterpolatingRebin` is used for rebinning. This algorithm
is geared towards SANS workspaces.


Usage
-----

.. include:: ../usagedata-note.txt

**Example - Convert sample workspace to wavelength and rebin:**

.. testcode:: ExSANSConvertToWavelengthAndRebin

    ws = CreateSampleWorkspace(NumBanks=1, BankPixelWidth=1)
    converted = SANSConvertToWavelengthAndRebin(InputWorkspace=ws, WavelengthLow=1, WavelengthHigh=4, WavelengthStep=1, WavelengthStepType="Lin", RebinMode="Rebin")

    print("There should be 4 values and got {}.".format(len(converted.dataX(0))))

.. testcleanup:: ExSANSConvertToWavelengthAndRebin

   DeleteWorkspace('ws')
   DeleteWorkspace('converted')


Output:

.. testoutput:: ExSANSConvertToWavelengthAndRebin

  There should be 4 values and got 4.

.. categories::

.. sourcelink::
