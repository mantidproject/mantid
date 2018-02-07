.. _TrainingNexusTools:

==================
Useful NeXus Tools
==================

NeXus Files
-----------

Presently at the ILL, only files in **.nxs** format produced by the following instruments are supported by Mantid:

* IN4, IN5, IN6
* IN16B
* D20, D2B
* D17, FIGARO
* D11, D22, D33

With the right tools it is quite easy to access and modify the contents of these files, without requiring Mantid to load them. The following tools can also be used to explore data saved by Mantid.

HDFView
-------

A visual tool which can be used for browsing and editing NeXus files. Installers for any platform can be found on the HDFView website at https://support.hdfgroup.org/products/java/hdfview/.

.. figure:: /images/Training/Introduction/HDFView.png
   :align: center
   :width: 750

NeXpy
-----

NeXpy allows some basic plotting of the data in the NeXus files, even for data that is stored in more than two dimensions. Installation instructions can be found at https://nexpy.github.io/nexpy/includeme.html.

.. figure:: /images/Training/Introduction/NeXpy.png
   :align: center
   :width: 600

|
