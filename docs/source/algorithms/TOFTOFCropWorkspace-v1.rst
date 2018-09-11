.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Applies algorithm :ref:`algm-Cropworkspace` to an input workspace or a group of workspaces to crop the empty time channels. Boundaries are calculated as follows:

    :math:`X_{min} = 0`

    :math:`X_{max} = N_{fc}\times\Delta t`

where :math:`N_{fc}` is the number of full time channels defined in the *full_channels* sample log and :math:`\Delta t` is the channel width defined in the *channel_width* sample log.


Restrictions on the input workspace
###################################

-  The unit of the X-axis must be **Time-of-flight**.
-  Workspace must contain *channel_width*, *full_channels* and *TOF1* sample logs.


Usage
-----

**Example**

.. testcode:: ExTOFTOFCropWorkspace

    # Load data
    ws=Load(Filename='TOFTOFTestdata.nxs')

    print("Input workspace")

    print("Total number of time channels: {}".format(len(ws.readX(0))))
    print ("Number of filled time channels: {}".format(ws.getRun().getLogData('full_channels').value))

    wscropped = TOFTOFCropWorkspace(ws)

    print("Output workspace")
    print("Total number of time channels: {}".format(len(wscropped.readX(0))))


Output:

.. testoutput:: ExTOFTOFCropWorkspace

    Input workspace
    Total number of time channels: 1025
    Number of filled time channels: 1020.0
    Output workspace
    Total number of time channels: 1020
    
.. categories::

.. sourcelink::
