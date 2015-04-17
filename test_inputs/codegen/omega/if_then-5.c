for (int c0 = 4, c0_end = 100; c0 <= c0_end; c0 += 4) {
  for (int c1 = 1, c1_end = 100; c1 <= c1_end; c1 += 1)
    s0(c0, c1);
  if (c0 >= 8 && c0 <= 96)
    for (int c1 = 10, c1_end = 100; c1 <= c1_end; c1 += 1)
      s1(c0 + 2, c1);
}
