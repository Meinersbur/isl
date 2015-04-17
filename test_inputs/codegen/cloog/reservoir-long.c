for (int c0 = 1, c0_end = O; c0 < c0_end; c0 += 1) {
  for (int c1 = Q, c1_end = N; c1 < c1_end; c1 += 1) {
    for (int c2 = P, c2_end = M; c2 < c2_end; c2 += 1)
      S1(c0, c1, c2);
    for (int c2 = 1, c2_end = M; c2 < c2_end; c2 += 1)
      S2(c0, c1, c2);
  }
  for (int c1 = 1, c1_end = N; c1 < c1_end; c1 += 1) {
    for (int c2 = P, c2_end = M; c2 < c2_end; c2 += 1)
      S3(c0, c1, c2);
    for (int c2 = 1, c2_end = M; c2 < c2_end; c2 += 1)
      S4(c0, c1, c2);
  }
}
