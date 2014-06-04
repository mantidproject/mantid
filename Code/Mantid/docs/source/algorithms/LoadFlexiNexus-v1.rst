.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Description
-----------

This algorithm is a flexible NeXus file loader. Data loading is driven
by a dictionary. Correspondingly the algorithm takes as arguments: a
filename, a path to a dictionary file and an output workspace name.

The dictionary itself is a list of key=value pairs, one per line.
Normally dictionary entries take the form key-path-into-Nexus-file. The
default action is to store the data found at path-into-NeXus-file under
key key in the Run information of the result workspace. But some keys
are interpreted specially:

data=path-into-nexus-file
    This is a required entry. path-into-nexus-file is the path in the
    NeXus file to the data which is the main bulk workspace data.
    Usually the counts. From the dimensionality of this data the type of
    result workspace is determined. If the rank is <= 2, then a
    Workspace2D is created, else a MDHistoWorkspace.
x-axis=path-into-nexus-file
    The data found under the path into the NeXus file will be used as
    axis 0 on the dataset
x-axis-name=text
    The text specified will become the name of the axis 0
y-axis=path-into-nexus-file
    The data found under the path into the NeXus file will be used as
    axis 1 on the dataset
y-axis-name=text
    The text specified will become the name of the axis 1
z-axis=path-into-nexus-file
    The data found under the path into the NeXus file will be used as
    axis 2 on the dataset
z-axis-name=text
    The text specified will become the name of the axis 0
title=path-into-nexus-file or text
    If the value contains a / then it is interpreted as a path into the
    NeXus file, the value of which will be stored as workspace title.
    Else the text value will be stored as the title name directly.
sample=path-into-nexus-file or text
    If the value contains a / then it is interpreted as a path into the
    NeXus file, the value of which will be stored as workspace sample.
    Else the text value will be stored as the sample name directly.

Please note that the dimensions on the MDHistoWorkspace are inverted
when compared with the ones in the NeXus file. This is a fix which
allows to efficiently transfer the NeXus data in C storage order into
the MDHistoWorkspace which has fortran storage order.

.. categories::
