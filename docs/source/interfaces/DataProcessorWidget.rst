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
interface and the minimal requirements needed to build it and use it. However, this guide is built with
Reflectometry at ISIS in mind, as it is the only used case at the moment.

What the widget is for
----------------------

The DataProcessor widget is a technique-independent *MantidWidget* that can be used to execute complex
batch-processing via *DataProcessorAlgorithms* by reducing sequentially groups of runs that can be later
post-processed.

The widget, displayed below, consists in a set of table editing options, a processing table where runs can be entered
for reduction, a progress bar reporting progress, a combo-box where different
instruments can be selected, a checkbox to request a notebook which will contain all the steps taking
place in the reduction, and a button to start processing the table.

.. figure:: /images/DataProcessorDevDocs/data-processor-widget.png

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

    whitelist = MantidQt.MantidWidgets.DataProcessor.WhiteList()
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

    whitelist = MantidQt.MantidWidgets.DataProcessor.WhiteList()
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

    preprocess_map = MantidQt.MantidWidgets.DataProcessor.PreprocessMap()
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

    preprocess_map = MantidQt.MantidWidgets.DataProcessor.PreprocessMap()
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
in a hinting line edit associated with the algorithm. Note that such hinting line edit is
not included by default in the widget, and has to be added manually.

Note that, at the moment, even if users are not supposed to enter more than
one run, you'll have to specify a pre-processing algorithm so that runs can be loaded.

.. note::

   This is an optional argument.

Note that if a pre-processed run already exists in the ADS with the same name, the widget will use it
to avoid loading it again.

.. _processing-algorithm-label:

Processing Algorithm
^^^^^^^^^^^^^^^^^^^^

The processing algorithm refers to the main reduction algorithm that is used to reduce the runs. Processing
algorithms must satisfy the following conditions:

- Only algorithms with at least one input workspace property are allowed.
- Only algorithms with at least one output workspace property are allowed.
- The above refers to :ref:`MatrixWorkspaces <MatrixWorkspace>` and :ref:`Workspaces <Workspace>` only.

A processing algorithm can be created like this:

.. code-block:: python

    alg = MantidQt.MantidWidgets.DataProcessor.ProcessingAlgorithm('ReflectometryReductionOneAuto','IvsQ_binned_, IvsQ_, IvsLam_')

This tells the widget that each rown in the table should be reduced with :ref:`algm-ReflectometryReductionOneAuto`, and
the output workspaces resulting from the reduction should be named with prefixes :literal:`IvsQ_binned_`, :literal:`Ivs_Q` and
:literal:`IvsLam_`. The number of comma-separated prefixes must match the number of output workspaces
of the algorithm, otherwise an excpetion will be thrown when constructing the widget. Additionally, a
blacklist of algorithms properties can be provided as a string of comma-separated algorithm property names:

