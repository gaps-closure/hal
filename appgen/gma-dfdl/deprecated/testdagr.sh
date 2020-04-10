DATASAMPLES="bw_write_111.infoset bw_write_221.infoset bw_write_222.infoset"
RULESETS="dagr.forward-bw.xsl dagr.reverse-bw-stretch.xsl dagr.reverse-bw.xsl"

for r in $RULESETS
do
  for i in $DATASAMPLES
  do
    echo "Applying rule $r to data item $i"
    echo "-------------------------------------------"
    xsltproc $r $i
    echo "-------------------------------------------"
  done
  echo
done
