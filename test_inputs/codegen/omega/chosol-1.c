{
  for (int c1 = 2, c1_end = n; c1 <= c1_end; c1 += 1)
    s0(c1);
  for (int c1 = 1, c1_end = n; c1 < c1_end; c1 += 1) {
    for (int c3 = c1 + 1, c3_end = n; c3 <= c3_end; c3 += 1)
      s1(c3, c1);
    s2(c1 + 1);
  }
}
