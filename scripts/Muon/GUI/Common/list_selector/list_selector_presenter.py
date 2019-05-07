
class ListSelectorPresenter(object):
    def __init__(self, view, model):
        self.view = view
        self.model = model
        self.filter_string = ''
        self.filter_type = 'Include'

        self.view.set_filter_line_edit_changed_action(self.handle_filter_changed)
        self.view.set_item_selection_changed_action(self.handle_selection_changed)
        self.view.set_filter_type_combo_changed_action(self.set_filter_type)
        self.view.set_select_all_checkbox_action(self.handle_select_all_checkbox_changed)

    def update_view_from_model(self):
        filtered_list = self.get_filtered_model()

        self.view.clearItems()
        self.view.addItems(filtered_list)

    def handle_filter_changed(self, filter_string):
        self.filter_string = filter_string
        self.update_view_from_model()

    def handle_selection_changed(self, name, state):
        for item in self.model:
            if item[0] == name:
                item[1] = state

    def handle_select_all_checkbox_changed(self, state):
        for item in self.get_filtered_model():
            item[1] = state

        self.update_view_from_model()

    def get_selected_items(self):
        return [item[0] for item in self.model if item[1]]

    def set_filter_type(self):
        self.filter_type = self.view.filter_type_combo_box.currentText()
        self.update_view_from_model()

    def get_filtered_model(self):
        if self.filter_type == 'Include':
            filtered_list = [item for item in self.model if self.filter_string in item[0]]
        else:
            filtered_list = [item for item in self.model if self.filter_string not in item[0]]

        return filtered_list
