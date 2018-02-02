#ifndef MANTIDQT_INSTRUMENTVIEW_QT_COMPAT_H_
#define MANTIDQT_INSTRUMENTVIEW_QT_COMPAT_H_

#if QT_VERSION >= 0x050000
#define fromAscii fromLatin1
#define toAscii toLatin1
#define intersect intersected
#endif

#endif // MANTIDQT_INSTRUMENTVIEW_QT_COMPAT_H_
