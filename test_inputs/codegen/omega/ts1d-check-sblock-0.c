{
  for (int c1 = 0, c1_end = 1; c1 <= c1_end; c1 += 1) {
    if (c1 == 1) {
      s0(1, 1, 1, 0, 0);
      s0(1, 1, 1, N - 1, 0);
    } else
      for (int c3 = 0, c3_end = N; c3 < c3_end; c3 += 1)
        s0(1, 0, 1, c3, 0);
  }
  for (int c1 = 0, c1_end = floord(T - 1, 1000); c1 <= c1_end; c1 += 1)
    for (int c2 = 1000 * c1 + 1, c2_end = min(N + T - 3, N + 1000 * c1 + 997); c2 <= c2_end; c2 += 1)
      for (int c3 = max(0, -N - 1000 * c1 + c2 + 2), c3_end = min(min(999, T - 1000 * c1 - 1), -1000 * c1 + c2 - 1); c3 <= c3_end; c3 += 1)
        s1(2, 1000 * c1 + c3, 1, -1000 * c1 + c2 - c3, 1);
}
