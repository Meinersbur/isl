for (int c0 = 1, c0_end = 36; c0 <= c0_end; c0 += 1) {
  if (c0 <= 3) {
    for (int c1 = 1, c1_end = 9; c1 <= c1_end; c1 += 1)
      s1(c1, c0);
  } else if (c0 <= 9) {
    for (int c1 = 1, c1_end = 9; c1 <= c1_end; c1 += 1) {
      if (c0 % 4 == 0)
        s0(c1, c0 / 4);
      s1(c1, c0);
    }
  } else if (c0 % 4 == 0)
    for (int c1 = 1, c1_end = 9; c1 <= c1_end; c1 += 1)
      s0(c1, c0 / 4);
}
