#ifndef MANTIDQT_CUSTOM_DIALOGS_GETNEGMUMUONICXRD_H_
#define MANTIDQT_CUSTOM_DIALOGS_GETNEGMUMUONICXRD_H_

#include "MantidQtAPI/AlgorithmDialog.h"

namespace MantidQt {

  namespace CustomDialogs {

   class GetNegMuMuonicXRDDialog : public API::AlgorithmDialog {
  Q_OBJECT

     public:
       GetNegMuMuonicXRDDialog(QWidget *parent = 0);

   protected:
	   void initLayout();
    };
  }
}
#endif // !MANTIDQT_CUSTOM_DIALOGS_GETNEGMUMUONICXRD_H_
