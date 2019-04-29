// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef PROJECTION3D_H_
#define PROJECTION3D_H_

#include "ProjectionSurface.h"
#include "Viewport.h"

#include "MantidGeometry/IComponent.h"

#include <QGLWidget>
#include <QString>

#include <boost/shared_ptr.hpp>

namespace MantidQt {
namespace MantidWidgets {

/**
This is an implementation of ProjectionSurface for viewing the instrument in 3D.

*/
class Projection3D : public ProjectionSurface {
  Q_OBJECT
  enum AxisDirection {
    XPOSITIVE,
    YPOSITIVE,
    ZPOSITIVE,
    XNEGATIVE,
    YNEGATIVE,
    ZNEGATIVE
  };

public:
  Projection3D(const InstrumentActor *rootActor, int winWidth, int winHeight);
  ~Projection3D() override;
  RectF getSurfaceBounds() const override;

  void setViewDirection(const QString &vd);
  void set3DAxesState(bool on);
  void setWireframe(bool on);

  void componentSelected(size_t componentIndex) override;
  void getSelectedDetectors(std::vector<size_t> &detIndices) override;
  void getMaskedDetectors(std::vector<size_t> &detIndices) const override;
  void resize(int /*unused*/, int /*unused*/) override;
  QString getInfoText() const override;
  /// Load settings for the 3D projection from a project file
  virtual void loadFromProject(const std::string &lines) override;
  /// Save settings for the 3D projection to a project file
  virtual std::string saveToProject() const override;

signals:
  void finishedMove();

protected slots:
  void initTranslation(int x, int y);
  void translate(int x, int y);
  void initZoom(int x, int y);
  void zoom(int x, int y);
  void wheelZoom(int x, int y, int d);
  void initRotation(int x, int y);
  void rotate(int x, int y);
  void finishMove();

protected:
  void init() override {}
  void drawSurface(MantidGLWidget *widget, bool picking = false) const override;
  void changeColorMap() override;

  void drawAxes(double axis_length = 100.0) const;
  void setLightingModel(bool picking) const;

  bool m_drawAxes;
  bool m_wireframe;

  Viewport m_viewport;

  friend class InstrumentWidgetEncoder;
  friend class InstrumentWidgetDecoder;
};

} // namespace MantidWidgets
} // namespace MantidQt

#endif /* PROJECTION3D_H_ */
