for (int c0 = 0; c0 <= 3; c0 += 1)
  for (int c1 = max(0, 2 * c0 - 3); c1 <= min(c0 + 1, 3); c1 += 1)
    for (int c2 = c0; c2 <= min(min(3, 3 * c1 + 2), 2 * c0 - c1 + 1); c2 += 1)
      for (int c3 = max(max(max(c2 - floord(c2 - 1, 3) - 1, c1 + c2 - (3 * c1 + c2) / 6 - 1), c1 - floord(-c1, 3) - 1), c0 - floord(-c2, 3) - 1); c3 <= min(c0 + c0 / 2 + 1, 3); c3 += 1)
        for (int c5 = max(max(max(max(c1 - floord(-c1, 3) - 1, 0), 2 * c3 - 4), c3 - c3 / 3 - 1), c2 - c2 / 3 - 1); c5 <= min(min(-c2 + 2 * c3 + floord(-c2 - 1, 3) + 2, c1 + 1), c3); c5 += 1)
          for (int c6 = max(max(max(max(max(250 * c3 + 1, 667 * c0 - 333 * c1 - (c0 + c1) / 3 - 333), -200 * c1 + 400 * c3 - 199), 333 * c1 - floord(-c1 - 1, 3) - 1), 1000 * c0 - 500 * c5 - 501), 333 * c2 - floord(-c2 + 1, 3)); c6 <= min(min(min(min(min(min(333 * c3 + floord(c3 - 1, 3) + 334, 1000), 333 * c2 + floord(c2 - 1, 3) + 333), 1000 * c0 - 500 * c5 + 997), 500 * c5 + 501), 500 * c0 + 499), -200 * c1 + 400 * c3 + 400); c6 += 1)
            for (int c7 = max(max(max(max(c6, 500 * c1 + c6 - c6 / 2), 1000 * c0 - c6), 500 * c5 + 2), 1000 * c3 - 2 * c6 + 2); c7 <= min(min(min(min(500 * c5 + 501, 2 * c6 + 1), 1000 * c3 - 2 * c6 + 1001), 1000 * c0 - c6 + 999), 500 * c1 + (c6 + 1) / 2 + 499); c7 += 1)
              s0(c0, c1, c2, c3, c2 / 3, c5, c6, c7);
