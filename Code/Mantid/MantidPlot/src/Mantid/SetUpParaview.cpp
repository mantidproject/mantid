#include "SetUpParaview.h"
#include "MantidKernel/ConfigService.h"
#include "MantidQtAPI/ManageUserDirectories.h"

SetUpParaview::SetUpParaview(QWidget *parent) : QDialog(parent)
{
  m_uiForm.setupUi(this);
  initLayout();
}

SetUpParaview::~SetUpParaview()
{
}

void SetUpParaview::initLayout()
{
  
}
