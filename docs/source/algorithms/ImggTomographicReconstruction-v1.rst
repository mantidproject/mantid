
.. algorithm::

.. summary::

.. alias::

.. properties::

Description
-----------

.. warning:: This is an early, experimental version of the algorithm.

The input and output workspaces are workspace groups where every
element is an image workspace. The input workspace must have one image
workspace per projection from a tomography imaging experiment. The
output workspace will have one image workspace for every slice of the
output reconstructed volume.

Methods supported: FBP (following the Tomopy implementation [TomoPy2014]).


The implementation of tomopy methods are based on the tomopy source
code available from https://github.com/tomopy/tomopy/, which is:

``
// Copyright 2015. UChicago Argonne, LLC. This software was produced
// under U.S. Government contract DE-AC02-06CH11357 for Argonne National
// Laboratory (ANL), which is operated by UChicago Argonne, LLC for the
// U.S. Department of Energy. The U.S. Government has rights to use,
// reproduce, and distribute this software.  NEITHER THE GOVERNMENT NOR
// UChicago Argonne, LLC MAKES ANY WARRANTY, EXPRESS OR IMPLIED, OR
// ASSUMES ANY LIABILITY FOR THE USE OF THIS SOFTWARE.  If software is
// modified to produce derivative works, such modified software should
// be clearly marked, so as not to confuse it with the version available
// from ANL.

// Additionally, redistribution and use in source and binary forms, with
// or without modification, are permitted provided that the following
// conditions are met:

//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.

//     * Redistributions in binary form must reproduce the above copyright
//       notice, this list of conditions and the following disclaimer in
//       the documentation and/or other materials provided with the
//       distribution.

//     * Neither the name of UChicago Argonne, LLC, Argonne National
//       Laboratory, ANL, the U.S. Government, nor the names of its
//       contributors may be used to endorse or promote products derived
//       from this software without specific prior written permission.

// THIS SOFTWARE IS PROVIDED BY UChicago Argonne, LLC AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
// FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL UChicago
// Argonne, LLC OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
// BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
// ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
``

References:

.. [TomoPy2014] Gursoy D, De Carlo F, Xiao X,
  Jacobsen C. (2014). TomoPy: a framework for the analysis of
  synchrotron tomographic data. J. Synchrotron Rad. 21. 1188-1193
  doi:10.1107/S1600577514013939


Usage
-----

**Example - LoadReconstructProjections**

.. testcode:: LoadReconstructProjections

    # Load an image
    wsg_name = 'images'
    wsg = LoadFITS(Filename='FITS_small_01.fits', LoadAsRectImg=1, OutputWorkspace=wsg_name)

.. testcleanup:: LoadReconstructProjections

    import os

    DeleteWorkspace(wsg_name)

Output:

.. testoutput:: LoadReconstructProjections

.. categories::

.. sourcelink::
