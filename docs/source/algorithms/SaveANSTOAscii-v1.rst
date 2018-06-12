.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

SaveANSTOAscii is an export-only Ascii-based save format with no associated loader. It is based on a python script by Maximilian Skoda, written for the ISIS Reflectometry GUI.

Limitations
###########

While Files saved with SaveANSTOAscii can be loaded back into mantid using LoadAscii, the resulting workspaces won't be useful as the data written by SaveANSTOAscii is not in the normal X,Y,E,DX format.

Usage
-----

**Example - Save a Workspace in ANSTO ASCII format**

.. testcode:: ExANSTOSimple

    #import the os path libraries for directory functions
    import os

    # create histogram workspace
    dataX1 = [0,1,2,3,4,5,6,7,8,9] # or use dataX1=range(0,10)
    dataY1 = [0,1,2,3,4,5,6,7,8] # or use dataY1=range(0,9)
    dataE1 = [1,1,1,1,1,1,1,1,1] # or use dataE1=[1]*9

    ws1 = CreateWorkspace(dataX1, dataY1, dataE1)

    #Create an absolute path by joining the proposed filename to a directory
    #os.path.expanduser("~") used in this case returns the home directory of the current user
    savefile = os.path.join(os.path.expanduser("~"), "ANSTOAsciiFile.txt")

    # perform the algorithm
    SaveANSTOAscii(InputWorkspace=ws1,Filename=savefile)

    print("File Exists: {}".format(os.path.exists(savefile)))

.. testcleanup:: ExANSTOSimple

    os.remove(savefile)

Output:

.. testoutput:: ExANSTOSimple

    File Exists: True

.. categories::

.. sourcelink::
