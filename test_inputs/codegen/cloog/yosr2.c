{
  for (int c1 = 1, c1_end = M; c1 <= c1_end; c1 += 1)
    S2(c1);
  for (int c0 = 2, c0_end = M; c0 <= c0_end; c0 += 1) {
    for (int c2 = c0 + 1, c2_end = M; c2 <= c2_end; c2 += 1)
      for (int c3 = 1, c3_end = c0; c3 < c3_end; c3 += 1)
        S3(c0, c2, c3);
    for (int c1 = 1, c1_end = c0; c1 < c1_end; c1 += 1)
      S4(c1, c0);
    for (int c2 = 1, c2_end = c0; c2 < c2_end; c2 += 1)
      S1(c0, c2);
  }
}
