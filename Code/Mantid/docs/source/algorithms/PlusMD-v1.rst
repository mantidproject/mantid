.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm sums two :ref:`MDHistoWorkspaces <MDHistoWorkspace>` or
merges two `MDEventWorkspaces <http://www.mantidproject.org/MDEventWorkspace>`_ together.

MDHistoWorkspaces
#################

-  **MDHistoWorkspace + MDHistoWorkspace**

   -  The operation is performed element-by-element.

-  **MDHistoWorkspace + Scalar** or **Scalar + MDHistoWorkspace**

   -  The scalar is subtracted from every element of the
      MDHistoWorkspace. The squares of errors are summed.

MDEventWorkspaces
#################

This algorithm operates similary to calling Plus on two
:ref:`EventWorkspaces <EventWorkspace>`: it combines the events from the
two workspaces together to form one large workspace.

Note for file-backed workspaces
###############################

The algorithm uses :ref:`algm-CloneMDWorkspace` to create the
output workspace, except when adding in place (e.g. :math:`A = A + B` ).
See :ref:`algm-CloneMDWorkspace` for details, but note that a
file-backed `MDEventWorkspace <http://www.mantidproject.org/MDEventWorkspace>`_ will have its file
copied.

-  If A is in memory and B is file-backed, the operation
   :math:`C = A + B` will clone the B file-backed workspace and add A to
   it.
-  However, the operation :math:`A = A + B` will clone the A workspace
   and add B into memory (which might be too big!)

Also, be aware that events added to a MDEventWorkspace are currently
added **in memory** and are not cached to file until :ref:`algm-SaveMD`
or another algorithm requiring it is called. The workspace is marked as
'requiring file update'.

.. categories::
