
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

.. warning::

   The property **ToFRanges** requires specific headers in the input
   (FITS) files and it is not enabled. At the moment, trying to use
   this property will produce an error.

This algorithm aggregates images from multiple energy bands or
wavelengths into one or more output wavelength bands. The algorithm
applies to energy selective imaging data. It aggregates images by
summing up counts, and assumes that the image pixel values represent
neutron counts.

Given an input path (directory) and an output path (directory) the
algorithm will combine images from the input path and write the result
into the output path. Two different uses are considered: neutron
radiography (single projection angle) and tomography (multiple
projection angles). The image format supported is FITS (using the
algorithm :ref:`algm-LoadFITS`).

The algorithm produces its main outputs as files on disk. It also
outputs two values: the number of projections (or angles) processed
successfully (which will be always one for neutron radiography), and
the number of wavelength bands aggregated. One and only one of the
options **UniformBands**, **IndexRanges** or **ToFRanges** must be
specified. This defines the number of output bands and how the input
bands are aggregated or combined into the output bands.

For the sake of simplicity let us explain first the files that the
algorithm produces when a single output band is generated. This single
output band could correspond to the aggregation of all or a subset of
the input bands. This is the case when the property UniformBands is
set to 1, or the properties **IndexRanges** or **ToFRanges** are set
to a single range (and not multiple ranges separated by commas).

For neutron radiography data the input path points to a set of image
files where every file is assumed to correspond to a different
wavelength or energy band. The algorithm will produce an image in the
output path by combining the energy bands selected (as specified in
the algorithm options, and using all the input images as default).

For neutron tomography data the input path points to a set of
subdirectories, one per projection angle, and each of them containing
image files. It is assumed that each of the directories corresponds to
a projection angle.  The algorithm will produce as many images in the
output path as subdirectories are found in the input path a single
image in the output path by combining the energy bands selected,
separately for every input subdirectory, i.e., projection angle
(selecting the bands as specified in the algorithm options, and using
all the input bands as default). The images are written in a
subdirectory that is named depending on the type of aggregation
used. For the uniform bands (**UniformBands** property) the
subdirectory will be named **bands_uniform_idx_<start>_to_<end>**,
where **<start>** and **<end>** are the limits of the range(s). When
the index ranges option (**IndexRanges**) is used the names of the
output subdirectory will be named
**bands_index_idx_<start>_to_<end>**. And when the time of flight
range option (**ToFRanges**) is used the names will be
**bands_tof_idx_<start>_to_<end>**. The initial prefix
(bands_uniform_, bands_index_, bands_tof_) can be mofified via the
input properties **OutputSubdirsPrefixUniformBands**,
**OutputSubdirsPrefixIndexBands**, **OutputSubdirsPrefixToFBands**,
respectively.

The output images are created as described above when there is a
single output band or stack of images. When multiple output ranges are
specified the outputs are produced similarly but every range will be
generated in a separate subdirectory created under the output path
given to the algorithm. For example the algorithm would produce
subdirectories named bands_by_index_idx_0_99,
bands_by_index_idx_50_149, and bands_by_index_idx_99_149, or
bands_uniform_idx_0_49, bands_uniform_idx_50_99.

A relevant consideration is how to use the range options to generate
multiple output stacks of images. This is very important for
performance reasons and concerns how to use the properties
**IndexRanges** and **ToFRanges**.  The comma separated list of ranges
enables the user to generate multiple output stacks of images in one
call to this algorithm. The same result can be obtained by calling the
algorithm repeatedly with the different individual ranges, one at a
time. This second alternative would be significantly slower though,
and it is discouraged to use that approach in scripts. That is because
the algorithm would need to read through the input images for every
individual call. This can make a big different in terms of run time,
given that this algorithm is meant to process large sets of images
which can take of the order of tens of minutes or hours to read and/or
write depending on the disk resources available.


As the algorithm tries to find images and subdirectories with images
from the input path, to decide whether to process the data as a single
projection (radiography) or multiple projections (tomography), there
can be ambiguities.  When an input path is given that contains both
image files and subdirectories with image files, the algorithm
processes the image files and does not try to process the
subdirectories. That is, it is assumed that the input path given
contains data for a single projection, and that the subdirectories do
not necessarily contain images for different projections (tomography
data).

Usage
-----

**Example - ImggAggregateWavelengthsSingleProjection**

.. code-block:: python

   # Create an image combining all energy bands
   projections, bands = ImggAggregateWavelengths(InputPath='D:\Data\RB000000\SampleA\',
                                                 OutputPath='D:\Data\RB000000\SampleA_all_wavelenghts'
                                                 UniformBands=1)

   if 1 != projections:
      print "An error happened. Expected to process one projection but processed: {0}".format(projections)
   # Print some details
   print "Wrote a projection image combining {1} wavelength bands".format(bands)

Output:

.. code-block:: none

  Wrote a projection image combining 2000 wavelength bands

**Example - ImggAggregateWavelengthsTomography**

.. code-block:: python

   # Create a stack of images that can be used in tomographic reconstruction
   angles, bands = ImggAggregateWavelengths(InputPath='D:\Data\RB000000\SampleA\',
                                            OutputPath='D:\Data\RB000000\SampleA\all_wavelenghts',
                                            UniformBands=1)

   # Print some details
   print "Wrote {0} projection images, each combining {1} wavelength bands".format(angles, bands)

Output:

.. code-block:: none

  Wrote 144 projection images, each combining 2000 wavelength bands

.. categories::

.. sourcelink::

