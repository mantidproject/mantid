#ifndef MANTIDGLWIDGET_H_
#define MANTIDGLWIDGET_H_

#include <QGLWidget>
#include <QString>

class UnwrappedSurface;

/**
  \class  MantidGLWidget
  \brief  OpenGL Qt Widget which renders Mantid Geometry ObjComponents
*/

class MantidGLWidget : public QGLWidget
{
  Q_OBJECT
public:
  enum InteractionMode {MoveMode = 0, PickMode = 1}; ///< Move around or select things
  enum PolygonMode{ SOLID, WIREFRAME };
  enum RenderMode{ FULL3D = 0, CYLINDRICAL_X, CYLINDRICAL_Y, CYLINDRICAL_Z, SPHERICAL_X, SPHERICAL_Y, SPHERICAL_Z, RENDERMODE_SIZE };
  MantidGLWidget(QWidget* parent=0); ///< Constructor
  virtual ~MantidGLWidget();         ///< Destructor
  void setInteractionModeMove();
  void setInteractionModePick();
  InteractionMode getInteractionMode()const{return m_interactionMode;}
  void setBackgroundColor(QColor);
  QColor currentBackgroundColor() const;
  void saveToFile(const QString & filename);
  RenderMode getRenderMode()const{return m_renderMode;}

public slots:
  void enableLighting(bool);
  void setWireframe(bool);
  void resetUnwrappedViews();
  //void setSelectionType(int);
  void setRenderMode(int);
  void refreshView();

protected:
  void initializeGL();
  void resetWidget();
  void MakeObject();
  void paintEvent(QPaintEvent *event);
  void resizeGL(int,int);
  void mousePressEvent(QMouseEvent*);
  void contextMenuEvent(QContextMenuEvent*);
  void mouseMoveEvent(QMouseEvent*);
  void mouseReleaseEvent(QMouseEvent*);
  void wheelEvent(QWheelEvent *);
  void keyPressEvent(QKeyEvent *);
  void keyReleaseEvent(QKeyEvent *);
  void draw();
  void checkGLError(const QString& funName);
  
private:
  void setRenderingOptions();
  void setLightingModel(int);

  QColor m_bgColor;                 ///< Background color
  InteractionMode m_interactionMode;
  RenderMode m_renderMode;       ///< 3D view or unwrapped
  bool isKeyPressed;
  int m_lightingState;           ///< 0 = light off; 2 = light on
  PolygonMode m_polygonMode;     ///< SOLID or WIREFRAME
  bool m_firstFrame;

  //// Unwrapping stuff
  UnwrappedSurface* m_unwrappedSurface;
  bool m_unwrappedSurfaceChanged;
  bool m_unwrappedViewChanged;   ///< set when the unwrapped image must be redrawn, but the surface is the same
  //boost::scoped_ptr<DetSelector> m_detSelector;    ///< draws the selection region

};

#endif /*MANTIDGLWIDGET_H_*/

