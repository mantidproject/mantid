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
 *  A mdi sub-window that contains other sub-windows arranged in rows and
 *columns.
 *  The child sub-windows can be selected, moved, deleted, dragged and dropped.
 *
 */
class TiledWindow : public MdiSubWindow {
  Q_OBJECT

public:
  TiledWindow(QWidget *parent, const QString &label,
              const QString &name = QString(), int nrows = 1, int ncols = 1,
              Qt::WFlags f = 0);

  /// Populate a menu with actions
  void populateMenu(QMenu *menu);

  static IProjectSerialisable *loadFromProject(const std::string &lines,
                                               ApplicationWindow *app,
                                               const int fileVersion);

  /// Serialises to a string that can be saved to a project file.
  std::string saveToProject(ApplicationWindow *app) override;
  /// Returns a list of workspace names that are used by this window
  std::vector<std::string> getWorkspaceNames() override;
  /// Returns the user friendly name of the window
  std::string getWindowName() override;

public slots:

  /// Get number of rows
  int rowCount() const;
  /// Get number of columns
  int columnCount() const;
  /// Re-shape the tile layout
  void reshape(int cols);
  /// Add a new widget
  void addWidget(MdiSubWindow *widget, int row, int col);
  /// Insert a new widget
  void insertWidget(MdiSubWindow *widget, int row, int col);
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
  /// Check if there are any selected tiles
  bool hasSelection() const;
  /// Remove the selection and make all windows docked
  void removeSelectionToDocked();
  /// Remove the selection and make all windows floating
  void removeSelectionToFloating();
  /// Remove the selection and put them into separate windows
  void removeSelectionToDefaultWindowType();
  /// Sow a position where tile can be inserted.
  void showInsertPosition(QPoint pos, bool global = true);
  /// Do the widget drop operation
  bool dropAtPosition(MdiSubWindow *w, QPoint pos, bool global = true);

  /// Save
  QString saveToString(const QString &info, bool = false) override;
  /// Restore
  void restore(const QStringList &) override;
  /// Print
  void print() override;

protected:
  void mousePressEvent(QMouseEvent *ev) override;
  void mouseReleaseEvent(QMouseEvent *ev) override;
  void mouseMoveEvent(QMouseEvent *ev) override;
  void dragEnterEvent(QDragEnterEvent *ev) override;
  void dragLeaveEvent(QDragLeaveEvent *) override;
  void dragMoveEvent(QDragMoveEvent *ev) override;
  void dropEvent(QDropEvent *ev) override;

private:
  /// Ways a widget can be removed from this window
  enum RemoveDestination { Default, Docked, Floating };

  /// initialize
  void init(int nrows, int ncols);
  /// Tile empty cells with Tiles
  void tileEmptyCells();
  /// Add a Tile widget at position(row,col) if it doesn't exist.
  Tile *getOrAddTile(int row, int col);
  /// Get a Tile widget at position(row,col).
  Tile *getTile(int row, int col) const;
  /// Check if a Tile at position (row,col) has a widget.
  bool hasWidget(int row, int col) const;
  /// Remove (but don't delete) a widget.
  MdiSubWindow *removeTile(int row, int col);
  /// Remove (but don't delete) a widget.
  MdiSubWindow *removeTile(Tile *tile);
  /// Get a tile at a mouse position (in pixels).
  Tile *getTileAtMousePos(const QPoint &pos);
  /// Get a list of all tiles.
  QList<Tile *> getAllTiles();
  /// Add a tile to the selection.
  void addToSelection(Tile *tile, bool append);
  /// Add a range of tiles to the selection.
  void addRangeToSelection(Tile *tile);
  /// Calculate a linear index of a tile .
  int calcFlatIndex(Tile *tile) const;
  /// Calculate a linear index from row and column numbers.
  int calcFlatIndex(int row, int col) const;
  /// Calculate tile's position in the layout.
  void calcTilePosition(int index, int &row, int &col) const;
  /// Deselect a tile
  bool deselectTile(Tile *tile);
  /// Tell all tiles to not show that they accept widget drops.
  void clearDrops();
  /// Check if a Tile can accept drops
  bool canAcceptDrops(Tile *tile) const;
  /// Remove a widget and make it floating or docked
  void removeWidgetTo(int row, int col, RemoveDestination to);
  /// Remove the selection and make its widgets floating or docked
  void removeSelectionTo(RemoveDestination to);
  /// Make a widget floating or docked
  void sendWidgetTo(MdiSubWindow *w, RemoveDestination to);

private slots:

  /// Remove (but don't delete) a widget.
  void removeWidget(MdiSubWindow *w);

signals:

  /// To be sent from the drop event handler in order to call dropAtPosition()
  /// indirectly via queued connection
  void dropAtPositionQueued(MdiSubWindow *w, QPoint pos, bool global);

private:
  /// The inner widget providing scrolling functionality.
  QScrollArea *m_scrollArea;
  /// The layout arranging the tiles into a grid.
  mutable QGridLayout *m_layout;
  /// Tile selection
  QList<Tile *> m_selection;
  /// mouse cursor position where dragging started
  QPoint m_dragStartPos;
  /// mouse left button is pressed
  bool m_buttonPressed;
};

/**
 * The widget holder. It displays the held MdiSubWindow, helps to implement
 * selection,
 * drag and drop operations.
 */
class Tile : public QFrame {
public:
  /// Constructor
  explicit Tile(QWidget *parent);
  /// Destructor
  ~Tile() override;
  /// Set the widget.
  void setWidget(MdiSubWindow *w);
  /// Remove the held widget without deleting it.
  void removeWidget();
  /// Get the held widget
  MdiSubWindow *widget() { return m_widget; }
  /// Display the tile as selected
  void makeSelected(bool yes);
  /// Display the tile ready to accept a drop of a widget
  void makeAcceptDrop(bool yes);
  /// Check if the tile is selected
  bool isSelected() const { return m_selected; }

protected:
  void paintEvent(QPaintEvent *ev) override;

private:
  /// The TiledWindow which has this Tile
  QWidget *m_tiledWindow;
  /// The layout
  QVBoxLayout *m_layout;
  /// A pointer to the displayed widget
  MdiSubWindow *m_widget;
  /// Selected flag
  bool m_selected;
  /// Accepts drops flag
  bool m_acceptDrop;
};

#endif // TiledWindow_H
