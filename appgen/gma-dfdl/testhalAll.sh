DAFFODIL=~/apache-daffodil-2.5.0-incubating-bin/bin/daffodil 
BWDFDL=gmabw.dfdl.xsd
BEDFDL=gmabe.dfdl.xsd
BWDATLST="bw_write_111 bw_write_221 bw_write_222"
BEDATLST="be_write_111 be_write_221 be_write_222"

for inf in $BWDATLST
do
  echo Processing $inf
  $DAFFODIL parse   -s $BWDFDL -o $inf.infoset  $inf.bin
  $DAFFODIL unparse -s $BWDFDL -o $inf.bin1     $inf.infoset
  diff -s $inf.bin $inf.bin1
  rm -f $inf.bin1
done

for inf in $BEDATLST
do
  echo Processing $inf
  $DAFFODIL parse   -s $BEDFDL -o $inf.infoset  $inf.bin
  $DAFFODIL unparse -s $BEDFDL -o $inf.bin1     $inf.infoset
  diff -s $inf.bin $inf.bin1
  rm -f $inf.bin1 
done

