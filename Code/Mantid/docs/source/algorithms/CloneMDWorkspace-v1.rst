.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm will clones an existing
`MDEventWorkspace <http://www.mantidproject.org/MDEventWorkspace>`_ or
:ref:`MDHistoWorkspace <MDHistoWorkspace>` into a new one.

If the InputWorkspace is a file-backed MDEventWorkspace, then the
algorithm will copy the original file into a new one with the suffix
'\_clone' added to its filename, in the same directory. Before the clone
operation, the file back-end will be updated using :ref:`algm-SaveMD`
with UpdateFileBackEnd=True. This may delay the operation.

If you wish to clone a file-backed MDEventWorkspace to an in-memory
MDEventWorkspace, we recommend that you first call :ref:`algm-SaveMD`
with UpdateFileBackEnd=True (if necessary), followed by a simple LoadMD
call to the file in question.

.. categories::
