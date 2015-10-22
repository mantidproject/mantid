#include "MantidQtAPI/FilePropertyWidget.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Property.h"
#include "MantidQtAPI/AlgorithmInputHistory.h"
#include "MantidQtAPI/FileDialogHandler.h"

using namespace Mantid::Kernel;
//using namespace Mantid::API;

namespace MantidQt
{
namespace API
{


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  FilePropertyWidget::FilePropertyWidget(Mantid::Kernel::Property * prop, QWidget * parent, QGridLayout * layout, int row)
  : TextPropertyWidget(prop, parent, layout, row)
  {
    m_fileProp = dynamic_cast<Mantid::API::FileProperty*>(prop);
    m_multipleFileProp = dynamic_cast<Mantid::API::MultipleFileProperty*>(prop);

    // Create a browse button
    m_browseButton = new QPushButton(tr("Browse"), m_parent);
    connect(m_browseButton, SIGNAL(clicked()), this, SLOT(browseClicked()));
    m_widgets.push_back(m_browseButton);

    // Add to the 2nd column
    m_gridLayout->addWidget(m_browseButton, m_row, 2, 0);

  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  FilePropertyWidget::~FilePropertyWidget()
  {
  }
  

  //----------------------------------------------------------------------------------------------
  /** Slot called when the browse button is clicked */
  void FilePropertyWidget::browseClicked()
  {
    // Open dialog to get the filename
    QString filename;
    if (m_fileProp)
    {
      filename = openFileDialog(m_prop);
    }
    else if (m_multipleFileProp)
    {
      // Current filename text
      filename = m_textbox->text();

      // Adjust the starting directory from the current file
      if( !filename.isEmpty() )
      {
        QStringList files = filename.split(",");
        if (files.size() > 0)
        {
          QString firstFile = files[0];
          AlgorithmInputHistory::Instance().setPreviousDirectory(QFileInfo(firstFile).absoluteDir().path());
        }
      }

      // Open multiple files in the dialog
      QStringList files = FilePropertyWidget::openMultipleFileDialog( m_prop );

      // Make into comma-sep string
      filename.clear();
      QStringList list = files;
      QStringList::Iterator it = list.begin();
      while(it != list.end())
      {
        if (it != list.begin()) filename += ",";
        filename += *it;
        it++;
      }
    }

    // TODO: set the value.
    if( !filename.isEmpty() )
    {
      m_textbox->clear();
      m_textbox->setText(filename);
      userEditedProperty();
    }
  }


  //-------------------------------------------------------------------------------------------------
  /** For file dialogs
   *
   * @param exts :: vector of extensions
   * @param defaultExt :: default extension to use
   * @return a string that filters files by extenstions
   */
  QString getFileDialogFilter(const std::vector<std::string>& exts, const std::string& defaultExt)
  {
    QString filter("");

    if( !defaultExt.empty() )
    {
      filter.append(QString::fromStdString(defaultExt) + " (*" + QString::fromStdString(defaultExt) + ");;");
    }

    if( !exts.empty() )
    {
      // --------- Load a File -------------
      auto iend = exts.end();
      // Push a wild-card onto the front of each file suffix
      for( auto itr = exts.begin(); itr != iend; ++itr)
      {
        if( (*itr) != defaultExt )
        {
          filter.append(QString::fromStdString(*itr) + " (*" + QString::fromStdString(*itr) + ");;");
        }
      }
      filter = filter.trimmed();
    }
    filter.append("All Files (*.*)");
    return filter;
  }

  //----------------------------------------------------------------------------------------------
  /** Open the file dialog for a given property
   *
   * @param baseProp :: Property pointer
   * @return full path to the file(s) to load/save
   */
  QString FilePropertyWidget::openFileDialog(Mantid::Kernel::Property * baseProp)
  {
    Mantid::API::FileProperty* prop =
      dynamic_cast< Mantid::API::FileProperty* >( baseProp );
    if( !prop ) return "";

    //The allowed values in this context are file extensions
    std::vector<std::string> exts = prop->allowedValues();
    std::string defaultExt = prop->getDefaultExt();

    /* MG 20/07/09: Static functions such as these that use native Windows and MAC dialogs
       in those environments are alot faster. This is unforunately at the expense of
       shell-like pattern matching, i.e. [0-9].
    */
    QString filename;
    if( prop->isLoadProperty() )
    {
      QString filter = getFileDialogFilter(exts, defaultExt);
      filename = QFileDialog::getOpenFileName(NULL, "Open file", AlgorithmInputHistory::Instance().getPreviousDirectory(), filter);
    }
    else if ( prop->isSaveProperty() )
    {
      // --------- Save a File -------------
      //Have each filter on a separate line with the default as the first
      QString filter;
      if( !defaultExt.empty() )
      {
        filter = "*" + QString::fromStdString(defaultExt) + ";;";
      }
      auto iend = exts.end();
      for( auto itr = exts.begin(); itr != iend; ++itr)
      {
        if( (*itr) != defaultExt )
        {
          filter.append("*"+QString::fromStdString(*itr) + ";;");
        }
      }
      //Remove last two semi-colons or else we get an extra empty option in the box
      filter.chop(2);
      // Prepend the default filter
      QString selectedFilter;
      filename = MantidQt::API::FileDialogHandler::getSaveFileName(NULL, "Save file", AlgorithmInputHistory::Instance().getPreviousDirectory(), filter, &selectedFilter);

      //Check the filename and append the selected filter if necessary
      if( QFileInfo(filename).completeSuffix().isEmpty() )
      {
        // Hack off the first star that the filter returns
        QString ext = selectedFilter;

        if( selectedFilter.startsWith("*.") )
        {
          // 1 character from the start
          ext = ext.remove(0,1);
        }
        else
        {
          ext = "";
        }

        if( filename.endsWith(".") && ext.startsWith(".") )
        {
          ext = ext.remove(0,1);
        }

        // Construct the full file name
        filename += ext;
      }
    }
    else if ( prop->isDirectoryProperty() )
    {
      filename = QFileDialog::getExistingDirectory(NULL, "Choose a Directory", AlgorithmInputHistory::Instance().getPreviousDirectory() );
    }
    else
    {
      throw std::runtime_error("Invalid type of file property! This should not happen.");
    }

    if( !filename.isEmpty() )
    {
      AlgorithmInputHistory::Instance().setPreviousDirectory(QFileInfo(filename).absoluteDir().path());
    }
    return filename;
  }



  //-------------------------------------------------------------------------------------------------
  /** Open a file selection box to select Multiple files to load.
   *
   * @param baseProp:: pointer to an instance of MultipleFileProperty used to set up the valid extensions for opening multiple file dialog. 
   * @return list of full paths to files
   */
  QStringList FilePropertyWidget::openMultipleFileDialog(Mantid::Kernel::Property * baseProp)
  {
    if( !baseProp ) return QStringList();
    Mantid::API::MultipleFileProperty* prop =
      dynamic_cast< Mantid::API::MultipleFileProperty* >( baseProp );
    if( !prop ) return QStringList();

    QString filter = getFileDialogFilter(prop->getExts(), prop->getDefaultExt());
    QStringList files = QFileDialog::getOpenFileNames(NULL, "Open Multiple Files", AlgorithmInputHistory::Instance().getPreviousDirectory(), filter);

    return files;
  }

} // namespace MantidQt
} // namespace API
