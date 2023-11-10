.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm saves the data and relevant metadata from a reduced ISIS Reflectometry workspace into the ASCII implementation of the ORSO data standard [#ORSO]_.
The orsopy library [#orsopy]_ is used to format and write out the ``.ort`` file.
Currently this algorithm is unable to collect all of the mandatory information required by the ORSO standard so it produces a file that is not fully ORSO compliant.
A comment is included at the top of the file to reflect this. This algorithm is only suitable for use with Reflectometry data collected at the ISIS Neutron and Muon facility.

The ``InputWorkspace`` passed to the algorithm should be a reduced Reflectometry workspace in units of momentum transfer (Q).
The algorithm attempts to find metadata for the file header and the resolution (dQ/Q) from the instrument, workspace history and sample logs associated with the workspace.
As a result, this algorithm may produce a file that is missing information if it is passed a workspace that wasn't reduced via the :ref:`ISIS Reflectometry Interface <interface-isis-refl>`.
See below for further information about where the metadata and resolution are searched for in the workspace.

This algorithm currently does not work correctly with workspace groups.

Data values
-----------

The saved ``.ort`` file contains at least three columns of data: the normal wavevector transfer (Qz), the reflectivity (R) and the error of the reflectivity.
The data is converted to point data using algorithm :ref:`algm-ConvertToPointData` before being saved to file.

If parameter ``WriteResolution`` is set to ``True`` then the algorithm will also attempt to include a fourth column that calculates the resolution of the normal wavevector transfer as: :math:`resolution * Qz`.
The resolution (dQ/Q) is looked up from the workspace history as follows:

- Find the last occurrence of :ref:`algm-Stitch1DMany` in the workspace history. If this can be found, then the absolute value of the stitch ``Params`` parameter is used for the resolution.
- Otherwise, find the last occurrence of :ref:`algm-ReflectometryReductionOneAuto`. This algorithm makes a call to :ref:`algm-Rebin` and the absolute value of the middle rebin ``Params`` parameter is used as the resolution.

If a resolution value cannot be found from the workspace history then a warning is logged and the file is saved without this column included.

Header Metadata
---------------

Some of the metadata for the ORSO file header is retrieved directly from the input workspace, as described below:

+---------------------+-----------------------------------------------------------------------------------------------+
| Header value        | Workspace location                                                                            |
+=====================+===============================================================================================+
| instrument          | The name of the instrument associated with the workspace.                                     |
+---------------------+-----------------------------------------------------------------------------------------------+
| start_date          | The value of the ``run_start`` sample log.                                                    |
+---------------------+-----------------------------------------------------------------------------------------------+
| proposalID          | The value of either the ``rb_proposal`` or ``experiment_identifier`` sample log.              |
+---------------------+-----------------------------------------------------------------------------------------------+
| sample name         | The workspace title (same as the value of the ``run_title`` sample log).                      |
+---------------------+-----------------------------------------------------------------------------------------------+
| reduction timestamp | The execution time of the last occurrence of :ref:`algm-ReflectometryReductionOneAuto` in the |
|                     | workspace history.                                                                            |
+---------------------+-----------------------------------------------------------------------------------------------+

Usage
-----

**Example - Save a workspace in ISIS reflectometry ORSO ASCII format**

.. testcode:: SaveISISReflectometryORSO_general_usage

    # import the os path libraries for directory functions
    import os

    ws = CreateSampleWorkspace(XUnit="MomentumTransfer")

    # Create an absolute path by joining the proposed filename to a directory
    # os.path.expanduser("~") used in this case returns the home directory of the current user
    file = os.path.join(os.path.expanduser("~"), "ws")

    # Add Sample Log entries
    AddSampleLog(Workspace=ws, LogName='rb_proposal', LogText='1234', LogType='Number')

    # Save the ORSO file
    SaveISISReflectometryORSO(InputWorkspace=ws, Filename=file, WriteResolution=False)

    # Open the file and read the first line
    if os.path.exists(file + ".ort"):
      with open((file + ".ort"), 'r') as myFile:
        print(myFile.readline())

.. testoutput:: SaveISISReflectometryORSO_general_usage
   :options: +NORMALIZE_WHITESPACE

   # # ORSO reflectivity data file | 1.0 standard | YAML encoding | https://www.reflectometry.org/

.. testcleanup:: SaveISISReflectometryORSO_general_usage

   if os.path.exists(file + ".ort"):
     # Delete file
     os.remove(file + ".ort")

References
----------

.. [#ORSO] ORSO file format specification: `https://www.reflectometry.org/file_format/specification <https://www.reflectometry.org/file_format/specification>`_
.. [#orsopy] orsopy Python library: `https://orsopy.readthedocs.io/en/latest/ <https://orsopy.readthedocs.io/en/latest/>`_

.. categories::

.. sourcelink::
