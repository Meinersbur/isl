{
  for (int c1 = 1, c1_end = n; c1 <= c1_end; c1 += 1)
    for (int c2 = 1, c2_end = m; c2 <= c2_end; c2 += 1) {
      s0(1, c1, c2, 0);
      s1(1, c1, c2, 0);
    }
  for (int c1 = 1, c1_end = n; c1 <= c1_end; c1 += 1) {
    s3(2, c1, 0, 0);
    s2(2, c1, 0, 0);
  }
  for (int c1 = 1, c1_end = m; c1 <= c1_end; c1 += 1) {
    for (int c3 = 1, c3_end = n; c3 <= c3_end; c3 += 1) {
      s5(3, c1, 1, c3);
      s4(3, c1, 1, c3);
    }
    for (int c3 = 1, c3_end = n; c3 <= c3_end; c3 += 1) {
      s7(3, c1, 2, c3);
      s6(3, c1, 2, c3);
    }
  }
  for (int c1 = 1, c1_end = m; c1 <= c1_end; c1 += 1) {
    s8(4, c1, 0, 0);
    s9(4, c1, 0, 0);
  }
}
