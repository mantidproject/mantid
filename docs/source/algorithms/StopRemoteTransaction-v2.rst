.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Stop a transaction on a (remote) compute resource. The transaction
must have been created previously with
:ref:`algm-StartRemoteTransaction`.

For more details on remote algorithms when using the Mantid web
service remote job submission API, see the :ref:`remote job submission API
docs <RemoteJobSubmissionAPI>`.

Previous Versions
-----------------

Version 1
#########

Version 1 stops transactions using the Stop action of the :ref:`Mantid
remote job submission API
<RemoteJobSubmissionAPI>`. This is
still supported as one of the variants of Versions 2 and above, when
the compute resource uses the Mantid remote job submission API as job
manager (underlying remote job scheduling mechanism).

.. categories::

.. sourcelink::
