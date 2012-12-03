#ifndef INSTRUMENTWINDOWTAB_H
#define INSTRUMENTWINDOWTAB_H

#include <QFrame>

class InstrumentWindowTab : public QFrame
{
    Q_OBJECT
public:
    explicit InstrumentWindowTab(QWidget *parent = 0);
    /// Called by InstrumentWindow when the tab is made visible
    virtual void initOnShow() {}
    
};

#endif // INSTRUMENTWINDOWTAB_H
