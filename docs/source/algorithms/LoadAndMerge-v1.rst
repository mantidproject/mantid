.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm loads multiple runs conforming the rules in :py:obj:`MultipleFileProperty <mantid.api.MultipleFileProperty>`.
When listing is requested, the output is a :ref:`WorkspaceGroup <WorkspaceGroup>`, containing items for each run.
When summing is requested, it is performed pair-wise *in-situ* with :ref:`MergeRuns <algm-MergeRuns>`.
By doing this, the metadata (SampleLogs) are also merged governed by the rules specified in the Instrument Parameter File (IPF)
as explained in :ref:`MergeRuns <algm-MergeRuns>`.
This rules can be overridden by the **MergeRunsOptions** input.
Specific loader can be given by the **LoaderName** and **LoaderVersion**.
If left as defaults, the algorithm will automatically find out the corresponding specific loader, deducing from the first run in the list.
It will set the name and the version of the suitable loader found as output properties in **LoaderName** and **LoaderVersion**.
The input runs must be loadable by the same specific loader, the files can not be mixed.
If the **Output** is hidden, i.e. the name starts with **__**, all the workspaces (intermediate and final) produced by this algorithm will also be hidden,
but stored in the AnalysisDataService in any case.

Usage
-----

.. include:: ../usagedata-note.txt

.. testsetup:: LoadAndMerge

  config.setFacility('ILL')
  config.appendDataSearchSubDir('ILL/IN16B/')

.. testcode:: LoadAndMerge

  out = LoadAndMerge(Filename='170257+170258,170300+170302')
  print('out is a WorkspaceGroup, containing {0} workspaces'.format(out.getNumberOfEntries()))
  print('the first item is the merged output of the runs 170257 and 170258 with the name {0}'.format(out.getItem(0).getName()))
  print('the second item is the merged output of the runs 170300 and 170302 with the name {0}'.format(out.getItem(1).getName()))

.. testcleanup:: LoadAndMerge

  mtd.clear()

Output
######

.. testoutput:: LoadAndMerge

  out is a WorkspaceGroup, containing 2 workspaces
  the first item is the merged output of the runs 170257 and 170258 with the name 170257_170258
  the second item is the merged output of the runs 170300 and 170302 with the name 170300_170302

Related algorithms
------------------

:ref:`Load <algm-Load>` performs the same operation, except the summing of runs is conducted by :ref:`Plus <algm-Plus>`.

.. categories::

.. sourcelink::
