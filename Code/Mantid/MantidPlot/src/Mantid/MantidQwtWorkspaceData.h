#ifndef MANTIDPLOT_MANTIDQWTWORKSPACEDATA_H
#define MANTIDPLOT_MANTIDQWTWORKSPACEDATA_H

#include "qwt_data.h"

/// Abstract Qwtdata type
class MantidQwtWorkspaceData:public QwtData
{
public:
  virtual void setLogScale(bool on) = 0;
  virtual bool logScale() const = 0;
  virtual void saveLowestPositiveValue(const double v) = 0;
};

#endif