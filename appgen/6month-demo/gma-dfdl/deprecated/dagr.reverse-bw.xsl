<?xml version="1.0"?>
<xsl:stylesheet version="1.0" 
	xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
	xmlns:gma="urn:gma:1.0">
  <xsl:output method="xml" indent="yes"/>

  <xsl:strip-space elements="*"/>

  <xsl:template match="@* | node()">
    <xsl:copy>
      <xsl:apply-templates select="@* | node()"/>
    </xsl:copy>
  </xsl:template>

  <xsl:variable name="MUX" select="/gma:GapsPDU/gma:SDHBWPDU/gma:SDHBWHeader/tagm"/>
  <xsl:variable name="SEC" select="/gma:GapsPDU/gma:SDHBWPDU/gma:SDHBWHeader/tags"/>
  <xsl:variable name="TYP" select="/gma:GapsPDU/gma:SDHBWPDU/gma:SDHBWHeader/tagt"/>

  <xsl:template match="/gma:GapsPDU">
    <xsl:choose>
      <xsl:when test="($MUX = '2' and $SEC = '2' and $TYP = '1') or ($MUX = '2' and $SEC = '2' and $TYP = '2')">
        <!-- <xsl:copy-of select="."/> -->
        <xsl:apply-templates select="node()"/>
      </xsl:when>
      <xsl:otherwise>
        <xsl:value-of select="concat('Blocking: ', $MUX, ',', $SEC, ',', $TYP)"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

</xsl:stylesheet>
