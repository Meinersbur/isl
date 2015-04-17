{
  for (int c1 = 0, c1_end = 9; c1 <= c1_end; c1 += 1)
    A(c1);
  for (int c0 = 0, c0_end = 9; c0 <= c0_end; c0 += 1)
    for (int c2 = 0, c2_end = 9; c2 <= c2_end; c2 += 1)
      B(c0, c2);
}
