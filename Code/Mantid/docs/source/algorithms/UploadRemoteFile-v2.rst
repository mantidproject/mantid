.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Transfers/uploads a file to the specified (remote) compute
resource. Presumably, the file is a script or input data
necessary to run a job on the remote compute resource.

The destination directory depends on the specified transaction ID, and
its interpretation (absolute or relative to a job or transaction
environment) is implementation dependent.  See
:ref:`algm-StartRemoteTransaction` for how to start a transaction.

For more details on remote algorithms when using the Mantid web
service remote job submission API, see the `remote job submission API
docs <http://www.mantidproject.org/Remote_Job_Submission_API>`_.

Previous Versions
-----------------

Version 1
#########

Version 1 transfers files using the `Mantid remote job submission API
<http://www.mantidproject.org/Remote_Job_Submission_API>`_. This is
still supported as one of the variants of Versions 2 and above, when
the compute resource uses the Mantid remote job submission API as job
manager (underlying remote job scheduling mechanism).

.. categories::
