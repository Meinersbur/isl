for (int c0 = 3; c0 <= n; c0 += 32)
  for (int c1 = c0; c1 <= min(c0 + 31, n); c1 += 1)
    s0(c0, c1);
