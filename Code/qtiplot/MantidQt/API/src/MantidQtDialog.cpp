//----------------------------------
// Includes
//----------------------------------
#include "MantidQtAPI/MantidQtDialog.h"

#include <QMessageBox>
#include <iostream>

using namespace MantidQt::API;

//------------------------------------------------------
// Public member functions
//------------------------------------------------------
/**
 * Default Constructor
 */
MantidQtDialog::MantidQtDialog(QWidget* parent):QDialog(parent)
{
}

/**
 * Destructor
 */
MantidQtDialog::~MantidQtDialog()
{
}

/**
 *   Checks if receiver derives from MantidQtDialog. If it does calls the virtual handleException method.
 *   @param receiver The Qt event receiver
 *   @param e The exception
 *   @return True if the exception was handled, false otherwise.
 */
bool MantidQtDialog::handle( QObject* receiver, const std::exception& e )
{
    QObject* obj = receiver;
    while(obj)
    {
        if (obj->inherits("MantidQt::API::MantidQtDialog"))
        {
            qobject_cast<MantidQtDialog*>(obj)->handleException(e);
            return true;
        }
        obj = obj->parent();
    };
    return false;
}

/** Override this method to handle an exception in a derived class.
 *  @param e exception to handle
 */
void MantidQtDialog::handleException( const std::exception& e )
{
    QMessageBox::critical(qobject_cast<QWidget*>(parent()),"Mantid - Error",
        "Exception is caught in dialog:\n\n"+QString::fromStdString(e.what()));
    close();
}
