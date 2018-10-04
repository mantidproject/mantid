.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm is called on a regular interval by the
:ref:`algm-MonitorLiveData` algorithm. and the whole process is
started by the :ref:`algm-StartLiveData` algorithm.  **It should not
be necessary to call LoadLiveData directly.**

.. figure:: /images/LoadLiveData_flow.png
   :alt: LoadLiveData_flow.png

   LoadLiveData\_flow.png

Data Processing
###############

-  Each time LoadLiveData is called, a chunk of data is loaded from the
   live listener.

   -  This consists of all the data collected since the previous call.
   -  The data is saved in a temporary :ref:`workspace <workspace>`.

-  You have two options on how to process this workspace:

Processing with an Algorithm
++++++++++++++++++++++++++++

-  Specify the name of the algorithm in the ``ProcessingAlgorithm``
   property.

   -  This could be a python algorithm written for this purpose
   -  The algorithm *must* have at least 2 properties: ``InputWorkspace``
      and ``OutputWorkspace``.
   -  Any other properties are set from the string in
      ``ProcessingProperties``.
   -  The algorithm is then run, and its ``OutputWorkspace`` is saved

Processing with a Python Script
+++++++++++++++++++++++++++++++

The python script is run using :ref:`algm-RunPythonScript`. Please see
its documentation for details of how the script is run.

-  Specify a python script in the ``ProcessingScript`` property.

   -  This can have several lines.
   -  Two variables have special meaning:

      -  ``input`` is a reference to the input workspace.
      -  ``output`` is the name of the processed, output workspace.

   -  Otherwise, your script can contain any legal python code including
      calls to other Mantid algorithms.
   -  If you create temporary workspaces, you should delete them in the
      script.

-  Specify a python script in the ``ProcessingScriptFilename`` property.

   - Contents of the file have the exact same rules as specifying the ``ProcessingScript``

.. note::

   When ``PreserveEvents`` is enabled and the data has not been binned
   during the process step (with ``ProcessingAlgorithm``,
   ``ProcessingScript``, or ``ProcessingScriptFilename``), the data
   will be rebinned at the end of the step to include all events. Use
   the Post-Process step instead for :ref:`EventWorkspaces <EventWorkspace>`.

Data Accumulation
#################

-  The ``AccumulationMethod`` property specifies what to do with each
   chunk.

   -  If you select ``Add``, the chunks of processed data will be added
      using :ref:`algm-Plus` or :ref:`algm-PlusMD`.
   -  If you select ``Replace``, then the output workspace will always be
      equal to the latest processed chunk.
   -  If you select ``Append``, then the spectra from each chunk will be
      appended to the output workspace.

.. warning::

   Beware! If you select ``PreserveEvents=True`` and your processing
   keeps the data as :ref:`EventWorkspaces <EventWorkspace>`, you may end
   up creating **very large** EventWorkspaces in long runs. Most plots
   require re-sorting the events, which is an operation that gets much
   slower as the list gets bigger (Order of :math:`N * log(N)`). This
   could cause Mantid to run very slowly or to crash due to lack of
   memory.

   It is highly recommended that early in the PostProcessing step one
   uses :ref:`CompressEvents <algm-CompressEvents>` if the data is going
   to remain in events.

Post-Processing Step
####################

- Optionally, you can specify some processing to perform *after*
  accumulation.

  -  You then need to specify the ``AccumulationWorkspace`` property.

- Using either the ``PostProcessingAlgorithm``, ``PostProcessingScript``,
  or ``PostProcessingScriptFilename`` (same way as above), the
  ``AccumulationWorkspace`` is processed into the ``OutputWorkspace``

Usage
-----

LoadLiveData is not intended for usage directly, it is part of he
process that is started using :ref:`algm-StartLiveData`.


.. categories::

.. sourcelink::
