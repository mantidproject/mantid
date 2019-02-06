from mantidqt.widgets.inline_find_replace_dialog.view import InlineFindReplaceDialogView


class InlineFindReplaceDialog(object):
    def __init__(self, parent, editor, view=None):
        self.view = view if view else InlineFindReplaceDialogView(parent, self)
        self.editor = editor
        self.visible = False
        self.find_in_progress = False

    def clear_search(self, _):
        self.find_in_progress = False

    def show(self):
        self.view.show()
        self.focus_find_field()
        self.visible = True

    def hide(self):
        self.view.hide()
        self.visible = False
        self.editor.setFocus()

    def toggle(self):
        if not self.visible:
            self.show()
        else:
            self.hide()

    def focus_find_field(self):
        self.view.find.setFocus()

    def action_next(self):
        search = self.view.find.currentText()

        if search == "":
            self.focus_find_field()

        if self.view.find.findText(search) == -1:
            self.view.find.addItem(search)

        if not self.find_in_progress:
            options = self.view.get_options()
            self.find_in_progress = self.editor.findFirst(search,
                                                          options.regex,
                                                          options.match_case,
                                                          options.words,
                                                          options.wrap_around,
                                                          not options.search_backwards)
        else:
            self.find_in_progress = self.editor.findNext()

    def action_replace(self):
        search = self.view.find.currentText()

        if search == "":
            self.focus_find_field()

        # if the editor hasn't been searched before, run a find first, but do not replace anything
        # this allows the user to see where the first match is, instead of having _one_ of the matches
        # being replaced to the replacement string, without knowing which one will be done
        if not self.editor.hasSelectedText() or self.editor.selectedText() != search:
            self.action_next()
            return
        replace = self.view.replace.currentText()
        self.editor.replace(replace)
        self.action_next()
        if self.view.replace.findText(replace) == -1:
            self.view.replace.addItem(replace)

    def action_replace_all(self):
        search = self.view.find.currentText()

        if search == "":
            self.focus_find_field()

        if self.view.find.findText(search) == -1:
            self.view.find.addItem(search)

        replace = self.view.replace.currentText()

        if self.view.replace.findText(replace) == -1:
            self.view.replace.addItem(replace)

        options = self.view.get_options()
        self.editor.replaceAll(search, replace, options.regex,
                               options.match_case,
                               options.words,
                               options.wrap_around,
                               not options.search_backwards)
