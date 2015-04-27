.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

Load FITS files, which typically contain images, into a
:ref:`WorkspaceGroup <WorkspaceGroup>`. FITS stands for Flexible Image
Transport System, see http://en.wikipedia.org/wiki/FITS. A new
workspace (of type :ref:`Workspace2D <Workspace2D>`) is created for
every FITS file loaded with this algorithm, and these workspaces are
added into a :ref:`WorkspaceGroup <WorkspaceGroup>`. The group
workspace is named as indicated in the OutputWorkspace input
property. The workspaces included in the group are named with the same
name and an appendix _1, _2, etc., incremented sequentially as new
files are loaded with the same OutputWorkspace property.

The way image pixels are loaded into the resulting workspaces depends
on the porperty LoadAsRectImg. If it is set as false, one spectrum
will be created for every image pixel. Otherwise, one spectrum will be
created for every image row.

When LoadAsRectImg is true, the workspaces created by this algorithm
contain one spectrum per row (and one bin per column). The first
spectrum corresponds to the first (topmost) row and the last spectrum
corresponds to the last (bottom) column. With this type of workspace
you can display the image with a "color fill plot" or plot2D.

When LoadAsRectImg is false, the workspaces created by this algorithm
contain one spectrum per pixel. The first spectrum corresponds to the
top-left corner and the last spectrum to the bottom-right corner. The
image pixels are assigned to spectra row by row, i.e., the first
spectra correspond to the first or topmost row, the next spectra
correspond to the next rows, going from top down, until finally the
last spectra correspond to the last or bottom row.


The current implementation of this algorithm only supports 2D files
(FITS images with two axes). With this condition, in principle this
algorithm should be able to load any standard FITS file, and also
those who do not deviate too much from the standard. See below an
explanation of the minimum set of basic standard header entries that
this algorithm requires (those should be present in any FITS file), So
far, the algorithm has been tested with files produced by the cameras
used for the IMAT instrument at ISIS, and Starlight Xpress CCD cameras
used for calibration of other instruments at ISIS.

FITS header entries
###################

At a very minimum, the standard header entries SIMPLE, BITPIX, NAXIS,
NAXIS1, and NAXIS2 must be present in the file.

This algorithm interprets extension headers defined for the IMAT
instrument (ISIS facility). The set of extension headers for the IMAT
instrument is being defined as of this writing and specific support
and/or functionality related to additional headers might be added in
this algorithm.

Child algorithms used
#####################

This algorithm uses one child algorithm:

- :ref:`algm-LoadInstrument`, which looks for a description of the
  instrument in the facilities definition file and if found reads it.
  This algorithm is used only once when loading a set of FITS file
  corresponding to the same instrument.

Usage
-----

Example 1: loading one spectrum per image row
#############################################

.. testcode:: LoadFITS1SpectrumPerRow


    ws_name = 'FITSimgs'
    wsg = LoadFITS(Filename='FITS_small_01.fits', LoadAsRectImg=1, OutputWorkspace=ws_name)
    ws = wsg.getItem(0)

    # A couple of standard FITS header entries
    bpp_log = 'BITPIX'
    try:
        log = ws.getRun().getLogData(bpp_log).value
        print "Bits per pixel: %s" % int(log)
    except RuntimeError:
        print "Could not find the keyword '%s' in this FITS file" % bpp_log

    axis1_log = 'NAXIS1'
    axis2_log = 'NAXIS2'
    try:
        log1 = ws.getRun().getLogData(axis1_log).value
        log2 = ws.getRun().getLogData(axis2_log).value
        print "FITS image size: %s x %s pixels" % (int(log1), int(log2))
        print "Number of spectra in the output workspace: %d" % ws.getNumberHistograms()
    except RuntimeError:
        print "Could not find the keywords '%s' and '%s' in this FITS file" % (axis1_log, axis2_log)

.. testcleanup:: LoadFITS1SpectrumPerRow

    DeleteWorkspace(ws_name)

Output:

.. testoutput:: LoadFITS1SpectrumPerRow

   Bits per pixel: 16
   FITS image size: 512 x 512 pixels
   Number of spectra in the output workspace: 512

Example 2: loading one spectrum per pixel
#########################################

.. testcode:: LoadFITSManySpectra

    ws_name = 'FITSws'
    wsg = LoadFITS(Filename='FITS_small_01.fits', OutputWorkspace=ws_name)
    ws = wsg.getItem(0)

    # A couple of standard FITS header entries
    bpp_log = 'BITPIX'
    try:
        log = ws.getRun().getLogData(bpp_log).value
        print "Bits per pixel: %s" % int(log)
    except RuntimeError:
        print "Could not find the keyword '%s' in this FITS file" % bpp_log

    axis1_log = 'NAXIS1'
    axis2_log = 'NAXIS2'
    try:
        log1 = ws.getRun().getLogData(axis1_log).value
        log2 = ws.getRun().getLogData(axis2_log).value
        print "FITS image size: %s x %s pixels" % (int(log1), int(log2))
        print "Number of spectra in the output workspace: %d" % ws.getNumberHistograms()
    except RuntimeError:
        print "Could not find the keywords '%s' and '%s' in this FITS file" % (axis1_log, axis2_log)

.. testcleanup:: LoadFITSManySpectra

    DeleteWorkspace(ws_name)

Output:

.. testoutput:: LoadFITSManySpectra

   Bits per pixel: 16
   FITS image size: 512 x 512 pixels
   Number of spectra in the output workspace: 262144

.. categories::
