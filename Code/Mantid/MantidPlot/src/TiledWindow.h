#ifndef TiledWindow_H
#define TiledWindow_H

#include "MdiSubWindow.h"

#include <QLabel>
#include <QList>

class QGridLayout;
class QVBoxLayout;
class Tile;

/**
 *
 *  A mdi sub-window that contains other sub-windows arranged in rows and columns.
 *  The child sub-windows can be selected, moved, deleted, dragged and dropped.
 *
 */
class TiledWindow: public MdiSubWindow
{
  Q_OBJECT

public:
  TiledWindow(QWidget* parent, const QString& label, const QString& name = QString(), Qt::WFlags f=0);
public slots:

  /// Get number of rows
  int rowCount() const;
  /// Get number of columns
  int columnCount() const;
  /// Re-shape the tile layout
  void reshape(int rows, int cols);
  /// Add a new widget
  void addWidget(MdiSubWindow *widget, int row, int col);
  /// Get a widget at a position
  MdiSubWindow *getWidget(int row, int col);
  /// Remove a widget and make it docked
  void removeWidgetToDocked(int row, int col);
  /// Remove a widget and make it floating
  void removeWidgetToFloating(int row, int col);

  QString saveToString(const QString &info, bool = false);
  void restore(const QStringList&);
  void print();

protected:
  void mousePressEvent(QMouseEvent *ev);

private:
  /// Tile empty cells with Tiles
  void tileEmptyCells();
  /// Get a Tile widget at position(row,col).
  Tile *getTile(int row, int col);
  /// Remove (but don't delete) a widget.
  MdiSubWindow *removeTile(int row, int col);
  /// Get a tile at a mouse position (in pixels).
  Tile *getTileAtMousePos( const QPoint& pos );
  /// Add a tile to the selection.
  void addToSelection(Tile *tile, bool append);
  /// Add a range of tiles to the selection.
  void addRangeToSelection(Tile *tile);
  /// Clear the selection.
  void clearSelection();
  /// Calculate a linear index of a tile .
  int calcFlatIndex(Tile *tile) const;
  /// Calculate tile's position in the layout.
  void calcTilePosition( int index, int &row, int &col ) const;

  /// The layout arranging the tiles into a grid.
  mutable QGridLayout *m_layout;
  /// Tile selection
  QList<Tile*> m_selection;
};

/**
 * A widget-placeholder showing an empty cell where a sub-window can be inserted.
 */
class Tile: public QFrame
{
public:
  Tile(QWidget *parent);
  ~Tile();
  void setWidget(MdiSubWindow *w);
  void removeWidget();
  MdiSubWindow *widget() {return m_widget;}
  void makeSelected();
  void makeNormal();
protected:
  void paintEvent(QPaintEvent *ev);
private:
  QVBoxLayout * m_layout;
  MdiSubWindow* m_widget;
  QColor m_border;
  int m_borderWidth;
};

#endif // TiledWindow_H
