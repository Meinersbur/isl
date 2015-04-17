{
  for (int c1 = 0, c1_end = 51; c1 <= c1_end; c1 += 1)
    S2(0, c1);
  for (int c0 = 1, c0_end = 24; c0 <= c0_end; c0 += 1) {
    S2(c0, 0);
    for (int c1 = 1, c1_end = 50; c1 <= c1_end; c1 += 1)
      S1(c0, c1);
    S2(c0, 51);
  }
  for (int c1 = 0, c1_end = 51; c1 <= c1_end; c1 += 1)
    S2(25, c1);
}
