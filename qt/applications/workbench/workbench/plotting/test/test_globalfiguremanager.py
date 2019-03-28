import unittest

from mantid.py3compat import mock
from mock import call, patch
from workbench.plotting.globalfiguremanager import FigureAction, GlobalFigureManager, GlobalFigureManagerObserver
from workbench.plotting.observabledictionary import DictionaryAction


class MockGlobalFigureManager:
    def __init__(self):
        self.notify_observers = mock.Mock()
        self.mock_figs = {}
        self.figs = mock.Mock()
        self.figs.keys = mock.Mock(return_value=self.mock_figs)


class MockCanvas:
    def __init__(self):
        self.mpl_disconnect = mock.Mock()
        self.figure = None
        self.draw_idle = mock.Mock()


class MockFigureManager:
    def __init__(self, num):
        self.num = num
        self.canvas = MockCanvas()
        self._cidgcf = -1231231
        self.destroy = mock.Mock()


class TestGlobalFigureManagerObserver(unittest.TestCase):
    def test_notify_create(self):
        mock_figure_manager = MockGlobalFigureManager()
        observer = GlobalFigureManagerObserver(mock_figure_manager)
        key = 12313
        observer.notify(DictionaryAction.Create, key)

        mock_figure_manager.notify_observers.assert_called_once_with(FigureAction.New, key)

    def test_notify_set(self):
        mock_figure_manager = MockGlobalFigureManager()
        observer = GlobalFigureManagerObserver(mock_figure_manager)
        key = 12313
        observer.notify(DictionaryAction.Set, key)

        mock_figure_manager.notify_observers.assert_called_once_with(FigureAction.Renamed, key)

    def test_notify_removed(self):
        mock_figure_manager = MockGlobalFigureManager()
        observer = GlobalFigureManagerObserver(mock_figure_manager)
        key = 12313
        observer.notify(DictionaryAction.Removed, key)

        mock_figure_manager.notify_observers.assert_called_once_with(FigureAction.Closed, key)

    def test_notify_update(self):
        mock_figure_manager = MockGlobalFigureManager()
        observer = GlobalFigureManagerObserver(mock_figure_manager)
        key = 12313
        observer.notify(DictionaryAction.Update, key)

        mock_figure_manager.notify_observers.assert_called_once_with(FigureAction.Update, key)

    def test_notify_clear(self):
        mock_figure_manager = MockGlobalFigureManager()
        observer = GlobalFigureManagerObserver(mock_figure_manager)
        key = 12313
        observer.notify(DictionaryAction.Clear, key)

        mock_figure_manager.figs.keys.assert_called_once_with()

    def test_raises_on_invalid_action(self):
        mock_figure_manager = MockGlobalFigureManager()
        observer = GlobalFigureManagerObserver(mock_figure_manager)
        key = 12313
        self.assertRaises(ValueError, observer.notify, 941949, key)


class TestGlobalFigureManagerInitialisation(unittest.TestCase):
    def test_correctly_initialises_on_import(self):
        # A figure observer should have been added with the import of the class
        self.assertEqual(1, len(GlobalFigureManager.figs.observers))


