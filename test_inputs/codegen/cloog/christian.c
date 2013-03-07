for (int c0 = -N + 1; c0 <= N; c0 += 1) {
  for (int c1 = max(c0 - 1, 0); c1 < min(N + c0 - 1, N); c1 += 1)
    S2(c1, -c0 + c1 + 1);
  for (int c1 = max(c0, 0); c1 < min(N + c0, N); c1 += 1)
    S1(c1, -c0 + c1);
}
