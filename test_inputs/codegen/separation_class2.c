{
  for (int c0 = 0, c0_end = -(n % 8) + n; c0 < c0_end; c0 += 8) {
    for (int c1 = 0, c1_end = -(n % 8) + n; c1 < c1_end; c1 += 8)
      for (int c2 = 0, c2_end = 7; c2 <= c2_end; c2 += 1)
        for (int c3 = 0, c3_end = 7; c3 <= c3_end; c3 += 1)
          A(c0 + c2, c1 + c3);
    for (int c2 = 0, c2_end = 7; c2 <= c2_end; c2 += 1)
      for (int c3 = 0, c3_end = n % 8; c3 < c3_end; c3 += 1)
        A(c0 + c2, -((n - 1) % 8) + n + c3 - 1);
  }
  for (int c1 = 0, c1_end = n; c1 < c1_end; c1 += 8)
    for (int c2 = 0, c2_end = n % 8; c2 < c2_end; c2 += 1)
      for (int c3 = 0, c3_end = min(7, n - c1 - 1); c3 <= c3_end; c3 += 1)
        A(-((n - 1) % 8) + n + c2 - 1, c1 + c3);
}
