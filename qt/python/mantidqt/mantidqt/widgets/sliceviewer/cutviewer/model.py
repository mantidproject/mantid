# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
from numpy import isclose, sum, argsort, ones, sqrt, zeros, mean, where, dot, cross, array, vstack
from numpy.linalg import det

DEFAULT_NBINS = 50


class CutViewerModel:
    def __init__(self, proj_matrix):
        self.proj_matrix = proj_matrix.T  # .T so now each basis vector is a row as in view table
        self.xvec = None  # q vector of colorfill x-axis
        self.yvec = None  # q vector of colorfill y-axis

    def get_default_bin_params(self, dims, ax_lims, slicept):
        states = dims.get_states()
        states[states.index(None)] = 2  # set last index as out of plane dimension
        vectors = self.proj_matrix[argsort(states), :]
        # write bin params for cut along horizontal axis (default)
        bin_params = dims.get_bin_params()  # nbins for viewed axes else integration width
        nbins = ones(vectors.shape[0], dtype=int)
        extents = ones((2, vectors.shape[0]), dtype=float)
        # get extents and nbins of slice axes (first two axes)
        for iaxis in range(vectors.shape[0] - 1):
            start, stop = ax_lims[iaxis]
            if iaxis == 0:
                # cut axis
                padding_frac = 0.25
                nbins[iaxis] = 2 * padding_frac * bin_params[states.index(iaxis)]
                padding = padding_frac * (stop - start)
                start, stop = start + padding, stop - padding
            else:
                # integrated axis (nbin=1)
                step = 2 * (stop - start) / bin_params[states.index(iaxis)]  # width = 2*step = 4*bin_width
                cen = (stop + start) / 2
                start, stop = cen - step, cen + step
            extents[:, iaxis] = [start, stop]
        # get extents of dimension out of plane of slice
        width = self.get_slicewidth(dims)
        extents[:, -1] = [slicept - width / 2, slicept + width / 2]
        return vectors, extents, nbins

    def valid_bin_params(self, vectors, extents, nbins):
        return (extents[-1, :] - extents[0, :] > 0).all() and sum(nbins > 1) == 1 and not isclose(det(vectors), 0.0)

    def get_slicewidth(self, dims):
        return dims.get_bin_params()[dims.get_states().index(None)]  # slicinfo not always up to date, so use dims

    def validate_vectors(self, irow, iunchanged, vectors):
        if abs(dot(vectors[irow], vectors[-1])) > 1e-15:
            # not a vector out of slice plane - reset
            vectors[irow] = cross(vectors[iunchanged], vectors[-1])
        else:
            # choose a new vector in plane that is not a linear combination of two other
            vectors[iunchanged] = cross(vectors[irow], vectors[-1])
        return vectors

    def validate_nbins(self, irow, iunchanged, nbins):
        if nbins[irow] < 1:
            nbins[irow] = 1
        if nbins[irow] == 1 and nbins[iunchanged] == 1:
            nbins[iunchanged] = DEFAULT_NBINS
        elif nbins[irow] > 1 and nbins[iunchanged] > 1:
            nbins[iunchanged] = 1
        return nbins

    def validate_step(self, irow, iunchanged, nbins, extents, step):
        step = abs(step)
        if nbins[irow] > 1:
            if step > 0:
                # step along cut axis changed
                nbin = (extents[1, irow] - extents[0, irow]) / step
                if nbin < 1:
                    # step greater than extent - swap cut direction to be along ivec
                    nbin = 1
                    nbins[iunchanged] = DEFAULT_NBINS
                elif nbin % 1 > 0:
                    extents[1, irow] = extents[1, irow] - (nbin % 1) * step  # so integer number of bins
                nbins[irow] = nbin
        else:
            # width of integrated axis changed
            cen = mean(extents[:, irow])
            extents[:, irow] = [cen - step / 2, cen + step / 2]
        return nbins, extents

    def validate_extents(self, irow, extents):
        # check extents have min < max - if not switch
        umin, umax = extents[:, irow]
        if umax < umin:
            extents[:, irow] = umax, umin
        return extents

    def calc_cut_representation_parameters(self, vectors, extents, nbins, states):
        self.xvec = self.proj_matrix[states.index(0), :]
        self.xvec /= sqrt(sum(self.xvec**2))
        self.yvec = self.proj_matrix[states.index(1), :]
        self.yvec /= sqrt(sum(self.yvec**2))
        # find x/y coord of start/end point of cut
        cens = mean(extents, axis=0)  # in u{1..3} basis of view table
        icut = where(nbins > 1)[0][0] if where(nbins > 1)[0].size > 0 else 0  # index of cut axis
        ivecs = list(range(len(vectors)))
        ivecs.pop(icut)
        zero_vec = zeros(vectors[0].shape)  # position at  0 along cut axis
        for ivec in ivecs:
            zero_vec = zero_vec + cens[ivec] * vectors[ivec]
        start = zero_vec + extents[0, icut] * vectors[icut, :]
        end = zero_vec + extents[1, icut] * vectors[icut, :]
        xmin = dot(start, self.xvec)
        xmax = dot(end, self.xvec)
        ymin = dot(start, self.yvec)
        ymax = dot(end, self.yvec)
        # get thickness of cut defined for unit vector perp to cut (so scale by magnitude of vector in the table)
        iint = nbins[:-1].tolist().index(1)
        thickness = (extents[1, iint] - extents[0, iint]) * sqrt(sum(vectors[iint, :] ** 2))
        return xmin, xmax, ymin, ymax, thickness

    def calc_bin_params_from_cut_representation(self, xmin, xmax, ymin, ymax, thickness, out_plane_vector):
        # get cut axis vector in u{1..3} basis of view table
        start = xmin * self.xvec + ymin * self.yvec
        stop = xmax * self.xvec + ymax * self.yvec
        u1 = stop - start
        u1 = u1 / sqrt(sum(u1**2))
        u1_min, u1_max = dot(start, u1), dot(stop, u1)
        # get integrated dim vector and thickness
        u2 = cross(u1, out_plane_vector)
        u2 = u2 / sqrt(sum(u2**2))
        u2_cen = dot(start, u2)
        u2_min, u2_max = u2_cen - thickness / 2, u2_cen + thickness / 2
        return vstack((u1, u2)), array([[u1_min, u2_min], [u1_max, u2_max]]), [50, 1]  # vecs, extents, nbins (first 2 rows)
