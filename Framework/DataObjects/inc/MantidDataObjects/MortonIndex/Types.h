// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_DATAOBJECTS_MORTONINDEX_TYPES_H_
#define MANTID_DATAOBJECTS_MORTONINDEX_TYPES_H_

#include <Eigen/Dense>

template <size_t ND, typename IntT> using IntArray = Eigen::Array<IntT, static_cast<int>(ND), 1>;

template <size_t ND> using MDCoordinate = Eigen::Array<float, static_cast<int>(ND), 1>;

template <size_t ND> using MDSpaceBounds = Eigen::Array<float, static_cast<int>(ND), 2>;
template <size_t ND> using MDSpaceDimensions = Eigen::Array<float, static_cast<int>(ND), 1>;
template <size_t ND> using MDSpaceSteps = Eigen::Array<float, static_cast<int>(ND), 1>;

template <typename CoordT, size_t ND>
using AffineND = Eigen::Transform<CoordT, static_cast<int>(ND), Eigen::Affine>;

template <size_t ND> using BinIndices = Eigen::Matrix<size_t, 1, static_cast<int>(ND)>;

#endif // MANTID_DATAOBJECTS_MORTONINDEX_TYPES_H_
