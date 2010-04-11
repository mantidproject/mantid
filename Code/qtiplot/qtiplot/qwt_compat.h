#ifndef MANTID_QWT_COMPAT
#define MANTID_QWT_COMPAT

#include <qwt_global.h>

#if QWT_VERSION >= 0x050200
// FAA 11/04/2010: QWT 5.2.0 changed some member functions
// QwtScaleEngine 
//   loMargin/hiMargin renamed to lowerMargin/upperMargin
// QwtScaleDiv 
//   lBound/hBound renamed to lowerBound/upperBound
#   define lBound() lowerBound()
#   define hBound() upperBound()
#   define loMargin() lowerMargin()
#   define hiMargin() upperMargin()
#endif /* QWT_VERSION >= 0x050200 */

#endif /* MANTID_QWT_COMPAT */
