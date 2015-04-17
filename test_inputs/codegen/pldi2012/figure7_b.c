for (int c0 = 1, c0_end = 100; c0 <= c0_end; c0 += 1) {
  if (n >= 2)
    s0(c0);
  for (int c1 = 1, c1_end = 100; c1 <= c1_end; c1 += 1) {
    if (n >= 2)
      s1(c0, c1);
    s2(c0, c1);
  }
}
