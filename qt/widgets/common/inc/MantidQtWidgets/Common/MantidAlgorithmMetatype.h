// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDALGORITHMMETATYPE_H
#define MANTIDALGORITHMMETATYPE_H

#include "MantidAPI/Algorithm.h"
#include <QMetaType>

/**

 Declare Qt metatype for IAlgorithm_sptr to allow its direct use with signals
 and slots.

 */

Q_DECLARE_METATYPE(Mantid::API::IAlgorithm_sptr)

#endif /* MANTIDALGORITHMMETATYPE_H */
