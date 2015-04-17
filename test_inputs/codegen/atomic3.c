for (int c0 = 0, c0_end = 64; c0 <= c0_end; c0 += 1) {
  if (c0 >= 63) {
    sync();
  } else if (c0 >= 1) {
    sync();
  } else
    sync();
}
