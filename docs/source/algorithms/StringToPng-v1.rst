.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

Saves a string as a png file, as part of autoreduction.


.. Note::

 Requires matplotlib.
 

Usage
-----

.. testcode:: StringToPng

    #write to a file
    try:
        import mantid
        filename=mantid.config.getString("defaultsave.directory")+"StringToPngTest.png"
        StringToPng("This is a string\nAnd this is a second line",OutputFilename=filename)
    except:
        pass
          
.. testcleanup:: StringToPng

   import os,mantid   
   filename=mantid.config.getString("defaultsave.directory")+"StringToPngTest.png"
   if os.path.isfile(filename):
       os.remove(filename)

Output:

.. testoutput:: StringToPng

    

The file should look like

.. figure:: /images/StringToPngTest.png
   :alt: StringToPngTest.png


.. categories::

.. sourcelink::
