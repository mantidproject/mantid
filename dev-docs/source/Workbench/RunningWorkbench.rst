.. _RunningWorkbench:

=================
Running Workbench
=================

How does Workbench get run
##########################

Mantid Workbench is run on two separate Processes. The first process, known as the parent process, creates a second process using the
`multiprocessing python package <https://docs.python.org/3/library/multiprocessing.html>`_, known as the child process. The responsibilities
of the two processes are roughly as follows:

- The child process is the process on which the Workbench is opened. If there is a terminating fault that occurs when using the Workbench,
  such as a segmentation fault, this process will exit. Alternatively, a close event triggered by a user will also cause this process to exit.

- The responsibility of the parent process is to wait for the child process to exit. When the child process exits, a decision is made whether
  an ErrorReporter window needs to be opened depending on the exit code of the Workbench.

A note on debugging
###################

The default behaviour of Workbench is to start as a parent and child process as described above. For debugging, we want to attach to the main process that users see (i.e. the child process). If we attach to the parent process, your breakpoints will not be hit. To ensure we attach to the correct process, we can run the Workbench with the ``--single-process`` flag.

.. code-block:: sh

  workbench --single-process

For a developer environment you might also want to specify the ``PYTHONPATH`` where ``<config>`` is only required when using the VS generator on Windows.

.. code-block:: sh

  PYTHONPATH=/path/to/build/bin/<config> workbench --single-process
