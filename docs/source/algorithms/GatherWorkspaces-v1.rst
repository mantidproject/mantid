.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

GatherWorkspaces is only available for MPI builds.
It stitches together the input workspaces provided by each of the processes into a single workspace in the root process.
The spectra in the output workspace will be ordered by the rank of the input processes.
It is up to the caller to ensure this results in the required ordering.
Furthermore, there are all sorts of things that ought to be consistent for this algorithm to make sense (e.g. the instrument). The general philosophy, though, is to leave the responsibility for this to the user and only check the vital things (i.e. that the number of bins is consistent).

.. categories::

.. sourcelink::
