#ifndef TiledWindow_H
#define TiledWindow_H

#include "MdiSubWindow.h"

#include <QLabel>
#include <QList>

class QGridLayout;
class QVBoxLayout;
class QScrollArea;
class QMenu;
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

  /// Populate a menu with actions 
  void populateMenu(QMenu *menu);

public slots:

  /// Get number of rows
  int rowCount() const;
  /// Get number of columns
  int columnCount() const;
  /// Re-shape the tile layout
  void reshape(int cols);
  /// Add a new widget
  void addWidget(MdiSubWindow *widget, int row, int col);
  /// Get a widget at a position
  MdiSubWindow *getWidget(int row, int col);
  /// Remove a widget and make it docked
  void removeWidgetToDocked(int row, int col);
  /// Remove a widget and make it floating
  void removeWidgetToFloating(int row, int col);
  /// Remove all widgets.
  void clear();

  /// Select a widget
  void selectWidget(int row, int col);
  /// Deselect a widget
  void deselectWidget(int row, int col);
  /// Check if a widget is selected
  bool isSelected(int row, int col);
  /// Select a range of Widgets.
  void selectRange(int row1, int col1, int row2, int col2);
  /// Clear the selection.
  void clearSelection();
  /// Remove the selection and make all windows docked
  void removeSelectionToDocked();
  /// Remove the selection and make all windows floating
  void removeSelectionToFloating();

  void showInsertPosition( QPoint pos, bool global = true );
  bool dropAtPosition( MdiSubWindow *w, QPoint pos, bool global = true );

  QString saveToString(const QString &info, bool = false);
  void restore(const QStringList&);
  void print();

protected:
  void mousePressEvent(QMouseEvent *ev);
  void mouseReleaseEvent(QMouseEvent *ev);
  void dragEnterEvent(QDragEnterEvent* ev);
  void dragMoveEvent(QDragMoveEvent* ev);
  void dropEvent(QDropEvent* ev);

private:
  void init();
  /// Tile empty cells with Tiles
  void tileEmptyCells();
  /// Add a Tile widget at position(row,col) if it doesn't exist.
  Tile *getOrAddTile(int row, int col);
  /// Get a Tile widget at position(row,col).
  Tile *getTile(int row, int col);
  /// Remove (but don't delete) a widget.
  MdiSubWindow *removeTile(int row, int col);
  /// Remove (but don't delete) a widget.
  MdiSubWindow *removeTile(Tile *tile);
  /// Get a tile at a mouse position (in pixels).
  Tile *getTileAtMousePos( const QPoint& pos );
  /// Get a list of all tiles.
  QList<Tile*> getAllTiles();
  /// Add a tile to the selection.
  void addToSelection(Tile *tile, bool append);
  /// Add a range of tiles to the selection.
  void addRangeToSelection(Tile *tile);
  /// Calculate a linear index of a tile .
  int calcFlatIndex(Tile *tile) const;
  /// Calculate tile's position in the layout.
  void calcTilePosition( int index, int &row, int &col ) const;
  /// Deselect a tile
  bool deselectTile(Tile *tile);
  /// Tell all tiles to not show that they accept widget drops.
  void clearDrops();
  /// Check if a Tile can accept drops
  bool canAcceptDrops(Tile *tile) const;

  /// The inner widget providing scrolling functionality.
  QScrollArea *m_scrollArea;
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
  void makeSelected(bool yes);
  void makeAcceptDrop(bool yes);
protected:
  void paintEvent(QPaintEvent *ev);
private:
  /// The TiledWindow which has this Tile
  QWidget *m_tiledWindow;
  /// The layout
  QVBoxLayout * m_layout;
  /// A pointer to the displayed widget
  MdiSubWindow* m_widget;
  /// Selected flag
  bool m_selected;
  /// Accepts drops flag
  bool m_acceptDrop;

};

#endif // TiledWindow_H
