.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Start a (new) transaction on a remote compute resource. You need to
start a transaction before submitting jobs with
:ref:`algm-StartRemoteTransaction`, and or uploading or downloading
files with :ref:`algm-UploadRemoteFile` and
:ref:`algm-DownloadRemoteFile`. The concept of transaction is
described in the :ref:`Mantid remote job submission API docs
<RemoteJobSubmissionAPI>`. Note that
(depending on the implementation of this algorithm) the environment,
and files and jobs available are specific to a transaction.

Transactions created with this algorithm can be cancelled or killed
with the algorithm :ref:`algm-StopRemoteTransaction`.

For more specific details on remote algorithms when using the Mantid
web service remote job submission API, see the :ref:`remote job submission
API docs <RemoteJobSubmissionAPI>`.

Previous Versions
-----------------

Version 1
#########

Version 1 creates transactions using the Start action of the :ref:`Mantid
remote job submission API
<RemoteJobSubmissionAPI>`. This is
still supported as one of the variants of Versions 2 and above, when
the compute resource uses the Mantid remote job submission API as job
manager (underlying remote job scheduling mechanism).

.. categories::

.. sourcelink::
