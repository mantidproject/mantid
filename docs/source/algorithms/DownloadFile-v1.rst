
.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This is a simple algorithm that will download the contents of a url address to a file.
It can support :literal:`http://` and :literal:`https://` based urls, and if the method is not supplied then :literal:`http://` will be assumed.
For example: If the address is :literal:`www.mantidproject.org`, then this will be adjusted to :literal:`http://www.mantidproject.org`.


Usage
-----

**Example - http**

.. code-block:: python

    #import the os path libraries for directory functions
    import os

    #Create an absolute path by joining the proposed filename to a directory
    #os.path.expanduser("~") used in this case returns the home directory of the current user
    savefile = os.path.join(os.path.expanduser("~"), "DownloadedFile.txt")

    DownloadFile("http://www.mantidproject.org", savefile)

    print("File Exists: {}".format(os.path.exists(savefile)))

Output:

.. code-block:: python

    File Exists: True


**Example - https**

.. code-block:: python

    #import the os path libraries for directory functions
    import os

    #Create an absolute path by joining the proposed filename to a directory
    #os.path.expanduser("~") used in this case returns the home directory of the current user
    savefile = os.path.join(os.path.expanduser("~"), "DownloadedFile.txt")

    DownloadFile("https://raw.githubusercontent.com/mantidproject/mantid/master/README.md", savefile)

    print("File Exists:".format(os.path.exists(savefile)))

Output:

.. code-block:: python

    File Exists: True

.. categories::

.. sourcelink::