class TestGlobalFigureManager(unittest.TestCase):
    def setUp(self):
        # reset the figure manager before each test
        GlobalFigureManager.destroy_all()

    def add_manager(self, num=0):
        mock_manager = MockFigureManager(num)
        mock_fig = mock.Mock()
        mock_manager.canvas.figure = mock_fig
        GlobalFigureManager.set_active(mock_manager)
        return mock_manager, mock_fig

    def test_get_fig_manager(self):
        mock_manager = MockFigureManager(0)
        GlobalFigureManager.set_active(mock_manager)
        manager = GlobalFigureManager.get_fig_manager(0)
        self.assertIsNotNone(manager)
        self.assertEqual(mock_manager, manager)

    def test_set_active(self):
        self.assertEqual(0, len(GlobalFigureManager._activeQue))
        mock_manager = MockFigureManager(0)
        GlobalFigureManager.set_active(mock_manager)
        self.assertEqual(1, len(GlobalFigureManager._activeQue))

    def test_get_active(self):
        mock_manager = MockFigureManager(0)
        GlobalFigureManager.set_active(mock_manager)
        self.assertEqual(mock_manager, GlobalFigureManager.get_active())

    def test_has_fignum(self):
        mock_manager = MockFigureManager(0)
        GlobalFigureManager.set_active(mock_manager)
        self.assertTrue(GlobalFigureManager.has_fignum(0))
        num = 3131313
        mock_manager = MockFigureManager(num)
        GlobalFigureManager.set_active(mock_manager)
        self.assertTrue(GlobalFigureManager.has_fignum(num))

    def test_destroy_doesnt_have_fig(self):
        with patch.object(GlobalFigureManager, 'has_fignum', return_value=False) as mock_has_fignum:
            num = 123123
            GlobalFigureManager.destroy(num)
            mock_has_fignum.assert_called_once_with(num)

    @patch('gc.collect')
    def test_destroy(self, mock_gc_collect):
        num = 0
        mock_manager = MockFigureManager(num)
        GlobalFigureManager.set_active(mock_manager)
        self.assertEqual(1, len(GlobalFigureManager._activeQue))
        self.assertEqual(1, len(GlobalFigureManager.figs))
        with patch.object(GlobalFigureManager, 'notify_observers') as mock_notify_observers:
            GlobalFigureManager.destroy(num)
            self.assertEqual(0, len(GlobalFigureManager._activeQue))
            self.assertEqual(0, len(GlobalFigureManager.figs))
            mock_gc_collect.assert_called_once_with(1)
            mock_notify_observers.assert_has_calls(
                [call(FigureAction.Closed, num), call(FigureAction.OrderChanged, -1)])

    def test_destroy_fig(self):
        num = 0
        mock_manager, mock_fig = self.add_manager(num)
        self.assertEqual(1, len(GlobalFigureManager._activeQue))
        self.assertEqual(1, len(GlobalFigureManager.figs))

        GlobalFigureManager.destroy_fig(mock_fig)
        self.assertEqual(0, len(GlobalFigureManager._activeQue))
        self.assertEqual(0, len(GlobalFigureManager.figs))

    def test_do_not_destroy_wrong_fig(self):
        num = 0
        self.add_manager(num)
        self.assertEqual(1, len(GlobalFigureManager._activeQue))
        self.assertEqual(1, len(GlobalFigureManager.figs))

        _, other_mock_fig = self.add_manager(num + 1)
        GlobalFigureManager.destroy_fig(other_mock_fig)
        # nothing should have been destroyed
        self.assertEqual(1, len(GlobalFigureManager._activeQue))
        self.assertEqual(1, len(GlobalFigureManager.figs))

    def test_destroy_all(self):
        num = 0
        self.add_manager(num)

        other_mock_fig = mock.Mock()
        other_mock_manager = MockFigureManager(num + 1)
        other_mock_manager.canvas.figure = other_mock_fig
        GlobalFigureManager.set_active(other_mock_manager)

        self.assertEqual(2, len(GlobalFigureManager._activeQue))
        self.assertEqual(2, len(GlobalFigureManager.figs))

        GlobalFigureManager.destroy_all()
        self.assertEqual(0, len(GlobalFigureManager._activeQue))
        self.assertEqual(0, len(GlobalFigureManager.figs))

    def test_get_all_fig_managers(self):
        num = 0
        mock_manager, _ = self.add_manager(num)
        other_mock_manager, _ = self.add_manager(num + 1)

        self.assertEqual(2, len(GlobalFigureManager._activeQue))
        self.assertEqual(2, len(GlobalFigureManager.figs))

        all_figs = GlobalFigureManager.get_all_fig_managers()

        self.assertEqual(all_figs[0], mock_manager)
        self.assertEqual(all_figs[1], other_mock_manager)

    def test_get_num_fig_managers(self):
        num = 0
        self.add_manager(num)
        self.add_manager(num + 1)

        self.assertEqual(2, len(GlobalFigureManager._activeQue))
        self.assertEqual(2, GlobalFigureManager.get_num_fig_managers())

    def setup_draw_all(self, stale):
        num = 0
        mock_managers = []
        # [0] takes the manager from the [manager, fig] tuple
        mock_managers.append(self.add_manager(num)[0])
        mock_managers.append(self.add_manager(num + 1)[0])
        mock_managers.append(self.add_manager(num + 2)[0])
        mock_managers.append(self.add_manager(num + 3)[0])
        for manager in mock_managers:
            manager.canvas.figure.stale = stale
        return mock_managers

    def test_draw_all_do_not_draw_non_stale(self):
        mock_managers = self.setup_draw_all(stale=False)

        GlobalFigureManager.draw_all()
        for manager in mock_managers:
            self.assertNotCalled(manager.canvas.draw_idle)

    def test_draw_all_draw_stale(self):
        mock_managers = self.setup_draw_all(stale=True)

        GlobalFigureManager.draw_all()
        for manager in mock_managers:
            manager.canvas.draw_idle.assert_called_once_with()

    def test_draw_all_some_stale(self):
        mock_managers = self.setup_draw_all(stale=True)
        stale_id = 1

        mock_managers[stale_id].canvas.figure.stale = False
        GlobalFigureManager.draw_all()
        for i in range(len(mock_managers)):
            manager = mock_managers[i]
            if i != stale_id:
                manager.canvas.draw_idle.assert_called_once_with()
            else:
                self.assertNotCalled(manager.canvas.draw_idle)

    def test_draw_all_force(self):
        mock_managers = self.setup_draw_all(stale=False)

        GlobalFigureManager.draw_all(force=True)
        for manager in mock_managers:
            manager.canvas.draw_idle.assert_called_once_with()

    def test_last_active_values(self):
        num = 0
        self.add_manager(num)
        self.add_manager(num + 1)
        self.add_manager(num + 2)
        self.add_manager(num + 3)

        last_active_values = GlobalFigureManager.last_active_values()
        self.assertEqual({0: 4, 1: 3, 2: 2, 3: 1}, last_active_values)

    def test_add_observer(self):
        good_observer = mock.Mock()
        good_observer.notify = mock.Mock()
        self.assertTrue(1, len(GlobalFigureManager.observers))

    def test_fail_adding_bad_observer(self):
        """
        Don't add an observer that does not have a notify function
        """
        bad_observer = {}
        self.assertRaises(AssertionError, GlobalFigureManager.add_observer, bad_observer)

    def test_notify_observers(self):
        GlobalFigureManager.observers = []
        num = 10
        mock_observers = []
        for i in range(num):
            good_observer = mock.Mock()
            good_observer.notify = mock.Mock()
            GlobalFigureManager.add_observer(good_observer)
            mock_observers.append(good_observer)

        mock_figure_number = 312312
        mock_args = [FigureAction.Update, mock_figure_number]
        GlobalFigureManager.notify_observers(*mock_args)

        for obs in mock_observers:
            obs.notify.assert_called_once_with(*mock_args)

    def test_figure_title_changed(self):
        mock_figure_number = 312312
        with patch.object(GlobalFigureManager, 'notify_observers', return_value=False) as mock_notify_observers:
            GlobalFigureManager.figure_title_changed(mock_figure_number)
            mock_notify_observers.assert_called_once_with(FigureAction.Renamed, mock_figure_number)

    def test_figure_visibility_changed(self):
        mock_figure_number = 99994
        with patch.object(GlobalFigureManager, 'notify_observers', return_value=False) as mock_notify_observers:
            GlobalFigureManager.figure_visibility_changed(mock_figure_number)
            mock_notify_observers.assert_called_once_with(FigureAction.VisibilityChanged, mock_figure_number)

    def assertNotCalled(self, mock):
        self.assertEqual(0, mock.call_count)


if __name__ == '__main__':
    unittest.main()
