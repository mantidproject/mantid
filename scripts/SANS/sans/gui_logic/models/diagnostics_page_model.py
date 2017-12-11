def run_integral(self, file, period, range, mask, integral):
    input_workspace = self.load_workspace(file)
    input_workspace = self.slice_period(period, input_workspace)
    ranges = self.parse_range(range)
    if mask:
        input_workspace = self.applymask(input_workspace)