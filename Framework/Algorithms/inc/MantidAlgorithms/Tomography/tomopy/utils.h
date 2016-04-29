/**
  Common functions for different reconstruction methods in Tomopy

  Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
  National Laboratory & European Spallation Source

  This file is part of Mantid.

  Mantid is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  Mantid is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http:www.gnu.org/licenses/>.

  File change history is stored at: <https:github.com/mantidproject/mantid>
  Code Documentation is available at: <http:doxygen.mantidproject.org>
*/
/**
 * This file is substantially modified but originally based on the
 * file utils.h from tomopy (available from
 * https:github.com/tomopy/tomopy/blob/master/src/utils.h) which is:
 */
/*
 Copyright (c) 2015, UChicago Argonne, LLC. All rights reserved.

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
*/
#ifndef MANTID_ALGORITHMS_TOMOGRAPHY_TOMOPY_UTILS_H_
#define MANTID_ALGORITHMS_TOMOGRAPHY_TOMOPY_UTILS_H_

void preprocessing(int ngridx, int ngridy, int dz, float center, float *mov,
                   float *gridx, float *gridy);

int calc_quadrant(float theta_p);

void calc_coords(int ngridx, int ngridy, float xi, float yi, float sin_p,
                 float cos_p, const float *gridx, const float *gridy,
                 float *coordx, float *coordy);

void trim_coords(int ngridx, int ngridy, const float *coordx,
                 const float *coordy, const float *gridx, const float *gridy,
                 int *asize, float *ax, float *ay, int *bsize, float *bx,
                 float *by);

void sort_intersections(int ind_condition, int asize, const float *ax,
                        const float *ay, int bsize, const float *bx,
                        const float *by, int *csize, float *coorx,
                        float *coory);

void calc_dist(int ngridx, int ngridy, int csize, const float *coorx,
               const float *coory, int *indi, float *dist);

void calc_simdata(int s, int p, int d, int ngridx, int ngridy, int dt, int dx,
                  int csize, const int *indi, const float *dist,
                  const float *model, float *simdata);

#endif /* MANTID_ALGORITHMS_TOMOGRAPHY_TOMOPY_UTILS_H_ */
