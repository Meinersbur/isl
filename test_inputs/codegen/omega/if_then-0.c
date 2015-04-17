if (m <= 1) {
  for (int c0 = 1, c0_end = n; c0 <= c0_end; c0 += 1)
    for (int c1 = 1, c1_end = n; c1 <= c1_end; c1 += 1)
      s2(c0, c1);
} else if (n >= m + 1) {
  for (int c0 = 1, c0_end = n; c0 <= c0_end; c0 += 1)
    for (int c1 = 1, c1_end = n; c1 <= c1_end; c1 += 1)
      s0(c0, c1);
} else
  for (int c0 = 1, c0_end = n; c0 <= c0_end; c0 += 1)
    for (int c1 = 1, c1_end = n; c1 <= c1_end; c1 += 1)
      s1(c0, c1);
