{
  for (int c0 = m; c0 <= min(p - 1, n); c0 += 1)
    S1(c0);
  for (int c0 = p; c0 <= min(q, m - 1); c0 += 1)
    S2(c0);
  for (int c0 = max(m, p); c0 <= min(q, n); c0 += 1) {
    S1(c0);
    S2(c0);
  }
  for (int c0 = max(max(m, p), q + 1); c0 <= n; c0 += 1)
    S1(c0);
  for (int c0 = max(max(m, n + 1), p); c0 <= q; c0 += 1)
    S2(c0);
}
