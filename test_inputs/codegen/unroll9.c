for (int c0 = 0, c0_end = 99; c0 <= c0_end; c0 += 1)
  for (int c1 = 0, c1_end = 99; c1 <= c1_end; c1 += 1) {
    A(c1, 0, c0);
    A(c1, 1, c0);
    B(c1, 0, c0);
    B(c1, 1, c0);
  }
