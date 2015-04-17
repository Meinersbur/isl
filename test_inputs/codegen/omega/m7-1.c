for (int c0 = 1, c0_end = 9; c0 <= c0_end; c0 += 1) {
  if (c0 % 2 == 0) {
    for (int c1 = 1, c1_end = 9; c1 <= c1_end; c1 += 1) {
      s0(c1, c0);
      s1(c1, c0);
    }
  } else
    for (int c1 = 1, c1_end = 9; c1 <= c1_end; c1 += 1)
      s0(c1, c0);
}
