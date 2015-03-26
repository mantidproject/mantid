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

The current implementation of this algorithm only supports 2D files
(FITS images with two axes), and it should in principle be able to
load FITS files from different sources.

The workspaces created by this algorithm contain one spectrum per
pixel. The first spectrum correspond to the top-left corner and the
last spectrum to the bottom-right corner.

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

**Example**

.. testcode:: LoadFITS

    ws_name = 'FITSws'
    wsg = LoadFITS(Filename='FITS_small_01.fits', OutputWorkspace=ws_name)
    ws = wsg.getItem(0)

    # A couple of standard FITS header entries
    bpp_log = 'BITPIX'
    try:
        log = ws.getRun().getLogData(bpp_log)
        print "Bits per pixel: %d" % bpp_log
    except RuntimeError:
        print "Could not find the keyword '%s' in this FITS file" % bpp_log

    axis1_log = 'NAXIS1'
    axis2_log = 'NAXIS2'
    try:
        log1 = ws.getRun().getLogData(axis1_log)
        log2 = ws.getRun().getLogData(axis2_log)
        print "FITS image size: %d x %d pixels" % (log1, log2)
        print "Number of spectra in the output workspace: %d" % ws.getNumberHistograms()
    except RuntimeError:
        print "Could not find the keywords '%s' and '%s' in this FITS file" % (axis1_log, axis2_log)

.. testcleanup:: LoadFITS

    DeleteWorkspace(ws_name)

Output:

.. testoutput:: LoadFITS

   Bits per pixel: 16
   FITS image size: 512 x 512 pixels
   Number of spectra in the output workspace: 262144

.. categories::
