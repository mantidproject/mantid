#include "InstrumentWindowTab.h"
#include "InstrumentWindow.h"

InstrumentWindowTab::InstrumentWindowTab(InstrumentWindow *parent) :
    QFrame(parent),m_instrWindow(parent)
{
}

/**
  * Return a pointer to the projection surface.
  */
boost::shared_ptr<ProjectionSurface> InstrumentWindowTab::getSurface() const
{
    return m_instrWindow->getSurface();
}
