{
  for (int c0 = m, c0_end = min(n, p - 1); c0 <= c0_end; c0 += 1)
    S1(c0);
  for (int c0 = p, c0_end = min(m - 1, q); c0 <= c0_end; c0 += 1)
    S2(c0);
  for (int c0 = max(m, p), c0_end = min(n, q); c0 <= c0_end; c0 += 1) {
    S1(c0);
    S2(c0);
  }
  for (int c0 = max(max(m, p), q + 1), c0_end = n; c0 <= c0_end; c0 += 1)
    S1(c0);
  for (int c0 = max(max(m, n + 1), p), c0_end = q; c0 <= c0_end; c0 += 1)
    S2(c0);
}
