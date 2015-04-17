{
  for (int c0 = 0, c0_end = 14; c0 <= c0_end; c0 += 1)
    for (int c1 = 0, c1_end = n - 14; c1 < c1_end; c1 += 1)
      S1(c0, c1);
  for (int c0 = 15, c0_end = n; c0 <= c0_end; c0 += 1) {
    for (int c1 = 0, c1_end = 9; c1 <= c1_end; c1 += 1)
      S1(c0, c1);
    for (int c1 = 10, c1_end = n - 14; c1 < c1_end; c1 += 1) {
      S1(c0, c1);
      S2(c0, c1);
    }
    for (int c1 = n - 14, c1_end = n; c1 <= c1_end; c1 += 1)
      S2(c0, c1);
  }
}
