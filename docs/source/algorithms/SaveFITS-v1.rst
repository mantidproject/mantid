
.. algorithm::

.. summary::

.. relatedalgorithms::

.. properties::

Description
-----------

This algorithm saves an image from a workspace into a file in FITS
format. FITS stands for `Flexible Image Transport System
<http://en.wikipedia.org/wiki/FITS>`_.

The input matrix workspace must have one spectrum per row (and one bin
per column). This corresponds to the workspaces loaded by
:ref:`algm-LoadFITS` when its option LoadAsRectImg is true (note its
default is false). The first spectrum corresponds to the first
(topmost) row and the last spectrum corresponds to the last (bottom)
column. Images stored in this type of workspace can be displayed with
a "color fill plot" or plot2D.

The current implementation of this algorithm only supports:
- 2D files (FITS images with two axes).
- 8, 16 or 32 bits as pixel bit depth (only for integer types)

The files produced by this algorithm follow the FITS standard and can
be loaded by widespread software and libraries such as `ImageJ/Fiji
<http://fiji.sc>`_, `fv <http://heasarc.gsfc.nasa.gov/ftools/fv/>`_ or
`pyfits <http://docs.astropy.org/en/stable/io/fits/index.html>`_. The
files include the following basic standard headers: SIMPLE, BITPIX,
NAXIS, (NAXIS1 and NAXIS2), EXTEND, and a COMMENT header with the
format description.

.. seealso:: :ref:`algm-LoadFITS`.


Usage
-----

**Example - LoadSaveLoadFITS**

.. testcode:: LoadSaveLoadFITS

    # Load an image
    wsg_name = 'images'
    wsg = LoadFITS(Filename='FITS_small_01.fits', LoadAsRectImg=1, OutputWorkspace=wsg_name)
    ws = wsg.getItem(0)

    save_name = 'out_fits_example.fits'
    SaveFITS(Filename=save_name, InputWorkspace=ws)

    wsg_reload_name = 'images_reloaded'
    wsg_reload = LoadFITS(Filename=save_name, LoadAsRectImg=1, OutputWorkspace=wsg_reload_name)
    ws_reload = wsg_reload.getItem(0)

    # Compare
    bpp_log = 'BITPIX'
    try:
        log = ws.getRun().getLogData(bpp_log).value
        print("Bits per pixel in first image: {0}".format(int(log)))
        log_reload = ws.getRun().getLogData(bpp_log).value
        print("Bits per pixel in second image: {0}".format(int(log_reload)))
    except RuntimeError:
        print("Could not find the keyword '{0}' in the FITS file".format(bpp_log))

    axis1_log = 'NAXIS1'
    axis2_log = 'NAXIS2'
    try:
        log1 = ws.getRun().getLogData(axis1_log).value
        log2 = ws.getRun().getLogData(axis2_log).value
        print("Image size in first image: {0} x {1} pixels".format(int(log1), int(log2)))
        log1_reload = ws_reload.getRun().getLogData(axis1_log).value
        log2_reload = ws_reload.getRun().getLogData(axis2_log).value
        print("Image size in second image: {0} x {1} pixels".format(int(log1_reload), int(log2_reload)))
    except RuntimeError:
        print("Could not find the keywords '{}' and '{}' in this FITS file".format(axis1_log, axis2_log))

    pos_x, pos_y = 22, 33
    print("Pixel value at coordinates ({0},{1}), first image: {2:.1f}, second image: {3:.1f}".
           format(pos_x, pos_y, ws.readY(pos_y)[pos_x], ws_reload.readY(pos_y)[pos_x]))

.. testcleanup:: LoadSaveLoadFITS

    import os

    DeleteWorkspace(wsg_name)
    DeleteWorkspace(wsg_reload_name)
    os.remove(save_name)

Output:

.. testoutput:: LoadSaveLoadFITS

    Bits per pixel in first image: 16
    Bits per pixel in second image: 16
    Image size in first image: 512 x 512 pixels
    Image size in second image: 512 x 512 pixels
    Pixel value at coordinates (22,33), first image: 63.0, second image: 63.0

.. categories::

.. sourcelink::
