.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

This algorithm loads multiple gsas files from a single directory into
mantid.

Usage
-----

.. include:: ../usagedata-note.txt

.. testcode:: LoadMultipleGSS

    LoadMultipleGSS(FilePrefix="PG3",RunNumbers="11485,11486",Directory="")
    
    #quick test:
    print "Found workspace PG3_11485",mtd.doesExist("PG3_11485")
    print "It has",mtd["PG3_11485"].getNumberHistograms(),"histogram, with",mtd["PG3_11485"].blocksize(),"bins"
    print "Found workspace PG3_11486",mtd.doesExist("PG3_11486")
    print "It has",mtd["PG3_11486"].getNumberHistograms(),"histogram, with",mtd["PG3_11486"].blocksize(),"bins"
   
.. testcleanup:: LoadMultipleGSS

    DeleteWorkspace('PG3_11485')
    DeleteWorkspace('PG3_11486')

Output:

.. testoutput:: LoadMultipleGSS

    Found workspace PG3_11485 True
    It has 1 histogram, with 2193 bins
    Found workspace PG3_11486 True
    It has 1 histogram, with 731 bins

.. categories::
