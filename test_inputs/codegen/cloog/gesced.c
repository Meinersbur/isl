{
  for (int c0 = 1, c0_end = N; c0 <= c0_end; c0 += 1)
    S1(c0);
  for (int c0 = N + 1, c0_end = 2 * N; c0 <= c0_end; c0 += 1)
    for (int c1 = 1, c1_end = N; c1 <= c1_end; c1 += 1)
      S2(c1, -N + c0);
  for (int c0 = 2 * N + 1, c0_end = M + N; c0 <= c0_end; c0 += 1) {
    for (int c1 = 1, c1_end = N; c1 <= c1_end; c1 += 1)
      S3(c1, -2 * N + c0);
    for (int c1 = 1, c1_end = N; c1 <= c1_end; c1 += 1)
      S2(c1, -N + c0);
  }
  for (int c0 = M + N + 1, c0_end = M + 2 * N; c0 <= c0_end; c0 += 1)
    for (int c1 = 1, c1_end = N; c1 <= c1_end; c1 += 1)
      S3(c1, -2 * N + c0);
}
