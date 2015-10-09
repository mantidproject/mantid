Tomographic Reconstruction
==========================

.. contents:: Table of Contents
  :local:

Overview
--------

This interface aims at integrating and simplifying the following tasks
related to tomographic reconstruction and imaging with neutrons. While
much of its functionality is being developed in a generic way, it is
presently being tested and trialed for the IMAT instrument at ISIS.

.. interface:: Tomographic Reconstruction
  :align: center
  :width: 600

An important feature of this interface is the capability to submit
jobs to a remote compute resource (a compute cluster for
example). Currently remote jobs are run on the `SCARF cluster
<http://www.scarf.rl.ac.uk/>`_, administered by the Scientific
Computing Department of STFC. You can also use this cluster via remote
login and through its `web portal <https://portal.scarf.rl.ac.uk/>`_.

Interface at a glance
---------------------

By default the interface shows the *Run* tab, where you can visualize
images, submit reconstruction jobs, see and control the status of the
jobs submitted recently.

.. interface:: Tomographic Reconstruction
  :widget: runTab
  :align: right
  :width: 300

In the setup tab you can set the details of the remote and/or local
compute resources. Importantly, here is where you can set you username
and password to log into the remote compute resource. To be able to
run jobs remotely you first need to log into the remote compute
resource. Once you log in, an automatic mechanism will periodically
query the status of jobs (for example every minute). You can also
update it at any time by clicking on the refresh button.

In this tab you also have to set the folders/directories where the
input data for reconstruction jobs is found. This information is
required every time you start analyzing a new dataset. The required
fields are:

Samples directory
  Directory containing the sample images

Open beam directory
  Where to find the open beam (flat/white) image(s)

Dark field directory
  Where to find the dark image(s)

.. interface:: Tomographic Reconstruction
  :widget: setupTab
  :align: right
  :width: 300

In principle, the use of this interface is straightforward. **NB**: as
this interface is in an early stage and under heavy development,
several practical details are missing. This implies that there may be
usability issues at times and some steps may not be as intuitive or
simple as they could. Please, do not hesitate to provide feedback.

The next sections provide further details that might be needed to
fully understand the process of generating tomographic reconstructions
with this interface.

Tools
-----

At the moment two reconstruction tools are being set up and trialed on
SCARF:

* `TomoPy
  <https://www1.aps.anl.gov/Science/Scientific-Software/TomoPy>`_

* `Astra Toolbox <http://sourceforge.net/p/astra-toolbox/wiki/Home/>`_

In the near future it is expected that support will be added for `Savu
<https://github.com/DiamondLightSource/Savu>`_, developed at the
Diamond Light Source.

Data formats
------------

In principle, users do not need to deal with specificities of
different file formats. That is the aim of this interface, but as it
is currently being developed, and for reference a brief list of
relevant file and data formats is given here:

* FITS: `Flexible Image Transport System format
  <http://en.wikipedia.org/wiki/FITS>`__ used to store images in
  files. You can see the details on how FITS images can be loaded into
  Mantid in the documentation of the algorithm LoadFITS
  `<http://docs.mantidproject.org/nightly/algorithms/LoadFITS-v1.html>`__.

* TIFF: `Tagged Image File Format
  <http://en.wikipedia.org/wiki/Tagged_Image_File_Format>`__ images
  used as FITS for image or slice files. This format is presently not
  supported in the Mantid data analysis framework but it is used in
  the tomographic reconstruction interface.

* Diamond Light Source (DLS) NXTomo: a specific NeXus format used by
  some of the tools that this interface supports or will support. See
  next sections for details.

These formats are used in different processing steps and parts of this
interface. For example, you can visualize FITS and TIFF images in the
**Run** tab. As another example, the reconstruction tools typically
need as inputs at least a stack of images which can be in different
formats, including a set of FITS or TIFF files, or a single DLS NXTomo
file. Other third party tools use files in these formats as inputs,
outputs or both.

Data locations
--------------

This is dependent on the facility and instrument.

TODO: this is work in progress. In principle data will be replicated
in the ISIS archive, the SCARF imat disk space, and possibly an
analysis machine located in R3.

Running jobs remotely
---------------------

To be able to run jobs on a remote compute resource (cluster, supercomputer, etc.)

* Log into the resource
* Select it and setup one reconstruction tool
* Use the **reconstruct** button in the **Run** tab of the interface

You can monitor the status of the jobs currently running (and recently
run) on remote compute resources in the same tab.

Setting common parameters for the reconstruction jobs
-----------------------------------------------------

Several parameters can be set in the "ROI etc." section or tab. These
parameters will be used for all the reconstruction jobs, regardless of
the tool and/or reconstruction method used.

* Region of interest (ROI) for the analysis
* Area for normalization (open beam, not blocked by sample)
* Center of rotation, for tomographic reconstruction

At any stage during the process of selecting the regions it is also
possible to see how the selections fit different images by sliding
through the images of the stack (using the slider or scroll bar).

The center of rotation can be selected interactively by clicking on
the select button and then clicking on an image pixel. To select the
regions of interest or the area of normalization, just click on the
respective "select" button and then click and drag with the mouse to
select a rectangle. The precise coordinates of the center and regions
can be set via the boxes of the right panel as well.

Once you have selected or set one of the regions, or the center, they
can be selected again by pushing the respective "Select" buttons
and/or editing their coordinates manually.

The default values, set in principle when a new stack of images is
loaded, is as follows. The region of intererest is set to cover all
the images. The regions of normalization is not set (empty), and the
center of rotation is set to the center of the image. The option to
find the center of rotation automatically is disabled at present.

If when selection a region the mouse is moved outside of the images,
it is possible to continue the selection of the region (second corner)
by clicking again inside the image. Alternatively, any selection can
be reset at any point by using the "reset" buttons.

Running jobs locally
--------------------

This functionality is not available at present.

Example
-------

TODO: ideally, come up with a good and small example data set.

TomoPy
------

TODO: how to use it. Hints.

Astra Toolbox
-------------

TODO: how to use it. Hints.

Astra Toolbox
-------------

TODO: how to use it. Hints.

Savu
----

TODO: how to use it. Hints.

Savu uses a specific file format developed by the Diamond Light
Source, the DLS NXTomo. A few examples can be found from `the savu
repository on GitHub
<https://github.com/DiamondLightSource/Savu/tree/master/test_data>`__.

Pipeline configuration
~~~~~~~~~~~~~~~~~~~~~~

A Savu reconstruction pipeline is defined by a list of processing
steps (or plugins) and their parameters. In the Savu setup dialog this
list is built on the right panel (current configuration) by adding and
sorting available plugins available from the tree shown on the left
panel. From the file menu, different savu configurations can be saved for
later use and loaded from previously saved files.

.. Leave this out for now. Not used at the moment.
   .. interface:: Tomographic Reconstruction
     :widget: savuConfigCentralWidget
     :align: right


.. categories:: Interfaces Diffraction
