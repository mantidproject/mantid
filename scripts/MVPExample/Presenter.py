class Presenter(object):
    # Pass the view and model into the presenter
    def __init__(self, demo_view, demo_model):
        self.model = demo_model
        self.view = demo_view

        # Define the initial view
        # Note that, in the view, the drop-down could be replaced with a set of
        # tick boxes and this line would remain unchanged - an advantage of
        # decoupling the presenter and view
        self.view.set_options('operations', ['+', '-'])
        self.view.set_options('display', ['print', 'update', 'print and update'])
        self.printToScreen = True
        self.view.hide_display()

        # Connect to the view's custom signals
        self.view.btnSignal.connect(self.handle_button)
        self.view.displaySignal.connect(self.display_update)

    # The final two methods handle the signals
    def display_update(self):
        display = self.view.get_display()
        if display == 'update':
            self.printToScreen = False
            self.view.show_display()
        elif display == 'print':
            self.printToScreen = True
            self.view.hide_display()
        else:
            self.printToScreen = True
            self.view.show_display()

    def handle_button(self):
        # Get the values from view
        value1 = self.view.get_value(0)
        operation = self.view.get_operation()
        value2 = self.view.get_value(2)
        # The model does the hard work for us
        result = self.model.calc(value1, operation, value2)

        if self.printToScreen:
            print result
        self.view.setResult(result)
