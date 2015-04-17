for (int c0 = 1, c0_end = n; c0 <= c0_end; c0 += 1)
  for (int c1 = 2, c1_end = n; c1 <= c1_end; c1 += 1) {
    for (int c3 = 1, c3_end = min(c0, c1); c3 < c3_end; c3 += 1)
      s1(c3, c1, c0);
    if (c1 >= c0 + 1)
      s0(c0, c1);
  }
