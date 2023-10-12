import re


class RBNumberValidator(object):
    def __init__(self):
        self.rb_number_pattern_str = r"^(?!.* /|.*/ )(?!.* \\|.*\\ )(?!^ )(?!.* $)[a-zA-Z0-9-_ ]+(?!.*[/\\]{3,})[a-zA-Z0-9-_/\\ ]+$"
        self.rb_number_pattern = re.compile(self.rb_number_pattern_str)

    def validate_rb_number(self, input_rb_number):
        return self.rb_number_pattern.match(input_rb_number)
