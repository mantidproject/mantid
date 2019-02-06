from mantidqt.widgets.inline_find_replace_dialog.view import InlineFindReplaceDialogView


class InlineFindReplaceDialog(object):
    def __init__(self, parent, view=None):
        self.view = view if view else InlineFindReplaceDialogView(parent, self)
        self.visible = False

    def show(self):
        self.view.show()
        self.visible = True

    def hide(self):
        self.view.hide()
        self.visible = False

    def toggle(self):
        if self.visible:
            self.view.hide()
        else:
            self.view.show()
            self.view.find.setFocus()

    def action_next(self):
        print("In action_next")

    def action_previous(self):
        print("In action_previous")

    def action_replace(self):
        print("In action_replace")

    def action_replace_all(self):
        print("In action_replace_all")
