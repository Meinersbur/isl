for (int c0 = 0, c0_end = 2 * n; c0 <= c0_end; c0 += 1)
  for (int c1 = max(0, -n + c0), c1_end = min(n, c0); c1 <= c1_end; c1 += 1)
    S1(c1, c0 - c1);
