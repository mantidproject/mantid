.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm converts the input workspace and a JSON list of wavelength tuples into n workspaces with wavelength units.
Subsequently it rebins the wavelength-valued workspace. Either :ref:`algm-Rebin` or :ref:`algm-InterpolatingRebin`
is used for rebinning. This algorithm is geared towards SANS workspaces.


Usage
-----

.. include:: ../usagedata-note.txt

**Example - Convert sample workspace to wavelength and rebin:**

.. testcode:: ExSANSConvertToWavelengthAndRebin

    ws = CreateSampleWorkspace(NumBanks=1, BankPixelWidth=1)
    wavelength = (1, 4)  # 1-4λ

    # import json
    # json_string = json.dumps([wavelength])  # this can be used for wavelength pairs too
    converted_ws_group = SANSConvertToWavelengthAndRebin(InputWorkspace=ws, WavelengthPairs="[[1, 4]]",
                                                        WavelengthStep="1", WavelengthStepType="Lin",
                                                        RebinMode="Rebin")
    first_workspace = converted_ws_group.getItem(0)
    print("There should be 4 values and got {}.".format(len(first_workspace.dataX(0))))

.. testcleanup:: ExSANSConvertToWavelengthAndRebin

   DeleteWorkspace('ws')
   DeleteWorkspace('converted_ws_group')


Output:

.. testoutput:: ExSANSConvertToWavelengthAndRebin

  There should be 4 values and got 4.

.. categories::

.. sourcelink::
