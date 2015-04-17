for (int c0 = a, c0_end = b + 20; c0 <= c0_end; c0 += 1) {
  if (b >= c0)
    s0(c0);
  if (c0 >= a + 10 && b + 10 >= c0)
    s1(c0);
  if (c0 >= a + 20)
    s2(c0);
}
