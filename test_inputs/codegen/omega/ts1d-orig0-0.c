{
  for (int c1 = 0; c1 <= N - 1; c1 += 1)
    s0(1, c1, 1, 0, 0);
  for (int c1 = 0; c1 <= T - 1; c1 += 1) {
    for (int c3 = 0; c3 <= N - 1; c3 += 1)
      s1(2, c1, 0, c3, 1);
    for (int c3 = 1; c3 <= N - 2; c3 += 1)
      s2(2, c1, 1, c3, 1);
  }
}
