#ifndef MANTIDQT_CUSTOM_DIALOGS_CATALOGPUBLISHDIALOG_H
#define MANTIDQT_CUSTOM_DIALOGS_CATALOGPUBLISHDIALOG_H

#include "MantidQtAPI/AlgorithmDialog.h"

namespace MantidQt
{
  namespace CustomDialogs
  {
    class CatalogPublishDialog : public MantidQt::API::AlgorithmDialog
    {
      Q_OBJECT

      public:
        /// Constructor
        CatalogPublishDialog(QWidget *parent = 0);
        /// Destructor
        ~CatalogPublishDialog();

      private:
        /// Create the inital layout.
        void initLayout();
    };
  }
}

#endif /* MANTIDQT_CUSTOM_DIALOGS_CATALOGPUBLISHDIALOG_H */
