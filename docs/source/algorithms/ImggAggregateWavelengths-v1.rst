
.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

.. warning::

   This algorithm is experimental and it is at the moment being
   developed for a specific instrument (IMAT at ISIS). It might be
   changed significantly, renamed, or even removed without a
   notification, should instrument scientists decide to do so.

This algorithm aggregates (sums counts) multiple energy bands or
wavelengths for energy selective imaging data.

Given an input path (directory) and an output path (directory) the
algorithm with combine images from the input path and write the result
into the output path. Two different uses are considered: neutron
radiography and tomography. The image format supported is FITS (using
the algorithm :ref:`algm-LoadFITS`).

For neutron radiography the input path points to a set of image files
where every file is assumed to correspond to a different energy
band. The algorithm will produce an image in the output path by
combining the energy bands selected (as specified in the algorithm
options, and using all the input images as default).

For neutron tomography the input path points to a set of
subdirectories, each of them containing image files. It is assumed
that each of the directories corresponds to a projection angle.  The
algorithm will produce as many images in the output path as
subdirectories are found in the input path a single image in the
output path by combining the energy bands selected, separately for
every input subdirectory, i.e., projection angle (selecting the bands
as specified in the algorithm options, and using all the input bands
as default).

The algorithm produces its main outputs as files on disk. It also
outputs two values: the number of angles or projections processed
successfully (which will be always one for neutron radiography), and
the number of wavelength bands aggregated.

Usage
-----

**Example - ImggAggregateWavelengths**

.. code-block:: python

   # Create a stack of images that can be used in tomographic reconstruction
   projections, bands = ImggAggregateWavelengths(InputPath='D:\Data\RB000000\SampleA\',
                                                 OutputPath='D:\Data\RB000000\SampleA\all_wavelenghts')

   if 1 != projections:
      print "An error happened. Expected to process one projection but processed: {0}".format(projections)
   # Print some details
   print "Wrote a projection images combining {1} wavelength bands".format(bands)

Output:

.. code-block:: none

  Wrote a projection image combining 2000 wavelength bands

**Example - ImggAggregateWavelengths**

.. code-block:: python

   # Create a stack of images that can be used in tomographic reconstruction
   angles, bands = ImggAggregateWavelengths(InputPath='D:\Data\RB000000\SampleA\',
                                            OutputPath='D:\Data\RB000000\SampleA\all_wavelenghts')

   # Print some details
   print "Wrote {0} projection images, each combining {1} wavelength bands".format(angles, bands)

Output:

.. code-block:: none

  Wrote 144 projection images, each combining 2000 wavelength bands

.. categories::

.. sourcelink::

