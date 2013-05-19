if (N >= 0 && M >= 0)
  for (int c1 = -4; c1 <= 3 * M + N; c1 += 1) {
    if (c1 >= 3 * M) {
      S2(M, -3 * M + c1);
    } else if (3 * floord(c1 - 2, 3) + 2 == c1 && c1 >= -1 && 3 * M >= c1 + 4)
      S1((c1 + 4) / 3, 0);
    for (int c3 = max(-3 * M + c1 + 3, c1 + 3 * floord(-c1 - 1, 3) + 3); c3 <= min(N - 1, c1); c3 += 3) {
      S2((c1 - c3) / 3, c3);
      S1((c1 - c3 + 3) / 3, c3 + 1);
    }
    if (N + 3 * floord(-N + c1, 3) == c1 && 3 * M + N >= c1 + 3 && c1 >= N) {
      S2((-N + c1) / 3, N);
    } else if (N >= c1 + 4)
      S1(0, c1 + 4);
    for (int c3 = max(-3 * M + c1, c1 + 3 * floord(-c1 - 1, 3) + 3); c3 <= min(N, c1); c3 += 3)
      S3((c1 - c3) / 3, c3);
  }
