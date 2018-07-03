.. algorithm::

.. summary::

.. relatedalgorithms::

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
    print("Found workspace PG3_11485 {}".format(mtd.doesExist("PG3_11485")))
    print("It has {} histogram, with {} bins".format(mtd["PG3_11485"].getNumberHistograms(), mtd["PG3_11485"].blocksize()))
    print("Found workspace PG3_11486 {}".format(mtd.doesExist("PG3_11486")))
    print("It has {} histogram, with {} bins".format(mtd["PG3_11486"].getNumberHistograms(), mtd["PG3_11486"].blocksize()))
   
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

.. sourcelink::
