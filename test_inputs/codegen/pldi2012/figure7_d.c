if (n >= 2) {
  for (int c0 = 1, c0_end = 100; c0 <= c0_end; c0 += 1) {
    s0(c0);
    for (int c1 = 1, c1_end = 100; c1 <= c1_end; c1 += 1) {
      s1(c0, c1);
      s2(c0, c1);
    }
  }
} else
  for (int c0 = 1, c0_end = 100; c0 <= c0_end; c0 += 1)
    for (int c1 = 1, c1_end = 100; c1 <= c1_end; c1 += 1)
      s2(c0, c1);
