for (int c0 = max(0, 32 * h0 - 1991), c0_end = min(999, 32 * h0 + 31); c0 <= c0_end; c0 += 1)
  if ((32 * h0 - c0 + 32) % 64 >= 1)
    for (int c1 = 0, c1_end = 999; c1 <= c1_end; c1 += 1)
      S1(c0, c1);
