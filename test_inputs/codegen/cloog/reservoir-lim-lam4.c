for (int c0 = 1, c0_end = 2 * M - 1; c0 < c0_end; c0 += 1) {
  for (int c1 = max(-M + 1, -c0 + 1), c1_end = 0; c1 < c1_end; c1 += 1) {
    for (int c3 = max(1, -M + c0 + 1), c3_end = min(M - 1, c0 + c1); c3 <= c3_end; c3 += 1)
      S1(c3, c0 + c1 - c3, -c1);
    for (int c2 = max(-M + c0 + 1, -c1), c2_end = min(M, c0); c2 < c2_end; c2 += 1)
      S2(c0 - c2, c1 + c2, c2);
  }
  for (int c3 = max(1, -M + c0 + 1), c3_end = min(M - 1, c0); c3 <= c3_end; c3 += 1)
    S1(c3, c0 - c3, 0);
}
