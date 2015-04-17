for (int c0 = 0, c0_end = n / 10; c0 <= c0_end; c0 += 1)
  for (int c1 = 10 * c0, c1_end = min(n, 10 * c0 + 9); c1 <= c1_end; c1 += 1)
    S1(c0, c1);
