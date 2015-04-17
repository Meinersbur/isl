for (int c0 = 3, c0_end = 2 * M; c0 < c0_end; c0 += 1) {
  for (int c1 = c0 / 2 + 2, c1_end = M; c1 <= c1_end; c1 += 1)
    for (int c3 = c0 / 2 + 1, c3_end = min(c0, c1); c3 < c3_end; c3 += 1)
      S1(c3, c0 - c3, c1);
  for (int c1 = max(1, -M + c0), c1_end = (c0 + 1) / 2; c1 < c1_end; c1 += 1)
    S2(c0 - c1, c1);
  for (int c1 = c0 / 2 + 2, c1_end = M; c1 <= c1_end; c1 += 1)
    for (int c3 = c0 / 2 + 1, c3_end = min(c0, c1); c3 < c3_end; c3 += 1)
      S3(c3, c0 - c3, c1);
}
