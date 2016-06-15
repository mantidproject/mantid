
.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

.. warning:: This is an early, experimental version of the algorithm.

The input and output workspaces are workspace groups where every
element is an image workspace. In the input workspace every image is a
2D projection from a different angle whereas in the output workspace
every image is a slice of a reconstructed 3D volume.  The input
workspace must have one image workspace per projection from a
tomography imaging experiment. The output workspace will have one
image workspace for every slice of the output reconstructed volume.

The following method is supported: FBP (following the TomoPy
implementation [TomoPy2014]).

The implementation of TomoPy methods are based on the TomoPy source
code available from https://github.com/tomopy/tomopy/, which is:

::

    Copyright 2015. UChicago Argonne, LLC. This software was produced
    under U.S. Government contract DE-AC02-06CH11357 for Argonne National
    Laboratory (ANL), which is operated by UChicago Argonne, LLC for the
    U.S. Department of Energy. The U.S. Government has rights to use,
    reproduce, and distribute this software.  NEITHER THE GOVERNMENT NOR
    UChicago Argonne, LLC MAKES ANY WARRANTY, EXPRESS OR IMPLIED, OR
    ASSUMES ANY LIABILITY FOR THE USE OF THIS SOFTWARE.  If software is
    modified to produce derivative works, such modified software should
    be clearly marked, so as not to confuse it with the version available
    from ANL.

    Additionally, redistribution and use in source and binary forms, with
    or without modification, are permitted provided that the following
    conditions are met:

        * Redistributions of source code must retain the above copyright
          notice, this list of conditions and the following disclaimer.

        * Redistributions in binary form must reproduce the above copyright
          notice, this list of conditions and the following disclaimer in
          the documentation and/or other materials provided with the
          distribution.

        * Neither the name of UChicago Argonne, LLC, Argonne National
          Laboratory, ANL, the U.S. Government, nor the names of its
          contributors may be used to endorse or promote products derived
          from this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY UChicago Argonne, LLC AND CONTRIBUTORS
    "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
    LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
    FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL UChicago
    Argonne, LLC OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
    BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
    CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
    LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
    ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
    POSSIBILITY OF SUCH DAMAGE.

References:

.. [TomoPy2014] Gursoy D, De Carlo F, Xiao X,
  Jacobsen C. (2014). TomoPy: a framework for the analysis of
  synchrotron tomographic data. J. Synchrotron Rad. 21. 1188-1193
  doi:10.1107/S1600577514013939


Usage
-----

**Example - ReconstructProjections**

.. testcode:: ReconstructProjections

   # Note: you would load FITS images like this:
   # wsg = LoadFITS(Filename='FITS_small_01.fits', LoadAsRectImg=1, OutputWorkspace='projections')
   wsg_name = 'projections'
   # Produce 16 projections with 32x32 pixels
   projections = []
   unit_label=UnitFactory.create('Label')
   unit_label.setLabel('width','cm')
   for proj in range(0, 8):
              wks_name = 'wks_proj_' + str(proj)
              wks = CreateSampleWorkspace(NumBanks=32, BankPixelWidth=1, XMin=0, XMax=32, BinWidth=1, OutputWorkspace=wks_name)
              wks.getAxis(0).setUnit('Label')
              projections.append(wks)
   wsg_proj = GroupWorkspaces(projections, OutputWorkspace=wsg_name)
   wsg_reconstructed = ImggTomographicReconstruction(InputWorkspace=wsg_proj, CenterOfRotation=15)
   rows = wsg_reconstructed.getItem(0).getNumberHistograms()
   columns = wsg_reconstructed.getItem(0).blocksize()
   print ("The output reconstruction has {0} slices of {1} by {2} pixels".
          format(wsg_reconstructed.size(), rows, columns))
   slice_idx = 2
   coord_x = 8
   coord_y = 15
   print ("Value of pixel at coordinate ({0},{1}) in slice {2}: {3:.1f}".
          format(coord_x, coord_y, slice_idx,
              wsg_reconstructed.getItem(2).readY(coord_y)[coord_x]))

.. testcleanup:: ReconstructProjections

   DeleteWorkspace(wsg_name)

Output:

.. testoutput:: ReconstructProjections

   The output reconstruction has 32 slices of 32 by 32 pixels
   Value of pixel at coordinate (8,15) in slice 2: 2.4

.. categories::

.. sourcelink::
