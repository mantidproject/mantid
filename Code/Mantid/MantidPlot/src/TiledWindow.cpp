#include "TiledWindow.h"
#include "ApplicationWindow.h"

#include <QScrollArea>
#include <QGridLayout>

// constants defining the minimum size of tiles
const int minimumTileWidth = 200;
const int minimumTileHeight = 300;

/**
 * Constructor.
 * @param parent :: The parent widget.
 * @param label :: The label.
 * @param name :: The name of the window.
 * @param f :: Window flags.
 */
TiledWindow::TiledWindow(QWidget* parent, const QString& label, const QString& name, Qt::WFlags f)
  : MdiSubWindow(parent, label, name, f)
{
  QScrollArea *scrollArea = new QScrollArea(this);
  scrollArea->setWidgetResizable(true);

  QWidget *innerWidget = new QWidget(scrollArea);
  m_layout = new QGridLayout();
  m_layout->setMargin(2);
  m_layout->setColumnMinimumWidth(0,minimumTileWidth);
  m_layout->setRowMinimumHeight(0,minimumTileHeight);
  innerWidget->setLayout( m_layout );

  scrollArea->setWidget(innerWidget);
  this->setWidget( scrollArea );
  setGeometry(0,0,500,400);
}

QString TiledWindow::saveToString(const QString &info, bool)
{
  QString s= "<tiled_widget>\n";
  s+="</tiled_widget>\n";
  return s;
}

void TiledWindow::restore(const QStringList& data)
{
}

void TiledWindow::print()
{
}

/**
 * Get number of rows
 */
int TiledWindow::rowCount() const
{
  return m_layout->rowCount();
}

/**
 * Get number of columns
 */
int TiledWindow::columnCount() const
{
  return m_layout->columnCount();
}

/**
 * Re-arrange the tiles according to a new layout shape.
 * @param rows :: A new number of rows in the layout.
 * @param cols :: A new number of columns in the layout.
 */
void TiledWindow::reshape(int rows, int cols)
{
}

/**
 * Add a new sub-window at a given position in the layout.
 * The row and column indices do not have to be within the current shape - 
 * it will change accordingly.
 * @param tile :: An MdiSubWindow to add.
 * @param row :: A row index at which to place the new tile.
 * @param col :: A column index at which to place the new tile.
 */
void TiledWindow::addTile(MdiSubWindow *tile, int row, int col)
{
  // detach the tile from ApplicationWindow
  tile->detach();
  m_layout->setColumnMinimumWidth(col,minimumTileWidth);
  m_layout->setRowMinimumHeight(row,minimumTileHeight);
  // attach to this window
  m_layout->addWidget(tile, row, col);
}

/**
 * Get a tile at a position in the layout.
 * @param row :: The row of the tile
 * @param col :: The column of the tile
 * @return :: A pointer to the MdiSubWindow at this position or NULL if the tile is empty.
 */
MdiSubWindow *TiledWindow::getTile(int row, int col) const
{
  QWidget *widget = m_layout->itemAtPosition( row, col )->widget();
  if ( widget )
  {
    MdiSubWindow *tile = dynamic_cast<MdiSubWindow*>( widget );
    if ( !tile )
    {
      throw std::logic_error("Widget of wrong type is found in TiledWindow.");
    }
    return tile;
  }
  return NULL;
}

/**
 * Take a tile at position (row,col), remove it and make docked.
 */
void TiledWindow::removeTileToDocked(int row, int col)
{
  MdiSubWindow *tile = getTile( row, col );
  if ( tile )
  {
    m_layout->removeWidget( tile );
    tile->dock();
  }
}

/**
 * Take a tile at position (row,col), remove it and make floating.
 */
void TiledWindow::removeTileToFloating(int row, int col)
{
  MdiSubWindow *tile = getTile( row, col );
  if ( tile )
  {
    m_layout->removeWidget( tile );
    tile->undock();
  }
}
