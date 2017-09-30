BEGIN { sum1 = 0; sum2 = 0; }
{ sum1 += $2; sum2 += $4 }
END { print sum1/200, sum2/200 }
