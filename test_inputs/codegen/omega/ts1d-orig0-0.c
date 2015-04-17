{
  for (int c1 = 0, c1_end = N; c1 < c1_end; c1 += 1)
    s0(1, c1, 1, 0, 0);
  for (int c1 = 0, c1_end = T; c1 < c1_end; c1 += 1) {
    for (int c3 = 0, c3_end = N; c3 < c3_end; c3 += 1)
      s1(2, c1, 0, c3, 1);
    for (int c3 = 1, c3_end = N - 1; c3 < c3_end; c3 += 1)
      s2(2, c1, 1, c3, 1);
  }
}
