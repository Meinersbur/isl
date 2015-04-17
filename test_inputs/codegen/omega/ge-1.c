for (int c0 = 2, c0_end = n; c0 <= c0_end; c0 += 1)
  for (int c1 = 1, c1_end = n; c1 <= c1_end; c1 += 1) {
    for (int c3 = 1, c3_end = min(c0, c1); c3 < c3_end; c3 += 1)
      s1(c3, c0, c1);
    if (c0 >= c1 + 1)
      s0(c1, c0);
  }
