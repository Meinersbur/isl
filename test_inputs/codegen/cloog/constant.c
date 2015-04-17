{
  for (int c1 = 0, c1_end = min(1023, M + 1024); c1 <= c1_end; c1 += 1) {
    S1(c1);
    S3(c1);
  }
  for (int c1 = max(0, M + 1025), c1_end = 1023; c1 <= c1_end; c1 += 1) {
    S2(c1);
    S3(c1);
  }
  for (int c0 = 0, c0_end = min(1023, M + 1024); c0 <= c0_end; c0 += 1) {
    S4(c0);
    S6(c0);
  }
  for (int c0 = max(0, M + 1025), c0_end = 1023; c0 <= c0_end; c0 += 1) {
    S5(c0);
    S6(c0);
  }
}
