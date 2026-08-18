#!/usr/bin/perl

sub bcd_to_int {
    $bcd = shift;
    printf "bcd in: 0x%02X => ", $bcd;

    $n_hi = ($bcd >> 4) * 10;
    $n_lo = ($bcd & 0x0F);
    $n = $n_hi + $n_lo;

	print "$n_hi $n_lo = $n\n";
    return $n;
}
sub int_to_bcd {
   $n = shift;
   print "int in: $n  => ";
   $n = $n % 100;

   $n_hi = ($n / 10) << 4;
   $n_lo = ($n % 10);
   $no = $n_hi | $n_lo;
   printf "%02X + %02X = %02X \n", $n_hi , $n_lo , $no;
   return $no;
}


$n1 = bcd_to_int(0x58);
$n2 = bcd_to_int(0x46);
$nx = $n1 + $n2;
print "nx: $nx\n";
int_to_bcd($nx);


int_to_bcd(20);
int_to_bcd(45);

