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
