# Copyright &copy; 2014-2015 ISIS Rutherford Appleton Laboratory, NScD
# Oak Ridge National Laboratory & European Spallation Source
#
# This file is part of Mantid.
# Mantid is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# Mantid is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
# File change history is stored at: <https://github.com/mantidproject/mantid>.
# Code Documentation is available at: <http://doxygen.mantidproject.org>

import numpy as np

# The code of the 'Wavelet-Fourier' stripe removal method is heavily based on
# the implementation of this method in tomopy
# The original code from Tomopy is:
# #########################################################################
# Copyright (c) 2015, UChicago Argonne, LLC. All rights reserved.         #
#                                                                         #
# Copyright 2015. UChicago Argonne, LLC. This software was produced       #
# under U.S. Government contract DE-AC02-06CH11357 for Argonne National   #
# Laboratory (ANL), which is operated by UChicago Argonne, LLC for the    #
# U.S. Department of Energy. The U.S. Government has rights to use,       #
# reproduce, and distribute this software.  NEITHER THE GOVERNMENT NOR    #
# UChicago Argonne, LLC MAKES ANY WARRANTY, EXPRESS OR IMPLIED, OR        #
# ASSUMES ANY LIABILITY FOR THE USE OF THIS SOFTWARE.  If software is     #
# modified to produce derivative works, such modified software should     #
# be clearly marked, so as not to confuse it with the version available   #
# from ANL.                                                               #
#                                                                         #
# Additionally, redistribution and use in source and binary forms, with   #
# or without modification, are permitted provided that the following      #
# conditions are met:                                                     #
#                                                                         #
#     * Redistributions of source code must retain the above copyright    #
#       notice, this list of conditions and the following disclaimer.     #
#                                                                         #
#     * Redistributions in binary form must reproduce the above copyright #
#       notice, this list of conditions and the following disclaimer in   #
#       the documentation and/or other materials provided with the        #
#       distribution.                                                     #
#                                                                         #
#     * Neither the name of UChicago Argonne, LLC, Argonne National       #
#       Laboratory, ANL, the U.S. Government, nor the names of its        #
#       contributors may be used to endorse or promote products derived   #
#       from this software without specific prior written permission.     #
#                                                                         #
# THIS SOFTWARE IS PROVIDED BY UChicago Argonne, LLC AND CONTRIBUTORS     #
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT       #
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS       #
# FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL UChicago     #
# Argonne, LLC OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,        #
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,    #
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;        #
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER        #
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT      #
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN       #
# ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE         #
# POSSIBILITY OF SUCH DAMAGE.                                             #
# #########################################################################

def remove_sino_stripes_rings_wf(data_vol, wv_levels, wavelet_name='db5', sigma=2, pad=True):
    """
    Removes horizontal stripes in sinograms (reducing ring artifacts in the reconstructed volume).
    Implements a combined Wavelet-Fourier filter (wf) as described in:

    Muench B, Trtrik P, Marone F, and Stampanoni M. 2009. Optics Express, 17(10):8567-8591.
    Stripe and ring artifact removal with combined wavelet-fourier filtering. 2009.

    This implementation is heavily based on the implementation from tomopy. This is not parallel
    at the moment.
    """
    try:
        import pywt
    except ImportError as exc:
        raise ImportError("Could not import the package pywt. Details: {0}".format(exc))

    dimx = data_vol.shape[0]
    n_x = dimx
    if pad:
        n_x = dimx + dimx / 8
    xshift = int((n_x - dimx) / 2.)

    for sino_idx in range(0, data_vol.shape[1]):
        sli = np.zeros((n_x, data_vol.shape[2]), dtype='float32')
        sli[xshift:dimx + xshift] = data_vol[:, sino_idx, :]

        # Wavelet decomposition
        c_H, c_V, c_D = [], [], []
        for _ in range(wv_levels):
            sli, (cHt, cVt, cDt) = pywt.dwt2(sli, wavelet_name)
            c_H.append(cHt)
            c_V.append(cVt)
            c_D.append(cDt)

        c_V = ft_horizontal_bands(c_V, wv_levels, sigma)

        # Wavelet reconstruction
        for nlvl in range(wv_levels)[::-1]:
            sli = sli[0:c_H[nlvl].shape[0], 0:c_H[nlvl].shape[1]]
            sli = pywt.idwt2((sli, (c_H[nlvl], c_V[nlvl], c_D[nlvl])),
                             wavelet_name)
        data_vol[:, sino_idx, :] = sli[xshift:dimx + xshift, 0:data_vol.shape[2]]

    return data_vol

def ft_horizontal_bands(c_V, wv_levels, sigma):
    """
    Fourier transform of horizontal frequency bands
    """
    for nlvl in range(wv_levels):
        # FT
        fcV = np.fft.fftshift(np.fft.fft(c_V[nlvl], axis=0))
        m_y, m_x = fcV.shape

        # Damping of ring artifact information
        y_hat = (np.arange(-m_y, m_y, 2, dtype='float32') + 1) / 2
        damp = 1 - np.exp(-np.power(y_hat, 2) / (2 * np.power(sigma, 2)))
        fcV = np.multiply(fcV, np.transpose(np.tile(damp, (m_x, 1))))

        # Inverse FT
        c_V[nlvl] = np.real(np.fft.ifft(np.fft.ifftshift(fcV), axis=0))

    return c_V

