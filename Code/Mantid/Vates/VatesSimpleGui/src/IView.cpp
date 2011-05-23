#include "iview.h"

#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqObjectBuilder.h"
#include "pqPipelineSource.h"
#include "pqRenderView.h"
#include "pqServer.h"
#include "pqServerManagerModel.h"

#include <QHBoxLayout>

IView::IView(QWidget *parent) : QWidget(parent)
{

}

pqRenderView* IView::createRenderView(QWidget* widget)
{
	QHBoxLayout *hbox = new QHBoxLayout(widget);
	hbox->setMargin(0);

	// Create a new render view.
	pqObjectBuilder* builder = pqApplicationCore::instance()->getObjectBuilder();
	pqRenderView *view = qobject_cast<pqRenderView*>(
			builder->createView(pqRenderView::renderViewType(),
					pqActiveObjects::instance().activeServer()));
	//pqActiveObjects::instance().setActiveView(view);

	// Place the widget for the render view in the frame provided.
	hbox->addWidget(view->getWidget());
	return view;
}

void IView::destroyFilter(pqObjectBuilder *builder, const QString &name)
{
	pqServer *server = pqActiveObjects::instance().activeServer();
	pqServerManagerModel *smModel = pqApplicationCore::instance()->getServerManagerModel();
	QList<pqPipelineSource *> sources;
	QList<pqPipelineSource *>::Iterator source;
	sources = smModel->findItems<pqPipelineSource *>(server);
	for (source = sources.begin(); source != sources.end(); ++source)
	{
		const QString sourceName = (*source)->getSMName();
		if (sourceName.startsWith(name))
		{
			builder->destroy(*source);
		}
	}

}
