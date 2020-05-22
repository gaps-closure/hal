<?xml version="1.0"?>
<xsl:stylesheet version="2.0" 
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:xs="http://www.w3.org/2001/XMLSchema"
  xmlns:functx="http://www.functx.com"
  xmlns:gma="urn:gma:1.0"
  xmlns:dagr="urn:dagr:1.0">
  <xsl:output method="xml" indent="yes"/>
  <xsl:strip-space elements="*"/>

  <!-- Top-level rule template -->
  <xsl:template match="/gma:GapsPDU">
    <xsl:choose>
      <xsl:when test="dagr:matchTags(.,'2','2','2')"> <xsl:copy-of select="dagr:Allow(.)"/>             </xsl:when>
      <xsl:when test="dagr:matchTags(.,'2','2','1')"> <xsl:copy-of select="dagr:Redact(.,'zeroposz')"/> </xsl:when>
      <xsl:otherwise>                                 <xsl:copy-of select="dagr:Block(.)"/>             </xsl:otherwise>
    </xsl:choose>
  </xsl:template>

  <!-- Function to check if pdu matches specified mux,sec,typ tags in SDHBWPDU -->
  <xsl:function name="dagr:matchTags" as="xs:boolean">
    <xsl:param name="pdu"/>
    <xsl:param name="mux"/>
    <xsl:param name="sec"/>
    <xsl:param name="typ"/>
    <xsl:variable name="thismux" select="$pdu/gma:SDHBWPDU/gma:SDHBWHeader/tagm/text()"/>
    <xsl:variable name="thissec" select="$pdu/gma:SDHBWPDU/gma:SDHBWHeader/tags/text()"/>
    <xsl:variable name="thistyp" select="$pdu/gma:SDHBWPDU/gma:SDHBWHeader/tagt/text()"/>
    <xsl:value-of select="$mux = $thismux and $sec = $thissec and $typ = $thistyp"/>
  </xsl:function>

  <!-- Function for Allow guard rule -->
  <xsl:function name="dagr:Allow">
    <xsl:param name="pdu"/>
    <xsl:sequence select="$pdu"/>
  </xsl:function>

  <!-- Function for Block guard rule -->
  <xsl:function name="dagr:Block">
    <xsl:param name="pdu"/>
    <xsl:variable name="thismux" select="$pdu/gma:SDHBWPDU/gma:SDHBWHeader/tagm/text()"/>
    <xsl:variable name="thissec" select="$pdu/gma:SDHBWPDU/gma:SDHBWHeader/tags/text()"/>
    <xsl:variable name="thistyp" select="$pdu/gma:SDHBWPDU/gma:SDHBWHeader/tagt/text()"/>
    <xsl:value-of select="concat('&#xa;','Blocked: ', $thismux, ',', $thissec, ',', $thistyp,'&#xa;')"/>
  </xsl:function>

  <!-- Function for Redact guard rules -->
  <xsl:function name="dagr:Redact">
    <xsl:param name="pdu"/>
    <xsl:param name="oper"/>
    <xsl:choose>
      <xsl:when test="$oper = 'zeroposz'">
        <xsl:copy-of select="functx:replace-elements-deep($pdu,'z','0.0')"/>
      </xsl:when>
      <!-- additional redaction procedures can be implemented here -->
      <xsl:otherwise>
        <xsl:copy-of select="dagr:Block($pdu)"/>
      </xsl:otherwise>
    </xsl:choose>
  </xsl:function>

  <!-- BEGIN low level manipulation functions derived from functx library -->
  <xsl:function name="functx:substring-after-if-contains" as="xs:string?">
    <xsl:param name="arg" as="xs:string?"/>
    <xsl:param name="delim" as="xs:string"/>
    <xsl:sequence select="
      if (contains($arg,$delim))
      then substring-after($arg,$delim)
      else $arg
      "/>
  </xsl:function>

  <xsl:function name="functx:name-test" as="xs:boolean">
    <xsl:param name="testname" as="xs:string?"/>
    <xsl:param name="names" as="xs:string*"/>
    <xsl:sequence select="
      $testname = $names
      or
      $names = '*'
      or
      functx:substring-after-if-contains($testname,':') =
        (for $name in $names return substring-after($name,'*:'))
      or
      substring-before($testname,':') =
        (for $name in $names[contains(.,':*')] return substring-before($name,':*'))
      "/>
  </xsl:function>

  <xsl:function name="functx:replace-element-values" as="element()*">
    <xsl:param name="elements" as="element()*"/>
    <xsl:param name="values" as="xs:anyAtomicType*"/>
    <xsl:for-each select="$elements">
      <xsl:variable name="seq" select="position()"/>
      <xsl:element name="{node-name(.)}">
        <xsl:sequence select="@*, $values[$seq]"/>
      </xsl:element>
    </xsl:for-each>
  </xsl:function>

  <xsl:function name="functx:replace-elements-deep" as="node()*">
    <xsl:param name="nodes" as="node()*"/>
    <xsl:param name="names" as="xs:string*"/>
    <xsl:param name="values" as="xs:string*"/>
    <xsl:for-each select="$nodes">
      <xsl:choose>
        <xsl:when test=". instance of element()">
          <xsl:choose>
            <xsl:when test="not(functx:name-test(name(),$names))">
              <xsl:element name="{node-name(.)}">
                <xsl:sequence select="@*, functx:replace-elements-deep(node(), $names, $values)"/>
              </xsl:element>
            </xsl:when>
            <xsl:otherwise>
              <xsl:sequence select="functx:replace-element-values(.,'0.0')"/>
            </xsl:otherwise>
          </xsl:choose>
        </xsl:when>
        <xsl:when test=". instance of document-node()">
          <xsl:document>
            <xsl:sequence select="functx:replace-elements-deep(node(), $names,$values)"/>
          </xsl:document>
        </xsl:when>
        <xsl:otherwise>
          <xsl:sequence select="."/>
        </xsl:otherwise>
      </xsl:choose>
    </xsl:for-each>
  </xsl:function>
  <!-- END low level manipulation functions derived from functx library -->

</xsl:stylesheet>
