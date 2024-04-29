<?xml version="1.0"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform" xmlns:gma="urn:gma:1.0">
  <xsl:output method="xml" indent="yes" />

  <xsl:variable name="M" select="/gma:GapsPDU/gma:SDHBWPDU/gma:SDHBWHeader/tagm" />
  <xsl:variable name="S" select="/gma:GapsPDU/gma:SDHBWPDU/gma:SDHBWHeader/tags" />
  <xsl:variable name="T" select="/gma:GapsPDU/gma:SDHBWPDU/gma:SDHBWHeader/tagt" />

  <!-- process GapsPDU/SDHBWPDU with matching tags, applying additional transforms as needed -->
  <xsl:template match="node()|@*">
    <xsl:choose>
      <xsl:when test="($M = '2' and $S = '2' and $T = '2')"> <xsl:copy> <xsl:apply-templates /> </xsl:copy> </xsl:when>
      <xsl:when test="($M = '2' and $S = '2' and $T = '1')"> <xsl:copy> <xsl:apply-templates /> </xsl:copy> </xsl:when>
      <xsl:otherwise />
    </xsl:choose>
  </xsl:template>

  <!-- zeroize the z coordinate of Position -->
  <xsl:template match="/gma:GapsPDU/gma:SDHBWPDU/gma:Position/z/text()">0.0</xsl:template>

</xsl:stylesheet>

  
