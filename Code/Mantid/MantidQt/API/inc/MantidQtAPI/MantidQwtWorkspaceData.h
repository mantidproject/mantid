#ifndef MANTIDQTAPI_MANTIDQWTWORKSPACEDATA_H
#define MANTIDQTAPI_MANTIDQWTWORKSPACEDATA_H

#include "qwt_data.h"

/// Abstract Qwtdata type
class MantidQwtWorkspaceData:public QwtData
{
public:
  virtual void setLogScale(bool on) = 0;
  virtual bool logScale() const = 0;
  virtual void saveLowestPositiveValue(const double v) = 0;
  virtual size_t esize() const = 0;
  virtual double e(size_t i)const = 0;
  virtual double ex(size_t i)const = 0;
};

#endif
