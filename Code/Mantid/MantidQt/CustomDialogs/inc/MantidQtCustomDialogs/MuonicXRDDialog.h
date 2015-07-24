#ifndef MANTIDQT_CUSTOM_DIALOGS_MUONICZRD_H_
#define MANTIDQT_CUSTOM_DIALOGS_MUONICXRD_H_

#include "MantidQtAPI/AlgorithmDialog.h"

namespace MantidQt {

  namespace CustomDialogs {

    class MuonicXRDDialog : public API::AlgorithmDialog {
  Q_OBJECT

public:
  MuonicXRDDialog(QWidget *parent = 0);

protected:
  void initLayout() = 0;
    };
  }
}
#endif // !MANTIDQT_CUSTOM_DIALOGS_MUONICZRD_H_
