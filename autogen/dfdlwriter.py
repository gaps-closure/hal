#!/usr/bin/python3
import xml.dom.minidom
import copy

#---------------------- Begin DFDL Boilerplate for GAPS ------------------------
XSDHEAD = '''<?xml version="1.0" encoding="UTF-8"?> 
<xs:schema xmlns:xs="http://www.w3.org/2001/XMLSchema"
xmlns:dfdl="http://www.ogf.org/dfdl/dfdl-1.0/"
xmlns:fn="http://www.w3.org/2005/xpath-functions"
xmlns:daf="urn:ogf:dfdl:2013:imp:daffodil.apache.org:2018:ext"
xmlns:gma="urn:gma:1.0" targetNamespace="urn:gma:1.0">
'''

XSDTAIL = '''
</xs:schema>
'''

DFDLDEFAULTS = '''
<xs:annotation>
<xs:appinfo source="http://www.ogf.org/dfdl/">
<dfdl:defineVariable name="ByteOrder" type="xs:string" />
<dfdl:defineFormat name="defaults">
<dfdl:format alignment="1" alignmentUnits="bits" binaryBooleanFalseRep="0"
binaryBooleanTrueRep="1" binaryCalendarEpoch="1970-01-01T00:00:00+00:00"
binaryCalendarRep="bcd" binaryDecimalVirtualPoint="0" binaryFloatRep="ieee"
binaryNumberCheckPolicy="lax" binaryNumberRep="binary" binaryPackedSignCodes="C
D F C" calendarCenturyStart="53" calendarCheckPolicy="lax"
calendarDaysInFirstWeek="4" calendarFirstDayOfWeek="Monday"
calendarLanguage="en-US" calendarObserveDST="yes"
calendarPatternKind="implicit" calendarPattern="yyyy-MM-dd'T'HH:mm:ss"
calendarTimeZone="UTC" choiceLengthKind="implicit" decimalSigned="yes"
documentFinalTerminatorCanBeMissing="no" emptyValueDelimiterPolicy="none"
encoding="utf-8" encodingErrorPolicy="replace" escapeSchemeRef=""
fillByte="%NUL;" floating="no" ignoreCase="no" initiatedContent="no"
initiator="" leadingSkip="0" lengthKind="implicit" lengthUnits="bits"
nilKind="literalValue" nilValueDelimiterPolicy="none" nilValue="NIL"
occursCountKind="implicit" outputNewLine="%CR;%LF;"
prefixIncludesPrefixLength="no" representation="binary" separator=""
separatorPosition="infix" sequenceKind="ordered" terminator="" textBidi="no"
textBooleanFalseRep="false" textBooleanJustification="left"
textBooleanPadCharacter="%SP;" textBooleanTrueRep="true"
textCalendarJustification="left" textCalendarPadCharacter="%SP;"
textNumberCheckPolicy="lax" textNumberJustification="right"
textNumberPadCharacter="0" textNumberPattern="#0" textNumberRep="standard"
textNumberRoundingIncrement="0.0" textNumberRoundingMode="roundUp"
textNumberRounding="pattern" textOutputMinLength="0" textPadKind="none"
textStandardBase="10" textStandardDecimalSeparator="."
textStandardGroupingSeparator="," textStandardInfinityRep="Inf"
textStandardNaNRep="NaN" textStandardZeroRep="" textStringJustification="left"
textStringPadCharacter="%SP;" textTrimKind="none"
textZonedSignStyle="asciiStandard" trailingSkip="0"
truncateSpecifiedLengthString="no" useNilForDefault="no" utf16Width="fixed"
bitOrder="mostSignificantBitFirst"/>
</dfdl:defineFormat>
<dfdl:format ref="gma:defaults" byteOrder="bigEndian" />
</xs:appinfo>
</xs:annotation>
'''

