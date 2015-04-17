{
  for (int c0 = 1, c0_end = 6; c0 <= c0_end; c0 += 2) {
    for (int c2 = 1, c2_end = c0; c2 <= c2_end; c2 += 1) {
      S1(c0, (c0 - 1) / 2, c2);
      S2(c0, (c0 - 1) / 2, c2);
    }
    for (int c2 = c0 + 1, c2_end = p; c2 <= c2_end; c2 += 1)
      S1(c0, (c0 - 1) / 2, c2);
  }
  for (int c0 = 7, c0_end = m; c0 <= c0_end; c0 += 2)
    for (int c2 = 1, c2_end = p; c2 <= c2_end; c2 += 1)
      S1(c0, (c0 - 1) / 2, c2);
}
