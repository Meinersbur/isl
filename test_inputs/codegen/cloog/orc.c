{
  for (int c0 = 0, c0_end = 2; c0 <= c0_end; c0 += 1) {
    S1(c0);
    for (int c1 = 0, c1_end = -c0 + 11; c1 <= c1_end; c1 += 1) {
      S2(c0, c1);
      S3(c0, c1);
    }
    S4(c0);
  }
  for (int c0 = 0, c0_end = 14; c0 <= c0_end; c0 += 1) {
    S5(c0);
    for (int c1 = 0, c1_end = 9; c1 <= c1_end; c1 += 1)
      S6(c0, c1);
    S7(c0);
  }
}
