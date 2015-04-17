for (int c0 = 1, c0_end = n; c0 <= c0_end; c0 += 1) {
  for (int c1 = 2, c1_end = n; c1 <= c1_end; c1 += 1)
    for (int c2 = 1, c2_end = min(c0, c1); c2 < c2_end; c2 += 1)
      S2(c2, c1, c0);
  for (int c3 = c0 + 1, c3_end = n; c3 <= c3_end; c3 += 1)
    S1(c0, c3);
}
