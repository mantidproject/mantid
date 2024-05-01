.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

The format used differs based on the type of workspace being saved.  For a table workspace the data will contain an optional row of column headers, followed by the row values, with each individual column value seperated by the defined seperator.

For a  matrix workspace the data are stored in the file in columns: the first column contains
the X-values, followed by pairs of Y and E values. Columns are separated by
commas. The resulting file can normally be loaded into a workspace by the
:ref:`algm-LoadAscii` algorithm.

There is limited support for MDHistoWorkspaces, and the algorithm only allows for the saving of 1D MDHisto Workspaces.
This functionality is specifically used to save cuts made using sliceviewr.
The saved format consists of a header comment that contains all of the workspace properties,
followed by three separate columns corresponding to X, Y, and E, respectively.

It is possible to create a header for the ASCII file containing selected sample log entries and its unit,
specified through `LogList` property. In case the requested log does not exist, it will be printed as
`Not defined`. This feature is not enabled for :ref:`TableWorkspace <Table Workspaces>`.

If the workspace (matrix workspace) contains several spectra, two options are
available:

* if OneSpectrumPerFile if false (default value), all spectra will be appended
  into the same file
* if OneSpectrumPerFile is true, each spectrum will be written in a separate
  file. The name of the file will be created as follows: <Filename property>_
  <spectrum index>_<axis value><axis unit>.<extension>

Limitations
###########

The algorithm assumes that the workspace has common X values for all spectra
(i.e. is not a :ref:`ragged workspace <Ragged_Workspace>` )

Usage
-----

**Example - Save a workspace in CSV format**

.. testcode:: ExSaveASCIISimple

    #import the os path libraries for directory functions
    import os

    # create histogram workspace
    dataX1 = [0,1,2,3,4,5,6,7,8,9] # or use dataX1=range(0,10)
    dataY1 = [0,1,2,3,4,5,6,7,8] # or use dataY1=range(0,9)
    dataE1 = [1,1,1,1,1,1,1,1,1] # or use dataE1=[1]*9

    ws1 = CreateWorkspace(dataX1, dataY1, dataE1)

    #Create an absolute path by joining the proposed filename to a directory
    #os.path.expanduser("~") used in this case returns the home directory of the current user
    savefile = os.path.join(os.path.expanduser("~"), "AsciiFile.txt")

    # perform the algorithm
    SaveAscii(InputWorkspace=ws1,Filename=savefile)

    print("File Exists: {}".format(os.path.exists(savefile)))

.. testcleanup:: ExSaveASCIISimple

    os.remove(savefile)

Output:

.. testoutput:: ExSaveASCIISimple

    File Exists: True

**Example - Save a workspace as ASCII with a different delimiter**

.. testcode:: ExSaveASCIIDelimiter

    #import the os path libraries for directory functions
    import os

    # create histogram workspace
    dataX1 = [0,1,2,3,4,5,6,7,8,9] # or use dataX1=range(0,10)
    dataY1 = [0,1,2,3,4,5,6,7,8] # or use dataY1=range(0,9)
    dataE1 = [1,1,1,1,1,1,1,1,1] # or use dataE1=[1]*9

    ws1 = CreateWorkspace(dataX1, dataY1, dataE1)

    #Create an absolute path by joining the proposed filename to a directory
    #os.path.expanduser("~") used in this case returns the home directory of the current user
    savefile = os.path.join(os.path.expanduser("~"), "AsciiFile.txt")

    # perform the algorithm
    SaveAscii(InputWorkspace=ws1,Filename=savefile,Separator="Space")

    print("File Exists: {}".format(os.path.exists(savefile)))

.. testcleanup:: ExSaveASCIIDelimiter

    os.remove(savefile)

Output:

.. testoutput:: ExSaveASCIIDelimiter

    File Exists: True


**Example - Save a workspace as ASCII with a different comment indicator**

.. testcode:: ExSaveASCIIComment

    #import the os path libraries for directory functions
    import os

    # create histogram workspace
    dataX1 = [0,1,2,3,4,5,6,7,8,9] # or use dataX1=range(0,10)
    dataY1 = [0,1,2,3,4,5,6,7,8] # or use dataY1=range(0,9)
    dataE1 = [1,1,1,1,1,1,1,1,1] # or use dataE1=[1]*9

    ws1 = CreateWorkspace(dataX1, dataY1, dataE1)

    #Create an absolute path by joining the proposed filename to a directory
    #os.path.expanduser("~") used in this case returns the home directory of the current user
    savefile = os.path.join(os.path.expanduser("~"), "AsciiFile.txt")

    # perform the algorithm
    # CommentIndicator can be changed, but when read back in must be specified
    SaveAscii(InputWorkspace=ws1,Filename=savefile,CommentIndicator="!")

    print("File Exists: {}".format(os.path.exists(savefile)))

.. testcleanup:: ExSaveASCIIComment

    os.remove(savefile)

Output:

.. testoutput:: ExSaveASCIIComment

    File Exists: True

.. categories::

.. sourcelink::
