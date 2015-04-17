for (int c0 = 2, c0_end = 3 * M; c0 < c0_end; c0 += 1) {
  if ((c0 - 2) % 3 == 0)
    S1((c0 + 1) / 3);
  for (int c1 = (c0 + 1) / 3 + 1, c1_end = min(M, c0 - 2); c1 <= c1_end; c1 += 1)
    for (int c2 = -c1 + (c0 + c1 + 1) / 2 + 1, c2_end = min(c1, c0 - c1); c2 <= c2_end; c2 += 1)
      S3(c0 - c1 - c2 + 1, c1, c2);
  for (int c1 = -c0 + 2 * ((2 * c0 + 1) / 3) + 2, c1_end = min(M, c0); c1 <= c1_end; c1 += 2)
    S2(((c0 - c1) / 2) + 1, c1);
}
