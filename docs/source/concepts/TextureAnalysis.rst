.. _TextureAnalysis:

Texture Analysis
================

.. contents::

Introduction to Texture Analysis
################################

For argument sake, lets say you have a sample and you are interested to know what the crystallographic texture is -- that is to say, you want to know what
the relationship is between the macroscopic structure of your sample and some given crystallographic plane e.g. :math:`(100)`?

.. figure:: /images/texture-example-sample.png
   :alt: An example cuboid sample and a corresponding mantid representation

Taking this sample as an example, you can see that, by merit of being a cuboid, the sample has unique height, width and length.
These directions may or may not also be correlated with some processing procedure (e.g. in this case: the length is the Rolling Direction;
The width is the Traverse Direction and the height is the Normal Direction).

Some questions you might have about your sample are:

- How is the underlying crystal structure orientated in relation to these macroscopic directions?
- Does this relationship change when looking at different points within the sample?
- Is this relationship a product of the processing?


These are the questions which the texture analysis pipeline in Mantid seeks to help you answer.

Within the software are able to produce Pole Figures for different Bragg reflections.
The pole figure plot will typically show the intensity of the peak associated with that reflection along different macroscopic sample directions.

Additionally, you are able to produce similar plots but looking at other features of the bragg peak, such as peak position for strain mapping.


Interpreting Pole Figures
#########################

The way to interpret the pole figure is to imagine that your sample is within a sphere.
Each unique direction relative to your sample will intercept the sphere at a unique point -- these points are the poles (like the North and South Poles of the Earth).

If we imagine being able to sample the intensity of your given reflection peak in every possible direction, this would correspond to sampling the surface of the sphere.
Plotting the intensities on this sphere, we would get a complete 3D representation of how the intensity changes along all macroscopic sample directions.
The second graphic shows the plot with the intensity values convolved into the radial coordinate, which gives another spatial representation of the intensity of the bragg reflection
as a function of direction around the sample.

.. figure:: /images/texture-direction-sphere.gif
   :alt: GIF showing how the set of direction vectors trace out the surface of a sphere

.. figure:: /images/texture-direction-peaks.gif
   :alt: GIF showing how the set of direction vectors trace out the surface of a sphere, with intensity convolved into position

Much like how maps of the world provide 2D representations of the 3D globe, we can do the same thing by projecting the 3D pole figures down into a 2D pole figure.
The below graphic shows the relationship between the 2D pole figure and the 3D sphere which defines all unique directions around a sample.

.. figure:: /images/texture-pole-figure-interpretation.gif
   :alt: GIF showing the relationship between the 3D and 2D representations of the pole figure

The surface of this sphere is coloured by the intensity of a selected bragg peak, giving a 3D pole figure.
Additionally, the graphic is shows the distortion between this spherical representation and the intensity convolved representation.

Depending upon the exact transformation, the 2D pole figure can be chosen to maintain/highlight a desired geometric relation in the 3D surface
(e.g. the azimuthal and stereographic projections provided maintain the angular relationship, which can be useful for viewing the symmetry of poles).
In reality, we cannot sample every possible point on this sphere, we are experimentally confined by our detector geometries and finite time, to only sample a subset of these points.
These are the points displayed in the experimental pole figure scatter plot.
(It is possible to interpolate between these points to get a more continuous representation -- which is given as an option to display the contour plot instead.)

.. figure:: /images/texture-pole-figure-displays.png
   :alt: Image comparing the scatter plot pole figure and the contour interpolation


Generating Pole Figures
#######################

The below figure shows how the orientation of the detectors, relative to the sample, relates to the 3D and 2D pole figures.
The top two graphics show the individual scattering vectors for two of the detectors, depicted as gold and pink arrows,
and how the intrinsic directions of the sample move relative to these scattering vectors as the orientation of that sample changes during the experiment.
The bottom left graphic then shows, in the fixed, intrinsic sample frame of the pole figures, the corresponding relative movement of these scattering vectors.
Here the sphere is coloured with the intensity of the complete pole figure.
The bottom right graphic shows how the scattering vectors (corresponding to all the 30 detectors) are then projected into the 2D pole figure, again, the pink and gold detectors are highlighted here.

.. figure:: /images/texture-pole-figure-lookup.gif
   :alt: GIF showing the relationship between the experimental geometry and the pole figure


This final graphic, below, shows how the intensities are determined for the points in the experimental pole figure.
Here the two detector banks have been split up into 3x5 grids. The summed spectra for each block in the grid is collected over the course of the experiment and these are shown on the left and right plots.
The pole figure for a given reflection is then generated by fitting a peak to the desired reflection and reading out the peak parameter of interest which, in the case shown, is the integrated intensity.
The bottom images show these integrated intensity values on the actual detector banks and how these are projected into the 2D pole figure.

.. figure:: /images/texture-pole-figure-detectors.gif
   :alt: GIF showing how intensities are calculated for each detector in the pole figure

Pole Figure Resolution and Coverage
###################################

A few factors will effect the final quality of the pole figure data, with the two main considerations being how the detector banks are grouped and
for what sample orientations data is collected.

In mantid, the first of these -- the detector groupings, can be decided in post, after the experiment has been run.
The reality here (at least for ENGIN-X), is that despite it being possible to generate an experimental pole figure using each individual detector pixel as a unique point,
the confidence in the metric which has been extracted from peak fit will be a product of the signal-to-noise-ratio of those individual signals. This can be improved by
grouping neighbouring pixels together, thus obtaining cleaner spectra to fit, at the trade off of spatial resolution. Alternatively, beam access permitting, longer collection times
can be used, allowing finer pixel groupings (or none at all) to be achievable and improve the spatial resolution.

The second factor -- sample orientations, is something which perhaps requires more consideration before hitting go on data collection. The factors to weigh up here are
optimising your balance of time vs uncertainty. If you are quite confident in some aspect of your texture (such as a known symmetry), you may be able to target data
collection to obtain datasets with the detectors covering only a few key sectors in the pole figure, saving time on fewer experimental runs. In contrast, if the texture
is unknown, the optimal strategy is likely to be obtaining even coverage across the entire figure, and aiming to do this in a time efficient manner. The other trade off
of this exploratory coverage, compared to a more targetted approach is that one will likely end up with fewer data points around the actual regions of interest. A discussion
of possible exploratory coverage schemes is given by Malamud [#detBanks]_.

As such, again time permitting, a dual approach may prove advantageous for unknown textures, where a preliminary full coverage dataset is collect and, unpon subsequent
inspection, addition runs are collected targeting the identified regions of interest.

References
----------

.. [#detBanks] J. Appl. Cryst. (2014). 47, 1337–1354 doi:10.1107/S1600576714012710

.. categories:: Concepts
