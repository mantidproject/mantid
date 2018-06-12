.. _Loading:

=============
 Loading Data
=============

Mantid is equipped with functionality to load many different types of files into workspaces. This service is provided by the :ref:`Load <algm-Load>` generic algorithm, that can be easily accessed by the **Load** button, and then clicking **File** item in the menu.

.. image:: /images/Training/MantidPlotBasics/load-button.png
  :alt: Load button
  :align: center

This will open the **Load Dialog**, where one can browse to the file, and click **Run**.
If the directory containing the file is added to the ``datasearch.directories`` (see the ...), it is enough to type in the run number.

.. image:: /images/Training/MantidPlotBasics/load-dialog.png
  :alt: Load dialog
  :align: center

Under the Hood
--------------

There exist a specific loader algorithm for each of the distinct file types supported. One can find these loaders in the list of the algorithms, under the category **DataHandling**.
As a general rule, their names start with ``Load``, usually also followed by the name of the facility, and some further specifier of the file type, e.g. :ref:`LoadILLTOF <algm-LoadILLTOF>`.
It is the job of the generic :ref:`Load <algm-Load>` algorithm to automatically infer the corresponding specific loader for the file supplied and execute it.
Note, that the **Load Dialog** will update itself dynamically based on the type of file selected, hence some input and output options may appear or disappear depending on the file.

Multiple File Loading
---------------------

The individual specific loaders are designed to load one file at a time.
However, the generic :ref:`Load <algm-Load>` algorithm is able to load many files simultaneously.
The syntax for multiple file loading involves the use of several context-sensitive operators.
A quick example is shown below, the full functionality is explained in :py:obj:`MultipleFileProperty <mantid.api.MultipleFileProperty>`.

+--------------+-----------------------------+-------------------+-----------------------------------+
| Name         | Usage                       | Example Input     | Example Result                    |
+==============+=============================+===================+===================================+
| List         | ``<run>,<run>``             | ``1,2,3``         | Loads runs 1,2 and 3              |
+--------------+-----------------------------+-------------------+-----------------------------------+
| Sum          | ``<run>+<run>``             | ``1+2+3``         | Loads and sums runs 1, 2 and 3    |
+--------------+-----------------------------+-------------------+-----------------------------------+
| Range        | ``<run>:<run>``             | ``1:4``           | Loads runs 1, 2, 3 and 4          |
+--------------+-----------------------------+-------------------+-----------------------------------+
| Summed Range | ``<run>-<run>``             | ``1-4``           | Loads and sums runs 1, 2, 3 and 4 |
+--------------+-----------------------------+-------------------+-----------------------------------+

When summing is requested, :ref:`Plus <algm-Plus>` algorithm is invoked. This sums the workspace data, but takes the metadata only from the first operand.
If the metadata needs to be treated (e.g. durations of data taking should also be summed), one should use :ref:`LoadAndMerge <algm-LoadAndMerge>` instead of :ref:`Load <algm-Load>`.
This will execute :ref:`MergeRuns <algm-MergeRuns>` instead for summing, where one can define the metadata merging behavior.
Note, that multiple file loading works only over a set of homogeneous files, i.e. files of the same type loadable by a single specific loader.

ILL Raw Data
------------
Presently at the ILL, only files in **.nxs** format produced by the following instruments are supported:

* IN4, IN5, IN6
* IN16B
* D20, D2B
* D17, FIGARO
* D11, D22, D33

Note, that since the ILL raw files do not contain the instrument name in the file names (in contrast to the other facilities), it is necessary to have the ``default.facility`` set correspondingly.
