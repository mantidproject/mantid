#ifndef MANTIDQTAPI_MANTIDQWTWORKSPACEDATA_H
#define MANTIDQTAPI_MANTIDQWTWORKSPACEDATA_H

#include "qwt_data.h"
#include "DllOption.h"

/// Abstract Qwtdata type
class EXPORT_OPT_MANTIDQT_API MantidQwtWorkspaceData:public QwtData
{
public:
  virtual void setLogScale(bool on) = 0;
  virtual bool logScale() const = 0;
  virtual void saveLowestPositiveValue(const double v) = 0;
  virtual size_t esize() const = 0;
  virtual double e(size_t i)const = 0;
  virtual double ex(size_t i)const = 0;
  virtual double getYMin() const = 0;
  virtual double getYMax() const = 0;
protected:
  // Assignment operator (virtualized).
  MantidQwtWorkspaceData& operator=(const MantidQwtWorkspaceData&);
};

#endif
