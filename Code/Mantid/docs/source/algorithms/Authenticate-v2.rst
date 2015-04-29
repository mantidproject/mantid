.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Authenticate to the remote compute resource. This must be executed
before calling any other remote algorithms. The authentication method
and outcome of ths algorithm is dependent on the particular
implementation (job manager underlying the algorithm). But typically,
if the authentication is successfull, a cookie is received that is
stored internally and re-used for all subsequent interactions with the
compute resource.

In the simplest use case of remote algorithms, once you authenticate
with this algorithm, you can create a transaction with
:ref:`algm-StartRemoteTransaction`. Then you can submit jobs (with
:ref:`algm-SubmitRemoteJob`, query the status of jobs (with
:ref:`algm-QueryAllRemoteJobs` and :ref:`algm-QueryRemoteJob`), upload
files (with :ref:`algm-UploadRemoteFile`) or download files (with
:ref:`algm-QueryRemoteFile` and :ref:`algm-DownloadRemoteFile`).

For specific details on remote algorithms when using the Mantid web
service remote job submission API, see the `remote job submission API
docs <http://www.mantidproject.org/Remote_Job_Submission_API>`_.

Previous Versions
-----------------

Version 1
#########

Version 1 authenticates to a Mantid web service using the `Mantid
remote job submission API
<http://www.mantidproject.org/Remote_Job_Submission_API>`_. This is
still supported as one of the variants of Versions 2 and above, when
the compute resource uses the Mantid remote job submission API as job
manager (underlying remote job scheduling mechanism).

.. categories::
