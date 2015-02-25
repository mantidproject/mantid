#include "MantidQtCustomInterfaces/UserInputValidator.h"

#include <QValidator>
#include <QLineEdit>
#include <QLabel>
#include <QString>

#include <cmath>

using namespace MantidQt::MantidWidgets;

namespace // anonymous
{
  template <typename T>
  void sortPair(std::pair<T, T> & pair)
  {
    if( pair.first > pair.second )
    {
      T temp = pair.first;
      pair.first = pair.second;
      pair.second = temp;
    }
  }
} // anonymous namespace

namespace MantidQt
{
  namespace CustomInterfaces
  {
    UserInputValidator::UserInputValidator() : m_errorMessages()
    {
    }

    /**
     * Check that a given QLineEdit field (with given name) is not empty.  If it is empty
     * then the given QLabel will be set to "*" and an error will be added to m_errorMessages.
     * Else set the label to "".
     *
     * @param name       :: name of the field, so as to be recognised by the user
     * @param field      :: the field object to be checked
     * @param errorLabel :: the "*" or "" label.
     * @returns True if the input was valid
     */
    bool UserInputValidator::checkFieldIsNotEmpty(const QString & name, QLineEdit * field, QLabel * errorLabel)
    {
      if(field->text() == "")
      {
        setErrorLabel(errorLabel, false);
        m_errorMessages.append(name + " has been left blank.");
        return false;
      }
      else
      {
        setErrorLabel(errorLabel, true);
        return true;
      }
    }

    /**
     * Check that the given QLineEdit field is valid as per any validators it might have.
     *
     * @param field        :: the field to be checked
     * @param errorLabel   :: the "" or "*" label
     * @param errorMessage :: the message to log if invalid
     * @returns True if the input was valid
     */
    bool UserInputValidator::checkFieldIsValid(const QString & errorMessage, QLineEdit * field, QLabel * errorLabel)
    {
      int dummyPos = 0;
      QString text = field->text();
      QValidator::State fieldState = field->validator()->validate(text, dummyPos);

      if( fieldState == QValidator::Acceptable )
      {
        setErrorLabel(errorLabel, true);
        return true;
      }
      else
      {
        setErrorLabel(errorLabel, false);
        m_errorMessages.append(errorMessage);
        return false;
      }
    }

    /**
     * Check that the given WorkspaceSelector is not empty.  Appends a message to m_errorMessages if so.
     *
     * @param name              :: the "name" of the workspace selector, so as to be recognised by the user
     * @param workspaceSelector :: the workspace selector to check
     * @returns True if the input was valid
     */
    bool UserInputValidator::checkWorkspaceSelectorIsNotEmpty(const QString & name, WorkspaceSelector * workspaceSelector)
    {
      if( workspaceSelector->currentText() == "" )
      {
        m_errorMessages.append("No " + name + " workspace has been selected.");
        return false;
      }

      return true;
    }

    /**
     * Check that the given MWRunFiles widget has valid files.
     *
     * @param name   :: the "name" of the widget so as to be recognised by the user.
     * @param widget :: the widget to check
     * @returns True if the input was valid
     */
    bool UserInputValidator::checkMWRunFilesIsValid(const QString & name, MWRunFiles * widget)
    {
      if( ! widget->isValid() )
      {
        m_errorMessages.append(name + " file error: " + widget->getFileProblem());
        return false;
      }

      return true;
    }

    /**
     * Check that the given DataSelector widget has valid files.
     *
     * @param name   :: the "name" of the widget so as to be recognised by the user.
     * @param widget :: the widget to check
     * @returns True if the input was valid
     */
    bool UserInputValidator::checkDataSelectorIsValid(const QString & name, DataSelector * widget)
    {
      if( ! widget->isValid() )
      {
        m_errorMessages.append(name + " error: " + widget->getProblem());
        return false;
      }

      return true;
    }

    /**
     * Check that the given start and end range is valid.
     *
     * @param name  :: the name of the range
     * @param range :: the range
     * @returns True if the input was valid
     */
    bool UserInputValidator::checkValidRange(const QString & name, std::pair<double, double> range)
    {
      if( range.second == range.first )
      {
        m_errorMessages.append(name + " must have a non-zero width.");
        return false;
      }

      if( range.second < range.first )
      {
        m_errorMessages.append("The start of " + name + " must be less than the end.");
        return false;
      }

      return true;
    }

