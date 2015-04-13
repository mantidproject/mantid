.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Download a file from a remote compute resource. Files are available
for transactions, and you can retrieve the list of available files
with :ref:`algm-QueryRemoteFile`. See
:ref:`algm-StartRemoteTransaction` for how to start (create) a
newtransaction.

For specific details on remote algorithms when using the Mantid web
service remote job submission API, see the `remote job submission API
docs <http://www.mantidproject.org/Remote_Job_Submission_API>`_.

Previous Versions
-----------------

Version 1
#########

Version 1 downloads files using the `Mantid remote job submission API
<http://www.mantidproject.org/Remote_Job_Submission_API>`_. This is
still supported as one of the variants of Versions 2 and above, when
the compute resource uses the Mantid remote job submission API as job
manager (underlying remote job scheduling mechanism).

.. categories::
