#ifndef MANTIDABOUT_H
#define MANTIDABOUT_H

#include <QDialog>

class MantidAbout: public QDialog
{
    Q_OBJECT
public:
    MantidAbout(QWidget *parent = 0);
    ~MantidAbout();
};


#endif /* MANTIDABOUT_H */
