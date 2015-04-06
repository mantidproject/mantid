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

.. categories::
