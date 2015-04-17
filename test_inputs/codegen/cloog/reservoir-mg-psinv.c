for (int c0 = 2, c0_end = O; c0 < c0_end; c0 += 1)
  for (int c1 = 3, c1_end = 2 * N - 2; c1 < c1_end; c1 += 2) {
    for (int c3 = 1, c3_end = M; c3 <= c3_end; c3 += 1) {
      S1(c0, (c1 + 1) / 2, c3);
      S2(c0, (c1 + 1) / 2, c3);
    }
    for (int c3 = 2, c3_end = M; c3 < c3_end; c3 += 1)
      S3(c0, (c1 + 1) / 2, c3);
  }
