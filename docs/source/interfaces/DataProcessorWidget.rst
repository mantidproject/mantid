.. _DataProcessorWidget_DevelopersGuide-ref:

DataProcessor widget: a guide for Mantid developers 
===================================================

.. contents:: Table of Contents
    :local:
    
Introduction
------------

This document is intended for Mantid developers as a guide to the architecture of the *DataProcessor* widget.
This is a C++ widget exposed to Python that can be found in *mantid/MantidQt/MantidWidgets/*. It consists of
several classes defining the widget behaviour as well as unit tests covering most of its functionality.

A design document including motivation, proposed solution and main requirements for the
widget can be found in https://github.com/mantidproject/documents/tree/master/Design/DataProcessorAlgorithmUI.

The purpose of this document is to explain and illustrate how the widget can be inserted into a custom
interface and the minimal requirements needed to build it and use it. However, this guide is built for
Reflectometry reduction at ISIS, as it is the only used case at the moment.

Say here that this is just the current implementation, but it doesn't mean that it cannot be
changed in the future if needed. Say also that any new functionality should be ideally tested.

What the widget is for
----------------------

The DataProcessor widget is a technique-independent *MantidWidget* that can be used to execute complex
batch-processing via *DataProcessorAlgorithms* by reducing sequentially groups of runs that can be later 
post-processed.

***Image of the widget here***

How to build the widget
-----------------------

Different elements can be specified to build the widget. Some of them are mandatory. Below is a description
of the available elements that can be used to build and define the behaviour of the widget.

.. _whitelist-label:

Whitelist
^^^^^^^^^

A *whitelist* is an object defining the number of columns, their names, and the algorithm property names linked to them.
Below is an example illustrating how to create a white list and add some columns using the public method *addElement()*:

.. code-block:: python

    whitelist = MantidQt.MantidWidgets.DataProcessorWhiteList()
    whitelist.addElement('Runs', 'InputWorkspace', 'The run to reduce')
    whitelist.addElement('Angle', 'ThetaIn', 'The incident angle')
    whitelist.addElement('Transmission Runs', 'FirstTransmissionRun', 'Transmission runs')
    whitelist.addElement('Q min', 'MomentumTransferMin', 'Q min')
    whitelist.addElement('Q max', 'MomentumTransferMax', 'Q max')
    whitelist.addElement('dQ/Q', 'MomentumTransferStep', 'Resolution')
    whitelist.addElement('Scale', 'ScaleFactor', 'Scale Factor')

Note that, in addition to the specified columns, an extra column called *Options* is always added. Users can use
this column to override default property values of the main reduction algorithm. It uses a *HintingLineEdit* delegate to
display the main reduction algorithm properties and provides auto-completion capability. See section
:ref:`processing-algorithm-label` for more information.

The example above defines a table with seven columns, where the first column is named :literal:`Run`, is linked to an algorithm property
named :literal:`InputWorkspace` and has a description :literal:`The run to reduce`, the second column is named :literal:`Angle`, it is
linked to an algorithm property called :literal:`ThetaIn`, and has description :literal:`Angle in degrees`, and so on. Note that
there is no restriction in terms of the number of columns, their names, or the name of the algorithm properties linked to them.

Descriptions are shown as part of the "What's This?" mode which can be accessed by clicking on the "What's This?" action on the
toolbar (last icon) and the column of interest. Below is an example:

.. figure:: /images/DataProcessorDevDocs/column_description.png

The method *addElement()* also accepts two extra optional arguments which are used to determine the name of the
reduced workspace. See section :ref:`output-workspace-name-label` for more details. These extra parameters have
no effect on the way the table is displayed. Therefore, the following whitelist:

.. code-block:: python

    whitelist = MantidQt.MantidWidgets.DataProcessorWhiteList()
    whitelist.addElement('Runs', 'InputWorkspace', 'The run to reduce', True, '')
    whitelist.addElement('Angle', 'ThetaIn', 'The incident angle', False, '')
    whitelist.addElement('Transmission Runs', 'FirstTransmissionRun', 'Transmission runs', False, '')
    whitelist.addElement('Q min', 'MomentumTransferMin', 'Q min', False, '')
    whitelist.addElement('Q max', 'MomentumTransferMax', 'Q max', False, '')
    whitelist.addElement('dQ/Q', 'MomentumTransferStep', 'Resolution', False, '')
    whitelist.addElement('Scale', 'ScaleFactor', 'Scale Factor', False, '')

will produce a the same table as the first example.

.. note::

   This is a mandatory argument.

.. _pre-processing-algorithm-label:

Pre-processing Algorithms and Pre-process Map
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Pre-processing algorithms are algorithms used to pre-process certain columns. In Reflectometry at ISIS, we
need to pre-process columns :literal:`Run` and :literal:`Transmission` when users enter more than one run. As
an example, consider the following table:

.. figure:: /images/DataProcessorDevDocs/pre-processing.png

We want the widget to load run :literal:`INTER00001` (note the instrument combo box below the table), run
:literal:`INTER00002` and run :literal:`INTER00003` and sum these runs together. For transmission runs, we
also want to load runs :literal:`INTER00010` and :literal:`INTER00011` but in this case the runs should be
combined using a different algorithm, :ref:`algm-CreateTransmissionWorkspaceAuto`. This is achieved by using a
*pre-process* map, an object that uses a map behind the scenes, where keys are column
names and values are pre-processing algorithms. In this example, a pre-process map would be defined as:

.. code-block:: python

    preprocess_map = MantidQt.MantidWidgets.DataProcessorPreprocessMap()
    preprocess_map.addElement('Runs', 'Plus')
    preprocess_map.addElement('Transmission Runs', 'CreateTransmissionWorkspaceAuto')

This object tells the widget that runs in column :literal:`Runs` have to be combined using the
:ref:`algm-Plus` algorithm, and runs in column :literal:`Transmission Runs` have to be pre-processed
with :ref:`algm-CreateTransmissionWorkspaceAuto`. There is no restriction in the number of columns
to pre-process, but there are some limitations in terms of the allowed pre-processing algorithms:

- Only algorithms with **two** input workspace properties are allowed.
- Only algorithms with **one** output workspace property are allowed.
- The above refers to :ref:`MatrixWorkspaces <MatrixWorkspace>` and :ref:`Workspaces <Workspace>` only.

The pre-processing is a follows:

#. The widget loads the first two runs and puts them into the ADS.
#. It then runs the specified pre-processing algorithm on both workspaces and keeps a temporary workspace that is not added to the ADS.
#. It loads the third workspace and runs the specified pre-processing algorithm on this workspace and the temporary one from the previous step.

Note that in this context pre-processing refers to algorithms used to combine two or more
runs specified in the same column. It does not refer to additional processing of the loaded runs. For instance, the widget is not
able to :ref:`algm-Load` and :ref:`algm-Rebin` a workspace or apply any other algorithms to the loaded
run. This means that, if only one run is specified no pre-processing is needed, as the widget only has to load the
corresponding run. Runs that need to be pre-processed must be specified as a list separated by :literal:`+`
or :literal:`,`.

When pre-processing using the above pre-process map, the widget will create different workspaces in the ADS:

- A workspace named :literal:`13460` that corresponds to the loaded run :literal:`INTER00013460`
- A workspace named :literal:`13462` that corresponds to the loaded run :literal:`INTER00013462`
- A workspace named :literal:`13460_13462` that corresponds to the sum of the two workspaces above
- A workspace named :literal:`13463`, corresponding to run :literal:`INTER00013463`
- A workspace named :literal:`13464`, corresponding to run :literal:`INTER00013464`
- A workspace named :literal:`13463_13464` corresponding to the combined transmission run

The names of the above workspaces can be controlled to some extent by providing an extra parameter when
creating the pre-process map. This extra parameter corresponds to a prefix that will be added to
the loaded workspaces. The example below:

.. code-block:: python

    preprocess_map = MantidQt.MantidWidgets.DataProcessorPreprocessMap()
    preprocess_map.addElement('Runs', 'Plus', 'TOF_')
    preprocess_map.addElement('Transmission Runs', 'CreateTransmissionWorkspaceAuto', 'TRANS_')

Produces the following workspaces:

- Workspace :literal:`TOF_13460`, corresponding to run :literal:`INTER00013460`
- Workspace :literal:`TOF_13462`, corresponding to run :literal:`INTER00013462`
- Workspace :literal:`TOF_13460_13462`, corresponding to the sum of the two workspaces above
- Workspace :literal:`TRANS_13463`, corresponding to run :literal:`INTER00013463`
- Workspace :literal:`TRANS_13464`, corresponding to run :literal:`INTER00013464`
- Workspace :literal:`TRANS_13463_13464` corresponding to the combined transmission run

Additionally, there is one more extra parameter that can be specified. It is a list of comma-separated
strings that refer to the blacklist of algorithm properties that should not be shown
in a hinting line edit associated with the algorithm. For more details please see
section :ref:`hinting-line-edit-label`.

Note that, at the moment, ***say here that even if users are not supposed to enter more than
one run, you'll have to specify a pre-processing algorithm so that runs can be loaded***.

.. note::

   This is an optional argument.

.. _processing-algorithm-label:

Using runs that already exist in the ADS
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Processing Algorithm
^^^^^^^^^^^^^^^^^^^^

The processing algorithm refers to the main reduction algorithm that is used to reduce the runs. Processing
algorithms must satify the following conditions:

- Only algorithms with at least one input workspace property are allowed.
- Only algorithms with at least one output workspace property are allowed.
- The above refers to :ref:`MatrixWorkspaces <MatrixWorkspace>` and :ref:`Workspaces <Workspace>` only.

A processing algorithm can be created like this:

.. code-block:: python

    alg = MantidQt.MantidWidgets.DataProcessorProcessingAlgorithm('ReflectometryReductionOneAuto','IvsQ_binned_, IvsQ_, IvsLam_')

This tells the widget that each rown in the table should be reduced with :ref:`algm-ReflectometryReductionOneAuto`, and
the output workspaces resulting from the reduction should be named with prefixes :literal:`IvsQ_binned_`, :literal:`Ivs_Q` and
:literal:`IvsLam_`. The number of comma-separated prefixes must match the number of output workspaces
of the algorithm, otherwise an excpetion will be thrown when constructing the widget. Additionally, a
blacklist of algorithms properties can be provided as a string of comma-separated algorithm property names:

.. code-block:: python

    alg = MantidQt.MantidWidgets.DataProcessorProcessingAlgorithm('ReflectometryReductionOneAuto', 'IvsQ_binned_, IvsQ_, IvsLam_',
															  'InputWorkspace,'
															  'ThetaIn,'
															  'FirstTransmissionWorkspace,'
															  'SecondTransmissionWorkspace,'
															  'MomentumTransferMin,'
															  'MomentumTransferMax,'
															  'MomentumTransferStep,'
															  'ScaleFactor,'
															  'OutputWorkspaceBinned,'
															  'OutputWorkspace,'
															  'OutputWorkspaceWavelength,')

The only effect of the blaklist is on the *Options* column, not in the reduction. This column uses a *HintingLineEdit* (a MantidWidget)
delegate to provide auto-completion functionality so that when users start typing in this column, they get a list of algorithm
property names they can easily select. The figure below illustrates this behaviour:

.. figure:: /images/DataProcessorDevDocs/options-hinting-line-edit.png

Note that only those algorithm properties that have not been blacklisted are shown: :literal:`MomentumTransferMin`,
:literal:`MomentumTransferMax` and :literal:`MomentumTransferStep`, which are also input properties of our main
reduction algorithm, :ref:`algm-ReflectometryReductionOneAuto`, are not displayed when users start typing with character :literal:`M`.
Normally, you'd want to black list the input/output workspace properties and properties that are linked to the table columns. In this example,
as :literal:`ThetaIn` is linked to column :literal:`Angle` (see the whitelist definition in section :ref:`whitelist-label`),
:literal:`MomentumTransferMin` is linked to column :literal:`Q min` and so on, it does not make sense for them to appear
as additional options for the reduction.

To illustrate how the reduction takes place, consider the white list and pre-processing map defined
in the previous sections, and consider the following table:

.. figure:: /images/DataProcessorDevDocs/processing-example.png

The widget iterates over each column. It the cell is not empty,
it checks if the column needs to be pre-processed (essentially by checking if the column name is contained
in the pre-process map), and if so, loads and pre-processes the specified runs. Then it gets the algorithm
property name linked to the column and sets the pre-processed run as the workspace for that property. If the
column does not need to be pre-processed, it simply assigns the value in the cell to the algorithm property.
Below is a summary in pseudocode:

.. code-block:: c

    IAlgorithm_sptr alg =
          AlgorithmManager::Instance().create(processing algorithm name);
    alg->initialize();

    for (int i = 0; i < columns - 1; i++)
	  if (cell is not empty)

	    get the algorithm_property linked to this column from the white list;

		if (column_name in pre_process_map)
		  load and pre-process_runs;
		  alg->setPropertyValue(algorithm_property, pre_processed_runs);
	      
		else
		  alg->setPropertyValue(algorithm_property, cell);

Column *Options* is treated separately: the value in this cell is expected to be a comma-separated list of
input properties with their values, as illustrated in the figure above. The widget simply parses this string:

.. code-block:: c

    auto optionsMap = parseKeyValueString(options);
    for (auto kvp = optionsMap.begin(); kvp != optionsMap.end(); ++kvp) {
      try {
        alg->setProperty(kvp->first, kvp->second);
      } catch (Mantid::Kernel::Exception::NotFoundError &) {
        throw std::runtime_error("Invalid property in options column: " +
                                 kvp->first);
      }
    }

Finally the algorithm is executed and the widget reduces the next row in the same way.

.. note::

   This is a mandatory argument.

Post-processing Algorithm
^^^^^^^^^^^^^^^^^^^^^^^^^

A post-processing algorithm defines the way in which a group of runs should be post-processed. As
an example, in Reflectometry at ISIS, a run typically constists in two or three runs measured
under the same conditions of temperature, magnetic field, etc, but at different incident
angles. These runs belong to the same group and need to be stitched together. The post-processing
algorithm is in this case :ref:`algm-Stitch1DMany`, and can be defined as:

.. code-block:: python

    post_alg = MantidQt.MantidWidgets.DataProcessorPostprocessingAlgorithm('Stitch1DMany', 'IvsQ_')

As with pre-processing and processing algorithms, a third parameter indicating the list of properties
to blacklist can be used. See section :ref:`hinting-line-edit-label` for more details.
	
.. code-block:: python

    post_alg = MantidQt.MantidWidgets.DataProcessorPostprocessingAlgorithm('Stitch1DMany', 'IvsQ_', 'InputWorkspaces, OutputWorkspaces')

.. note::

   This is an optional argument.

Note that this is an optional argument. When a post-processing algorithm is defined, the table is arranged
as a two-level tree where parent items are groups and child items are runs. Different runs (items) can belong
to the same group(parent) as shown on the left figure below. Note that you can enter a name for the group but
it will be unused in the reduction. However, when no post-processing is defined, parent items no longer make
sense, and the table is arranged as a on-level tree, as illustrated on the right figure below. Note that
table editing options referring to groups are also removed.

.. figure:: /images/DataProcessorDevDocs/table-with-post-processing.png

Loading Algorithm
^^^^^^^^^^^^^^^^^

Not exposed to Python but would be easy to do.

Table editing actions & Notebook
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Context menu.

Plotting result workspaces
~~~~~~~~~~~~~~~~~~~~~~~~~~

How to build the widget (II)
----------------------------

.. _output-workspace-name-label:

How to control the name of the output workspaces
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

How to auto-populate columns after the reduction
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

How to specify the list of available instruments
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

How to use global options for the reduction
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. _hinting-line-edit-label:

Use of HintingLineEdit
----------------------

Transferring runs to the table
------------------------------

How to use the widget from a C++ interface
------------------------------------------

How to use the widget from a Python interface
---------------------------------------------

Re-definining the behaviour
---------------------------

If the current approach does not work for your technique area, you may still be able to use
most of the functionality related to the table, although, you may need to re-define how the
processing takes place. If this is the case, you can always derive from the C++
class :literal:`GenericDataProcessorPresenter` and re-implement method `process()`. When
doing so, you will have total freedom to define how the processing should take place. Below
is an example of such situation.

In SANS at ISIS, the table will typically consists of a certain number of columns
referring to different runs:

.. figure:: /images/DataProcessorDevDocs/sans-table.png

Changing the table editing features
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^



