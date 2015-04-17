{
  for (int c3 = 1, c3_end = n; c3 <= c3_end; c3 += 1)
    s2(c3);
  for (int c0 = 0, c0_end = n - 1; c0 < c0_end; c0 += 1) {
    for (int c3 = 0, c3_end = n - c0 - 1; c3 < c3_end; c3 += 1)
      s0(c0 + 1, n - c3);
    for (int c3 = 0, c3_end = n - c0 - 1; c3 < c3_end; c3 += 1)
      for (int c6 = c0 + 2, c6_end = n; c6 <= c6_end; c6 += 1)
        s1(c0 + 1, n - c3, c6);
  }
  for (int c0 = n - 1, c0_end = 2 * n - 1; c0 < c0_end; c0 += 1) {
    if (c0 >= n)
      for (int c2 = -n + c0 + 2, c2_end = n; c2 <= c2_end; c2 += 1)
        s3(c2, -n + c0 + 1);
    s4(-n + c0 + 2);
  }
}
