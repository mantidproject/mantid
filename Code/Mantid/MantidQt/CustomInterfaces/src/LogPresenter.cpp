#include "MantidQtCustomInterfaces/LogPresenter.h"
#include "MantidQtCustomInterfaces/LoanedMemento.h"
#include "MantidQtCustomInterfaces/LogView.h"

namespace MantidQt
{
  namespace CustomInterfaces
  {
    LogPresenter::LogPresenter(LoanedMemento& memento) : m_WsMemento(memento)
    {
    }

    LogPresenter::~LogPresenter()
    {
    }

    void LogPresenter::update()
    {
    }

    void LogPresenter::acceptView(LogView*)
    {
    }
  }
}