    /**
     * Check that the given ranges dont overlap.
     *
     * @param rangeA :: the start of the range
     * @param rangeB :: the end of the range
     * @returns True if the input was valid
     */
    bool UserInputValidator::checkRangesDontOverlap(std::pair<double, double> rangeA, std::pair<double, double> rangeB)
    {
      sortPair(rangeA);
      sortPair(rangeB);

      if( !(rangeA.second < rangeB.first || rangeB.second < rangeA.first) )
      {
        QString message = QString("The ranges must not overlap: [%1,%2], [%3,%4].")
          .arg(rangeA.first).arg(rangeA.second).arg(rangeB.first).arg(rangeB.second);
        m_errorMessages.append( message );
        return false;
      }

      return true;
    }

    /**
     * Check that the given "outer" range completely encloses the given "inner" range.
     *
     * @param outerName :: the end of the range
     * @param outer :: pair of range bounds
     * @param innerName :: the start of the range
     * @param inner :: pair of range bounds
     * @returns True if the input was valid
     */
    bool UserInputValidator::checkRangeIsEnclosed(const QString & outerName, std::pair<double, double> outer,
                                                  const QString & innerName, std::pair<double, double> inner)
    {
      sortPair(inner);
      sortPair(outer);

      if( inner.first < outer.first || inner.second > outer.second )
      {
        m_errorMessages.append(outerName + " must completely enclose " + innerName + ".");
        return false;
      }

      return true;
    }

    /**
     * Given a range defined by lower and upper bounds, checks to see if it can be divided evenly into bins
     * of a given width.  Due to the nature of doubles, we use a tolerance value.
     *
     * @param lower     :: the lower bound of the range
     * @param binWidth  :: the wdith of the bin
     * @param upper     :: the upper bound of the range
     * @param tolerance :: the tolerance with which to judge range / bin suitablility
     * @returns True if the input was valid
     */
    bool UserInputValidator::checkBins(double lower, double binWidth, double upper, double tolerance)
    {
      double range = upper - lower;
      if( range < 0 )
      {
        m_errorMessages.append("The start of a binning range must be less than the end.");
        return false;
      }
      if( range == 0 )
      {
        m_errorMessages.append("Binning ranges must be non-zero.");
        return false;
      }
      if( binWidth == 0 )
      {
        m_errorMessages.append("Bin width must be non-zero.");
        return false;
      }
      if( binWidth < 0 )
      {
        m_errorMessages.append("Bin width must be a positive value.");
        return false;
      }

      while( range > tolerance )
        range -= binWidth;

      if( std::abs(range) > tolerance )
      {
        m_errorMessages.append("Bin width must allow for even splitting of the range.");
        return false;
      }

      return true;
    }

    /**
     * Checks two values are not equal.
     *
     * @param name Name of value
     * @param x First value
     * @param y Second value (defaults to zero)
     * @param tolerance Tolerance to which to compare
     * @return True if input was valid
     */
    bool UserInputValidator::checkNotEqual(const QString & name, double x, double y, double tolerance)
    {
      double delta = x - y;

      if(std::abs(delta) <= tolerance)
      {
        std::stringstream msg;
        msg << name.toStdString() << " (" << x << ")"
            << " should not be equal to " << y << ".";
        QString msgStr = QString::fromStdString(msg.str());
        m_errorMessages.append(msgStr);
        return false;
      }

      return true;
    }

    /**
     * Add a custom error message to the list.
     *
     * @param message :: the message to add to the list
     */
    void UserInputValidator::addErrorMessage(const QString & message)
    {
      m_errorMessages.append(message);
    }

    /**
     * Generates and returns an error message which contains all the error messages raised by
     * the check functions.
     */
    QString UserInputValidator::generateErrorMessage()
    {
      if( m_errorMessages.isEmpty() )
        return "";

      return "Please correct the following:\n" + m_errorMessages.join("\n");
    }

    /**
     * Checks if the user input checked is valid.
     *
     * @return True if all input is valid, false otherwise
     */
    bool UserInputValidator::isAllInputValid()
    {
      return m_errorMessages.isEmpty();
    }

    /**
     * Sets a validation label that is displyed next to the widget on the UI.
     *
     * @param errorLabel Label to change
     * @param valid If the input was valid
     */
    void UserInputValidator::setErrorLabel(QLabel * errorLabel, bool valid)
    {
      // Do nothing if no error label was provided
      if(errorLabel == NULL)
        return;

      if(!valid)
      {
        // Set the label to be red
        QPalette palette = errorLabel->palette();
        palette.setColor(errorLabel->foregroundRole(), Qt::red);
        errorLabel->setPalette(palette);

        errorLabel->setText("*");
      }
      else
      {
        errorLabel->setText("");
      }

      // Only show the label if input is invalid
      errorLabel->setVisible(!valid);
    }

  }
}
