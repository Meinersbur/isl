{
  for (int c1 = 0, c1_end = 7; c1 <= c1_end; c1 += 1)
    s0(1, c1);
  for (int c0 = 2, c0_end = 6; c0 <= c0_end; c0 += 1) {
    for (int c1 = 0, c1_end = c0 - 1; c1 < c1_end; c1 += 1)
      s1(c0, c1);
    for (int c1 = c0 - 1, c1_end = 4; c1 <= c1_end; c1 += 1) {
      s1(c0, c1);
      s0(c0, c1);
    }
    for (int c1 = 5, c1_end = 7; c1 <= c1_end; c1 += 1)
      s0(c0, c1);
  }
  for (int c0 = 7, c0_end = 8; c0 <= c0_end; c0 += 1)
    for (int c1 = c0 - 1, c1_end = 7; c1 <= c1_end; c1 += 1)
      s0(c0, c1);
}
