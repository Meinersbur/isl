{
  for (int c0 = 1, c0_end = 4; c0 <= c0_end; c0 += 1)
    for (int c1 = 5, c1_end = M - 9; c1 < c1_end; c1 += 1)
      S1(c0, c1);
  for (int c0 = 5, c0_end = M - 9; c0 < c0_end; c0 += 1) {
    for (int c1 = -c0 + 1, c1_end = 4; c1 <= c1_end; c1 += 1)
      S2(c0 + c1, c0);
    for (int c1 = 5, c1_end = min(M - 10, M - c0); c1 <= c1_end; c1 += 1) {
      S1(c0, c1);
      S2(c0 + c1, c0);
    }
    for (int c1 = M - c0 + 1, c1_end = M - 9; c1 < c1_end; c1 += 1)
      S1(c0, c1);
    for (int c1 = M - 9, c1_end = M - c0; c1 <= c1_end; c1 += 1)
      S2(c0 + c1, c0);
  }
  for (int c0 = M - 9, c0_end = M; c0 <= c0_end; c0 += 1)
    for (int c1 = 5, c1_end = M - 9; c1 < c1_end; c1 += 1)
      S1(c0, c1);
}
