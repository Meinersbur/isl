{
  for (int c1 = 0, c1_end = m; c1 <= c1_end; c1 += 1)
    S1(1, c1);
  for (int c0 = 2, c0_end = n; c0 <= c0_end; c0 += 1) {
    for (int c1 = 0, c1_end = c0 - 1; c1 < c1_end; c1 += 1)
      S2(c0, c1);
    for (int c1 = c0 - 1, c1_end = n; c1 <= c1_end; c1 += 1) {
      S1(c0, c1);
      S2(c0, c1);
    }
    for (int c1 = n + 1, c1_end = m; c1 <= c1_end; c1 += 1)
      S1(c0, c1);
  }
  for (int c0 = n + 1, c0_end = m + 1; c0 <= c0_end; c0 += 1)
    for (int c1 = c0 - 1, c1_end = m; c1 <= c1_end; c1 += 1)
      S1(c0, c1);
}
