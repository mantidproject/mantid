// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef VATESPARAVIEWAPPLICATION_H_
#define VATESPARAVIEWAPPLICATION_H_

#include "MantidKernel/Logger.h"
#include "MantidVatesSimpleGuiViewWidgets/WidgetDllOption.h"

#include <QObject>
#include <QPointer>

namespace Mantid {
namespace Vates {
namespace SimpleGui {
/**
 *
 This class creates four views of the given dataset. There are three 2D views
 for the three orthogonal Cartesian planes and one 3D view of the dataset
 showing the planes.

 @author
 @date
 */
class EXPORT_OPT_MANTIDVATES_SIMPLEGUI_VIEWWIDGETS VatesParaViewApplication
    : public QObject {
  Q_OBJECT

public:
  static VatesParaViewApplication *instance();
  void setupParaViewBehaviors();

protected:
  VatesParaViewApplication();
  ~VatesParaViewApplication() override;

private:
  Q_DISABLE_COPY(VatesParaViewApplication)
  Mantid::Kernel::Logger m_logger;
  bool m_behaviorsSetup;
};
} // namespace SimpleGui
} // namespace Vates
} // namespace Mantid
#endif
