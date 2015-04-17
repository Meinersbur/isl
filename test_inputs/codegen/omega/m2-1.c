{
  for (int c0 = 2, c0_end = 4; c0 <= c0_end; c0 += 1)
    for (int c1 = 2, c1_end = 9; c1 <= c1_end; c1 += 1)
      s0(c0, c1);
  for (int c0 = 5, c0_end = 9; c0 <= c0_end; c0 += 1) {
    s1(c0, 1);
    for (int c1 = 2, c1_end = 9; c1 <= c1_end; c1 += 1) {
      s1(c0, c1);
      s0(c0, c1);
    }
  }
}
