.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm reduces HFIR POWDER (HB-2A) data.

You can either specify the filenames of data you want to reduce or provide the IPTS, exp and scan number. *e.g.* the following are equivalent:

.. code-block:: python

   ws = HB2AReduce('/HFIR/HB2A/IPTS-21073/exp666/Datafiles/HB2A_exp0666_scan0024.dat')
   # and
   ws = HB2AReduce(IPTS=21073, exp=666, ScanNumbers=24)

You can specify any number of filenames or scan numbers (in a comma separated list).

Vanadium
########

By default the correct vcorr file (``HB2A_exp???__Ge_[113|115]_[IN|OUT]_vcorr.txt``) adjacent to the data file will be used. Alternatively either the vcorr file or a vanadium scan file can be provided to the ``Vanadium`` option. If a vanadium scan file is provided then the vanadium counts can be taken into account when calculating the uncertainty which can not be done with using the vcorr file.

If ``Normalise=False`` then no normalisation will be performed.

ExcludeDetectors
################

By default the file ``HB2A_exp???__exclude_detectors.txt`` adjacent to the data file will be used unless a list of detectors to exclude are provided by ``ExcludeDetectors``

IndividualDetectors
###################

If this option is True then a separate spectra will be created in the output workspace for every anode. This allows you to compare adjacent anodes.

Binning Data
############

If ``BinData=True`` (default) then the data will be binned on a regular grid with a width of ``BinWidth``. The output can be scaled by an arbitrary amount by setting ``Scale``.

Saving reduced data
###################

The output workspace can be saved to ``XYE``, ``Maud`` and ``TOPAS`` format using :ref:`SaveFocusedXYE <algm-SaveFocusedXYE>`. *e.g.*

.. code-block:: python

   # XYE with no header
   SaveFocusedXYE(ws, Filename='data.xye', SplitFiles=False, IncludeHeader=False)

   # TOPAS format
   SaveFocusedXYE(ws, Filename='data.xye', SplitFiles=False, Format='TOPAS')

   # Maud format
   SaveFocusedXYE(ws, Filename='data.xye', SplitFiles=False, Format='MAUD')

Usage
-----

**Individual Detectors**

.. code-block:: python

   ws=HB2AReduce('HB2A_exp0666_scan0024.dat', IndividualDetectors=True)

   # Plot anodes 40, 41 and 42
   import matplotlib.pyplot as plt
   from mantid import plots
   fig, ax = plt.subplots(subplot_kw={'projection':'mantid'})
   for num in [40,41,42]:
       ax.plot(ws, specNum=num)
   plt.legend()
   #fig.savefig('HB2AReduce_1.png')
   plt.show()

.. figure:: /images/HB2AReduce_1.png


**Unbinned data**

.. code-block:: python

   ws=HB2AReduce('HB2A_exp0666_scan0024.dat', BinData=False)

   # Plot
   import matplotlib.pyplot as plt
   from mantid import plots
   fig, ax = plt.subplots(subplot_kw={'projection':'mantid'})
   ax.plot(ws)
   #fig.savefig('HB2AReduce_2.png')
   plt.show()

.. figure:: /images/HB2AReduce_2.png


**Binned data**

.. code-block:: python

   ws=HB2AReduce('HB2A_exp0666_scan0024.dat')

   # Plot
   import matplotlib.pyplot as plt
   from mantid import plots
   fig, ax = plt.subplots(subplot_kw={'projection':'mantid'})
   ax.plot(ws)
   #fig.savefig('HB2AReduce_3.png')
   plt.show()

.. figure:: /images/HB2AReduce_3.png


**Exclude detectors: 1-20,40,41,42**

.. code-block:: python

   ws=HB2AReduce('HB2A_exp0666_scan0024.dat', ExcludeDetectors='1-20,40,41,42')

   # Plot
   import matplotlib.pyplot as plt
   from mantid import plots
   fig, ax = plt.subplots(subplot_kw={'projection':'mantid'})
   ax.plot(ws)
   #fig.savefig('HB2AReduce_4.png')
   plt.show()

.. figure:: /images/HB2AReduce_4.png


**Combining multiple files**

.. code-block:: python

   ws=HB2AReduce('HB2A_exp0666_scan0024.dat, HB2A_exp0666_scan0025.dat')

   # Plot
   import matplotlib.pyplot as plt
   from mantid import plots
   fig, ax = plt.subplots(subplot_kw={'projection':'mantid'})
   ax.plot(ws)
   #fig.savefig('HB2AReduce_5.png')
   plt.show()

.. figure:: /images/HB2AReduce_5.png


.. categories::

.. sourcelink::
