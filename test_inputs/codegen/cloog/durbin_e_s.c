{
  S4(1, 0, 0);
  S7(1, 0, 0);
  S8(1, 0, 3);
  for (int c0 = 2, c0_end = 9; c0 <= c0_end; c0 += 1) {
    S2(c0, -7, 0);
    for (int c1 = -7, c1_end = c0 - 8; c1 < c1_end; c1 += 1)
      S3(c0, c1, 1);
    S6(c0, c0 - 9, 2);
    S8(c0, 0, 3);
    for (int c1 = 1, c1_end = c0; c1 < c1_end; c1 += 1)
      S5(c0, c1, 3);
  }
  S2(10, -7, 0);
  for (int c1 = -7, c1_end = 1; c1 <= c1_end; c1 += 1)
    S3(10, c1, 1);
  S6(10, 1, 2);
  for (int c1 = 1, c1_end = 9; c1 <= c1_end; c1 += 1) {
    S5(10, c1, 3);
    S1(10, c1, 4);
  }
  S1(10, 10, 4);
}
