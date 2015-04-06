.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Submit a job to be executed on the specified (remote) compute
resource. After this, you can query the status of this job or all the
jobs using the algorithms :ref:`algm-QueryRemoteJob` and
:ref:`algm-QueryAllRemoteJobs`, respectively. Jobs submitted with this
algorithm can be cancelled/killed/aborted with the algorithm
:ref:`algm-AbortRemoteJob`.

Note that the script and script parameters properties are used in an
implementation dependent way. For example, if using the Mantid web
service job submission API, the script parameters properties is used
to provide the actual content of a python script to run. For other
variants (underlying job schedulers), such as Platform LSF, the
parameters property is used for different application specific command
line options.  For more details on remote algorithms when using the
Mantid web service remote job submission API, see the `remote job
submission API docs
<http://www.mantidproject.org/Remote_Job_Submission_API>`_.

.. categories::