GAPSTYPES = '''
<xs:simpleType name="gapsdouble" dfdl:lengthKind="explicit" dfdl:length="8" dfdl:lengthUnits="bytes" dfdl:byteOrder="littleEndian">
<xs:restriction base="xs:double"/>
</xs:simpleType>
<xs:simpleType name="gapsfloat" dfdl:lengthKind="explicit" dfdl:length="4" dfdl:lengthUnits="bytes" dfdl:byteOrder="littleEndian">
<xs:restriction base="xs:float"/>
</xs:simpleType>
<xs:simpleType name="gapsuint64" dfdl:lengthKind="explicit" dfdl:length="64" dfdl:lengthUnits="bits">
<xs:restriction base="xs:unsignedLong"/>
</xs:simpleType>
<xs:simpleType name="gapsuint32" dfdl:lengthKind="explicit" dfdl:length="32" dfdl:lengthUnits="bits">
<xs:restriction base="xs:unsignedInt"/>
</xs:simpleType>
<xs:simpleType name="gapsuint16" dfdl:lengthKind="explicit" dfdl:length="16" dfdl:lengthUnits="bits">
<xs:restriction base="xs:unsignedInt"/>
</xs:simpleType>
<xs:simpleType name="gapsuint8" dfdl:lengthKind="explicit" dfdl:length="8" dfdl:lengthUnits="bits">
<xs:restriction base="xs:unsignedInt"/>
</xs:simpleType>
<xs:simpleType name="gapsint64" dfdl:lengthKind="explicit" dfdl:length="64" dfdl:lengthUnits="bits">
<xs:restriction base="xs:long"/>
</xs:simpleType>
<xs:simpleType name="gapsint32" dfdl:lengthKind="explicit" dfdl:length="32" dfdl:lengthUnits="bits">
<xs:restriction base="xs:int"/>
</xs:simpleType>
<xs:simpleType name="gapsint16" dfdl:lengthKind="explicit" dfdl:length="16" dfdl:lengthUnits="bits">
<xs:restriction base="xs:short"/>
</xs:simpleType>
<xs:simpleType name="gapsint8" dfdl:lengthKind="explicit" dfdl:length="8" dfdl:lengthUnits="bits">
<xs:restriction base="xs:byte"/>
</xs:simpleType>
'''

GAPSTRAILER = '''
<xs:element name="GapsTrailer">
<xs:complexType>
<xs:sequence dfdl:byteOrder="bigEndian">
<xs:element name="seq" type="gma:gapsuint32" />
<xs:element name="rqr" type="gma:gapsuint32" />
<xs:element name="oid" type="gma:gapsuint32" />
<xs:element name="mid" type="gma:gapsuint16" />
<xs:element name="crc" type="gma:gapsuint16" />
</xs:sequence>
</xs:complexType>
</xs:element>
'''

GAPSPDU_BE = '''
<xs:element name="GapsPDU">
<xs:complexType>
<xs:choice> <!-- no way to discriminate SDHBW or SDHBE, so uncomment one -->
<xs:element ref="gma:SDHBEPDU" />
<!-- <xs:element ref="gma:SDHBWPDU" /> -->
</xs:choice>
</xs:complexType>
</xs:element>
<xs:element name="SDHBEPDU">
<xs:complexType>
<xs:sequence>
<xs:element ref="gma:SDHBEHeader" />
<xs:element ref="gma:ApplicationData" />
<xs:element ref="gma:GapsTrailer" />
</xs:sequence>
</xs:complexType>
</xs:element>
'''

GAPSPDU_BW = '''
<xs:element name="GapsPDU">
<xs:complexType>
<xs:choice> <!-- no way to discriminate SDHBW or SDHBE, so uncomment one -->
<!-- <xs:element ref="gma:SDHBEPDU" /> -->
<xs:element ref="gma:SDHBWPDU" />
</xs:choice>
</xs:complexType>
</xs:element>
<xs:element name="SDHBWPDU">
<xs:complexType>
<xs:sequence>
<xs:element ref="gma:SDHBWHeader" />
<xs:element ref="gma:ApplicationData" />
<xs:element ref="gma:GapsTrailer" />
</xs:sequence>
</xs:complexType>
</xs:element>
'''

SDHBEHDR_v1 = '''
<xs:element name="SDHBEHeader">
<xs:complexType>
<xs:sequence dfdl:byteOrder="bigEndian">
<xs:element name="stag" type="gma:gapsuint32" />
<xs:element name="mtag" type="gma:gapsuint32" />
<xs:element name="cnt"  type="gma:gapsuint32" />
<xs:element name="dtag" type="gma:gapsuint32" />
<xs:element name="its"  type="gma:gapsuint64" />
<xs:element name="hts"  type="gma:gapsuint64" />
<xs:element name="len"  type="gma:gapsuint32" />
</xs:sequence>
</xs:complexType>
</xs:element>
'''

SDHBWHDR_v1 = '''
<xs:element name="SDHBWHeader">
<xs:complexType>
<xs:sequence dfdl:byteOrder="bigEndian">
<xs:element name="tag0" type="gma:gapsuint8" />
<xs:element name="tagm" type="gma:gapsuint8" />
<xs:element name="tags" type="gma:gapsuint8" />
<xs:element name="tagt" type="gma:gapsuint8" />
<xs:element name="len" type="gma:gapsuint16" />
<xs:element name="crc" type="gma:gapsuint16" />
</xs:sequence>
</xs:complexType>
</xs:element>
'''
#---------------------- End DFDL Boilerplate for GAPS   ------------------------

