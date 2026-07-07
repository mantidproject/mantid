.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm saves the data and relevant metadata from a reduced ISIS Reflectometry workspace into either the ASCII (``.ort``) or Nexus (``.orb``) implementation of the ORSO data standard [#ORSO]_.
The orsopy library [#orsopy]_ is used to format and write out the file.
Currently this algorithm is unable to collect all of the mandatory information required by the ORSO standard so it produces a file that is not fully ORSO compliant.
A comment is included at the top of the file to reflect this. This algorithm is only suitable for use with Reflectometry data collected at the ISIS Neutron and Muon facility.

The ``WorkspaceList`` passed to the algorithm should be a list of one or more reduced Reflectometry workspaces, or workspace groups, in units of momentum transfer (:math:`Q`).
For each individual workspace (When in ``History`` mode for the ``MetadataSource`` property), the algorithm attempts to find metadata for the file header and the resolution (:math:`dQ/Q`) from the instrument, workspace history and sample logs associated with the workspace.
As a result, this algorithm may produce a file that is missing information if it is passed a workspace that wasn't reduced via the :ref:`ISIS Reflectometry Interface <interface-isis-refl>`.
See below for further information about where the metadata and resolution are searched for in the workspace.

If more than one workspace name is passed in then the algorithm will save all the data and metadata to a single ORSO file as a series of datasets.
Dataset names must be unique, so an error will be thrown if duplicate dataset names are generated from the workspaces in the list. See below for more information about how these are generated.

When passed a workspace group, the algorithm will save each workspace in the group as an individual dataset within the same file, as described above.

The ``Filename`` property is used to provide the save location and name for the file. The save path must end with a valid ORSO file extension - use ``.ort`` to save into the ORSO ASCII format or ``.orb`` to save into the ORSO Nexus format.

Dataset names
-------------

A dataset name is generated automatically for each individual workspace in the input list or workspace group. This is done as follows:

- If there is a call to :ref:`algm-Stitch1DMany` in the workspace history then the dataset is given the name "Stitched".
- If it is not a stitched dataset then, if available in the workspace reduction history, the value of theta that was used for conversion to Q is given as the dataset name.
- If a dataset name cannot be generated from either of the above, then the workspace name is used as the dataset name.

For a workspace that is a member of a workspace group, if the generated dataset name is either "Stitched" or the theta value, we include the following additional
information in the name to make it unique:

- If the data is polarized, then we include the spin state using the ORSO notation for polarization.
- If the data is not polarized, then we include the individual workspace name.

Depending on the combination of workspaces and workspace groups passed in, it is possible that duplicate dataset names will be generated. If this happens then the algorithm will give an error and
fail to save out a file.

Metadata Sources
----------------

In its default state, the algorithm will attempt to get as many of the metadata as it can from the workspace history, which requires
that the workspace has been processed using the same workflow used in the :ref:`ISIS Reflectometry Interface <interface-isis-refl>`.
This is not compatible with all reduction workflows, however, and so this algorith also provides methods for fine-tuning where the
saving process sources any metadata.

+--------------------------+-----------------------------------------------------------------------------------------------+
| MetadataSource           | Collection Process                                                                            |
+==========================+===============================================================================================+
| ``History``              | The input workspace's history and sample logs will be searched for each of the metadata items.|
|                          | Any that cannot be found will be omitted from the final saved ORSO file.                      |
+--------------------------+-----------------------------------------------------------------------------------------------+
| ``HistoryWherePossible`` | As above, but any missing metadata will be marked as an error on the relevant algorithm       |
|                          | property during validation. If any marked properties are not expected to be found or are not  |
|                          | required, a list of those properties names should be given as inputs to ``IgnoredProperties``.|
+--------------------------+-----------------------------------------------------------------------------------------------+
| ``Manual``               | Only the values entered for the relevant properties will be included in the output file. A    |
|                          | list of the metadata and their relevant properties is included below.                         |
+--------------------------+-----------------------------------------------------------------------------------------------+

Data values
-----------

The saved ORSO file contains at least three columns of data: the normal wavevector transfer (:math:`Qz`), the reflectivity (:math:`R`) and the error of the reflectivity.
The data is converted to point data using algorithm :ref:`algm-ConvertToPointData` before being saved to file.

If parameter ``WriteResolution`` is set to ``True`` then the algorithm will also attempt to include a fourth column that calculates the resolution of the normal wavevector transfer as: :math:`resolution * Qz`.
The resolution (:math:`dQ/Q`) is looked up as follows:

- In ``History`` or the first step of ``HistoryWherePossible`` mode:

  - Find the last occurrence of :ref:`algm-Stitch1DMany` in the workspace history. If this can be found, then the absolute value of the stitch ``Params`` parameter is used for the resolution.
  - Otherwise, find the last occurrence of :ref:`algm-ReflectometryReductionOneAuto`. This algorithm makes a call to :ref:`algm-Rebin` and the absolute value of the middle rebin ``Params`` parameter is used as the resolution.

- In ``Manual`` or the manual step of ``HistoryWherePossible`` mode:

  - The value is taken from the ``Resolution`` property.

If a resolution value cannot be found from the workspace history or from the property then the file is saved without this column included.

If parameter ``IncludeAdditionalColumns`` is set to ``True`` then the value of parameter ``WriteResolution`` is ignored and the algorithm will output the four columns described above for stitched datasets.
For non-stitched datasets there will be the four columns described above plus an additional four columns as follows:

- *lambda* - the wavelength values.

  - If the original conversion to Q was performed using :ref:`algm-RefRoi` then the ``Qz`` column values are converted back to wavelength using: :math:`\lambda=\frac{4\pi}{Q}sin(\theta)`.
  - If the original conversion was performed using :ref:`algm-ConvertUnits` then this algorithm is used to convert back to wavelength.
  - Note: The method used is either determined from the workspace history (in ``History`` or ``HistoryWherePossible`` modes) or from the ``QConversionMethod`` property (in ``Manual`` or
    ``HistoryWherePossible`` mode. When using the ``HistoryWherePossible`` mode, the value in the workspace history will override the setting chosen in the property.)

- *error of lambda* - currently assumed to be 0.
- *incident theta* - the value of theta used for the final conversion to :math:`Q`.
- *error of incident theta* - calculated as :math:`resolution * \theta`.

If it is not possible to calculate the values for the additional columns then a warning is logged and they are excluded from the file.

Header Metadata
---------------

Some of the metadata for the ORSO file header is retrieved directly from the input workspace, as detailed below.
For values retrieved from the workspace history, in ``History`` mode, if any information cannot be extracted from the history then
the file is saved without this metadata included.

+---------------------+-----------------------------------------------------------------------------------------------+---------------------------------+
| Header value        | Workspace location                                                                            | Manual Metadata Property        |
+=====================+===============================================================================================+=================================+
| instrument          | The name of the instrument associated with the workspace.                                     | N/A                             |
+---------------------+-----------------------------------------------------------------------------------------------+---------------------------------+
| start_date          | The value of the ``run_start`` sample log.                                                    | N/A                             |
+---------------------+-----------------------------------------------------------------------------------------------+---------------------------------+
| proposalID          | The value of either the ``rb_proposal`` or ``experiment_identifier`` sample log.              | N/A                             |
+---------------------+-----------------------------------------------------------------------------------------------+---------------------------------+
| sample name         | The workspace title (same as the value of the ``run_title`` sample log).                      | N/A                             |
+---------------------+-----------------------------------------------------------------------------------------------+---------------------------------+
| reduction timestamp | The execution time of the last occurrence of :ref:`algm-ReflectometryReductionOneAuto` in the | ``ReductionTimestamp``          |
|                     | workspace history.                                                                            |                                 |
+---------------------+-----------------------------------------------------------------------------------------------+---------------------------------+
| reduction call      | The sequence of algorithm calls from the workspace history that is generated by               | ``ReductionScript``             |
|                     | :ref:`algm-GeneratePythonScript`. This is excluded for workspaces that are members of a       |                                 |
|                     | workspace group.                                                                              |                                 |
+---------------------+-----------------------------------------------------------------------------------------------+---------------------------------+
| measurement         | The individual file names for all of the run numbers passed to the ``InputRunList`` parameter | | ``AngleFileList``             |
| data_files          | from all calls to :ref:`algm-ReflectometryISISLoadAndProcess` in the workspace history.       | | ``AngleFileThetaList``        |
+---------------------+-----------------------------------------------------------------------------------------------+---------------------------------+
| measurement         | The individual file names for all of the run numbers passed to parameters                     | | ``FirstTransmissionFileList`` |
| additional_files    | ``FirstTransmissionRunList`` and ``SecondTransmissionRunList`` from all calls to              | | ``SecondTransmissionFileList``|
|                     | :ref:`algm-ReflectometryISISLoadAndProcess` in the workspace history. Also the flood          | | ``FloodCorrectionSource``     |
|                     | correction workspace or file name and the calibration file name from                          | | ``CalibrationFile``           |
|                     | :ref:`algm-ReflectometryISISLoadAndProcess` in the workspace history.                         |                                 |
+---------------------+-----------------------------------------------------------------------------------------------+---------------------------------+
| polarization        | For input workspaces containing the ``spin_state_ORSO`` sample log, polarization information  | N/A                             |
|                     | will be added to the header using the ORSO format [#ORSO]_.                                   |                                 |
+---------------------+-----------------------------------------------------------------------------------------------+---------------------------------+

Usage
-----

**Example - Save a workspace in ISIS Reflectometry ORSO ASCII format**

.. testcode:: SaveISISReflectometryORSO_general_usage

    # import the os path libraries for directory functions
    import os

    ws = CreateSampleWorkspace(XUnit="MomentumTransfer", NumBanks=1, BankPixelWidth=1)

    # Create an absolute path by joining the proposed filename to a directory
    # os.path.expanduser("~") used in this case returns the home directory of the current user
    # Specify the .ort extension to save to the ORSO ASCII format
    file = os.path.join(os.path.expanduser("~"), "ws.ort")

    # Add Sample Log entries
    AddSampleLog(Workspace=ws, LogName='rb_proposal', LogText='1234', LogType='Number')

    # Save the ORSO file
    SaveISISReflectometryORSO(WorkspaceList=ws, Filename=file, WriteResolution=False)

    # Open the file and read the first line
    if os.path.exists(file):
      with open((file), 'r') as myFile:
        print(myFile.readline())

.. testoutput:: SaveISISReflectometryORSO_general_usage
   :options: +ELLIPSIS +NORMALIZE_WHITESPACE

   # # ORSO reflectivity data file | ... standard | YAML encoding | https://www.reflectometry.org/

.. testcleanup:: SaveISISReflectometryORSO_general_usage

   if os.path.exists(file):
     # Delete file
     os.remove(file)

References
----------

.. [#ORSO] ORSO file format specification: `https://www.reflectometry.org/file_format/specification <https://www.reflectometry.org/file_format/specification>`_
.. [#orsopy] orsopy Python library: `https://orsopy.readthedocs.io/en/latest/ <https://orsopy.readthedocs.io/en/latest/>`_

.. categories::

.. sourcelink::
