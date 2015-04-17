for (int c0 = 1, c0_end = min(4, floord(2 * m - 1, 17) + 1); c0 <= c0_end; c0 += 1)
  for (int c1 = 1, c1_end = 2; c1 <= c1_end; c1 += 1)
    for (int c2 = 0, c2_end = min(2, -c0 - c1 + (2 * m + 3 * c0 + 10 * c1 + 6) / 20 + 1); c2 <= c2_end; c2 += 1)
      for (int c3 = 8 * c0 + (c0 + 1) / 2 - 8, c3_end = min(min(30, m - 5 * c1 - 10 * c2 + 5), 8 * c0 + c0 / 2); c3 <= c3_end; c3 += 1)
        for (int c4 = 5 * c1 + 10 * c2 - 4, c4_end = min(5 * c1 + 10 * c2, m - c3 + 1); c4 <= c4_end; c4 += 1)
          s0(c0, c1, c2, c3, c4, -9 * c0 + c3 + c0 / 2 + 9, -5 * c1 - 5 * c2 + c4 + 5);
