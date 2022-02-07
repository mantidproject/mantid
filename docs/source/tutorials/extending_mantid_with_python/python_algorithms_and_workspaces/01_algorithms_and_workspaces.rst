.. _01_algorithms_and_workspaces:

=========================
Algorithms And Workspaces
=========================

Declaring Workspace Properties
===============================

Workspaces are the main source of data for input and most algorithms output
another data workspace. Workspace properties are required in order to either
provide input data for an algorithm or store data at the end of execution.
Mantid has a variety of different workspace types, for example
:ref:`MatrixWorkspace`, :ref:`EventWorkspace` or :ref:`Table Workspaces`.
A generic Workspace type also exists which can refer to any of these.

Declaring input workspace properties restricts the types of workspace that
will be accepted when a user wants to pass a workspace into the algorithm.
The example below declares a ``WorkspaceProperty`` which means that any
workspace type will be accepted as input.

.. code-block:: python

    from mantid.kernel import *
    from mantid.api import *

    class WorkspaceProperties(PythonAlgorithm):

        def PyInit(self):
            self.declareProperty(WorkspaceProperty(name="InputWorkspace",
                                                   defaultValue="",
                                                   direction=Direction.Input),
                                                   doc="Documentation for this property")

        def PyExec(self):
            pass

Other Workspace Types
=====================

As discussed above there are many different types of workspace. The above
declarations allow any workspace type to be passed in to an algorithm. It
is possible to restrict the type that can be accepted, for example to
disallow table workspace types, by simply using a different class in the
``self.declareProperty`` method.

As an example, consider the definition above using ``WorkspaceProperty``.
If we wanted to restrict the type to a ``MatrixWorkspace`` type then we
can simply replace ``WorkspaceProperty`` with
``MatrixWorkspaceProperty``

.. code-block:: python

    from mantid.kernel import *
    from mantid.api import *

    class MatrixWorkspaceProperties(PythonAlgorithm):

        def PyInit(self):
            self.declareProperty(MatrixWorkspaceProperty("InputWorkspace",
                                                         "",
                                                         Direction.Input),
                                                         "Documentation for this property")

        def PyExec(self):
            pass

Any workspace passed as input to this algorithm must now be of type
``MatrixWorkspace``.

The currently available types of property are:

* ``WorkspaceProperty``
* ``MatrixWorkspaceProperty``
* ``ITableWorkspaceProperty``
* ``IPeaksWorkspaceProperty``
