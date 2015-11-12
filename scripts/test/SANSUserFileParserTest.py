import unittest
import mantid
import SANSUserFileParser as UserFileParser

class BackCommandParserTest(unittest.TestCase):
    def test_can_parse_correctly_initial_command(self):
        # Arrange
        correct1 = "MAIN/sdlf/sdf/sdf" # Correct MAIN
        correct2 = "hab /sdlf/sdf /sdf"  # Correct HAB
        correct3 = "FRoNT/sdlf/sdf/sdf" # Correct FRONT
        correct4 = " REAR/sdlf/sdf/sdf" # Correct REAR
        correct5 = "MON/RUN=1234/sdf/sdf" # Correct Mon/RUN=
        parser = UserFileParser.BackCommandParser()

        # Act and assert
        self.assertTrue(parser.can_attempt_to_parse(correct1))
        self.assertTrue(parser.can_attempt_to_parse(correct2))
        self.assertTrue(parser.can_attempt_to_parse(correct3))
        self.assertTrue(parser.can_attempt_to_parse(correct4))
        self.assertTrue(parser.can_attempt_to_parse(correct5))

    def test_cannot_parse_correctly_initial_command(self):
        # Arrange
        correct1 = "MAI/sdlf/sdf/sdf"
        correct2 = "habj/sdlf/sdf/sdf"
        correct3 = "FRoNT=/sdlf/sdf/sdf"
        correct4 = "MON/sdf/sdf/sdf"
        correct5 = "MAIN/sdf" # Correct first but incorrect length

        parser = UserFileParser.BackCommandParser()

        # Act and assert
        self.assertFalse(parser.can_attempt_to_parse(correct1))
        self.assertFalse(parser.can_attempt_to_parse(correct2))
        self.assertFalse(parser.can_attempt_to_parse(correct3))
        self.assertFalse(parser.can_attempt_to_parse(correct4))
        self.assertFalse(parser.can_attempt_to_parse(correct5))

    def do_test_can_parse_correctly(self, arguments, expected_uniform,
                                    expected_mean, expected_run_number,
                                    expected_detector):
        # Arrange
        parser = UserFileParser.BackCommandParser()
        # Act
        reducer = None
        parser.parse_and_set(arguments,reducer)
        # Assert
        self.assertEquals(parser._use_mean, expected_mean)
        self.assertEquals(parser._use_time, expected_uniform)
        self.assertEquals(parser._detector, expected_detector)
        self.assertEquals(parser._run_number, expected_run_number)

    def do_test_parsing_fails(self, arguments, expected_uniform,
                                    expected_mean, expected_run_number,
                                    expected_detector):
        # Arrange
        parser = UserFileParser.BackCommandParser()
        # Act
        reducer = None
        args = [arguments,reducer]
        self.assertRaises(RuntimeError, parser.parse_and_set,*args)

    def test_that_can_parse_MAIN_TIME_MEAN_RUN(self):
        argument = "MAIN/TIME/ mEAN/RuN=SANS2D1111111"
        uniform = True
        mean = True
        run_number ="SANS2D1111111" 
        detector = "MAIN"
        self.do_test_can_parse_correctly(argument, uniform, mean, run_number, detector)

    def test_that_can_parse_REAR_UAMP_TOF_RUN(self):
        argument = "ReaR/Uamp/ToF /Run=2222"
        uniform = False
        mean = False
        run_number ="2222"
        detector = "REAR"
        self.do_test_can_parse_correctly(argument, uniform, mean, run_number, detector)

    def test_that_can_parse_FRONT_TIME_MEAN_RUN(self):
        argument = "FRoNT/TIME/tof/run=LOQ33333333"
        uniform = True
        mean = False
        run_number ="LOQ33333333"
        detector = "FRONT"
        self.do_test_can_parse_correctly(argument, uniform, mean, run_number, detector)

    def test_that_can_parse_REAR_UAMP_TOF_RUN(self):
        argument = "HAB/UAMP/mean/RuN=444444444"
        uniform = False
        mean = True
        run_number ="444444444"
        detector = "HAB"
        self.do_test_can_parse_correctly(argument, uniform, mean, run_number, detector)


    def test_that_can_parse_MON_RUN_TIME_MEAN(self):
        argument = "MON/RUN=123124/time/mean  "
        uniform = True
        mean = True
        run_number ="123124"
        detector = "MON"
        self.do_test_can_parse_correctly(argument, uniform, mean, run_number, detector)

    def test_rejects_bad_first_value(self):
        argument = "GUN/RUN=123124/time/mean  "
        uniform = True
        mean = True
        run_number ="123124"
        detector = "MON"
        self.do_test_parsing_fails(argument, uniform, mean, run_number, detector)

    def test_rejects_bad_second_value(self):
        argument = "HAB/mean/UAMP//RuN=444444444"
        uniform = False
        mean = True
        run_number ="444444444"
        detector = "HAB"
        self.do_test_parsing_fails(argument, uniform, mean, run_number, detector)

    def test_rejects_bad_third_value(self):
        argument = "HAB/UAMP/meanTT/RuN=444444444"
        uniform = False
        mean = True
        run_number ="444444444"
        detector = "HAB"
        self.do_test_parsing_fails(argument, uniform, mean, run_number, detector)

    def test_rejects_bad_fourth_value(self):
        argument = "HAB/UAMP/mean/RuN 44444"
        uniform = False
        mean = True
        run_number ="444444444"
        detector = "HAB"
        self.do_test_parsing_fails(argument, uniform, mean, run_number, detector)


if __name__ == "__main__":
    unittest.main()
