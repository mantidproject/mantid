#ifndef THREESLICEVIEW_H
#define THREESLICEVIEW_H

#include <QtGui/QWidget>
#include <QPointer>
#include "iview.h"
#include "ui_threesliceview.h"

class pqPipelineRepresentation;
class pqPipelineSource;
class pqRenderView;

class ThreeSliceView : public IView, public Ui::ThreeSliceView
{
    Q_OBJECT

public:
    ThreeSliceView(QWidget *parent = 0);
    virtual ~ThreeSliceView();

    pqRenderView* getView();
    void render();

protected:
    pqRenderView *create2dRenderView(QWidget *container);

private:
    Q_DISABLE_COPY(ThreeSliceView);

    void makeSlice(IView::Direction i, pqRenderView *view,
    		pqPipelineSource *cut, pqPipelineRepresentation *repr);
    void makeThreeSlice();
    void renderAll();

    QPointer<pqPipelineSource> origSource;
    QPointer<pqPipelineRepresentation> originSourceRepr;

    QPointer<pqRenderView> mainView;
    QPointer<pqPipelineSource> xCut;
    QPointer<pqPipelineRepresentation> xCutRepr;
    QPointer<pqRenderView> xView;
    QPointer<pqPipelineSource> yCut;
    QPointer<pqPipelineRepresentation> yCutRepr;
    QPointer<pqRenderView> yView;
    QPointer<pqPipelineSource> zCut;
    QPointer<pqPipelineRepresentation> zCutRepr;
    QPointer<pqRenderView> zView;

};

#endif // THREESLICEVIEW_H
