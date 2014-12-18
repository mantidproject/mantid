
.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This is a simple algorithm that will download the contents of a url address to a file.
It can support http:// and https:// based urls, and if the method is not supplied then http:// will be assumed.
For example: If the address is www.mantidproject.org, then this will be adjusted to http://www.mantidproject.org.


Usage
-----

**Example - http**

.. testcode:: DownloadFileHttp

    #import the os path libraries for directory functions
    import os

    #Create an absolute path by joining the proposed filename to a directory
    #os.path.expanduser("~") used in this case returns the home directory of the current user
    savefile = os.path.join(os.path.expanduser("~"), "DownloadedFile.txt")

    DownloadFile("http://www.mantidproject.org", savefile)

    print "File Exists:", os.path.exists(savefile)

.. testcleanup:: DownloadFileHttp

    os.remove(savefile)

Output:

.. testoutput:: DownloadFileHttp

    File Exists: True


**Example - https**

.. testcode:: DownloadFileHttps

    #import the os path libraries for directory functions
    import os

    #Create an absolute path by joining the proposed filename to a directory
    #os.path.expanduser("~") used in this case returns the home directory of the current user
    savefile = os.path.join(os.path.expanduser("~"), "DownloadedFile.txt")

    DownloadFile("https://raw.githubusercontent.com/mantidproject/mantid/master/README.md", savefile)

    print "File Exists:", os.path.exists(savefile)

.. testcleanup:: DownloadFileHttps

    os.remove(savefile)

Output:

.. testoutput:: DownloadFileHttps

    File Exists: True

.. categories::

