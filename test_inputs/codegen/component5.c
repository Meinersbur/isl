for (int c0 = 0, c0_end = 9; c0 <= c0_end; c0 += 1)
  for (int c1 = 0, c1_end = 9; c1 <= c1_end; c1 += 1) {
    if (c0 == 0)
      A(c1);
    B(c0, c1);
  }
