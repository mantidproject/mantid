.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Manipulating the data with constants
####################################

There are three properties which will modify the data after it is
reduced: ``PushDataPositive``, ``ScaleData``, ``OffsetData``. These
options are all performed on the data after all other operations. They
will be done as

.. math:: wksp = ScaleData * wksp + OffsetData

followed by :ref:`ResetNegatives <algm-ResetNegatives>` with
``AddMinimum`` being ``True`` if
``PushDataPositive='AddMinimum'``. :ref:`ResetNegatives
<algm-ResetNegatives>` is not run when ``PushDataPositive='None'``.

About Filter Wall
#################

Filter wall is enabled by setting ``FilterCharacterizations`` property
to true.  Then the ``_getTimeFilterWall`` routine is used to build the
filter wall from the data defined by ``SplitInformationWorkspace`` and
``SplittersWorkspace`` properties.  Time filter wall is used in
``_loadData`` to load data in a certain range of time. Here is how the
filter is used:

1. There is NO filter if filter wall is ``NONE``

2. There is NO lower boundary of the filter wall if ``wall[0]`` is
   ``ZERO``

3. There is NO upper boundary of the filter wall if ``wall[1]`` is
   ``ZERO``

More information is found in :ref:`EventFiltering`.

Calibration, grouping, and masking
##################################

There are two properties related to calibration (``CalibrationFile``
and ``GroupingFile``) and three implicit workspace dependencies
(``<instrument>_mask``, ``<instrument>_group``, and
``<instrument>_cal``). If the workspaces do not exist, they will be
created during the algorithm's execution.

If specified, the ``CalibrationFile`` is loaded using
:ref:`algm-LoadDiffCal`. If the ``GroupingFile`` is specified, the
grouping it specifies takes precedence over the one that is in the
``CalibrationFile``. :ref:`algm-LoadDetectorsGroupingFile` performs
the loading. The order of preference in which to use is (first found wins):

* Calibration: ``<instrument>_cal``, then ``CalibrationFile``

* Grouping: ``<instrument>_group`` workspace, then ``GroupingFile``,
  then ``CalibrationFile``

* Masking: ``<instrument>_mask``, then ``CalibrationFile``


:ref:`algm-AlignAndFocusPowder` does the actual resolution of which information to use.

Characterization runs
#####################

The ``CharacterizationRunsFile``, ``ExpIniFilename``,
``BackgroundNumber``, ``VanadiumNumber``, ``VanadiumBackgroundNumber``
all contribute to determine which runs to use for background
subtraction and normalization. Specifying the various ``*Number``
properties as ``0``, indicates using the values found in the
files. Specifying ``-1`` indicates to not do the corrections at
all. The files are loaded using :ref:`algm-PDLoadCharacterizations`
which also contains the focus positions. Which runs to used are
determined by :ref:`algm-PDDetermineCharacterizations`.



Workflow
--------

.. diagram:: SNSPowderReduction-v1_workflow.dot

.. diagram:: SNSPowderReduction-v1_vanadium.dot

.. diagram:: SNSPowderReduction-v1_process_container.dot

.. diagram:: SNSPowderReduction-v1_focusandsum.dot

.. diagram:: SNSPowderReduction-v1_focuschunks_workflow.dot

.. diagram:: SNSPowderReduction-v1_loadandsum.dot

Usage
-----

This is a worksflow algorithm used to process and the results of
powder diffraction scattering experimens on SNS
instruments. Processing data from instruments not at SNS is
unsupported. Sample usage can be found in the `system tests <https://github.com/mantidproject/mantid/blob/master/Testing/SystemTests/tests/analysis/SNSPowderRedux.py>`_.

.. categories::

.. sourcelink::
