{
  for (int c0 = 1, c0_end = n; c0 < c0_end; c0 += 1) {
    for (int c1 = 1, c1_end = c0; c1 < c1_end; c1 += 1)
      for (int c2 = c1 + 1, c2_end = n; c2 <= c2_end; c2 += 1)
        S2(c1, c2, c0);
    for (int c2 = c0 + 1, c2_end = n; c2 <= c2_end; c2 += 1)
      S1(c0, c2);
  }
  for (int c1 = 1, c1_end = n; c1 < c1_end; c1 += 1)
    for (int c2 = c1 + 1, c2_end = n; c2 <= c2_end; c2 += 1)
      S2(c1, c2, n);
}
