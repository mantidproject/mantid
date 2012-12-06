#include "SimpleWidget.h"
#include "ProjectionSurface.h"

/// Constructor
SimpleWidget::SimpleWidget(QWidget* parent):QWidget(parent),m_surface(NULL)
{
    // Receive mouse move events
    setMouseTracking( true );
    // Receive keyboard events
    setFocusPolicy(Qt::StrongFocus);
}

/// Assign a surface to draw on
void SimpleWidget::setSurface(ProjectionSurface* surface)
{
    m_surface = surface;
    connect(m_surface,SIGNAL(redrawRequired()),this,SLOT(repaint()),Qt::QueuedConnection);
}

/// Redraw the view
void SimpleWidget::updateView()
{
    if(m_surface)
    {
      m_surface->updateView();
      update();
    }
}

/// Update the detector information (count values) and redraw
void SimpleWidget::updateDetectors()
{
    if(m_surface)
    {
      m_surface->updateDetectors();
      update();
    }
}

void SimpleWidget::paintEvent(QPaintEvent*)
{
    if(m_surface)
    {
      m_surface->drawSimple(this);
    }
}

/**
* Mouse press callback method, It implements mouse button press initialize methods.
* @param event :: This is the event variable which has the position and button states
*/
void SimpleWidget::mousePressEvent(QMouseEvent* event)
{
    if (m_surface)
    {
      m_surface->mousePressEvent(event);
    }
    update();
}

/**
* This is mouse move callback method. It implements the actions to be taken when the mouse is
* moved with a particular button is pressed.
* @param event :: This is the event variable which has the position and button states
*/
void SimpleWidget::mouseMoveEvent(QMouseEvent* event)
{
    if (m_surface)
    {
      m_surface->mouseMoveEvent(event);
    }
    repaint();
}

/**
* This is mouse button release callback method. This resets the cursor to pointing hand cursor
* @param event :: This is the event variable which has the position and button states
*/
void SimpleWidget::mouseReleaseEvent(QMouseEvent* event)
{
    if (m_surface)
    {
      m_surface->mouseReleaseEvent(event);
    }
    repaint();
}

/**
* Mouse wheel event to set the zooming in and out
* @param event :: This is the event variable which has the status of the wheel
*/
void SimpleWidget::wheelEvent(QWheelEvent* event)
{
    if (m_surface)
    {
      m_surface->wheelEvent(event);
    }
    update();
}

/**
* Key press event
* @param event :: This is the event variable which has the status of the keyboard
*/
void SimpleWidget::keyPressEvent(QKeyEvent *event)
{
    if (m_surface)
    {
      m_surface->keyPressEvent(event);
    }
    update();
}
