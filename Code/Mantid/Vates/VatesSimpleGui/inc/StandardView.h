#ifndef STANDARDVIEW_H
#define STANDARDVIEW_H

#include <QtGui/QWidget>
#include <QPointer>
#include "IView.h"
#include "ui_StandardView.h"

class pqPipelineRepresentation;
class pqPipelineSource;
class pqRenderView;

class StandardView : public IView, public Ui::StandardView
{
    Q_OBJECT

public:
    StandardView(QWidget *parent = 0);
    virtual ~StandardView();

    pqRenderView* getView();
    void render();

protected slots:
    void onCutButtonClicked();
    void onRebinButtonClicked();

private:
    Q_DISABLE_COPY(StandardView);
    QPointer<pqPipelineSource> origSource;
    QPointer<pqPipelineRepresentation> originSourceRepr;
    QPointer<pqPipelineSource> rebinCut;
    QPointer<pqRenderView> view;
};

#endif // STANDARDVIEW_H
