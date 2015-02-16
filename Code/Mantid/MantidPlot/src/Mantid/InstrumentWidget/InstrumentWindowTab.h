#ifndef INSTRUMENTWINDOWTAB_H
#define INSTRUMENTWINDOWTAB_H

#include <QFrame>
#include <boost/shared_ptr.hpp>

//--------------------------------------------------
//  Forward declarations
//--------------------------------------------------
class InstrumentWindow;
class ProjectionSurface;

class QSettings;
class QMenu;

class InstrumentWindowTab : public QFrame
{
    Q_OBJECT
public:
    explicit InstrumentWindowTab(InstrumentWindow *parent);
    /// Called by InstrumentWindow after the projection surface crated
    /// Use it for surface-specific initialization
    virtual void initSurface() {}
    /// Save tab's persistent settings to the provided QSettings instance
    virtual void saveSettings(QSettings&)const{}
    /// Load (read and apply) tab's persistent settings from the provided QSettings instance
    virtual void loadSettings(const QSettings&){}
    /// Add tab-specific items to the context menu
    /// Return true if at least 1 item was added or false otherwise.
    virtual bool addToDisplayContextMenu(QMenu&) const {return false;}
    /// Get the projection surface
    boost::shared_ptr<ProjectionSurface> getSurface() const;
protected:
    /// The parent InstrumentWindow
    InstrumentWindow* m_instrWindow;

};

#endif // INSTRUMENTWINDOWTAB_H
