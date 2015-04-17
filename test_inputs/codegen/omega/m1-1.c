for (int c0 = 1, c0_end = 9; c0 <= c0_end; c0 += 1) {
  if (c0 >= 6) {
    for (int c1 = 1, c1_end = 9; c1 <= c1_end; c1 += 1)
      s0(c0, c1);
  } else if (c0 <= 4) {
    for (int c1 = 1, c1_end = 9; c1 <= c1_end; c1 += 1)
      s0(c0, c1);
  } else
    for (int c1 = 1, c1_end = 9; c1 <= c1_end; c1 += 1) {
      s0(5, c1);
      s1(5, c1);
    }
}
