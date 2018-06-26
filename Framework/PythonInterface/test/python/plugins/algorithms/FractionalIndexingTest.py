import unittest
import numpy as np
import numpy.testing as npt
import fractional_indexing as indexing


class FractionIndexingTests(unittest.TestCase):

    def test_find_bases_with_1d_modulation(self):
        qs = np.array([
            [0, 0, .13],
        ])

        _, ndim, bases = indexing.find_bases(qs, tolerance=.02)

        self.assertEqual(ndim, 1)
        npt.assert_array_equal(bases[1], qs[0])

    def test_find_bases_with_2d_modulation(self):
        qs = np.array([
            [0, 0, .1],
            [0, .1, 0],
        ])

        _, ndim, bases = indexing.find_bases(qs, tolerance=.02)

        print bases

        self.assertEqual(ndim, 2)
        npt.assert_array_equal(bases[1], qs[0])
        npt.assert_array_equal(bases[2], qs[1])

    def test_find_bases_with_3d_modulation(self):
        qs = np.array([
            [0, 0, .1],
            [0, .1, 0],
            [.1, 0, 0],
        ])

        _, ndim, bases = indexing.find_bases(qs, tolerance=.02)

        self.assertEqual(ndim, 3)
        npt.assert_array_equal(bases[1], qs[0])
        npt.assert_array_equal(bases[2], qs[1])
        npt.assert_array_equal(bases[3], qs[2])

    def test_find_bases_with_4d_modulation(self):
        qs = np.array([
            [0, 0, .1],
            [0, .1, 0],
            [.1, 0, 0],
            [.15, 0, 0],
        ])

        _, ndim, bases = indexing.find_bases(qs, tolerance=.02)

        self.assertEqual(ndim, 4)
        npt.assert_array_equal(bases[1], qs[0])
        npt.assert_array_equal(bases[2], qs[1])
        npt.assert_array_equal(bases[3], qs[2])

    def test_find_bases_with_2d_modulation_with_linear_combination(self):
        qs = np.array([
            [0, .1, .1],
            [0, 0, .1],
            [0, .1, 0],
            [0, 0, .15],
            [0, .1, .2],
        ])

        _, ndim, bases = indexing.find_bases(qs, tolerance=.02)

        self.assertEqual(ndim, 3)
        npt.assert_array_equal(bases[1], np.array([0, .1, 0]))
        npt.assert_array_equal(bases[2], np.array([0, 0, .1]))
        npt.assert_array_equal(bases[3], np.array([0, 0, .15]))

    def test_generate_grid(self):
        bases = np.array([
            [0, .1, 0],
            [0, 0, .1],
            [0, 0, .14],
        ])
        expected_result = np.array([
            [-0., -0.2, -0.28],
            [-0., -0.2, -0.2],
            [-0., -0.2, -0.14],
            [-0., -0.2, -0.1],
            [0., -0.2,  0.],
            [0., -0.2,  0.1],
            [0., -0.2,  0.14],
            [-0., -0.1, -0.28],
            [-0., -0.1, -0.2],
            [-0., -0.1, -0.14],
            [-0., -0.1, -0.1],
            [0., -0.1,  0.],
            [0., -0.1,  0.1],
            [0., -0.1,  0.14],
            [0.,  0., -0.28],
            [0.,  0., -0.2],
            [0.,  0., -0.14],
            [0.,  0., -0.1],
            [0.,  0.,  0.],
            [0.,  0.,  0.1],
            [0.,  0.,  0.14],
            [0.,  0.1, -0.28],
            [0.,  0.1, -0.2],
            [0.,  0.1, -0.14],
            [0.,  0.1, -0.1],
            [0.,  0.1,  0.],
            [0.,  0.1,  0.1],
            [0.,  0.1,  0.14]
        ])

        result = indexing.generate_grid(bases, 2)
        npt.assert_array_equal(result, expected_result)

    def test_index_with_2d_moduldation_and_noise(self):
        qs = np.array([
            [0, .1, .1],
            [0, -.1, 0],
            [0, 0, .11],
            [0, 0, -.14],
            [0, 0, .17],
            [0, .1, .2],
        ])

        expected_indexing = np.array([
            [1, 1, 0],
            [-1, 0, 0],
            [0, 1, 0],
            [0, 0, 1],
            [0, 0, -1],
            [1, 2, 0]
        ])

        actual_indexing = indexing.index_q_vectors(qs, tolerance=.04)
        npt.assert_array_equal(actual_indexing, expected_indexing)


if __name__ == "__main__":
    unittest.main()
