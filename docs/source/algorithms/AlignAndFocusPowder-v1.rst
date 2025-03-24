.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This is a workflow algorithm that does the bulk of the work for time focusing diffraction data.
This is done by executing several sub-algorithms as listed below.
The way that algorithms are connected together is better described in the workflow diagram

- :ref:`algm-ApplyDiffCal`
- :ref:`algm-CompressEvents` (event workspace only)
- :ref:`algm-ConvertDiffCal` to convert legacy offsets workspace to difcal table
- :ref:`algm-ConvertUnits`
- :ref:`algm-DiffractionFocussing`
- :ref:`algm-EditInstrumentGeometry`
- :ref:`algm-LoadDetectorsGroupingFile` when grouping file is specified without a calibration file
- :ref:`algm-LoadDiffCal`
- :ref:`algm-AppendSpectra` only used when ``LowResRef`` is specified
- :ref:`algm-LorentzCorrection`
- :ref:`algm-MaskBins` for filtering out absorption resonances
- :ref:`algm-MaskBinsFromTable` for filtering out single crystal peaks from powder data
- :ref:`algm-MaskDetectors`
- :ref:`algm-Rebin`
- :ref:`algm-ResampleX`
- :ref:`algm-RebinRagged`
- :ref:`algm-RemoveLowResTOF` ``CropWavelengthMin`` and ``CropWavelengthMax`` are prefered
- :ref:`algm-RemovePromptPulse`
- :ref:`algm-SortEvents` (event workspace only)



Workflow
########

The main workflow of the algorithm can be described in following diagram.
In this diagram, the ``CalibrationWorkspace``, ``MaskWorkspace``, and ``GroupingWorkspace`` are abstractions of the many ways that this information can be provided (see below).

.. diagram:: AlignAndFocusPowder-v1_wkflw.dot

Calibration
###########

The way that calibration is supplied can be confusing.
This section will attempt to clarify it.

**Workspaces provided:**
If the ``GroupingWorkspace``, ``CalibrationWorkspace``, or ``MaskWorkspace`` are supplied as parameters they are used.
If the ``OffsetsWorkspace`` is supplied it is converted to a ``CalibrationWorkspace`` using :ref:`algm-ConvertDiffCal`.
The values of ``CalFileName`` and ``GroupFilename`` will be ignored

**Filenames provided, Workspaces not:**
Assuming the instrument short-name is ``<INSTR>`` (replace with the actual instrument short-name), the algorithm will look for the workspaces ``<INSTR>_group``, ``<INSTR>_cal`` (falling back to ``<INSTR>_offsets``), and ``<INSTR>_mask`` and use them without consulting the files.
This behavior is to reduce the amount of overhead in processing a collection of input data.
When loading information from a file, all 3 workspaces can be read from either type of calibration file (``.h5`` or ``.cal``) and will get the default names.
If the ``GroupFilename`` is provided, that will override the grouping information in the ``CalFilename``.

.. note::
   If the user wishes to force reading the supplied calibration file(s), they must delete the workspaces ``<INSTR>_group``, ``<INSTR>_cal``, ``<INSTR>_offsets``, and ``<INSTR>_mask``.


Usage
-----

**Example: A simple Powgen example**

The files needed for this example are not present in our standard usage data
download due to their size.  They can however be downloaded using these links:
`PG3_9830_event.nxs <https://github.com/mantidproject/systemtests/blob/master/Data/PG3_9830_event.nxs?raw=true>`_
and
`pg3_mantid_det.cal <https://testdata.mantidproject.org/ftp/external-data/MD5/e2b281817b76eadbc26a0a2617477e97>`_.

You will have to rename :literal:`pg3_mantid_det.cal` manually, as its name in the link above is a list of random characters.

.. code-block:: python

    PG3_9830_event = Load('PG3_9830_event.nxs')
    PG3_9830_event = AlignAndFocusPowder(PG3_9830_event,
                                         CalFileName='pg3_mantid_det.cal', Params='100')


.. categories::

.. sourcelink::
