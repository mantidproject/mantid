.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Extract the values for momentum transfer stored in the vertical axis of a MatrixWorkspace
representing the dynamic factor :math:`S(Q,E)`, or compute the values of a MatrixWorkspace
representing the dynamic factor :math:`S(\theta,E)`.

Usage
-----

** Extract Q values from :math:`S(Q,E)` workspace **

.. testcode:: ExtractQvalues

    workspace = LoadDaveGrp("BASIS_59689_divided.dat")
    qvalues = GetQsInQENSData(workspace)
    # print the first three values
    vals = ' '.join(['{0:6.3f}'.format(Q) for Q in qvalues[0:3]])
    print("'{}'".format(vals))

.. testcleanup:: ExtractQvalues

    DeleteWorkspace(workspace)

.. testoutput:: ExtractQvalues

    ' 0.300  0.500  0.700'

.. testcode:: ComputeQvalues

    workspace = LoadNexus("osiris97944_graphite002_red")
    qvalues = GetQsInQENSData(workspace)
    # print the first three values
    vals = ' '.join(['{0:6.3f}'.format(Q) for Q in qvalues[0:3]])
    print("'{}'".format(vals))

.. testcleanup:: ComputeQvalues

    DeleteWorkspace(workspace)

.. testoutput:: ComputeQvalues

    ' 0.189  0.244  0.298'

.. categories::

.. sourcelink::
