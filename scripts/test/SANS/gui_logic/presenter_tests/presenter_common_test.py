import unittest

from mantid.py3compat import mock
from sans.gui_logic.presenter.presenter_common import PresenterCommon


class PresenterCommonMock(PresenterCommon):
    def default_gui_setup(self):
        pass

    def update_model_from_view(self):
        pass

    def update_view_from_model(self):
        pass


class PresenterCommonTest(unittest.TestCase):

    def test_set_on_model(self):
        mocked_view = mock.Mock()
        # This should not change as it's the presenters copy
        presenter_model = mock.Mock()
        custom_model = mock.Mock()

        attr_to_set = 'test_attr'
        original_val = 'not_changed'
        new_val = 'views val'

        setattr(presenter_model, attr_to_set, original_val)
        setattr(custom_model, attr_to_set, original_val)

        setattr(mocked_view, attr_to_set, new_val)

        mocked_presenter = PresenterCommonMock(view=mocked_view, model=presenter_model)

        self.assertEqual(original_val, getattr(presenter_model, attr_to_set))
        self.assertEqual(original_val, getattr(custom_model, attr_to_set))

        mocked_presenter._set_on_custom_model(attribute_name=attr_to_set, model=custom_model)
        self.assertEqual(new_val, getattr(custom_model, attr_to_set))
        self.assertEqual(original_val, getattr(presenter_model, attr_to_set))


if __name__ == '__main__':
    unittest.main()
