%ModuleCode
#include "MantidQtIcons/Icon.h"
using namespace MantidQt::Icons;
%End

%Include ../../common_sip/vector.sip

QIcon getIcon(const QString &iconName, const QString &color = QString("black"), const double scaleFactor = 1.0);

QIcon getIcon(const QStringList &iconNames, const QList<QVariant> &options);