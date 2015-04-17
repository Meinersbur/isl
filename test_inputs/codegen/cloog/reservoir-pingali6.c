for (int c0 = 1, c0_end = M; c0 <= c0_end; c0 += 1) {
  for (int c2 = 2, c2_end = N; c2 < c2_end; c2 += 1)
    for (int c3 = 2, c3_end = N; c3 < c3_end; c3 += 1)
      S1(c0, c2, c3);
  for (int c2 = 2, c2_end = N; c2 < c2_end; c2 += 1)
    for (int c3 = 2, c3_end = N; c3 < c3_end; c3 += 1)
      S2(c0, c2, c3);
}
