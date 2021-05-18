.. _01_creating_workspaces:

===================
Creating Workspaces
===================

Most algorithms will want to create a workspace as part of the execution and
set this as output. The first step in this process is to declare the output
property that will store the workspace after it has been created.

It is currently only possible to create a ``Workspace2D`` and
``TableWorkspace`` from Python:

* A ``Workspace2D`` output should use a :ref:`mantid.api.WorkspaceProperty`
  or :ref:`mantid.api.MatrixWorkspaceProperty` and
* a ``TableWorkspace`` output should use a
  :ref:`mantid.api.ITableWorkspaceProperty`

The workspace is created using an object called ``WorkspaceFactory`` and a
method called ``create``. To create a brand new workspace from scratch:

.. code-block:: python

    def PyInit(self):
        # Creates a generic output property
        self.declareProperty(WorkspaceProperty(name="OutputWorkspace",
                                               defaultValue="",
                                               direction=Direction.Output))

    def PyExec(self):
        # A workspace with 5 rows with 9 bins & 10 bin boundaries
        # (i.e. a histogram)
        output_ws = WorkspaceFactory.create("Workspace2D", NVectors=5,
                                            XLength=10, YLength=9)
        self.setProperty("OutputWorkspace", output_ws)

The above code will create a new 2D workspace filled with zeroes. The
``setProperty`` call is important as it is required for Mantid to store the
workspace outside of the algorithm.

To set the data in the workspace use the ``data[X,Y,E]`` member of the
workspace like so:

.. code-block:: python

    # ...snipped...
    # A workspace with 5 rows with 9 bins & 10 bin boundaries
    nrows = 5
    nbins = 9
    output_ws = WorkspaceFactory.create("Workspace2D", NVectors=nrows,
                                        XLength=nbins+1, YLength=nbins)
    for i in range(nrows):
        xdata = output_ws.dataX(i)
        ydata = output_ws.dataY(i)
        edata = output_ws.dataE(i)
        for j in range(nbins):
            xdata[j] = j
            ydata[j] = i*i
            edata[j] = j
        # end for loop
        # final bin boundary
        xdata[nbins] = nbins

    self.setProperty("OutputWorkspace", output_ws)

The ``WorkspaceFactory`` can also create a workspace using another as a
template. In addition to taking the size from the original it will also copy
across meta-information such as the instrument and logs. The creation by this
method can also create a workspace of a different size, e.g.

.. code-block:: python

    # ...snipped...
    # A workspace with 5 rows with 9 bins & 10 bin boundaries
    nrows = 5
    nbins = 9
    output_ws = WorkspaceFactory.create("Workspace2D", NVectors=nrows,
                                        XLength=nbins+1, YLength=nbins)

    # Copies meta-data and creates a single bin workspace with 5 rows
    second_ws = WorkspaceFactory.create(output_ws, NVectors=5,
                                        XLength=2,YLength=1)

    # Copies meta-data and creates a workspace with 1 rows that is
    # the same length as the original in Y & X
    third_ws = WorkspaceFactory.create(output_ws, NVectors=1)

Numpy as Data Source
====================

Numpy arrays can be used to set a 1D array straight from a numpy array. This
is more efficient than looping over the arrays and workspaces and setting
each element in python:

.. code-block:: python

    # ...snipped...
    # A workspace with 5 rows with 9 bins & 10 bin boundaries
    nrows = 5
    nbins = 9
    output_ws = WorkspaceFactory.create("Workspace2D", NVectors=nrows,
                                        XLength=nbins+1, YLength=nbins)

    xdata = numpy.arange(float(nbins + 1)) # filled with 0->9
    ydata = 100*numpy.arange(float(nbins))
    edata = numpy.sqrt(ydata) # filled with 0->sqrt(800)

    for i in range(nrows):
        output_ws.setX(i, xdata)
        output_ws.setY(i, ydata)
        output_ws.setE(i, edata)

    self.setProperty("OutputWorkspace", output_ws)
