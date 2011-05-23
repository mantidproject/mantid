#ifndef IVIEW_H_
#define IVIEW_H_

#include <QtGui/QWidget>

class pqObjectBuilder;
class pqRenderView;
class QString;

class IView : public QWidget {
public:
	IView(QWidget *parent = 0);
	virtual ~IView() {}
	virtual pqRenderView *createRenderView(QWidget *container);
	virtual void destroyFilter(pqObjectBuilder *builder, const QString &name);
	virtual pqRenderView *getView() = 0;
	virtual void render() = 0;

    enum Direction {X, Y, Z};
};

#endif /* IVIEW_H_ */
