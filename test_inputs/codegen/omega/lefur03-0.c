for (int c0 = 0; c0 <= 3; c0 += 1)
  for (int c1 = max(0, 2 * c0 - 3); c1 <= min(c0 + c0 / 2 + 1, 3); c1 += 1)
    for (int c2 = c0; c2 <= min(min(3, 3 * c1 + 2), 2 * c0 - c1 + 1); c2 += 1)
      for (int c3 = max(max(max(c1 - floord(-c1, 3) - 1, 0), c1 + c2 - (3 * c1 + c2) / 6 - 1), 2 * c0 - (c0 + c1 + 1) / 3 - 1); c3 <= min(c0 + 1, 3); c3 += 1)
        for (int c4 = max(max(max(max(-200 * c1 + 400 * c3 - 199, 333 * c1 - floord(-c1 - 1, 3) - 1), 333 * c2 - floord(-c2 + 1, 3)), 667 * c0 - 333 * c1 - (c0 + c1) / 3 - 333), 250 * c3 + 1); c4 <= min(min(min(min(500 * c0 + 499, -200 * c1 + 400 * c3 + 400), 333 * c3 + floord(c3 - 1, 3) + 334), 1000), 333 * c2 + floord(c2 - 1, 3) + 333); c4 += 1)
          for (int c5 = max(max(max(1000 * c3 - 2 * c4 + 2, 1000 * c0 - c4), 500 * c1 + c4 - c4 / 2), c4); c5 <= min(min(min(1000 * c3 - 2 * c4 + 1001, 1000 * c0 - c4 + 999), 500 * c1 + (c4 + 1) / 2 + 499), 2 * c4 + 1); c5 += 1)
            s0(c0, c1, c2, c3, c4, c5);
