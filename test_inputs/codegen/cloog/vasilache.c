{
  S1();
  S2();
  for (int c0 = 0, c0_end = N; c0 < c0_end; c0 += 1)
    for (int c1 = 0, c1_end = N; c1 < c1_end; c1 += 1) {
      S4(c0, c1);
      S5(c0, c1);
    }
  for (int c0 = 0, c0_end = N; c0 < c0_end; c0 += 1)
    for (int c1 = 0, c1_end = N; c1 < c1_end; c1 += 1)
      for (int c2 = 0, c2_end = (N - 1) / 32; c2 <= c2_end; c2 += 1) {
        S7(c0, c1, c2, 32 * c2);
        for (int c3 = 32 * c2 + 1, c3_end = min(N - 1, 32 * c2 + 31); c3 <= c3_end; c3 += 1) {
          S6(c0, c1, c2, c3 - 1);
          S7(c0, c1, c2, c3);
        }
        if (32 * c2 + 31 >= N) {
          S6(c0, c1, c2, N - 1);
        } else
          S6(c0, c1, c2, 32 * c2 + 31);
      }
  S8();
}
