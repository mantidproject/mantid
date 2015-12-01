
.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

The maximum entropy method (MEM) is used as a signal processing technique for reconstructing
images from noisy data. The algorithm maximizes the entropy :math:`S` subject to the constraint:

.. math:: \chi^2 = \sum_i \frac{\left(d_i - d_i^C\right)^2}{\sigma_i^2} \leq C_{target}

Usage
-----
..  Try not to use files in your examples,
    but if you cannot avoid it then the (small) files must be added to
    autotestdata\UsageData and the following tag unindented
    .. include:: ../usagedata-note.txt

**Example - MaxEnt**

.. testcode:: MaxEntExample

   # Create a host workspace
   ws = CreateWorkspace(DataX=range(0,3), DataY=(0,2))
   or
   ws = CreateSampleWorkspace()

   wsOut = MaxEnt()

   # Print the result
   print "The output workspace has %i spectra" % wsOut.getNumberHistograms()

Output:

.. testoutput:: MaxEntExample

  The output workspace has ?? spectra

.. categories::

.. sourcelink::

