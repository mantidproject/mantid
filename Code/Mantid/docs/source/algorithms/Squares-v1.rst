.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

An example python algorithm.

This creates a workspace with a value that increases as the square of the bin index.
It also optionally logs a message and saves a file.

Usage
-----

**Example**

.. testcode:: SquaresEx

    #import the os path libraries for directory functions
    import os

    #Create an absolute path by joining the proposed filename to a directory
    #os.path.expanduser("~") used in this case returns the home directory of the current user
    savefile = os.path.join(os.path.expanduser("~"), "Square.txt")

    # create histogram workspace
    ws=Squares(MaxRange='20', Preamble='Hello', Sum=True, OutputFile=savefile)

    print "The first five values are:"
    print ws.readY(0)[0:5]

    #clean up the file I saved
    os.remove(savefile)

Output:

.. testoutput:: SquaresEx

    The first five values are:
    [  1.   4.   9.  16.  25.]


.. categories::


