#ifndef MANTIDQTCUSTOMINTERFACES_DYNAMICPDF_BACKGROUNDREMOVER_H_
#define MANTIDQTCUSTOMINTERFACES_DYNAMICPDF_BACKGROUNDREMOVER_H_

#include <QDialog>
#include "ui_BackgroundRemover.h"
#include <boost/shared_ptr.hpp>

namespace MantidQt {
namespace CustomInterfaces {
namespace DynamicPDF {

// Forward declaration
class WorkspaceRecord;
class SliceSelector;

class BackgroundRemover: public QDialog
{
  Q_OBJECT

public:

  BackgroundRemover(SliceSelector *parent);


public:

  void refreshSlice(const boost::shared_ptr<WorkspaceRecord> loadedWorkspace,
                const size_t &workspaceIndex);

};

}
}
}

#endif // MANTIDQTCUSTOMINTERFACES_DYNAMICPDF_BACKGROUNDREMOVER_H_
