for (int c0 = 0, c0_end = n; c0 <= c0_end; c0 += 32)
  for (int c1 = c0, c1_end = min(n, c0 + 31); c1 <= c1_end; c1 += 1)
    s0(c0, c1);
