.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Query information status for all the jobs available on the (remote)
compute resource. In principle this includes all the jobs submitted,
but in some cases (implementation dependent) the compute resource may
not report anything about old finished jobs. Note that the output
properties are all arrays. There will be one element for each job that
was reported by the compute resource.

For specific details on remote algorithms when using the Mantid web
service remote job submission API, see the `remote job submission API
docs <http://www.mantidproject.org/Remote_Job_Submission_API>`_.

Previous Versions
-----------------

Version 1
#########

Version 1 queries the status of jobs using the `Mantid remote job
submission API
<http://www.mantidproject.org/Remote_Job_Submission_API>`_. This is
still supported as one of the variants of Versions 2 and above, when
the compute resource uses the Mantid remote job submission API as job
manager (underlying remote job scheduling mechanism).

.. categories::