.. code-block:: python

    alg = MantidQt.MantidWidgets.DataProcessor.ProcessingAlgorithm('ReflectometryReductionOneAuto', 'IvsQ_binned_, IvsQ_, IvsLam_',
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

The only effect of the blacklist is on the *Options* column, not in the reduction. This column uses a *HintingLineEdit* (a MantidWidget)
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

The widget iterates over each column. If the cell is not empty,
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
an example, in Reflectometry at ISIS, a run typically consists in two or three runs measured
under the same conditions of temperature, magnetic field, etc, but at different incident
angles. These runs belong to the same group and need to be stitched together. The post-processing
algorithm is in this case :ref:`algm-Stitch1DMany`, and can be defined as:

.. code-block:: python

    post_alg = MantidQt.MantidWidgets.DataProcessor.PostprocessingAlgorithm('Stitch1DMany', 'IvsQ_')

As with pre-processing and processing algorithms, a third parameter indicating the list of properties
to blacklist can be used. As with the pre-process map, you must add manually a hinting line edit
and link the post-processing black list to it, as this functionality is not available by default.

.. code-block:: python

    post_alg = MantidQt.MantidWidgets.DataProcessor.PostprocessingAlgorithm('Stitch1DMany', 'IvsQ_', 'InputWorkspaces, OutputWorkspaces')

.. note::

   This is an optional argument.

Note that this is an optional argument. When a post-processing algorithm is defined, the table is arranged
as a two-level tree where parent items are groups and child items are runs. Different runs (child items) can belong
to the same group (parent) as shown on the left figure below. Note that you can enter a name for the group but
it will not be used in the reduction. However, when no post-processing is defined, parent items no longer make
sense, and the table is arranged as a on-level tree, as illustrated on the right figure below. Note that
table editing options referring to groups are also removed. See section :ref:`table-editing-and-notebook-label`
for more details.

.. figure:: /images/DataProcessorDevDocs/table-with-post-processing.png

.. note::

   Note that, because the widget is currently only used in the ISIS Reflectometry interface with a
   post-processing algorithm, the functionality without it is not well tested in terms of the
   table-editing options, more specifically in terms of adding/deleting/copying/pasting rows.

In addition to the post-processing algorithm, a post-process map can also be specified (note that this is a C++
feature only which is not currently exposed to Python). A post-process map is a STL map where keys are
column names and values are algorithm property names referring to the post-processing algorithm. This can
be used when you need to use the values in a column as input properties to the post-processing algorithm.

The widget
^^^^^^^^^^

One you have defined all the elements above, at least the mandatory ones, the widget can be created
like this in Python (in C++ the code would be the equivalent of the code below):

.. code-block:: python

    data_processor_table = MantidQt.MantidWidgets.QDataProcessorWidget(whitelist, preprocess_map, alg, post_alg, self)

Loading Algorithm
^^^^^^^^^^^^^^^^^

By default, the widget will use :ref:`algm-Load` to load the runs.

In C++ it is possible to specify the loading algorithm the widget should use (for instance,
in Reflectometry at ISIS we use :ref:`algm-LoadISISNexus`). However, at the moment
this is only possible if both pre-processing and post-processing algorithms are specified. The only
reason for this is that it was requested by Reflectometry scientists at ISIS, who work with pre-processing
and post-processing. However, if you need to implement this, all you need to do is add an optional
string argument to the relevant :literal:`GenericDataProcessorPresenter` constructor. For instance,
assuming that you don't need to pre-process and post-process groups of runs, the constructor:

.. code-block:: c

    // Constructor: no pre-processing, no post-processing
    GenericDataProcessorPresenter(
        const WhiteList &whitelist,
        const ProcessingAlgorithm &processor);

should become:

.. code-block:: c

    // Constructor: no pre-processing, no post-processing
    GenericDataProcessorPresenter(
        const WhiteList &whitelist,
        const ProcessingAlgorithm &processor,
        const std::string &loader = "Load");

Then in the implementation, the following should be enough:

.. code-block:: c

    /**
    * Delegating constructor: no pre-processing, no post-processing
    * @param whitelist : The set of properties we want to show as columns
    * @param processor : The processing algorithm
    * @param loader :: The loading algorithm
    */
    GenericDataProcessorPresenter::GenericDataProcessorPresenter(
        const WhiteList &whitelist,
        const ProcessingAlgorithm &processor,
        const std::string &loader)
        : GenericDataProcessorPresenter(
              whitelist,
              std::map<std::string, PreprocessingAlgorithm>(),
              processor, PostprocessingAlgorithm(),
              std::map<std::string, std::string>(), loader) {}

In addition, if you are using the widget in a Python interface, you will have to expose this
functionality using SIP. You first need to modify the relevant :literal:`QDataProcessorWidget`
constructor and make it pass the loading algorithm to the :literal:`GenericDataProcessorPresenter`.
Assuming the example above, i.e. no pre-processing and no post-processing, the constructor:

.. code-block:: c

    // Constructor: no pre-processing, no post-processing
    QDataProcessorWidget(const WhiteList &,
                         const ProcessingAlgorithm &,
                         QWidget *parent);

should become:

.. code-block:: c

    // Constructor: no pre-processing, no post-processing
    QDataProcessorWidget(const WhiteList &,
                         const ProcessingAlgorithm &,
                         const QString &loader,
                         QWidget *parent);

and then the implementation would be:

.. code-block:: c

    /** Delegating constructor, no pre-processing, no post-processing
    * @param whitelist :: The white list
    * @param algorithm :: The processing algorithm
    * @param loader :: The loading algorithm
    * @param parent :: The parent of this view
    */
    QDataProcessorWidget::QDataProcessorWidget(
        const WhiteList &whitelist,
        const ProcessingAlgorithm &algorithm,
		const QString &loader, QWidget *parent)
        : QDataProcessorWidget(
              std::make_unique<GenericDataProcessorPresenter>(whitelist,
                                                                         algorithm,
                                                                         loader.toStdString()),
              parent) {}

Finally, you will need to modify file :literal:`qt/python/mantidqtpython/mantidqtpython_def.sip` to include the
above constructor:

.. code-block:: c

    class QDataProcessorWidget : QWidget
    {
    %TypeHeaderCode
    #include "MantidQtWidgets/DataProcessorUI/QDataProcessorWidget.h"
    %End
    public:
    QDataProcessorWidget(const MantidQt::MantidWidgets::DataProcessor::WhiteList &,
                         const MantidQt::MantidWidgets::DataProcessor::ProcessingAlgorithm &,
                         const QString &,
                         QWidget *parent );
    ...
    }

.. _table-editing-and-notebook-label:

Table editing actions
^^^^^^^^^^^^^^^^^^^^^

The widget comes with a set of table-editing options. Some of them are shown in the toolbar above the
processing table:

.. figure:: /images/DataProcessorDevDocs/table-editing-options.png

These are also shown in a context menu when clicking on a row in the table:

.. figure:: /images/DataProcessorDevDocs/table-editing-options-context-menu.png

Other actions are not shown by default but the widget can export them so that they can be added to the parent
widget containing the data processor widget. In the example below, all the available editing options
have been added to two menus: a *File* menu, which contains actions to save/load/open a new table, as well
as general options related to error/warning messages and rounding, and an *Edit* menu containing
the options shown on the toolbar:

.. figure:: /images/DataProcessorDevDocs/table-editing-options-outside-widget.png

Note that when no post-processing algorithm are defined, some of the options that refer to groups
do not make sense, and therefore, they are not shown and cannot be accessed. Below is a description
of the available actions.

+------------------+-------------------------------------------------------------------------------------------------------------------------+
| Action           | Description                                                                                                             |
+==================+=========================================================================================================================+
| Open Table       | A submenu containing a list of valid TableWorkspaces that can be loaded in the processing table                         |
|                  | for processing. Valid table workspaces are those who have the same number of columns as the processing table.           |
+------------------+-------------------------------------------------------------------------------------------------------------------------+
| New Table        | Discards the current contents of the processing table                                                                   |
|                  | presenting a blank table.                                                                                               |
+------------------+-------------------------------------------------------------------------------------------------------------------------+
| Save Table       | Saves the current contents of the processing to the TableWorkspaces it came from. If no such                            |
|                  | workspace already exists, a new one can be created.                                                                     |
+------------------+-------------------------------------------------------------------------------------------------------------------------+
| Save Table As    | Saves the current contents of the processing table to a new table workspace.                                            |
+------------------+-------------------------------------------------------------------------------------------------------------------------+
| Import .TBL      | Opens a :ref:`LoadTBL <algm-LoadTBL>` dialog, enabling you to load a ``.tbl`` file into the processing table. A table   |
|                  | workspace is also created in the ADS.                                                                                   |
+------------------+-------------------------------------------------------------------------------------------------------------------------+
| Export .TBL      | Opens a :ref:`SaveTBL <algm-SaveTBL>` dialog, enabling you to save a table workspace to a ``.tbl`` file.                |
+------------------+-------------------------------------------------------------------------------------------------------------------------+
| Options          | Opens the *Options* menu. This menu allows to adjust settings related to warning/error messages and rounding options.   |
+------------------+-------------------------------------------------------------------------------------------------------------------------+
| Process          | Processes the selected runs, or, if no runs are selected, all of the runs in the table. When post-processing is         |
|                  | defined and a group is selected, runs belonging to the same group are post-processed together.                          |
+------------------+-------------------------------------------------------------------------------------------------------------------------+
| Expand Selection | This action is only available when post-processing is defined. It expands your selection such that the group containing |
|                  | the row you have selected is selected.                                                                                  |
+------------------+-------------------------------------------------------------------------------------------------------------------------+
| Plot Selected    | Creates a plot of the reduced workspaces generated by any of the selected rows.                                         |
| Rows             |                                                                                                                         |
+------------------+-------------------------------------------------------------------------------------------------------------------------+
| Plot Selected    | Only available when post-processing is defined. Creates a plot of the post-processed workspaces generated               |
| Groups           | by any of the selected groups.                                                                                          |
+------------------+-------------------------------------------------------------------------------------------------------------------------+
| Insert Row       | Adds a new row after the first selected row, or at the end of the group if a group was selected. If nothing             |
|                  | was selected the new row is appended at the end of the                                                                  |
|                  | last group.                                                                                                             |
+------------------+-------------------------------------------------------------------------------------------------------------------------+
| Insert Group     | Only available when post-processing is defined. Adds a new group after the first selected group, or at                  |
|                  | the end of the table if no groups were selected.                                                                        |
+------------------+-------------------------------------------------------------------------------------------------------------------------+
| Group Rows       | Only available when post-processing is defined. Takes all the selected rows and places them in a group                  |
|                  | together, separate from any other group.                                                                                |
+------------------+-------------------------------------------------------------------------------------------------------------------------+
| Copy Rows        | Copies the selected rows to the clipboard. In the clipboard, each column's value is separated by a tab, and             |
|                  | each row is placed on a new line.                                                                                       |
+------------------+-------------------------------------------------------------------------------------------------------------------------+
| Cut Rows         | Copies the selected rows, and then deletes them.                                                                        |
+------------------+-------------------------------------------------------------------------------------------------------------------------+
| Paste Rows       | Pastes the contents of the clipboard into the selected rows. If no rows are selected, new rows are inserted.            |
+------------------+-------------------------------------------------------------------------------------------------------------------------+
| Clear Rows       | Resets the cells in any selected rows to their initial value, in other words, blank.                                    |
+------------------+-------------------------------------------------------------------------------------------------------------------------+
| Delete Row       | Deletes any selected rows. If no rows are selected, nothing happens. For groups, if the single row of a group is        |
|                  | selected for deletion, the group will also be deleted.                                                                  |
+------------------+-------------------------------------------------------------------------------------------------------------------------+
| Delete Group     | Only available when post-processing is defined. Deletes any selected Groups. If no groups are selected,                 |
|                  | nothing happens.                                                                                                        |
+------------------+-------------------------------------------------------------------------------------------------------------------------+
| What's This      | Provides guidance on what various parts of the interface are for.                                                       |
+------------------+-------------------------------------------------------------------------------------------------------------------------+

Signals emitted by the widget
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The widget emits a :literal:`runPythonrunPythonCode(const QString &)` signal to plot workspace and load/save
a .tbl file. The parent widget containing the data processor widget must catch this signal and re-emit it
so that the python code is executed.

Notebook
^^^^^^^^

The widget includes a checkbox called "Output Notebook" that, when selected, will produce an IPython Notebook
including all the steps taking place in the reduction.

.. figure:: /images/DataProcessorDevDocs/notebook.png

How to build the widget (II)
----------------------------

.. _output-workspace-name-label:

How to control the name of the output workspaces
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The widget will use the data in the table to generate a name for the output workspace. The way
in which the output name is generated also depends on the way the whitelist has been defined and
on the prefixes specified in the processing algorithm (and post-processing algorithm if defined).

First, the name of the reduced workspaces will start with the prefix specified when constructing
the processing algorithm, that is, if the processing algorithm was created as:

.. code-block:: python

    alg = MantidQt.MantidWidgets.ProcessingAlgorithm('ReflectometryReductionOneAuto','IvsQ_binned_, IvsQ_, IvsLam_','')

the name of the first output workspace returned by the processing algorithm will start with prefix
:literal:`IvsQ_binned_`, the name of the second output workspace return by the algorithm will start with
:literal:`IvsQ_`, and the third output workspace name will start with :literal:`IvsLam_`.
Next, the whitelist is considered: only those columns with fourth argument set to true will be considered.
In addition, if a prefix was also specified, it will be added to the name too. For instance, if we have a white list:

.. code-block:: python

    # White list
    whitelist.addElement('Runs', 'InputWorkspace', 'The run to reduce', True, '')
    whitelist.addElement('Angle', 'ThetaIn', 'The incident angle', False, '')
    whitelist.addElement('Transmission Runs', 'FirstTransmissionRun', 'Transmission runs', False, '')
    whitelist.addElement('Q min', 'MomentumTransferMin', 'Q min', True, 'q_')
    whitelist.addElement('Q max', 'MomentumTransferMax', 'Q max', False, '')
    whitelist.addElement('dQ/Q', 'MomentumTransferStep', 'Resolution', False, '')
    whitelist.addElement('Scale', 'ScaleFactor', 'Scale Factor', False, '')

and a table:

.. figure:: /images/DataProcessorDevDocs/output-ws-names.png

The names of the reduced workspaces will be :literal:`IvsQ_binned_13460_q_0.01_0.3`,
:literal:`IvsQ_13460_q_0.01_0.3` and :literal:`IvsLam_13460_q_0.01_0.3` respectively for the first row, as columns
:literal:`Runs`, :literal:`Q min` and :literal:`Q max` have been marked to generate the workspace names,
and in addition a prefix has been added to column :literal:`Q min`. Analogously, for the second row,
the reduced workspaces will be named :literal:`IvsQ_binned_13462_q_0.01_0.3`,
:literal:`IvsQ_13462_q_0.01_0.3` and :literal:`IvsLam_13462_q_0.01_0.3` respectively.

If a post-processing algorithm is defined:

.. code-block:: python

    post_alg = MantidQt.MantidWidgets.DataProcessor.PostprocessingAlgorithm('Stitch1DMany', 'stitched_', 'InputWorkspaces, OutputWorkspaces')

the name of the post-processed workspace will start with prefix specified in the post-processing algorithm, stitched in this case,
plus the names of the reduced workspaces without their prefixes joined with "_". That is, in this example we would get a workspace
called :literal:`stitched_13460_q_0.01_0.06_13462_q_0.035_0.3`.

How to auto-populate columns after the reduction
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Columns left empty will be auto-populated after the reduction with the default values of
the corresponding algorithm properties.

How to specify the list of available instruments
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Once the widget has been created, you can specify the list of instruments that should appear in the instrument combo box:

.. figure:: /images/DataProcessorDevDocs/instrument-combo-box.png

like this:

.. code-block:: python

    data_processor_table.setInstrumentList('INTER, POLREF, OFFSPEC', 'INTER')

where the first argument is a comma-separated list of instruments, and the second argument is the instrument that will be
set by default when opening the interface. In C++, this can be done in a similar way using the method
:literal:`GenericDataProcessorPresenter::setInstrumentList(const std::vector<std::string> &instruments, const std::string &defaultInstrument)`,
where the list of instruments is specified as a vector of strings.

Transferring runs to the table
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Runs can be transferred to the table using the method :literal:`transfer()`. In C++ this method takes a vector of maps as the argument, where
each vector represents a row, and maps contain for each row (key) the value that should be inserted into the table (value). For instance, the following:

.. code-block:: c

    std::vector<std::map<std::string, std::string>> runs = {
      {{"Group", "0"}, {"Runs", "13460"}, {"Angle", "0.5"}, {"Scale", "1"}},
      {{"Group", "0"},{{"Runs", "13462"}, {"Angle", "1.5"}, {"Scale", "2"}}}};

will add two new rows populated with values:

.. figure:: /images/DataProcessorDevDocs/transferred-runs.png

Note that a key "Group" must be specify with a value corresponding to the name of the group where the runs will be added.
If no post-processing algorithm is specified, it can be omitted. The equivalent in Python is a Qlist<QString> as shown below:

.. code-block:: python

    self.data_processor_table.transfer(['Group:0,Runs:13460,Angle:0.5,Scale:1', 'Group:0,Runs:13462,Angle:1.5,Scale:2'])

How to use global options for the reduction
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Global options are options common to all the rows in the processing table. At the moment, they have to be specified outside the widget.
In Reflectometry for instance, this is done via a separate tab called *Settings* where users can enter values that will be used to reduce all the rows
in the processing table. This behaviour is achieved by making the parent containing the widget inherit from *DataProcessorMainPresenter*.
This is an abstract base class defining methods to retrieve global options for pre-processing, processing, and post-processing. More
specifically, the methods you will have to implement are listed below:

- getPreprocessingOptionsAsString(): returns pre-processing options as a string. As there may be more than one column that need
  pre-processing, the string returned by this method must have the following format:
  - Options to different pre-processing algorithms (i.e. columns) must be separated by ";"
  - For each column, the name of the column and the different options must be specified as comma-separated strings of :literal:`key=value` pairs.
  Example: :literal:`Runs, AllowDifferentNumberSpectra=1; Transmission Runs, WavelengthMin=2.0, WavelengthMax=4.0` will make the
  widget apply :literal:`AllowDifferentNumberSpectra=1` to :ref:`algm-Plus` and :literal:`WavelengthMin=2.0, WavelengthMax=4.0` to
  :ref:`algm-CreateTransmissionWorkspaceAuto`, assuming the pre-process map defined in section :ref:`pre-processing-algorithm-label`.
- getProcessingOptions(): returns processing options as a string of comma-separated :literal:`key=value` pairs. Example:
  :literal:`WavelengthMin=3.5, Params="1,2,3"`.
- getPostprocessingOptions(): similar to the previous one. Example: :literal:`ScaleRHSWorkspace=1, ManualScaleFactor=0.5`.

In addition, because the widget is a *WorkspaceObserver* observing changes in the ADS, you may want to implement method
notifyADSChanged(). The purpose of this method is to update the *OpenTable* action (see section :ref:`table-editing-and-notebook-label`
for more details about this command) with the list of table workspace that can be loaded into the interface.

How to use the widget from a C++ interface
------------------------------------------

An example of a C++ interface currently using the widget is the Reflectometry GUI at ISIS. The relevant
classes creating and communicating with the widget are listed below:

- :literal:`QtReflRunsTabView`
- :literal:`ReflRunsTabPresenter`

Both can be found in MantidQt/CustomInterfaces, the first one is the view in the MVP pattern, responsible
for creating the widget, and the second one is the presenter in the MVP pattern, responsible for interacting
with the widget in terms of providing global options for the reduction.

How to use the widget from a Python interface
---------------------------------------------

There is a toy example written in Python that can be found in mantid/scripts/Interface/ui/dataprocessorinterface.
It is currently invisible to users, but you can make it visible for you by adding :literal:`Utility/DataProcessorInterface.py` to
Framework/Properties/Mantid.properties.template. This will make the toy example appear under category :literal:`Utility`.
