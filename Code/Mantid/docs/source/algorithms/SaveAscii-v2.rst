.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

The workspace data are stored in the file in columns: the first column contains the X-values, followed by pairs of Y and E values. Columns are separated by commas. The resulting file can normally be loaded into a workspace by the :ref:`algm-LoadAscii` algorithm.

Limitations
###########

The algorithm assumes that the workspace has common X values for all spectra (i.e. is not a `ragged workspace <Ragged Workspace>`__). Only the X values from the first spectrum in the workspace are saved out.

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

    print "File Exists:", os.path.exists(savefile)

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

    print "File Exists:", os.path.exists(savefile)

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

    print "File Exists:", os.path.exists(savefile)

.. testcleanup:: ExSaveASCIIComment

    os.remove(savefile)

Output:

.. testoutput:: ExSaveASCIIComment

    File Exists: True

.. categories::
