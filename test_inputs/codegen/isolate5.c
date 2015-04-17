{
  for (int c0 = 0, c0_end = 9; c0 <= c0_end; c0 += 1)
    for (int c1 = 0, c1_end = 1; c1 <= c1_end; c1 += 1) {
      if (c0 % 2 == 0) {
        A(c0 / 2, c1);
      } else
        B((c0 - 1) / 2, c1);
    }
  for (int c0 = 10, c0_end = 89; c0 <= c0_end; c0 += 1)
    for (int c1 = 0, c1_end = 1; c1 <= c1_end; c1 += 1) {
      if (c0 % 2 == 0) {
        A(c0 / 2, c1);
      } else
        B((c0 - 1) / 2, c1);
    }
  for (int c0 = 90, c0_end = 199; c0 <= c0_end; c0 += 1)
    for (int c1 = 0, c1_end = 1; c1 <= c1_end; c1 += 1) {
      if (c0 % 2 == 0) {
        A(c0 / 2, c1);
      } else
        B((c0 - 1) / 2, c1);
    }
}
