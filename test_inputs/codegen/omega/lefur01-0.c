for (int c0 = 0; c0 <= 15; c0 += 1)
  for (int c1 = max(c0 - (c0 + 1) / 2, 2 * c0 - 15); c1 <= min(c0 + 1, 15); c1 += 1)
    for (int c2 = max(max(max(67 * c0 - (c0 + 1) / 3, 133 * c0 - 67 * c1 + (c0 + c1 + 1) / 3 - 66), 67 * c1 - floord(c1 - 1, 3) - 1), 1); c2 <= min(min(133 * c0 - 67 * c1 + floord(c0 + c1 - 1, 3) + 133, 100 * c0 + 99), 1000); c2 += 1)
      for (int c3 = max(max(200 * c0 - c2, 100 * c1 + c2 - c2 / 2), c2); c3 <= min(min(100 * c1 + (c2 + 1) / 2 + 99, 2 * c2 + 1), 200 * c0 - c2 + 199); c3 += 1)
        s0(c0, c1, c2, c3);