'''
  <xs:element name="Distance">
    <xs:complexType>
      <xs:sequence dfdl:byteOrder="bigEndian">
        <xs:element name="x" type="gma:gapsdouble" />
        <xs:element name="y" type="gma:gapsdouble" />
        <xs:element name="z" type="gma:gapsdouble" />
      </xs:sequence>
    </xs:complexType>
  </xs:element>
'''

gapstyp = {
  'double': 'gapsdouble',
  'ffloat': 'gapsfloat',
  'int8':   'gapsint8',
  'uint8':  'gapsuint8',
  'int16':  'gapsint16',
  'uint16': 'gapsuint16',
  'int32':  'gapsint32',
  'uint32': 'gapsuint32',
  'int64':  'gapsint64',
  'uint64': 'gapsuint64'
}
  
class DFDLWriter:
  def make_array(self, f):
    appstr = ''
    return appstr

  def make_scalar(self, f):
    appstr = ''
    if f[0] in gapstyp:
      appstr += '<xs:element name="' + f[1] + '" type="gma:' + gapstyp[f[0]] + '" />' + '\n'
    else:
      raise Exception('Unhandled type: ' + f[0])
    return appstr
    
  def make_appdata(self,tree,pdutype):
    dtypid = 0
    appstr = ''
    appstr += '<xs:element name="ApplicationData">' + '\n'
    appstr += '<xs:complexType>' + '\n'
    appstr += '<xs:choice>' + '\n'

    for dtypnm in [l[0] for l in tree]:
      appstr += '<xs:element ref="gma:' + dtypnm + '">' + '\n'
      appstr += '<xs:annotation>' + '\n'
      appstr += '<xs:appinfo source="http://www.ogf.org/dfdl/">' + '\n'
      appstr += '<dfdl:discriminator test='
      if pdutype == 'be_v1':
        appstr += '"{../gma:SDHBEHeader/dtag eq 1}"'
      elif pdutype == 'bw_v1':
        appstr += '"{../gma:SDHBWHeader/tagt eq 1}"'
      else:
        raise Exception('Unknown pdutype: ' + pdutype)
      appstr += '/>' + '\n'
      appstr += '</xs:appinfo>' + '\n'
      appstr += '</xs:annotation>' + '\n'
      appstr += '</xs:element>' + '\n'

    appstr += '</xs:choice>' + '\n'
    appstr += '</xs:complexType>' + '\n'
    appstr += '</xs:element>' + '\n'

    for l in tree:
      fldnm = l.pop(0)
      appstr += '<xs:element name="' + fldnm + '">' + '\n'
      appstr += '<xs:complexType>' + '\n'
      appstr += '<xs:sequence dfdl:byteOrder="bigEndian">' + '\n'

      for f in l:
        if   len(f) == 2: appstr += self.make_scalar(f)
        elif len(f) == 3: appstr += self.make_array(f)
        else:             raise Exception('Unhandled field: ' + f)

      appstr += '</xs:sequence>' + '\n'
      appstr += '</xs:complexType>' + '\n'
      appstr += '</xs:element>' + '\n'

    return appstr

  def write(self, outfname, tree, pdutype):
    try:
      dfdlstr = ''.join([XSDHEAD, DFDLDEFAULTS, GAPSTYPES, GAPSTRAILER])
      if pdutype == 'be_v1':
        dfdlstr += GAPSPDU_BE
        dfdlstr += SDHBEHDR_v1
      elif pdutype == 'bw_v1':
        dfdlstr += GAPSPDU_BW
        dfdlstr += SDHBWHDR_v1
      else:
        raise Exception('Unknown pdutype: ' + pdutype)
      dfdlstr += self.make_appdata(tree,pdutype)
      dfdlstr += XSDTAIL
      dom     = xml.dom.minidom.parseString(dfdlstr)
      with open(outfname, 'w') as f:
        f.write(dom.toprettyxml(indent=' ', newl=''))
    except Exception as e:
      print("Error in export: ", e)

if __name__ == '__main__':
  pt = [['Position', ['double', 'x'], ['double', 'y'], ['double', 'z']], ['Distance', ['double', 'dx'], ['double', 'dy'], ['double', 'dz']]]
  print('Writing test DFDL to betest.dfdl.xsd')
  DFDLWriter().write('betest.dfdl.xsd', copy.deepcopy(pt), 'be_v1')
  print('Writing test DFDL to bwtest.dfdl.xsd')
  DFDLWriter().write('bwtest.dfdl.xsd', copy.deepcopy(pt), 'bw_v1')
