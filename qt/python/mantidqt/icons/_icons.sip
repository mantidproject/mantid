%ModuleCode
#include "MantidQtIcons/Icon.h"
using namespace MantidQt::Icons;
%End

%Include ../../common_sip/vector.sip

QIcon getIcon(const QString &iconName, const QString &color = QString("black"), const double scaleFactor = 1.0);

// To achieve this exposure, we would need to add to vector.sip and add support for
// std::vector<QHash<QString, QVariant>> as a specialisation.
// QIcon getIcon(const std::vector<QString> &iconNames, const std::vector<QHash<QString, QVariant>> &options);